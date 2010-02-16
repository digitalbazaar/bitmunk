/*
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk/customcatalog/ListingUpdater.h"

#include "bitmunk/customcatalog/CustomCatalogModule.h"
#include "bitmunk/portmapper/IPortMapper.h"
#include "monarch/data/json/JsonWriter.h"
#include "monarch/net/Url.h"
#include "monarch/rt/RunnableDelegate.h"

using namespace std;
using namespace bitmunk::common;
using namespace bitmunk::customcatalog;
using namespace bitmunk::portmapper;
using namespace bitmunk::node;
using namespace monarch::config;
using namespace monarch::crypto;
using namespace monarch::data::json;
using namespace monarch::event;
using namespace monarch::fiber;
using namespace monarch::modest;
using namespace monarch::net;
using namespace monarch::rt;
using namespace monarch::upnp;
using namespace monarch::util;

#define EVENT_NET_ACCESS_SUCCESS     "bitmunk.catalog.NetAccess.success"
#define EVENT_NET_ACCESS_EXCEPTION   "bitmunk.catalog.NetAccess.exception"

#define EXCEPTION_NET_ACCESS         "bitmunk.mastercatalog.NetAccessTestError"

// default gateway discovery timeout is 5 seconds
#define GATEWAY_DISCOVERY_TIMEOUT 5*1000

/*
 Implementation details regarding client-side seller listing/catalog
 synchronization with a remote end:

 Local edits can occur concurrently with synchronization with the remote end:

 To local add/update ware:
 1. Set its "dirty" flag, clear its "deleted" flag.
 2. Do not touch "updating" flag.

 To local delete ware:
 1. Set "dirty" and "deleted" flags.
 2. Do not touch "updating" flag.
 3. The ware must not be actually removed from its database until sync'ing
    with the remote end is complete.

 To get the next seller listing update:
 1. We only need to update when:
  A. There are wares with their "dirty" flags set or there is a pending update.
  B. The remote end is reporting an update ID that is 1 greater than our stored
     update ID.
  C. The remote end is reporting an update ID that is either less than our
     stored update ID or is more than 1 greater than it.

 The proper execution order to check each case:
 Case C, Case B and then Case A.

 Case A: The normal case - REGULAR UPDATE
 ----------------------------------------
 Here we've made changes to our catalog that haven't been uploaded yet. This is
 the most common case. Either we have a pending update because we failed to
 contact the server in our last attempt, or we have dirty wares. If we have
 any wares with their "updating" flags already set, use them in our update and
 clear their "dirty" flags. Note that this occurs when we marked wares for
 an update but we completely failed to notify the server of the update. This
 is different from the case where the server started or completed an update
 for us, but we failed to receive notification from the server. In this
 current case, the remote end's update ID will be the same as our own, in
 the other case (described below) it will be exactly 1 ahead of ours.

 If we don't have any "updating" wares, then grab up to X wares that have
 their "dirty" flags set. X should be the maximum # of wares that can be
 sent up in a single update. Set those wares' "updating" flags, clear their
 "dirty" flags, and set the current update ID + 1. Grab the marked wares
 and put them in an update object with its update ID set to the current
 update ID plus 1.

 ListingUpdater: Request the next update object from the catalog.
 Catalog: Do the rest of the work.

 Case B: Recoverable crash - REPEAT LAST UPDATE
 ----------------------------------------------
 We must have been in the middle of an update that at least partially
 succeeded.  The remote end saved our update ID but we crashed before we
 could mark our local stuff indicating what worked in the update and what
 didn't. The wares that we sent in that update will have their "updating"
 flags set. Add all wares with their "updating" flags set to the update
 object and set the object's update ID to the remote end's update ID. If
 there are no wares to send up, then we crashed after we cleared all ware's
 updating flags but before we were able to set our new stored update ID. In
 that case, just set the new stored update ID.

 ListingUpdater: Request the previous update object from the catalog.
 Catalog: Do the rest of the work.

 Case C: Unrecoverable error - UPDATE EVERYTHING
 -----------------------------------------------
 Since we only set our stored update ID after we have received it from the
 remote end and we only do 1 update at a time, there has been a fatal error
 and we must assume all remote listings are corrupt. Therefore, we must resend
 all wares. Set the current global ID and the server ID to 0 AND mark all wares
 as dirty, set their update IDs to 0, and clear all "updating" flags. This must
 all be done atomically (in a database transaction). Next, tell the remote end
 to delete the old server ID. Send an "updateRequest" message. When we handle
 the next request, a new server ID will be requested since it has been set
 to 0.

 ListingUpdater: Determine that the remote listings must be assumed corrupt
    and make a call to "reset all wares" (or whatever), schedule messages to
    delete server ID, register for a new one, and to send an update.
 CatalogDatabase: Create method for marking wares appropriately.

 All cases: Once an update completes, first clear all "updating" flags on all
 wares. Do not touch their "dirty" flags. Then store the new update ID.

 Race conditions and their results:

 If we can't provide perfect atomicity while grabbing the wares that will be
 updated, we don't really care. Here's why: If a change to a ware sneaks in
 right before we can grab it to include it in an update (after it has been
 marked as "updating" and its "dirty" flag cleared), then we'll just end up
 duplicating our update in the next call, without doing any evil harm. It
 doesn't matter if the ware is deleted or just has its properties changed.

 When processing a listing update result, wares should be deleted from the
 catalog where their "updating" flag is set, their "deleting" flag is set,
 and their "dirty" flag is NOT set. This ensures that wares are only removed
 from the catalog once the remote end has been notified.
*/

ListingUpdater::ListingUpdater(Node* node, Catalog* catalog) :
   NodeFiber(node, 0),
   mCatalog(catalog),
   mRemoteUpdateId(-1),
   mState(Idle),
   mOperation(NULL),
   mUpdateRequestPending(NULL),
   mTestNetAccessPending(NULL)
{
}

ListingUpdater::~ListingUpdater()
{
}

void ListingUpdater::processMessages()
{
   MO_CAT_DEBUG(BM_CUSTOMCATALOG_CAT,
      "running ListingUpdater for user %" PRIu64, getUserId());

   // send first listing update message
   {
      DynamicObject msg;
      msg["updateRequest"] = true;
      messageSelf(msg);
   }

   // send first net access test message
   {
      DynamicObject msg;
      msg["testNetAccess"] = true;
      messageSelf(msg);
   }

   /*
    Note: Algorithm for keeping seller listings up to date with bitmunk:

    1. Wait for a specific list of messages.

    2. If an update request has arrived, then grab the next update based
       on the last remote end update ID and current stored update ID. If
       no server ID is set, register for one and send another update request.
       If no last remote end update ID exists, send an empty update (heartbeat).
       Any update sent should be done in an operation. Wait again.

    3. If an update response has arrived, check for an error. If found,
       log it and wait again if nothing else can be done with the error. If
       the server ID is invalid or the update response indicates a bogus
       catalog, then start an operation to delete the old server ID as
       necessary and register for a new server ID. Otherwise, run the catalog
       db code to handle the update and then wait again.

    4. If a register response has arrived, see if it includes an error. If
       found, log it and wait again. If not, store the server ID and send
       an update request. Wait again.

    5. If a test net access message has arrived, set the state to busy and
       run a net access test and update the server URL if necessary. Wait again.

    6. If a test net access response has arrived, set the state to idle and
       wait again.

    7. If an interrupt message has arrived, quit waiting.

    */

   // wait for messages with any of these keys
   const char* keys[] = {
      "updateRequest",
      "updateResponse",
      "registerResponse",
      "testNetAccess",
      "testNetAccessResponse",
      "interrupt",
      NULL
   };
   DynamicObject msgs(NULL);
   DynamicObjectIterator mi(NULL);
   bool quit = false;
   while(!quit)
   {
      msgs = waitForMessages(keys, &mOperation);

      // iterate over latest messages
      mi = msgs.getIterator();
      while(!quit && mi->hasNext())
      {
         DynamicObject& msg = mi->next();

         if(msg->hasMember("updateRequest"))
         {
            // only handle new requests if we're idle, otherwise ignore them
            // and new requests will be scheduled when we're finished being
            // busy (we can only be idle here if an update/register/netaccess
            // thread is running ... and when netaccess finishes it will
            // schedule another update requset if one is still pending, and
            // if it's an update/register that is running we don't need to
            // run another one right now)
            if(mState == Idle)
            {
               mUpdateRequestPending = false;
               handleUpdateRequest(msg);
            }
            else
            {
               mUpdateRequestPending = true;
            }
         }
         else if(msg->hasMember("updateResponse"))
         {
            handleUpdateResponse(msg);
         }
         else if(msg->hasMember("registerResponse"))
         {
            handleRegisterResponse(msg);
         }
         else if(msg->hasMember("testNetAccess"))
         {
            // only handle new requests if we're idle, otherwise ignore them
            // and new requests will be scheduled when we're finished being
            // busy (we can only be idle here if an update/register/netaccess
            // thread is running ... and when update/register finishes it
            // will schedule another net access test if one is still pending,
            // and if it's a netaccess test that is running we don't need to
            // run another one right now)
            if(mState == Idle)
            {
               mTestNetAccessPending = false;
               handleTestNetAccess(msg);
            }
            else
            {
               mTestNetAccessPending = true;
            }
         }
         else if(msg->hasMember("testNetAccessResponse"))
         {
            // no longer busy
            mState = Idle;
            mOperation.setNull();
         }
         else if(msg->hasMember("interrupt"))
         {
            // fiber interrupted, so quit
            quit = true;
         }

         // yield before next message
         yield();
      }
   }

   MO_CAT_DEBUG(BM_CUSTOMCATALOG_CAT,
      "ListingUpdater for user %" PRIu64 " exited", getUserId());
}

void ListingUpdater::handleUpdateRequest(DynamicObject& msg)
{
   // get user ID
   UserId userId = getUserId();

   // get seller info from catalog
   Seller seller;
   string serverToken;
   if(!mCatalog->populateSeller(userId, seller, serverToken))
   {
      // could not get server ID
      MO_CAT_ERROR(BM_CUSTOMCATALOG_CAT,
         "ListingUpdater could not get seller information for "
         "user ID: %" PRIu64 ", %s",
         userId,
         JsonWriter::writeToString(Exception::getAsDynamicObject()).c_str());
   }
   // see if server ID is invalid (0)
   else if(!BM_SERVER_ID_VALID(seller["serverId"]))
   {
      // change state
      mState = Busy;

      // register for a new server ID
      mRemoteUpdateId = -1;
      RunnableRef r = new RunnableDelegate<ListingUpdater>(
         this, &ListingUpdater::registerServerId);
      mOperation = r;
      if(!getNode()->runUserOperation(userId, mOperation))
      {
         // could not start user operation
         MO_CAT_ERROR(BM_CUSTOMCATALOG_CAT,
            "ListingUpdater could not register for new server ID: %s",
            JsonWriter::writeToString(Exception::getAsDynamicObject()).c_str());
      }
      else
      {
         // operation failed, go idle again
         mState = Idle;
      }
   }
   // set remote update ID if it isn't set yet
   else if(mRemoteUpdateId == -1)
   {
      // send heartbeat (empty update)
      SellerListingUpdate update;
      if(!mCatalog->populateHeartbeatListingUpdate(userId, update))
      {
         // could not get server ID
         MO_CAT_ERROR(BM_CUSTOMCATALOG_CAT,
            "ListingUpdater could not get heartbeat update: %s",
            JsonWriter::writeToString(Exception::getAsDynamicObject()).c_str());
      }
      else
      {
         // change state
         mState = Busy;

         RunnableRef r = new RunnableDelegate<ListingUpdater>(
            this, &ListingUpdater::sendUpdate, update);
         mOperation = r;
         if(!getNode()->runUserOperation(userId, mOperation))
         {
            // could not start user operation
            MO_CAT_ERROR(BM_CUSTOMCATALOG_CAT,
               "ListingUpdater could not send heartbeat: %s",
               JsonWriter::writeToString(
                  Exception::getAsDynamicObject()).c_str());
         }
         else
         {
            // operation failed, go idle again
            mState = Idle;
         }
      }
   }
   // do real update, complete pending or do next
   else
   {
      // prepare to build the update
      SellerListingUpdate update;
      bool pass = false;
      bool corrupt = false;

      // get current update ID
      uint32_t updateId;
      if(mCatalog->getUpdateId(userId, updateId))
      {
         // if our update ID + 1 == the remote update ID then we must have
         // crashed in the middle of an update... so finish the pending update
         if((updateId + 1) == mRemoteUpdateId)
         {
            pass = mCatalog->populatePendingListingUpdate(userId, update);
         }
         // either we're in-sync with the remote end or we've got some local
         // updates to push up... so get the next update, which may be empty
         // and will therefore function as a heartbeat
         else if(updateId == mRemoteUpdateId)
         {
            pass = mCatalog->populateNextListingUpdate(userId, update);
         }
         // as far as we can tell, the remote update ID is totally hosed, we
         // must start over from scratch
         else
         {
            corrupt = true;
            pass = mCatalog->resetListingUpdateCounter(userId);

            // request another update
            DynamicObject updateRequest;
            updateRequest["updateRequest"] = true;
            messageSelf(updateRequest);
         }
      }

      if(pass && !corrupt)
      {
         // change state
         mState = Busy;

         // send update
         RunnableRef r = new RunnableDelegate<ListingUpdater>(
            this, &ListingUpdater::sendUpdate, update);
         mOperation = r;
         pass = getNode()->runUserOperation(userId, mOperation);
         if(!pass)
         {
            // operation failed, go idle again
            mState = Idle;
         }
      }

      if(!pass)
      {
         // log error
         MO_CAT_ERROR(BM_CUSTOMCATALOG_CAT,
            "ListingUpdater could not handle update request: %s",
            JsonWriter::writeToString(Exception::getAsDynamicObject()).c_str());
      }
   }
}

void ListingUpdater::handleUpdateResponse(DynamicObject& msg)
{
   // only process message if there was no irrecoverable error
   // when performing update (any such error will be logged there)
   if(!msg["error"]->getBoolean())
   {
      // if update was a heartbeat to get remote update ID, save it
      if(mRemoteUpdateId == -1)
      {
         // save remote update ID
         mRemoteUpdateId = msg["updateId"]->getUInt32();

         // send a new update request message
         DynamicObject updateRequest;
         updateRequest["updateRequest"] = true;
         messageSelf(updateRequest);
      }
      // else update was a normal update
      else
      {
         SellerListingUpdate update = msg["update"];
         SellerListingUpdate result = msg["updateResult"];

         MO_CAT_DEBUG(BM_CUSTOMCATALOG_CAT,
            "ListingUpdater received listing update result: %s",
            JsonWriter::writeToString(result).c_str());

         // save remote update ID
         mRemoteUpdateId = result["updateId"]->getUInt32();

         // process update by passing to the catalog
         if(!mCatalog->processListingUpdateResponse(
            getUserId(), update, result))
         {
            // log error
            MO_CAT_ERROR(BM_CUSTOMCATALOG_CAT,
               "ListingUpdater could not process update result: %s",
               JsonWriter::writeToString(
                  Exception::getAsDynamicObject()).c_str());
         }
         else if(msg["scheduleUpdateRequest"]->getBoolean())
         {
            // request another update
            DynamicObject updateRequest;
            updateRequest["updateRequest"] = true;
            messageSelf(updateRequest);
         }
      }
   }
   // must reset listing update counter and re-register
   else if(msg["reset"]->getBoolean())
   {
      mRemoteUpdateId = -1;
      if(mCatalog->resetListingUpdateCounter(getUserId()))
      {
         // request another update
         DynamicObject updateRequest;
         updateRequest["updateRequest"] = true;
         messageSelf(updateRequest);
      }
      else
      {
         // log error
         MO_CAT_ERROR(BM_CUSTOMCATALOG_CAT,
            "ListingUpdater could not reset listing counter: %s",
            JsonWriter::writeToString(
               Exception::getAsDynamicObject()).c_str());
      }
   }

   // no longer busy
   mState = Idle;
   mOperation.setNull();
}

void ListingUpdater::handleRegisterResponse(DynamicObject& msg)
{
   // only process message if there was no error
   if(!msg["error"]->getBoolean())
   {
      // request was successful, store server ID and token
      Seller seller;
      seller["serverId"] = msg["serverId"];
      seller["url"] = msg["serverUrl"];
      ServerToken serverToken = msg["serverToken"]->getString();
      if(mCatalog->updateSeller(getUserId(), seller, serverToken))
      {
         // success, send next update request message
         DynamicObject updateRequest;
         updateRequest["updateRequest"] = true;
         messageSelf(updateRequest);

         MO_CAT_DEBUG(BM_CUSTOMCATALOG_CAT,
            "ListingUpdater registered for server ID %u, server token '%s' "
            "for user %" PRIu64,
            BM_SERVER_ID(msg["serverId"]), serverToken, getUserId());
      }
      else
      {
         // log error
         MO_CAT_ERROR(BM_CUSTOMCATALOG_CAT,
            "ListingUpdater could not save server ID and token: %s",
            JsonWriter::writeToString(Exception::getAsDynamicObject()).c_str());
      }
   }
   else
   {
      // send event
      Event e;
      e["type"] = "bitmunk.catalog.ServerIdRegistration.exception";
      e["details"]["userId"] = getUserId();
      e["details"]["exception"] = msg["exception"];
      mNode->getEventController()->schedule(e);
   }

   // no longer busy
   mState = Idle;
   mOperation.setNull();
}

void ListingUpdater::handleTestNetAccess(DynamicObject& msg)
{
   // only do net access test if remote update ID is valid
   if(mRemoteUpdateId != -1)
   {
      // get user ID
      UserId userId = getUserId();

      // get seller info from catalog
      Seller seller;
      string serverToken;
      if(!mCatalog->populateSeller(userId, seller, serverToken))
      {
         // could not get server ID
         MO_CAT_ERROR(BM_CUSTOMCATALOG_CAT,
            "ListingUpdater could not get seller information for "
            "user ID: %" PRIu64 ", %s",
            userId,
            JsonWriter::writeToString(Exception::getAsDynamicObject()).c_str());
      }
      else
      {
         // change state
         mState = Busy;

         // update the server URL
         RunnableRef r = new RunnableDelegate<ListingUpdater>(
            this, &ListingUpdater::updateServerUrl, seller);
         mOperation = r;
         if(!getNode()->runUserOperation(userId, mOperation))
         {
            // could not start user operation
            MO_CAT_ERROR(BM_CUSTOMCATALOG_CAT,
               "ListingUpdater could not update server URL: %s",
               JsonWriter::writeToString(
                  Exception::getAsDynamicObject()).c_str());
         }
         else
         {
            // operation failed, go idle again
            mState = Idle;
         }
      }
   }
}

/**
 * Adds a port mapping to allow public internet access to a user's catalog.
 *
 * @param cfg the port mapping configuration to use.
 * @param port the local port.
 * @param ipm the interface to the portmapper module.
 *
 * @return true if successful, false if not.
 */
static bool _addPortMapping(IPortMapper* ipm, Config& cfg, uint32_t localPort)
{
   bool rval = true;

   MO_CAT_DEBUG(BM_CUSTOMCATALOG_CAT,
      "ListingUpdater attempting to add port mapping.");

   // discover gateways if necessary
   DeviceList gateways = ipm->getDiscoveredGateways();
   if(gateways->length() == 0)
   {
      int count = ipm->discoverGateways(GATEWAY_DISCOVERY_TIMEOUT);
      if(count == 0)
      {
         ExceptionRef e = new Exception(
            "No UPnP-supported internet gateways detected.",
            "bitmunk.portmapper.NoUPnPInternetGateways");
         Exception::set(e);
         rval = false;
      }
      else if(count < 0)
      {
         rval = false;
      }
   }

   if(rval)
   {
      // get external port (if 0, use local port)
      uint32_t externalPort = cfg["externalPort"]->getUInt32();
      if(externalPort == 0)
      {
         externalPort = localPort;
      }

      // build port mapping (remote host and internal client are blank so
      // they will be auto-discovered)
      PortMapping pm;
      pm["NewRemoteHost"] = "";
      pm["NewExternalPort"] = externalPort;
      pm["NewProtocol"] = "TCP";
      pm["NewInternalPort"] = localPort;
      pm["NewInternalClient"] = "";
      pm["NewEnabled"] = "1";
      pm["NewPortMappingDescription"] = cfg["description"]->getString();
      pm["NewLeaseDuration"] = 0;

      // just use first gateway
      Device igd = ipm->getDiscoveredGateways().first();
      rval = ipm->addPortMapping(igd, pm);
      if(rval)
      {
         MO_CAT_DEBUG(BM_CUSTOMCATALOG_CAT,
            "ListingUpdater added port mapping: %s",
            JsonWriter::writeToString(pm).c_str());
      }
   }

   if(!rval)
   {
      MO_CAT_ERROR(BM_CUSTOMCATALOG_CAT,
         "ListingUpdater failed to add port mapping: %s",
         JsonWriter::writeToString(Exception::getAsDynamicObject()).c_str());
   }

   return rval;
}

static bool _doReverseNetTest(
   Node* node, Config& cfg, UserId userId, const char* token,
   string& serverUrl)
{
   bool rval = false;

   // start creating public server URL
   serverUrl = "https://";

   // setup post data, use blank host to indicate to the reverse netaccess
   // service that our public IP should be used in the netaccess test
   // use a 15 seconds timeout in waiting for a response
   DynamicObject out;
   out["host"] = "";
   out["port"] = cfg["port"];
   out["path"] = "/api/3.0/catalog/netaccess/test";
   out["sellerId"] = userId;
   out["token"] = token;
   out["timeout"] = 15;

   DynamicObject in;

   // post to bitmunk
   Url url;
   url.format("/api/3.0/catalog/netaccess/rtest");
   Messenger* m = node->getMessenger();
   if(m->postSecureToBitmunk(&url, &out, &in, userId))
   {
      rval = true;
      serverUrl.append(in["ip"]->getString());
      serverUrl.push_back(':');
      serverUrl.append(cfg["port"]->getString());

      // send event
      Event ev;
      ev["type"] = EVENT_NET_ACCESS_SUCCESS;
      ev["details"]["userId"] = userId;
      ev["details"]["serverUrl"] = serverUrl.c_str();
      node->getEventController()->schedule(ev);
   }

   return rval;
}

bool ListingUpdater::testNetAccess(string& serverUrl)
{
   bool rval = false;

   // get user ID
   UserId userId = getUserId();

   // get netaccess token and then
   // do a reverse netaccess test to acquire the public IP that
   // bitmunk and other peers can hit to access our catalog/negotiate with us
   string token;
   if(mCatalog->getConfigValue(userId, "netaccessToken", token))
   {
      // get configuration
      Config cfg = mNode->getConfigManager()->getNodeConfig();

      // do reverse net test
      rval = _doReverseNetTest(mNode, cfg, userId, token.c_str(), serverUrl);

      // if the reverse net access test failed, see if port mapping is
      // available (i.e. through upnp)
      if(!rval && Exception::get()->isType(EXCEPTION_NET_ACCESS))
      {
         // save exception
         DynamicObject nonUPnP = Exception::getAsDynamicObject();
         Config pmCfg = mNode->getConfigManager()->getModuleConfig(
            "bitmunk.catalog.CustomCatalog");
         if(!pmCfg.isNull() && pmCfg->hasMember("portMapping") &&
            pmCfg["portMapping"]["enabled"]->getBoolean())
         {
            // get port mapper interface
            IPortMapper* ipm = dynamic_cast<IPortMapper*>(
               mNode->getModuleApiByType("bitmunk.portmapper"));
            if(ipm == NULL)
            {
               MO_CAT_ERROR(BM_CUSTOMCATALOG_CAT,
                  "ListingUpdater could not create port mapping, no portmapper "
                  "module found.");
            }
            else
            {
               // try to create port mapping and try reverse net test again
               rval =
                  _addPortMapping(
                     ipm, pmCfg["portMapping"], cfg["port"]->getUInt32()) &&
                  _doReverseNetTest(
                     mNode, cfg, userId, token.c_str(), serverUrl);
               if(!rval)
               {
                  ExceptionRef e = Exception::get();
                  e->getDetails()["beforeUPnPException"] = nonUPnP;
               }
            }
         }
      }
   }

   if(rval)
   {
      // clear any existing exception from the database
      mCatalog->setConfigValue(userId, "netaccessException", "");
   }
   else
   {
      // failed to confirm netaccess to this server
      ExceptionRef e = new Exception(
         "Could not confirm public network access to the local server.",
         "bitmunk.catalog.NetAccessTestError");
      Exception::push(e);

      // write exception to database
      DynamicObject dyno = Exception::getAsDynamicObject();
      mCatalog->setConfigValue(
         userId, "netaccessException", JsonWriter::writeToString(dyno).c_str());

      // send event
      Event ev;
      ev["type"] = EVENT_NET_ACCESS_EXCEPTION;
      ev["details"]["userId"] = userId;
      ev["details"]["exception"] = dyno;
      mNode->getEventController()->schedule(ev);
   }

   return rval;
}

void ListingUpdater::registerServerId()
{
   MO_CAT_DEBUG(BM_CUSTOMCATALOG_CAT,
      "ListingUpdater registering for new server ID for user %" PRIu64,
      getUserId());

   // create message to send after registration
   DynamicObject msg;
   msg["registerResponse"] = true;
   msg["error"] = false;

   // do net access test
   string serverUrl;
   bool pass = false;
   if(testNetAccess(serverUrl))
   {
      DynamicObject out;
      out["url"] = serverUrl.c_str();
      out["listingUpdateId"] = "0";

      DynamicObject in;

      // post to bitmunk
      Url url;
      url.format("/api/3.0/catalog/sellers/%" PRIu64, getUserId());
      Messenger* m = mNode->getMessenger();
      if(m->postSecureToBitmunk(&url, &out, &in, getUserId()))
      {
         pass = true;

         // send registration data in message
         msg["serverId"] = in["serverId"];
         msg["serverToken"] = in["serverToken"];
         msg["serverUrl"] = out["url"];
      }
   }

   if(!pass)
   {
      // failed to register
      ExceptionRef e = new Exception(
         "Could not register for new server ID.",
         "bitmunk.catalog.ServerIdRegistrationError");
      Exception::push(e);
      DynamicObject d = Exception::getAsDynamicObject();
      msg["error"] = true;
      msg["exception"] = d;

      // log error
      MO_CAT_ERROR(BM_CUSTOMCATALOG_CAT,
         "ListingUpdater: %s", JsonWriter::writeToString(d).c_str());
   }

   // send message to self
   messageSelf(msg);

   // schedule a pending test net access request
   if(mTestNetAccessPending)
   {
      mTestNetAccessPending = false;
      DynamicObject msg;
      msg["testNetAccess"] = true;
      messageSelf(msg);
   }
}

void ListingUpdater::updateServerUrl(Seller& seller)
{
   // get user ID
   UserId userId = getUserId();

   MO_CAT_DEBUG(BM_CUSTOMCATALOG_CAT,
      "ListingUpdater testing public internet access to catalog for user %"
      PRIu64,
      userId);

   // create message to send after test and update
   DynamicObject msg;
   msg["testNetAccessResponse"] = true;

   // do net access test
   string serverUrl;
   bool pass = false;
   if(testNetAccess(serverUrl))
   {
      // if server URL has NOT changed, we're done
      if(strcmp(seller["url"]->getString(), serverUrl.c_str()) == 0)
      {
         pass = true;

         MO_CAT_DEBUG(BM_CUSTOMCATALOG_CAT,
            "ListingUpdater net access test passed and server URL has not "
            "changed for user ID: %" PRIu64 ", serverUrl: '%s'",
            userId, serverUrl.c_str());
      }
      // server URL has changed, post to bitmunk to update it
      else
      {
         MO_CAT_DEBUG(BM_CUSTOMCATALOG_CAT,
            "ListingUpdater net access test passed and server URL has "
            "changed for user ID: %" PRIu64 ", "
            "oldServerUrl: '%s', newServerUrl: '%s'",
            userId, seller["url"]->getString(), serverUrl.c_str());

         DynamicObject out;
         out["url"] = serverUrl.c_str();

         DynamicObject in;

         // post to bitmunk to update server URL
         Url url;
         url.format("/api/3.0/catalog/sellers/%" PRIu64 "/%" PRIu32,
            userId, BM_SERVER_ID(seller["serverId"]));
         Messenger* m = mNode->getMessenger();
         if(m->postSecureToBitmunk(&url, &out, &in, userId))
         {
            // save new server URL
            pass = mCatalog->setConfigValue(
               userId, "serverUrl", serverUrl.c_str());
         }
      }
   }

   if(!pass)
   {
      // failed to update server URL
      ExceptionRef e = new Exception(
         "Could not test public internet access and update server URL.",
         "bitmunk.catalog.UpdateServerUrlError");
      Exception::push(e);

      // log error
      MO_CAT_ERROR(BM_CUSTOMCATALOG_CAT,
         "ListingUpdater: %s",
         JsonWriter::writeToString(Exception::getAsDynamicObject()).c_str());
   }

   // send message to self
   messageSelf(msg);

   // schedule a pending update request
   if(mUpdateRequestPending)
   {
      mUpdateRequestPending = false;
      DynamicObject msg;
      msg["updateRequest"] = true;
      messageSelf(msg);
   }
}

void ListingUpdater::sendUpdate(SellerListingUpdate& update)
{
   MO_CAT_DEBUG(BM_CUSTOMCATALOG_CAT,
      "ListingUpdater sending seller listing update for user %" PRIu64 ": %s",
      getUserId(), JsonWriter::writeToString(update).c_str());

   // create message to send after update
   DynamicObject msg;
   msg["updateResponse"] = true;
   msg["scheduleUpdateRequest"] = false;
   msg["error"] = false;

   // post to bitmunk
   DynamicObject result;
   Url url("/api/3.0/catalog/listings");
   Messenger* m = mNode->getMessenger();
   if(m->postSecureToBitmunk(&url, &update, &result, getUserId()))
   {
      // send result in message, include update ID separately so that
      // handling heartbeats above is simpler -- it doesn't matter if
      // the current update ID was valid or not, it will be stored in
      // "updateId" field
      msg["update"] = update;
      msg["updateResult"] = result;
      msg["updateId"] = result["updateId"]->getString();

      // if the update wasn't a heartbeat, include a note that another
      // update should be run immediately after this result is processed
      if(update["payeeSchemes"]["updates"]->length() > 0 ||
         update["payeeSchemes"]["removals"]->length() > 0 ||
         update["listings"]["updates"]->length() > 0 ||
         update["listings"]["removals"]->length() > 0)
      {
         msg["scheduleUpdateRequest"] = true;
      }
   }
   else
   {
      // if the error was an invalid update ID, do not record it as an error
      // and see if we can recover from it
      ExceptionRef ex = Exception::get();
      if(ex->isType("bitmunk.mastercatalog.UpdateIdNotCurrent"))
      {
         msg["updateIdNotCurrent"] = true;
         msg["updateId"] = ex->getDetails()["currentUpdateId"]->getString();
      }
      else
      {
         // FIXME: determine if error is a network error or if there
         // were just some bad wares that weren't accepted/bad remote update ID

         // failed to update
         ExceptionRef e = new Exception(
            "Could not send seller listing update.",
            "bitmunk.catalog.SellerListingUpdateError");
         Exception::push(e);
         DynamicObject d = Exception::getAsDynamicObject();
         msg["error"] = true;
         msg["exception"] = d;

         // see if we need to register for a new server ID because our catalog
         // expired and we no longer exist as a seller
         // FIXME: we want a different exception name for this
         if(!ex->getCauseOfType("bitmunk.database.NotFound").isNull() ||
            !ex->getCauseOfType(
               "bitmunk.database.Catalog.InvalidServerToken").isNull())
         {
            // log message ... do new server registration
            MO_CAT_DEBUG(BM_CUSTOMCATALOG_CAT,
               "ListingUpdater user's (user ID %" PRIu64
               ") seller does not exist, "
               "has expired, or server token is invalid: %s",
               getUserId(), JsonWriter::writeToString(d).c_str());
            msg["reset"] = true;
         }
         else
         {
            // log generic error
            MO_CAT_ERROR(BM_CUSTOMCATALOG_CAT,
               "ListingUpdater: %s", JsonWriter::writeToString(d).c_str());
         }
      }
   }

   // send message to self
   messageSelf(msg);

   // schedule a pending test net access request
   if(mTestNetAccessPending)
   {
      mTestNetAccessPending = false;
      DynamicObject msg;
      msg["testNetAccess"] = true;
      messageSelf(msg);
   }
}

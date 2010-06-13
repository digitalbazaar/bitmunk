/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk/customcatalog/Catalog.h"

#include "bitmunk/common/TypeDefinitions.h"
#include "bitmunk/customcatalog/CatalogDatabase.h"
#include "bitmunk/customcatalog/CustomCatalogModule.h"
#include "bitmunk/customcatalog/ListingUpdater.h"
#include "monarch/data/json/JsonWriter.h"
#include "monarch/event/ObserverDelegate.h"
#include "monarch/sql/Row.h"
#include "monarch/sql/Statement.h"
#include "monarch/util/Random.h"

#include "sqlite3.h"

using namespace std;
using namespace monarch::config;
using namespace monarch::data::json;
using namespace monarch::event;
using namespace monarch::fiber;
using namespace monarch::rt;
using namespace monarch::sql;
using namespace monarch::util;
using namespace bitmunk::common;
using namespace bitmunk::customcatalog;
using namespace bitmunk::medialibrary;
using namespace bitmunk::node;

// use events
#define EVENT_USER_LOGGED_IN    "bitmunk.common.User.loggedIn"

// download state events
#define EVENT_DOWNLOAD_STATE    "bitmunk.purchase.DownloadState"

// auto-sell event
#define EVENT_AUTOSELL          "bitmunk.medialibrary.File.updated"

// ware events
#define EVENT_WARE              "bitmunk.common.Ware"

// payee scheme events
#define EVENT_PAYEE_SCHEME      "bitmunk.common.PayeeScheme"

// listing events
#define EVENT_SYNC_SELLER_LISTINGS "bitmunk.common.SellerListing.sync"
#define EVENT_TEST_NET_ACCESS      "bitmunk.catalog.NetAccess.test"

// config change events
#define EVENT_CONFIG_CHANGED "monarch.config.Config.changed"

// FIXME: add event handler for config changed events...
// filter on user's config ID (i.e. "user 900" ... get this from a call to the
// node config manager which should be exposed if it isn't already) ...
// and also filter on the specific values in ["details"]["config"] that
// we're interested in for autosell, etc.

Catalog::Catalog() :
   mNode(NULL),
   mMediaLibrary(NULL),
   mUserLoggedInObserver(NULL),
   mAutoSellObserver(NULL),
   mListingSyncDaemon(NULL),
   mNetAccessTestDaemon(NULL)
{
}

Catalog::~Catalog()
{
}

bool Catalog::init(Node* node)
{
   bool rval = false;

   mNode = node;

   // get the media library interface
   mMediaLibrary = dynamic_cast<IMediaLibrary*>(
      node->getModuleApiByType("bitmunk.medialibrary"));
   if(mMediaLibrary == NULL)
   {
      ExceptionRef e = new Exception(
         "Could not access media library.",
         "bitmunk.catalog.NoMediaLibrary");
      Exception::set(e);
   }
   else
   {
      // add self as media library extension
      mMediaLibrary->addMediaLibraryExtension(this);

      // register observer for user logged in events
      mUserLoggedInObserver = new ObserverDelegate<Catalog>(
         this, &Catalog::userLoggedIn);
      node->getEventController()->registerObserver(
         &(*mUserLoggedInObserver), EVENT_USER_LOGGED_IN);
      MO_CAT_DEBUG(BM_CUSTOMCATALOG_CAT,
         "Catalog registered for " EVENT_USER_LOGGED_IN);

      // register observer for auto-sell events
      DynamicObject filter;
      filter["details"]["userData"]["source"] = "purchase";
      mAutoSellObserver = new ObserverDelegate<Catalog>(
         this, &Catalog::checkAutoSell);
      node->getEventController()->registerObserver(
         &(*mAutoSellObserver), EVENT_AUTOSELL, &filter);
      MO_CAT_DEBUG(BM_CUSTOMCATALOG_CAT,
         "Catalog registered for " EVENT_AUTOSELL);

      // register observer for sync listing events
      mListingSyncDaemon = new ObserverDelegate<Catalog>(
         this, &Catalog::syncSellerListings);
      node->getEventController()->registerObserver(
         &(*mListingSyncDaemon), EVENT_SYNC_SELLER_LISTINGS);
      MO_CAT_DEBUG(BM_CUSTOMCATALOG_CAT,
         "Catalog registered for " EVENT_SYNC_SELLER_LISTINGS);

      // register observer for testing net access
      mNetAccessTestDaemon = new ObserverDelegate<Catalog>(
         this, &Catalog::testNetAccess);
      node->getEventController()->registerObserver(
         &(*mNetAccessTestDaemon), EVENT_TEST_NET_ACCESS);
      MO_CAT_DEBUG(BM_CUSTOMCATALOG_CAT,
         "Catalog registered for " EVENT_TEST_NET_ACCESS);

      rval = true;
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Could not initialize custom catalog.",
         "bitmunk.catalog.InitializationError");
      Exception::push(e);
   }

   return rval;
}

static bool _schedulePayeeSchemeUpdatedEvent(
   Node* node, PayeeSchemeId psId, UserId userId, bool added)
{
   bool rval = true;

   // schedule a payee scheme added/updated event
   Event e;
   e["type"] = EVENT_PAYEE_SCHEME ".updated";
   e["details"]["payeeSchemeId"] = psId;
   e["details"]["userId"] = userId;
   e["details"]["isNew"] = added;
   node->getEventController()->schedule(e);

   // we return true here because we want to chain this function with the
   // addPayeeScheme calls
   return rval;
}

void Catalog::cleanup(Node* node)
{
   if(mMediaLibrary != NULL)
   {
      // stop listening to test net access events
      node->getEventController()->unregisterObserver(
         &(*mNetAccessTestDaemon), EVENT_TEST_NET_ACCESS);
      mNetAccessTestDaemon.setNull();
      MO_CAT_DEBUG(BM_CUSTOMCATALOG_CAT,
         "Catalog unregistered for " EVENT_TEST_NET_ACCESS);

      // stop listening to sync listing events
      node->getEventController()->unregisterObserver(
         &(*mListingSyncDaemon), EVENT_SYNC_SELLER_LISTINGS);
      mListingSyncDaemon.setNull();
      MO_CAT_DEBUG(BM_CUSTOMCATALOG_CAT,
         "Catalog unregistered for " EVENT_SYNC_SELLER_LISTINGS);

      // stop listening to auto-sell events
      node->getEventController()->unregisterObserver(
         &(*mAutoSellObserver), EVENT_AUTOSELL);
      mAutoSellObserver.setNull();
      MO_CAT_DEBUG(BM_CUSTOMCATALOG_CAT,
         "Catalog unregistered for " EVENT_AUTOSELL);

      // stop listening to user logged in events
      node->getEventController()->unregisterObserver(
         &(*mUserLoggedInObserver), EVENT_USER_LOGGED_IN);
      mUserLoggedInObserver.setNull();
      MO_CAT_DEBUG(BM_CUSTOMCATALOG_CAT,
         "Catalog unregistered for " EVENT_USER_LOGGED_IN);

      // remove media library extension
      mMediaLibrary->removeMediaLibraryExtension(this);

      // clear media library interface
      mMediaLibrary = NULL;
   }
}

bool Catalog::mediaLibraryInitialized(
   UserId userId, Connection* conn, DatabaseClientRef& dbc)
{
   bool rval = true;

   // create tables if they do not exist:
   rval = mCatalogDb.initialize(conn);

   // create and set netaccess token
   if(rval)
   {
      uint64_t n = Random::next(1, 1000000000);
      char tmp[20];
      snprintf(tmp, 20, "%" PRIu64, n);
      rval = mCatalogDb.setConfigValue(userId, "netaccessToken", tmp, conn);
      if(rval)
      {
         MO_CAT_DEBUG(BM_CUSTOMCATALOG_CAT,
            "Created netaccess token '%s' for user %" PRIu64, tmp, userId);
      }
   }

   if(rval)
   {
      // FIXME: validate config in initialize? if we do it there.. what if
      // it changes between then and now?

      // start the listing updater unless the configuration file specifies
      // otherwise
      bool uploadListings = true;
      Config cfg = mNode->getConfigManager()->getModuleUserConfig(
         "bitmunk.catalog.CustomCatalog", userId);
      if(!cfg.isNull() && cfg->hasMember("uploadListings"))
      {
         uploadListings = cfg["uploadListings"]->getBoolean();
      }

      if(uploadListings)
      {
         // start listing updater fiber
         ListingUpdater* fiber = new ListingUpdater(mNode, this);
         fiber->setUserId(userId);
         FiberId id = mNode->getFiberScheduler()->addFiber(fiber);
         mListingUpdaterMap[userId] = id;

         // get sync listing interval
         uint64_t slInterval = 0;
         if(!cfg.isNull() && cfg->hasMember("listingSyncInterval"))
         {
            slInterval = cfg["listingSyncInterval"]->getUInt64() * 1000;
         }

         if(slInterval == 0)
         {
            // default to 5 minutes
            slInterval = 5 * 60 * 1000;
         }

         // set up sync listings daemon event
         {
            Event e;
            e["type"] = EVENT_SYNC_SELLER_LISTINGS;
            e["parallel"] = true;
            e["details"]["userId"] = userId;
            mNode->getEventDaemon()->add(e, slInterval, -1);
         }

         // get test net access interval
         uint64_t tnaInterval = 0;
         if(!cfg.isNull() && cfg->hasMember("testNetAccessInterval"))
         {
            tnaInterval = cfg["testNetAccessInterval"]->getUInt64() * 1000;
         }

         if(tnaInterval == 0)
         {
            // default to 1 hour
            tnaInterval = 60 * 60 * 1000;
         }

         // set up test net access daemon event
         {
            Event e;
            e["type"] = EVENT_TEST_NET_ACCESS;
            e["parallel"] = true;
            e["details"]["userId"] = userId;
            mNode->getEventDaemon()->add(e, tnaInterval, -1);
         }
      }
   }

   return rval;
}

void Catalog::mediaLibraryCleaningUp(UserId userId)
{
   // remove fiber from listing updater map
   mListingUpdaterMap.erase(userId);

   // remove sync listings daemon event
   {
      Event e;
      e["type"] = EVENT_SYNC_SELLER_LISTINGS;
      e["parallel"] = true;
      e["details"]["userId"] = userId;
      mNode->getEventDaemon()->remove(e);
   }

   // remove test net access daemon event
   {
      Event e;
      e["type"] = EVENT_TEST_NET_ACCESS;
      e["parallel"] = true;
      e["details"]["userId"] = userId;
      mNode->getEventDaemon()->remove(e);
   }
}

bool Catalog::updateWare(UserId userId, Ware& ware)
{
   bool rval = false;

   // keep track of whether or not ware was added or updated
   bool wareAdded = false;

   // get database connection
   Connection* c = mMediaLibrary->getConnection(userId);
   if(c != NULL)
   {
      MO_CAT_DEBUG(BM_CUSTOMCATALOG_CAT,
         "Updating ware, ID: %s", BM_WARE_ID(ware["id"]));

      // prepare statements:

      // begin transaction
      if((rval = c->begin()))
      {
         // find payee scheme ID
         PayeeSchemeId psId = 0;
         if(ware->hasMember("payeeSchemeId"))
         {
            psId = BM_PAYEE_SCHEME_ID(ware["payeeSchemeId"]);
            rval = mCatalogDb.isPayeeSchemeIdValid(psId, c);
         }
         else
         {
            // add new payee scheme
            rval = mCatalogDb.addPayeeScheme(
               mNode, userId,
               psId, ware["description"]->getString(), ware["payees"], c) &&
               _schedulePayeeSchemeUpdatedEvent(mNode, psId, userId, true);
            BM_ID_SET(ware["payeeSchemeId"], psId);
         }

         // update ware entry
         rval = rval && mCatalogDb.updateWare(ware, psId, c, &wareAdded);

         // commit transaction
         rval = rval ? c->commit() : c->rollback() && false;
      }

      // close connection
      c->close();
   }

   if(rval)
   {
      // schedule a ware update event
      Event e;
      e["type"] = EVENT_WARE ".updated";
      BM_ID_SET(e["details"]["wareId"], BM_WARE_ID(ware["id"]));
      BM_ID_SET(e["details"]["userId"], userId);
      e["details"]["isNew"] = wareAdded;
      mNode->getEventController()->schedule(e);

      // if the ware was added, fire an update event for its related
      // payee scheme because its ware count has changed
      Event ev;
      ev["type"] = EVENT_PAYEE_SCHEME ".updated";
      BM_ID_SET(
         ev["details"]["payeeSchemeId"],
         BM_PAYEE_SCHEME_ID(ware["payeeSchemeId"]));
      BM_ID_SET(ev["details"]["userId"], userId);
      ev["details"]["isNew"] = false;
      mNode->getEventController()->schedule(ev);
   }

   return rval;
}

bool Catalog::removeWare(UserId userId, Ware& ware)
{
   bool rval = false;

   // get database connection
   Connection* c = mMediaLibrary->getConnection(userId);
   if(c != NULL)
   {
      MO_CAT_DEBUG(BM_CUSTOMCATALOG_CAT,
         "Removing ware, ID: %s", BM_WARE_ID(ware["id"]));

      // remove the ware from the database
      rval = c->begin() && mCatalogDb.removeWare(ware, c);
      rval = rval ? c->commit() : c->rollback() && false;

      // close connection
      c->close();
   }

   if(rval)
   {
      // schedule a ware removed event
      Event e;
      e["type"] = EVENT_WARE ".removed";
      BM_ID_SET(e["details"]["wareId"], BM_WARE_ID(ware["id"]));
      BM_ID_SET(e["details"]["userId"], userId);
      mNode->getEventController()->schedule(e);

      // fire an update event for its related payee scheme because its
      // ware count has changed
      Event ev;
      ev["type"] = EVENT_PAYEE_SCHEME ".updated";
      BM_ID_SET(
         ev["details"]["payeeSchemeId"],
         BM_PAYEE_SCHEME_ID(ware["payeeSchemeId"]));
      BM_ID_SET(ev["details"]["userId"], userId);
      ev["details"]["isNew"] = false;
      mNode->getEventController()->schedule(ev);
   }

   return rval;
}

bool Catalog::populateWare(UserId userId, Ware& ware)
{
   bool rval = false;

   // get database connection
   Connection* c = mMediaLibrary->getConnection(userId);
   if(c != NULL)
   {
      MO_CAT_DEBUG(BM_CUSTOMCATALOG_CAT,
         "Populating ware, ID: %s", BM_WARE_ID(ware["id"]));

      // FIXME: what do we do about "problem" wares? ... and should
      // we stat the related files in here so we can update their
      // problem flags if the files don't exist?

      // populate the ware from the database
      rval = mCatalogDb.populateWare(userId, ware, mMediaLibrary, c);

      // close connection
      c->close();
   }

   return rval;
}

bool Catalog::populateWareSet(
   UserId userId, DynamicObject& query, ResourceSet& wareSet)
{
   bool rval = false;

   // get database connection
   Connection* c = mMediaLibrary->getConnection(userId);
   if(c != NULL)
   {
      MO_CAT_DEBUG(BM_CUSTOMCATALOG_CAT,
         "Populating ware set with %d wareIds and %d fileIds",
         query->hasMember("ids") ? query["ids"]->length() : 0,
         query->hasMember("fileIds") ? query["fileIds"]->length() : 0);

      // FIXME: what do we do about "problem" wares? ... and should
      // we stat the related files in here so we can update their
      // problem flags if the files don't exist?

      // populate the ware from the database
      rval = mCatalogDb.populateWareSet(
         userId, query, wareSet, mMediaLibrary, c);

      // close connection
      c->close();
   }

   return rval;
}

bool Catalog::populateWareBundle(
   UserId userId, Ware& ware, MediaPreferenceList& prefs)
{
   bool rval = false;

   ExceptionRef e = new Exception(
      "Ware bundle support is not available with this catalog.",
      "bitmunk.catalog.FeatureUnsupported");
   Exception::set(e);

   return rval;
}

bool Catalog::populateFileInfo(UserId userId, FileInfo& fi)
{
   // populate file info using media library
   return mMediaLibrary->populateFile(userId, fi);
}

bool Catalog::populateFileInfos(UserId userId, Ware& ware)
{
   bool rval = false;

   // get database connection
   Connection* c = mMediaLibrary->getConnection(userId);
   if(c != NULL)
   {
      MO_CAT_DEBUG(BM_CUSTOMCATALOG_CAT,
         "Populating ware file infos, ware ID: %s", BM_WARE_ID(ware["id"]));

      // clear the existing file info from the ware
      ware["fileInfos"]->clear();

      // populate the file information into the given ware
      rval = mCatalogDb.populateWareFileInfo(userId, ware, mMediaLibrary, c);

      // close connection
      c->close();
   }

   return rval;
}

bool Catalog::populateFileInfoList(UserId userId, FileInfoList& fil)
{
   bool rval = true;

   fil->setType(Array);
   int length = fil->length();
   if(length > 0)
   {
      rval = false;

      // get database connection
      Connection* c = mMediaLibrary->getConnection(userId);
      if(c != NULL)
      {
         // FIXME: add method to media library to populate a list of files?
         int count = 0;
         FileInfoIterator fii = fil.getIterator();
         while(rval && fii->hasNext())
         {
            FileInfo& fi = fii->next();
            rval = mMediaLibrary->populateFile(userId, fi, 0, c);
            if(rval)
            {
               ++count;
            }
         }

         // count must equal number of file infos in list or else
         // some were not found in the database
         if(count != fil->length())
         {
            ExceptionRef e = new Exception(
               "Could not populate FileInfoList. At least one file ID "
               "not found.", "bitmunk.catalog.InvalidFileId");
            Exception::set(e);
            rval = false;
         }

         // close connection
         c->close();
      }
   }

   return rval;
}

bool Catalog::updatePayeeScheme(
   UserId userId, PayeeSchemeId& psId,
   const char* description, PayeeList& payees)
{
   bool rval = false;

   // get database connection
   Connection* c = mMediaLibrary->getConnection(userId);
   if(c != NULL)
   {
      if(c->begin())
      {
         // payee scheme is new
         if(psId == 0)
         {
            rval =
               mCatalogDb.addPayeeScheme(
                  mNode, userId, psId, description, payees, c) &&
               _schedulePayeeSchemeUpdatedEvent(mNode, psId, userId, true);
         }
         // replacing old payee scheme
         // do replacement in a transaction so it happens atomically
         else
         {
            PayeeScheme ps;
            BM_ID_SET(ps["id"], psId);
            ps["description"] = description;
            ps["payees"] = payees;
            rval =
               mCatalogDb.updatePayeeScheme(ps, c) &&
               _schedulePayeeSchemeUpdatedEvent(mNode, psId, userId, false);
         }

         rval = rval ? c->commit() : c->rollback() && false;
      }

      c->close();
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Failed to update payee scheme.",
         "bitmunk.catalog.UpdatePayeeSchemeFailed");
      e->getDetails()["payees"] = payees;
      e->getDetails()["description"] = description;
      BM_ID_SET(e->getDetails()["payeeSchemeId"], psId);
      Exception::push(e);
   }

   return rval;
}

bool Catalog::populatePayeeSchemes(
   UserId userId, DynamicObject& filters, ResourceSet& rs)
{
   bool rval = false;

   // get database connection
   Connection* c = mMediaLibrary->getConnection(userId);
   if(c != NULL)
   {
      MO_CAT_DEBUG(BM_CUSTOMCATALOG_CAT, "Populating payee schemes");

      // populate the resource set with payee schemes using the given filters
      rval = mCatalogDb.populatePayeeSchemes(rs, filters, c);

      // set the associated userId of each payee scheme
      if(rs->hasMember("resources"))
      {
         ResourceSetIterator rsi = rs["resources"].getIterator();
         while(rsi->hasNext())
         {
            PayeeScheme& ps = rsi->next();
            BM_ID_SET(ps["userId"], userId);
         }
      }

      c->close();
   }

   return rval;
}

bool Catalog::populatePayeeScheme(
   UserId userId, PayeeScheme& ps)
{
   DynamicObject filters;
   filters["default"] = true;
   return populatePayeeScheme(userId, filters, ps);
}

bool Catalog::populatePayeeScheme(
   UserId userId, DynamicObject& filters, PayeeScheme& ps)
{
   bool rval = false;

   // get database connection
   Connection* c = mMediaLibrary->getConnection(userId);
   if(c != NULL)
   {
      MO_CAT_DEBUG(BM_CUSTOMCATALOG_CAT,
         "Populating payee scheme, ID: %" PRIu32,
         BM_PAYEE_SCHEME_ID(ps["id"]));

      // populate the payee scheme and list of payees
      rval = mCatalogDb.populatePayeeScheme(ps, filters, c);
      BM_ID_SET(ps["userId"], userId);
      c->close();
   }

   return rval;
}

bool Catalog::removePayeeScheme(UserId userId, PayeeSchemeId psId)
{
   bool rval = false;

   // get database connection
   Connection* c = mMediaLibrary->getConnection(userId);
   if(c != NULL)
   {
      // must do removal in a transaction so we don't see concurrency issues
      // between selecting wares that depend on the payee scheme and actually
      // removing the payee scheme
      rval = c->begin();
      if(rval)
      {
         MO_CAT_DEBUG(BM_CUSTOMCATALOG_CAT,
            "Removing payee scheme, ID: %" PRIu32, psId);

         // try to remove the payee scheme
         rval = mCatalogDb.removePayeeScheme(psId, c);

         // end transaction
         rval = rval ? c->commit() : c->rollback() && false;
      }

      // close connection
      c->close();
   }

   if(rval)
   {
      // schedule a payee scheme removed event
      Event e;
      e["type"] = EVENT_PAYEE_SCHEME ".removed";
      BM_ID_SET(e["details"]["payeeSchemeId"], psId);
      BM_ID_SET(e["details"]["userId"], userId);
      mNode->getEventController()->schedule(e);
   }

   return rval;
}

bool Catalog::updateSeller(
   UserId userId, Seller& seller, ServerToken serverToken)
{
   bool rval = false;

   // get database connection
   Connection* c = mMediaLibrary->getConnection(userId);
   if(c != NULL)
   {
      MO_CAT_DEBUG(BM_CUSTOMCATALOG_CAT,
         "Updating seller for user ID: %" PRIu64, userId);

      // begin transaction
      if((rval = c->begin()))
      {
         rval =
            mCatalogDb.setConfigValue(
               userId, "serverId", seller["serverId"]->getString(), c) &&
            mCatalogDb.setConfigValue(
               userId, "serverToken", serverToken, c) &&
            mCatalogDb.setConfigValue(
               userId, "serverUrl", seller["url"]->getString(), c);

         // commit transaction
         rval = rval ? c->commit() : c->rollback() && false;
      }

      // close connection
      c->close();
   }

   return rval;
}

bool Catalog::populateSeller(UserId userId, Seller& seller)
{
   string serverToken;
   return populateSeller(userId, seller, serverToken);
}

bool Catalog::populateSeller(
   UserId userId, Seller& seller, string& serverToken)
{
   bool rval = false;

   // get database connection
   Connection* c = mMediaLibrary->getConnection(userId);
   if(c != NULL)
   {
      MO_CAT_DEBUG(BM_CUSTOMCATALOG_CAT,
         "Populating seller configuration for ID: %" PRIu64, userId);

      // populate the seller configuration
      rval = mCatalogDb.populateSeller(userId, seller, serverToken, c);

      if(!rval)
      {
         ExceptionRef e = new Exception(
            "Failed to populate seller information for the given user ID.",
            "bitmunk.catalog.PopulateSellerFailed");
         BM_ID_SET(e->getDetails()["userId"], userId);
         Exception::push(e);
      }

      // close connection
      c->close();
   }

   return rval;
}

bool Catalog::getUpdateId(UserId userId, uint32_t& updateId)
{
   bool rval = false;

   // get database connection
   Connection* c = mMediaLibrary->getConnection(userId);
   if(c != NULL)
   {
      MO_CAT_DEBUG(BM_CUSTOMCATALOG_CAT,
         "Getting current update ID for user ID: %" PRIu64, userId);

      rval = mCatalogDb.getConfigValue("updateId", updateId, c);

      // close connection
      c->close();
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Failed to get current update ID for the given userId.",
         "bitmunk.catalog.GetUpdateIdFailed");
      BM_ID_SET(e->getDetails()["userId"], userId);
      Exception::push(e);
   }

   return rval;
}

bool Catalog::populateHeartbeatListingUpdate(
   UserId userId, SellerListingUpdate& update)
{
   bool rval = false;

   // get database connection
   Connection* c = mMediaLibrary->getConnection(userId);
   if(c != NULL)
   {
      MO_CAT_DEBUG(BM_CUSTOMCATALOG_CAT,
         "Populating heartbeat seller listing update for user ID: %" PRIu64,
         userId);

      // populate the seller configuration
      Seller seller;
      string serverToken;
      uint32_t updateId;
      if((rval =
         mCatalogDb.populateSeller(userId, seller, serverToken, c) &&
         mCatalogDb.getConfigValue("updateId", updateId, c)))
      {
         update["seller"] = seller;
         update["serverToken"] = serverToken.c_str();
         update["updateId"] = updateId;
         update["updateId"]->setType(String);
         update["newUpdateId"] = updateId;
         update["newUpdateId"]->setType(String);
      }
      else
      {
         ExceptionRef e = new Exception(
            "Failed to retrieve seller configuration details.",
            "bitmunk.catalog.InvalidConfigurationDetails");
         Exception::push(e);
      }

      // close connection
      c->close();
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Failed to retrieve a heartbeat listing update for the given userId.",
         "bitmunk.catalog.PopulateHeartbeatListingUpdateFailed");
      BM_ID_SET(e->getDetails()["userId"], userId);
      Exception::push(e);
   }

   return rval;
}

bool Catalog::populatePendingListingUpdate(
   UserId userId, SellerListingUpdate& update)
{
   bool rval = false;

   // get database connection
   Connection* c = mMediaLibrary->getConnection(userId);
   if(c != NULL)
   {
      MO_CAT_DEBUG(BM_CUSTOMCATALOG_CAT,
         "Populating pending seller listing update for user ID: %" PRIu64,
          userId);

      // FIXME: why do we need to do the update in a transaction?
      if(c->begin())
      {
         // populate the seller configuration
         Seller seller;
         string serverToken;
         uint32_t updateId;
         if((rval =
            mCatalogDb.populateSeller(userId, seller, serverToken, c) &&
            mCatalogDb.getConfigValue("updateId", updateId, c)))
         {
            // we need to increase our update ID by 1 to match the
            // server since we're re-doing an update that the server
            // already processed
            update["seller"] = seller;
            update["serverToken"] = serverToken.c_str();
            update["updateId"] = updateId + 1;
            update["updateId"]->setType(String);
            update["newUpdateId"] = updateId + 1;
            update["newUpdateId"]->setType(String);

            // retrieve the list of currently updating seller listings
            rval = mCatalogDb.populateUpdatingSellerListings(
               userId, BM_SERVER_ID(seller["serverId"]), mMediaLibrary,
               update["listings"], update["payeeSchemes"], c);
         }
         else
         {
            ExceptionRef e = new Exception(
               "Failed to retrieve seller configuration details.",
               "bitmunk.catalog.InvalidConfigurationDetails");
            Exception::push(e);
         }

         // FIXME: commit transaction - is this needed?
         rval = rval ? c->commit() : c->rollback() && false;
      }

      // close connection
      c->close();
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Failed to retrieve the list of pending updates for the given userId.",
         "bitmunk.catalog.PopulatePendingListingUpdateFailed");
      BM_ID_SET(e->getDetails()["userId"], userId);
      Exception::push(e);
   }

   return rval;
}

bool Catalog::populateNextListingUpdate(
   UserId userId, SellerListingUpdate& update)
{
   bool rval = false;

   // Note: If there is nothing to update, then our next listing update
   // will just be a heartbeat with the same update ID as the current one.

   // get database connection
   Connection* c = mMediaLibrary->getConnection(userId);
   if(c != NULL)
   {
      MO_CAT_DEBUG(BM_CUSTOMCATALOG_CAT,
         "Populating next seller listing update for user ID: %" PRIu64,
         userId);

      // do update in a transaction
      if(c->begin())
      {
         uint32_t updateId;
         uint32_t newUpdateId;

         Seller seller;
         string serverToken;
         if((rval =
            mCatalogDb.populateSeller(userId, seller, serverToken, c) &&
            mCatalogDb.getConfigValue("updateId", updateId, c)))
         {
            // get new update ID that will be applied to any dirty/pending
            // wares ... if there aren't any, this won't be saved anywhere
            // and we can just throw it out and do a heartbeat instead
            newUpdateId = updateId + 1;

            // mark next set of dirty wares as "updating", clean them, and then
            // retrieve them
            rval =
               mCatalogDb.markNextListingUpdate(c) &&
               mCatalogDb.populateUpdatingSellerListings(
                  userId, BM_SERVER_ID(seller["serverId"]), mMediaLibrary,
                  update["listings"], update["payeeSchemes"], c);
            if(rval)
            {
               update["seller"] = seller;
               update["serverToken"] = serverToken.c_str();
               update["updateId"] = updateId;
               update["updateId"]->setType(String);

               // only use a new update ID if there are updates
               if(update["listings"]["updates"]->length() > 0 ||
                  update["listings"]["removals"]->length() > 0 ||
                  update["payeeSchemes"]["updates"]->length() > 0 ||
                  update["payeeSchemes"]["removals"]->length() > 0)
               {
                  // we're updating/removing something, so change update ID
                  update["newUpdateId"] = newUpdateId;
               }
               else
               {
                  // keep same update ID, no updates/removals
                  update["newUpdateId"] = updateId;
               }
               update["newUpdateId"]->setType(String);
            }
         }

         // commit transaction
         rval = rval ? c->commit() : c->rollback() && false;
      }

      // close connection
      c->close();
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Failed to generate the list of seller listing updates.",
         "bitmunk.catalog.PopulateNextListingUpdateFailed");
      BM_ID_SET(e->getDetails()["userId"], userId);
      Exception::push(e);
   }

   return rval;
}

bool Catalog::processListingUpdateResponse(
   UserId userId, SellerListingUpdate update, SellerListingUpdate result)
{
   bool rval = false;

   // for storing wares and schemes with problems
   DynamicObject problemWares;
   problemWares->setType(Array);
   DynamicObject problemSchemes;
   problemSchemes->setType(Array);

   // for storing updated wares and schemes that did not have problems
   DynamicObject goodWares;
   goodWares->setType(Array);
   DynamicObject goodSchemes;
   goodSchemes->setType(Array);

   // get successful updated wares
   {
      DynamicObjectIterator i = update["listings"]["updates"].getIterator();
      while(i->hasNext())
      {
         SellerListing& sl = i->next();
         string wareId = StringTools::format(
            "bitmunk:file:%" PRIu64 "-%s",
            BM_MEDIA_ID(sl["fileInfo"]["mediaId"]),
            BM_FILE_ID(sl["fileInfo"]["id"]));
         goodWares[wareId.c_str()] = true;
      }
   }

   // get successful updated schemes
   {
      DynamicObjectIterator i = update["payeeSchemes"]["updates"].getIterator();
      while(i->hasNext())
      {
         PayeeScheme& ps = i->next();
         string psId = ps["id"]->getString();
         goodSchemes[psId.c_str()] = true;
      }
   }

   // get database connection
   Connection* c = mMediaLibrary->getConnection(userId);
   if(c != NULL)
   {
      MO_CAT_DEBUG(BM_CUSTOMCATALOG_CAT,
         "Processing seller listing update response for user ID: %" PRIu64,
         userId);

      // start a transaction
      if((rval = c->begin()))
      {
         // any failures will be listed in the updates and removals lists
         // for listings and payee schemes... we can merge update and
         // removal failures together because their problems are handled
         // the same way... for each one:

         // 1. Produce a problem string and get its associated problem ID.
         // 2. Mark the related ware or payee scheme with the given problem
         //    ID unless it has been marked as dirty (it has been changed
         //    during our update and needs to be sent to the server again).

         // merge problem updates and removals together:
         DynamicObject failedListings;
         failedListings->setType(Array);
         result["listings"]["updates"]->setType(Array);
         result["listings"]["removals"]->setType(Array);
         failedListings.merge(result["listings"]["updates"], true);
         failedListings.merge(result["listings"]["removals"], true);

         DynamicObject failedSchemes;
         failedSchemes->setType(Array);
         result["payeeSchemes"]["updates"]->setType(Array);
         result["payeeSchemes"]["removals"]->setType(Array);
         failedSchemes.merge(result["payeeSchemes"]["updates"], true);
         failedSchemes.merge(result["payeeSchemes"]["removals"], true);

         // handle problem listings first
         SellerListingIterator sli = failedListings.getIterator();
         while(rval && sli->hasNext())
         {
            SellerListing& listing = sli->next();
            uint64_t problemId = 0;
            string json = JsonWriter::writeToString(listing["exception"], true);

            // save problem ware for sending event
            DynamicObject& d = problemWares->append();
            string wareId = StringTools::format(
               "bitmunk:file:%" PRIu64 "-%s",
               BM_MEDIA_ID(listing["fileInfo"]["mediaId"]),
               BM_FILE_ID(listing["fileInfo"]["id"]));
            BM_ID_SET(d["wareId"], wareId.c_str());
            d["exception"] = listing["exception"].clone();

            // update the ware problem ID
            rval =
               mCatalogDb.getProblemId(json.c_str(), problemId, c) &&
               mCatalogDb.updateWareProblemId(
                  problemId, BM_FILE_ID(listing["fileInfo"]["id"]),
                  BM_MEDIA_ID(listing["fileInfo"]["mediaId"]), c);

            // remove ware ID from good wares
            goodWares->removeMember(wareId.c_str());
         }

         // handle problem payee schemes
         PayeeSchemeIterator psi = failedSchemes.getIterator();
         while(rval && psi->hasNext())
         {
            PayeeScheme& ps = psi->next();
            uint64_t problemId = 0;
            string json = JsonWriter::writeToString(ps["exception"], true);

            // save problem scheme for sending event
            DynamicObject& d = problemSchemes->append();
            BM_ID_SET(d["psId"], BM_PAYEE_SCHEME_ID(ps["id"]));
            d["exception"] = ps["exception"].clone();

            rval =
               mCatalogDb.getProblemId(json.c_str(), problemId, c) &&
               mCatalogDb.updatePayeeSchemeProblemId(
                  problemId, BM_PAYEE_SCHEME_ID(ps["id"]), c);

            // remove ps ID from good schemes
            string psId = ps["id"]->getString();
            goodSchemes->removeMember(psId.c_str());
         }

         rval =
            // delete all "updating" wares with "deleted" set and "dirty"
            // not set and where there is no problem set
            mCatalogDb.purgeDeletedEntries(CC_TABLE_WARES, c) &&
            // clear all ware "updating" flags
            mCatalogDb.clearUpdatingFlags(CC_TABLE_WARES, c) &&
            // delete all "updating" payee schemes with "deleted" set and
            // "dirty" not set and where there is no problem set
            mCatalogDb.purgeDeletedEntries(CC_TABLE_PAYEE_SCHEMES, c) &&
            // clear all payee scheme "updating" flags
            mCatalogDb.clearUpdatingFlags(CC_TABLE_PAYEE_SCHEMES, c) &&
            // clean up all non-referenced problems
            mCatalogDb.removeUnreferencedProblems(c);

         // update the catalog updateID to the new updateId
         if(rval)
         {
            rval = mCatalogDb.setConfigValue(
               userId, "updateId", result["updateId"]->getString(), c);
         }

         // commit transaction
         rval = rval ? c->commit() : c->rollback() && false;
      }

      // close connection
      c->close();
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Failed to process seller listing update response.",
         "bitmunk.catalog.ProcessListingUpdateResponseFailed");
      e->getDetails()["userId"] = userId;
      Exception::push(e);
   }
   else
   {
      // fire events for good wares
      {
         DynamicObjectIterator i = goodWares.getIterator();
         while(i->hasNext())
         {
            // schedule a ware updated event
            i->next();
            Event e;
            e["type"] = EVENT_WARE ".updated";
            BM_ID_SET(e["details"]["wareId"], i->getName());
            BM_ID_SET(e["details"]["userId"], userId);
            e["details"]["isNew"] = false;
            mNode->getEventController()->schedule(e);
         }
      }

      // fire events for problem wares
      {
         DynamicObjectIterator i = problemWares.getIterator();
         while(i->hasNext())
         {
            // schedule a ware exception event
            DynamicObject& d = i->next();
            Event e;
            e["type"] = EVENT_WARE ".exception";
            BM_ID_SET(e["details"]["wareId"], BM_WARE_ID(d["wareId"]));
            BM_ID_SET(e["details"]["userId"], userId);
            e["details"]["exception"] = d["exception"];
            mNode->getEventController()->schedule(e);
         }
      }

      // fire events for good schemes
      {
         DynamicObjectIterator i = goodSchemes.getIterator();
         while(i->hasNext())
         {
            // schedule a payee scheme updated event
            i->next();
            Event e;
            e["type"] = EVENT_PAYEE_SCHEME ".updated";
            BM_ID_SET(e["details"]["payeeSchemeId"], i->getName());
            BM_ID_SET(e["details"]["userId"], userId);
            e["details"]["isNew"] = false;
            mNode->getEventController()->schedule(e);
         }
      }

      // fire events for problem schemes
      {
         DynamicObjectIterator i = problemSchemes.getIterator();
         while(i->hasNext())
         {
            // schedule a payee scheme exception event
            DynamicObject& d = i->next();
            Event e;
            e["type"] = EVENT_PAYEE_SCHEME ".exception";
            BM_ID_SET(
               e["details"]["payeeSchemeId"],
               BM_PAYEE_SCHEME_ID(d["psId"]));
            BM_ID_SET(e["details"]["userId"], userId);
            e["details"]["exception"] = d["exception"];
            mNode->getEventController()->schedule(e);
         }
      }
   }

   return rval;
}

bool Catalog::resetListingUpdateCounter(UserId userId)
{
   bool rval = false;

   // get database connection
   Connection* c = mMediaLibrary->getConnection(userId);
   if(c != NULL)
   {
      MO_CAT_DEBUG(BM_CUSTOMCATALOG_CAT,
         "Resetting seller listing update counter for user ID: %" PRIu64,
         userId);

      // start a transaction
      if((rval = c->begin()))
      {
         bool dirty = true;
         bool updating = false;

         // reset update ID, server ID, and serverToken
         rval =
            mCatalogDb.setConfigValue(userId, "updateId", "0", c) &&
            mCatalogDb.setConfigValue(userId, "serverId", "0", c) &&
            mCatalogDb.setConfigValue(userId, "serverToken", "0", c) &&
            // mark all wares as dirty, clear all updating flags
            mCatalogDb.setTableFlags(CC_TABLE_WARES, dirty, updating, c) &&
            // mark all payee schemes as dirty, clear all updating flags
            mCatalogDb.setTableFlags(
               CC_TABLE_PAYEE_SCHEMES, dirty, updating, c);

         // commit transaction
         rval = rval ? c->commit() : c->rollback() && false;
      }

      // close connection
      c->close();
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Failed to reset seller listing update counter.",
         "bitmunk.catalog.ResetListingUpdateCounterFailed");
      e->getDetails()["userId"] = userId;
      Exception::push(e);
   }
   else
   {
      MO_CAT_DEBUG(BM_CUSTOMCATALOG_CAT,
         "Seller listing update counter reset for user ID: %" PRIu64,
         userId);
   }

   return rval;
}

bool Catalog::setConfigValue(
   UserId userId, const char* name, const char* value)
{
   bool rval = false;

   Connection* c = mMediaLibrary->getConnection(userId);
   if(c != NULL)
   {
      rval = mCatalogDb.setConfigValue(userId, name, value, c);
      c->close();
   }

   return rval;
}

bool Catalog::getConfigValue(UserId userId, const char* name, string& value)
{
   bool rval = false;

   Connection* c = mMediaLibrary->getConnection(userId);
   if(c != NULL)
   {
      rval = mCatalogDb.getConfigValue(name, value, c);
      c->close();
   }

   return rval;
}

void Catalog::userLoggedIn(Event& e)
{
   // get user's config
   UserId userId = BM_USER_ID(e["details"]["userId"]);
   Config cfg = mNode->getConfigManager()->getModuleUserConfig(
      "bitmunk.catalog.CustomCatalog", userId);

   MO_CAT_DEBUG(BM_CUSTOMCATALOG_CAT,
      "Initializing catalog for user %" PRIu64, userId);

   // get the user's first payee scheme (this will also initialize the user's
   // media library if it hasn't been already, so do not remove this code)
   bool pass = false;
   PayeeSchemeId psId = 0;
   Connection* c = mMediaLibrary->getConnection(userId);
   if(c != NULL)
   {
      pass = mCatalogDb.getFirstPayeeSchemeId(psId, c);
      c->close();
   }

   // if the user has no payee schemes, auto sell is enabled and
   // has a payee scheme ID set to zero, then create a default payee
   // scheme
   if(pass && !BM_PAYEE_SCHEME_ID_VALID(psId) &&
      !cfg.isNull() && cfg["autoSell"]["enabled"]->getBoolean() &&
      !BM_PAYEE_SCHEME_ID_VALID(
         BM_PAYEE_SCHEME_ID(cfg["autoSell"]["payeeSchemeId"])))
   {
      // get the user's first account
      Messenger* m = mNode->getMessenger();
      monarch::net::Url url;
      url.format(
         "/api/3.0/accounts?owner=%" PRIu64
         "&deleted=false&start=0&num=1\n", userId);
      DynamicObject in;
      pass = m->getSecureFromBitmunk(&url, in, userId);
      if(pass)
      {
         if(in["resources"]->length() == 0)
         {
            MO_CAT_ERROR(BM_CUSTOMCATALOG_CAT,
               "Could not create auto-generated auto-sell payee scheme, "
               "user %" PRIu64 " has no financial accounts.", userId);
         }
         else
         {
            // FIXME: do we want the default amount to be configurable as well?
            Payee payee;
            BM_ID_SET(payee["id"], BM_ACCOUNT_ID(in["resources"][0]["id"]));
            payee["description"] = "Auto-generated payee";
            payee["amountType"] = "flatFee";
            payee["amount"] = "0.05";
            payee["percentage"] = "0.00";
            payee["min"] = "0.00";

            PayeeList payees;
            payees->append(payee);

            // do payee insert in a database transaction to avoid silliness
            c = mMediaLibrary->getConnection(userId);
            pass = (c != NULL);
            if(pass)
            {
               pass = c->begin();
               if(pass)
               {
                  // ensure user still has no payee schemes
                  pass = mCatalogDb.getFirstPayeeSchemeId(psId, c);
                  if(pass && psId == 0)
                  {
                     pass = mCatalogDb.addPayeeScheme(
                        mNode, userId, psId,
                        "Auto-generated payment scheme", payees, c);
                  }

                  pass = pass ? c->commit() : c->rollback() && pass;
               }

               c->close();

               if(pass)
               {
                  PayeeScheme ps;
                  ps["id"] = psId;
                  ps["payees"] = payees;
                  MO_CAT_DEBUG(BM_CUSTOMCATALOG_CAT,
                     "Created auto-generated auto-sell payee scheme for "
                     "user %" PRIu64 ": %s", userId,
                     JsonWriter::writeToString(ps).c_str());

                  // update auto-sell config
                  cfg = mNode->getConfigManager()->getUserConfig(userId, true);
                  Config& asCfg = cfg
                     [ConfigManager::MERGE]
                     ["bitmunk.catalog.CustomCatalog"]["autoSell"];
                  asCfg["enabled"] = true;
                  BM_ID_SET(asCfg["payeeSchemeId"], psId);
                  mNode->getConfigManager()->getConfigManager()->setConfig(cfg);
                  mNode->getConfigManager()->saveUserConfig(userId);
               }
            }
         }
      }
   }

   if(!pass)
   {
      MO_CAT_ERROR(BM_CUSTOMCATALOG_CAT,
         "Could not create auto-generated auto-sell payee scheme for "
         "user %" PRIu64 ": %s", userId,
         JsonWriter::writeToString(Exception::getAsDynamicObject()).c_str());
   }
}

void Catalog::checkAutoSell(Event& e)
{
   // check user's config
   UserId userId = BM_USER_ID(e["details"]["userId"]);
   Config cfg = mNode->getConfigManager()->getModuleUserConfig(
      "bitmunk.catalog.CustomCatalog", userId);
   if(!cfg.isNull() && cfg["autoSell"]["enabled"]->getBoolean())
   {
      // FIXME: add ware entry regardless of re-sellability?
      // FIXME: check purchasedWare["media"] distribution rules for "peer"?
      // FIXME: remember media may be a collection
      // FIXME: add an ability to check all unsellable wares with that flag at
      // a later point in time?
      //Ware purchasedWare = e["details"]["userData"]["ware"];

      // create a ware for the file
      Ware ware;
      FileInfo fi = e["details"]["fileInfo"].clone();
      ware["fileInfos"]->append(fi);
      BM_ID_SET(ware["id"], StringTools::format("bitmunk:file:%" PRIu64 "-%s",
         BM_MEDIA_ID(fi["mediaId"]), BM_FILE_ID(fi["id"])).c_str());
      BM_ID_SET(ware["mediaId"], BM_MEDIA_ID(fi["mediaId"]));
      // FIXME: get description from config as a format string?
      ware["description"] = "";

      // use payee scheme ID from config
      ware["payeeSchemeId"] = BM_PAYEE_SCHEME_ID(
         cfg["autoSell"]["payeeSchemeId"]);

      // try to add ware
      if(updateWare(userId, ware))
      {
         MO_CAT_DEBUG(BM_CUSTOMCATALOG_CAT,
            "Auto-selling ware: %s", BM_WARE_ID(ware["id"]));
      }
      else
      {
         MO_CAT_ERROR(BM_CUSTOMCATALOG_CAT,
            "Could not auto-sell ware: %s",
            JsonWriter::writeToString(Exception::getAsDynamicObject()).c_str());
      }
   }
}

void Catalog::syncSellerListings(monarch::event::Event& e)
{
   // get ID of user with listings to sync
   UserId userId = BM_USER_ID(e["details"]["userId"]);

   // get associated fiber ID
   ListingUpdaterMap::iterator i = mListingUpdaterMap.find(userId);
   if(i != mListingUpdaterMap.end())
   {
      // send update request message to fiber
      DynamicObject msg;
      msg["updateRequest"] = true;
      mNode->getFiberMessageCenter()->sendMessage(i->second, msg);
   }
}

void Catalog::testNetAccess(monarch::event::Event& e)
{
   // get ID of user
   UserId userId = BM_USER_ID(e["details"]["userId"]);

   // get associated fiber ID
   ListingUpdaterMap::iterator i = mListingUpdaterMap.find(userId);
   if(i != mListingUpdaterMap.end())
   {
      // send test net access message to fiber
      DynamicObject msg;
      msg["testNetAccess"] = true;
      mNode->getFiberMessageCenter()->sendMessage(i->second, msg);
   }
}

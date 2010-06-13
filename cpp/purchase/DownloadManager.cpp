/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk/purchase/DownloadManager.h"

#include "bitmunk/common/Tools.h"
#include "bitmunk/purchase/Negotiator.h"
#include "bitmunk/purchase/PieceDownloader.h"
#include "bitmunk/purchase/PurchaseModule.h"
#include "bitmunk/purchase/SellerPicker.h"
#include "bitmunk/purchase/SellerPoolUpdater.h"
#include "monarch/config/ConfigManager.h"
#include "monarch/event/ObserverDelegate.h"
#include "monarch/io/File.h"
#include "monarch/util/Date.h"

#include <algorithm>
#include <cmath>

using namespace bitmunk::common;
using namespace bitmunk::purchase;
using namespace bitmunk::node;
using namespace monarch::config;
using namespace monarch::crypto;
using namespace monarch::event;
using namespace monarch::fiber;
using namespace monarch::io;
using namespace monarch::net;
using namespace monarch::rt;
using namespace monarch::util;
using namespace std;

#define EVENT_PURCHASE              "bitmunk.purchase"
#define EVENT_DOWNLOAD_MANAGER      EVENT_PURCHASE ".DownloadManager"
#define EVENT_DOWNLOAD_STATE        EVENT_PURCHASE ".DownloadState"
#define EVENT_CHECK_COMPLETION      EVENT_DOWNLOAD_MANAGER ".checkCompletion"
#define EVENT_POOL_TIMEOUT          EVENT_DOWNLOAD_MANAGER ".sellerPoolTimeout"
#define EVENT_PROGRESS_POLL         EVENT_DOWNLOAD_STATE ".progressPoll"
#define EVENT_POOLS_UPDATED         EVENT_DOWNLOAD_STATE ".sellerPoolsUpdated"

#define CHECK_COMPLETION_INTERVAL   1000 * 5      // 5 seconds
#define DEFAULT_POOL_TIMEOUT        1000 * 60 * 2 // 2 minutes
#define WINDOW_LENGTH               1000 * 1      // 1 second

// define internal message types
enum
{
   MsgInterrupt,
   MsgAssignOptionalPiece,
   MsgPieceUpdate,
   MsgPoolTimeout,
   MsgPoolUpdate,
   MsgPoolError,
   MsgNegotiateComplete,
   MsgPauseDownload,
   MsgProgressPoll,
   MsgIgnore
};

DownloadManager::DownloadManager(Node* node) :
   NodeFiber(node),
   DownloadStateFiber(node, "DownloadManager", &mFiberExitData),
   mSellerPoolsInitializing(true),
   mNegotiating(false),
   mSellerPoolsUpdating(false),
   mAssignOptionalPiece(false),
   mInterrupted(false),
   mPieceDownloaderId(0)
{
}

DownloadManager::~DownloadManager()
{
}

void DownloadManager::handleCheckCompletionEvent(Event& e)
{
   // send a message to self
   DynamicObject msg;
   msg["type"] = MsgAssignOptionalPiece;
   messageSelf(msg);
}

void DownloadManager::handleSellerPoolTimeoutEvent(Event& e)
{
   // send a message to self
   DynamicObject msg;
   msg["type"] = MsgPoolTimeout;
   messageSelf(msg);
}

void DownloadManager::handleProgressPollEvent(Event& e)
{
   // send a message to self
   DynamicObject msg;
   msg["type"] = MsgProgressPoll;
   messageSelf(msg);
}

void DownloadManager::processMessages()
{
   // set processor ID on download state
   mPurchaseDatabase->setDownloadStateProcessorId(mDownloadState, 0, getId());

   logDownloadStateMessage("starting...");

   // ensure download state has been initialized,
   // ensure license has been acquired and its price meets user's prefs
   // unassign any previously assigned but incomplete pieces
   bool error = !(checkInit() && checkLicense() && unassignPieces());
   if(!error)
   {
      // schedule a seller pool timeout to initialize the seller pool
      DynamicObject msg;
      msg["type"] = MsgPoolTimeout;
      messageSelf(msg);

      // register event handlers
      ObserverList* list = new ObserverList();
      registerEventHandlers(list);

      // handle messages
      error = !handleMessages();

      // unregister event handlers
      list->unregisterFrom(getNode()->getEventController());
      delete list;
   }

   // finished
   logDownloadStateMessage("exiting...");

   if(error)
   {
      // send error event
      sendErrorEvent();
   }

   // FIXME: download state processing needs to be changed to a queue
   // system where you can queue up what needs to happen next with the
   // download state ... and cancel certain actions if you want to ...
   // this is hacky and messy (consider this queue idea when doing the
   // redesign of the purchase module)

   // stop processing download state
   mPurchaseDatabase->stopProcessingDownloadState(mDownloadState, getId());
}

bool DownloadManager::checkInit()
{
   bool rval = true;

   if(!mDownloadState["initialized"]->getBoolean())
   {
      // download state not initialized, send event
      ExceptionRef e = new Exception(
         "Could not start download. Download state not initialized.",
         EVENT_DOWNLOAD_STATE ".NotInitialized");
      Exception::set(e);
      rval = false;
   }

   return rval;
}

bool DownloadManager::checkLicense()
{
   bool rval = true;

   // see if a signed license has been acquired
   if(!mDownloadState["licenseAcquired"]->getBoolean())
   {
      // no license acquired yet
      ExceptionRef e = new Exception(
         "Could not start download. No license acquired yet!",
         EVENT_DOWNLOAD_STATE ".MissingLicense");
      Exception::set(e);
      rval = false;
   }
   else
   {
      // ensure the license price is within the buyer's preferred price
      PurchasePreferences& prefs = mDownloadState["preferences"];
      Media& media = mDownloadState["contract"]["media"];
      BigDecimal max = prefs["price"]["max"]->getString();
      BigDecimal license = media["licenseAmount"]->getString();
      if(license > max)
      {
         // license is too pricey for user's preferences
         ExceptionRef e = new Exception(
            "Could not start download. License costs more than user's "
            "preferred price maximum.",
            EVENT_DOWNLOAD_STATE ".LicenseTooExpensive");
         e->getDetails()["price"]["max"] =
            prefs["price"]["max"]->getString();
         e->getDetails()["licenseAmount"] =
            media["licenseAmount"]->getString();
         Exception::set(e);
         rval = false;
      }
   }

   return rval;
}

bool DownloadManager::unassignPieces()
{
   FileProgressIterator fpi = mDownloadState["progress"].getIterator();
   while(fpi->hasNext())
   {
      FileProgress& progress = fpi->next();
      FilePieceIterator i = progress["assigned"].getIterator();
      while(i->hasNext())
      {
         progress["unassigned"].merge(i->next(), true);
      }
      progress["assigned"]->clear();
   }

   return true;
}

/**
 * A helper function for getting the purchase module config.
 *
 * @param node the node with the module.
 * @param userId the current user's ID.
 *
 * @return the config for the purchase module config.
 */
static Config getConfig(Node* node, UserId userId)
{
   return node->getConfigManager()->getModuleUserConfig(
      "bitmunk.purchase.Purchase", userId);
}

void DownloadManager::registerEventHandlers(monarch::event::ObserverList* list)
{
   // create event handlers
   ObserverRef h1(new ObserverDelegate<DownloadManager>(
      this, &DownloadManager::handleCheckCompletionEvent));
   ObserverRef h2(new ObserverDelegate<DownloadManager>(
      this, &DownloadManager::handleSellerPoolTimeoutEvent));
   ObserverRef h3(new ObserverDelegate<DownloadManager>(
      this, &DownloadManager::handleProgressPollEvent));

   // create event filter for this fiber
   EventFilter filter1;
   filter1["details"]["fiberId"] = getId();

   // create event filter for the download state
   UserId userId = BM_USER_ID(mDownloadState["userId"]);
   EventFilter filter2;
   BM_ID_SET(filter2["details"]["userId"], userId);
   filter2["details"]["downloadStateId"] = mDownloadState["id"]->getUInt64();

   // register to receive events
   getNode()->getEventController()->registerObserver(
      &(*h1), EVENT_CHECK_COMPLETION, &filter1);
   getNode()->getEventController()->registerObserver(
      &(*h2), EVENT_POOL_TIMEOUT, &filter1);
   getNode()->getEventController()->registerObserver(
      &(*h3), EVENT_PROGRESS_POLL, &filter2);

   // store event handlers for later unregistration
   list->add(h1);
   list->add(h2);
   list->add(h3);

   // get seller pool timeout
   Config cfg = getConfig(mNode, userId);
   if(!cfg.isNull())
   {
      uint64_t timeout = DEFAULT_POOL_TIMEOUT;
      if(cfg["DownloadManager"]->hasMember("sellerPoolTimeout"))
      {
         timeout = cfg["DownloadManager"]["sellerPoolTimeout"]->getUInt32();
      }

      // schedule event to indicate seller pool timeout
      Event e;
      e["type"] = EVENT_POOL_TIMEOUT;
      e["parallel"] = true;
      e["details"]["fiberId"] = getId();
      getNode()->getEventDaemon()->add(e, timeout, 1);
   }
}

/**
 * A helper function for getting the type of a message so it can be used
 * in a switch-statement to determine how to handle the message.
 *
 * @param msg the message to get the type of.
 * @param fiberId the ID of the current DownloadManager fiber.
 *
 * @return the message type.
 */
static int getMessageType(DynamicObject& msg, FiberId fiberId)
{
   // by default, ignore unknown messages
   int rval = MsgIgnore;

   // check origin of message, self-messages have types already assigned
   FiberId id = msg["fiberId"]->getUInt32();
   if(id == fiberId)
   {
      // get type from message
      rval = msg["type"]->getInt32();
   }
   else if(msg->hasMember("pieceReceived") || msg->hasMember("pieceFailed"))
   {
      rval = MsgPieceUpdate;
   }
   else if(msg->hasMember("negotiationComplete") ||
           msg->hasMember("negotiationError"))
   {
      rval = MsgNegotiateComplete;
   }
   else if(msg->hasMember("sellerPoolUpdateComplete"))
   {
      rval = MsgPoolUpdate;
   }
   else if(msg->hasMember("sellerPoolUpdateError"))
   {
      rval = MsgPoolError;
   }
   else if(msg->hasMember("pause"))
   {
      rval = MsgPauseDownload;
   }
   else if(msg->hasMember("interrupt"))
   {
      rval = MsgInterrupt;
   }

   return rval;
}

bool DownloadManager::handleMessages()
{
   bool rval = true;

   // send download started event
   {
      Event e;
      e["type"] = EVENT_DOWNLOAD_STATE ".downloadStarted";
      sendDownloadStateEvent(e);
   }

   // schedule check completion events
   {
      Event e;
      e["type"] = EVENT_CHECK_COMPLETION;
      e["parallel"] = true;
      e["details"]["fiberId"] = getId();
      getNode()->getEventDaemon()->add(e, CHECK_COMPLETION_INTERVAL, -1);
   }

   // continue handling any incoming messages until download stopped
   bool complete = false;
   while(rval && !mInterrupted && !(complete = isComplete()))
   {
      // if not negotiating and not initializing the seller pools,
      // assign a piece if appropriate
      if(!mNegotiating && !mSellerPoolsInitializing)
      {
         // if assigning a piece failed and there are no piece downloaders,
         // then set rval to false to quit, an exception will have been
         // set by tryAssignPiece() indicating no sellers are available
         if(!tryAssignPiece() && mPieceDownloaders.empty())
         {
            rval = false;
         }
      }

      if(rval && !mInterrupted)
      {
         // keep looping until a non-progress update message occurs
         int nonProgressUpdate = 0;
         while(rval && nonProgressUpdate == 0)
         {
            // wait for incoming message(s)
            sleep();

            // process messages
            FiberMessageQueue* msgs = getMessages();
            while(!msgs->empty())
            {
               // get first message
               DynamicObject& msg = msgs->front();

               // take note of non-progress updates
               if(getMessageType(msg, getId()) != MsgProgressPoll)
               {
                  ++nonProgressUpdate;
               }

               // handle the message
               rval = handleMessage(msg);
               if(!rval)
               {
                  // interrupt download because of failure
                  getNode()->interruptFiber(getId());
               }

               // pop message
               msgs->pop_front();

               // yield if there is another message to process or
               // if we will break out of the non-progress update loop
               // before sleeping again
               if(nonProgressUpdate != 0 || !msgs->empty())
               {
                  yield();
               }
            }
         }
      }
   }

   // remove check completion events
   {
      Event e;
      e["type"] = EVENT_CHECK_COMPLETION;
      e["parallel"] = true;
      e["details"]["fiberId"] = getId();
      getNode()->getEventDaemon()->remove(e);
   }

   logDownloadStateMessage("waiting for child fibers to exit...");

   // wait for child fibers to finish:
   // all piece downloaders
   // negotiator
   // seller pool updater
   {
      const char* keys[] = {
         "pieceReceived", "pieceFailed",
         "negotiationComplete", "negotiationError",
         "sellerPoolUpdateComplete", "sellerPoolUpdateError",
         NULL};
      while(mNegotiating || mSellerPoolsUpdating || !mPieceDownloaders.empty())
      {
         DynamicObject list = waitForMessages(keys);
         DynamicObjectIterator i = list.getIterator();
         while(i->hasNext())
         {
            DynamicObject& msg = i->next();
            switch(getMessageType(msg, getId()))
            {
               case MsgPieceUpdate:
                  pieceUpdate(msg);
                  // yield if there is another message to process
                  if(i->hasNext())
                  {
                     yield();
                  }
                  break;
               case MsgNegotiateComplete:
                  mNegotiating = false;
                  break;
               case MsgPoolUpdate:
               case MsgPoolError:
                  mSellerPoolsUpdating = false;
                  break;
               default:
                  // ignore
                  break;
            }
         }
      }
   }

   logDownloadStateMessage("all child fibers exited");

   // stop processing download state
   mPurchaseDatabase->stopProcessingDownloadState(mDownloadState, getId());

   if(mInterrupted)
   {
      // send interrupted event
      {
         Event e;
         e["type"] = EVENT_DOWNLOAD_STATE ".downloadInterrupted";
         e["details"]["deleting"] = mDownloadState["deleting"]->getBoolean();
         sendDownloadStateEvent(e);
      }

      // if not deleting the download state, send download paused event
      if(!mDownloadState["deleting"]->getBoolean())
      {
         Event e;
         e["type"] = EVENT_DOWNLOAD_STATE ".downloadPaused";
         sendDownloadStateEvent(e);
      }
   }

   // send download stopped event
   {
      Event e;
      e["type"] = EVENT_DOWNLOAD_STATE ".downloadStopped";
      e["completed"] = (complete && !mInterrupted);
      e["interrupted"] = mInterrupted;
      sendDownloadStateEvent(e);
   }

   if(complete && !mInterrupted)
   {
      // send download completed event
      Event e;
      e["type"] = EVENT_DOWNLOAD_STATE ".downloadCompleted";
      sendDownloadStateEvent(e);
   }

   return rval;
}

bool DownloadManager::handleMessage(DynamicObject& msg)
{
   bool rval = true;

   // get the type on the message
   int msgType = getMessageType(msg, getId());
   switch(msgType)
   {
      case MsgInterrupt:
         rval = interrupt(msg);
         break;
      case MsgAssignOptionalPiece:
         mAssignOptionalPiece = true;
         break;
      case MsgPieceUpdate:
         rval = pieceUpdate(msg);
         break;
      case MsgPoolTimeout:
         rval = poolTimeout(msg);
         break;
      case MsgPoolUpdate:
         rval = poolUpdate(msg);
         break;
      case MsgPoolError:
      {
         // ignore: message is non-fatal and existing seller pools may be
         // sufficient to assign pieces and complete download, but we are
         // also no longer performing the initial seller pool refresh
         mSellerPoolsInitializing = false;
         mSellerPoolsUpdating = false;
         break;
      };
      case MsgNegotiateComplete:
         rval = negotiationComplete(msg);
         break;
      case MsgPauseDownload:
         rval = pauseDownload(msg);
         break;
      case MsgProgressPoll:
         rval = progressPolled(msg);
         break;
      case MsgIgnore:
         // do nothing
         break;
      default:
         // garbage message, ignore
         MO_CAT_WARNING(BM_PURCHASE_CAT,
            "Ignoring unknown DownloadManager message type: %i", msgType);
         break;
   }

   return rval;
}

bool DownloadManager::isComplete()
{
   bool rval = false;

   if(mDownloadState["remainingPieces"]->getUInt32() == 0)
   {
      // FIXME:
      // double check that there are file piece files for every piece and
      // that they are readable
      /*
      FileProgressIterator fpi = mDownloadState["progress"].getIterator();
      while(fpi->hasNext())
      {
      }
      */
      // FIXME: if above test does not fail, then complete, else reset
      // pieces that failed to unassigned state

      MO_CAT_DEBUG(BM_PURCHASE_CAT,
         "All pieces received, uid: %" PRIu64 ", dsid: %" PRIu64,
         BM_USER_ID(mDownloadState["userId"]),
         mDownloadState["id"]->getUInt64());

      logDownloadStateMessage("all pieces received");
      rval = true;

      // process progress update message for download completion
      DynamicObject msg;
      msg["type"] = MsgProgressPoll;
      progressPolled(msg);
   }

   return rval;
}

bool DownloadManager::tryAssignPiece()
{
   bool rval = true;

   // determine if a piece should be assigned
   bool must;
   if(shouldAssignPiece(must))
   {
      // calculate file costs
      mFileCostCalculator.calculate(this, mDownloadState);

      // pick a seller
      SellerData sd(NULL);
      if(pickSeller(sd, must))
      {
         // assign piece to seller
         rval = assignPiece(sd);
      }
      else if(mNegotiating)
      {
         // no piece to assign yet, doing negotiation
         rval = true;
      }
      else if(must)
      {
         // piece *had* to be assigned but no seller could be picked
         rval = false;
      }
   }

   return rval;
}

bool DownloadManager::shouldAssignPiece(bool& must)
{
   bool rval = false;

   // check all files for assigned pieces and to see if there is an
   // unassigned piece
   must = false;
   uint32_t assigned = 0;
   bool unassigned = false;
   FileProgressIterator fpi = mDownloadState["progress"].getIterator();
   while(fpi->hasNext())
   {
      FileProgress& fp = fpi->next();
      if(fp["assigned"]->length() > 0)
      {
         assigned += fp["assigned"]->length();
      }
      if(!unassigned && fp["unassigned"]->length() > 0)
      {
         // at least one piece has not been assigned
         unassigned = true;
      }
   }

   // if there are no assigned pieces, then we *must* assign one
   if(assigned == 0)
   {
      must = true;
      rval = true;
   }
   // if there is an unassigned piece, then see if we should assign it
   else if(unassigned && mAssignOptionalPiece)
   {
      mAssignOptionalPiece = false;

      // if we aren't at our cap for maximum concurrent pieces and there
      // is enough unused excess bandwidth to allow for another piece to
      // be assigned, then we should do so
      UserId userId = BM_USER_ID(mDownloadState["userId"]);
      Config cfg = getConfig(mNode, userId);
      if(!cfg.isNull())
      {
         // check maximum number of concurrent pieces (0 = unlimited)
         uint32_t maxPieces = cfg["maxPieces"]->getUInt32();
         if(maxPieces == 0 || assigned < maxPieces)
         {
            // calculate current excess bandwidth in bytes
            uint32_t maxExcess = cfg["maxExcessBandwidth"]->getUInt32();
            int rate = (int)mTotalDownloadRate->getItemsPerSecond();
            int rateLimit = mDownloadThrottler->getRateLimit();
            uint32_t excess = (rate > rateLimit) ? 0 : rateLimit - rate;

            // if rate is greater than 0 (we're downloading) and
            // either we have no rate limit or we have no maximum excess
            // bandwidth or the excess bandwidth we've calculated is
            // greater than our maximum excess allowed, then try to
            // get another piece concurrently
            if(rate > 0 &&
               (rateLimit == 0 || maxExcess == 0 || excess > maxExcess))
            {
               rval = true;
               logDownloadStateMessage(
                  "unused excess bandwidth indicates another "
                  "piece should be downloaded");
            }
         }
      }
   }

   return rval;
}

bool DownloadManager::pickSeller(SellerData& sd, bool must)
{
   bool rval = false;

   logDownloadStateMessage("choosing seller to assign a piece to...");

   // combine all of the current sellers into a single list of SellerData
   // excluding sellers for complete files, "bad" sellers, sellers with
   // already assigned pieces, and sellers over their associated budget,
   // also keep track of the total seller count
   SellerDataList list;
   list->setType(Array);
   string key;
   BigDecimal price;
   price.setPrecision(7, Down);
   BigDecimal budget;
   budget.setPrecision(7, Down);
   int sellerCount = 0;
   FileProgressIterator fpi = mDownloadState["progress"].getIterator();
   while(fpi->hasNext())
   {
      FileProgress& fp = fpi->next();

      // count pieces already downloaded
      uint32_t downloaded = 0;
      DynamicObjectIterator i = fp["downloaded"].getIterator();
      while(i->hasNext())
      {
         downloaded += i->next()->length();
      }

      // if there are more pieces to be downloaded for this
      // particular file, then include its seller count
      if(fp["sellerPool"]["pieceCount"]->getUInt32() > downloaded)
      {
         sellerCount += fp["sellerData"]->length();
      }

      // only look to add sellers to assign pieces to if there
      // are unassigned pieces left for this file
      if(fp["unassigned"]->length() > 0)
      {
         budget = fp["budget"]->getString();
         DynamicObjectIterator i = fp["sellerData"].getIterator();
         while(i->hasNext())
         {
            SellerData& fpsd = i->next();
            key = Tools::createSellerServerKey(fpsd["seller"]);
            // FIXME: "activeSellers" contains # of connections to
            // sellers, could allow the number to go higher than 1 in
            // the future by checking its value instead of ->hasMember()
            if(key.length() > 0 &&
               !mDownloadState["blacklist"]->hasMember(key.c_str()) &&
               !mDownloadState["activeSellers"]->hasMember(key.c_str()))
            {
               price = fpsd["price"]->getString();
               if(price <= budget)
               {
                  list->append(fpsd);
               }
            }

            yield();
         }
      }
   }

   // get buyer's preferences
   PurchasePreferences& prefs = mDownloadState["preferences"];

   // pick the seller
   if(SellerPicker::pickSeller(prefs, list, sd))
   {
      MO_CAT_ERROR(BM_PURCHASE_CAT,
         "Picked seller %s, uid: %" PRIu64 ", dsid: %" PRIu64,
         Tools::createSellerServerKey(sd["seller"]).c_str(),
         BM_USER_ID(mDownloadState["userId"]),
         mDownloadState["id"]->getUInt64());

      rval = true;
   }
   else
   {
      // FIXME: we want to set a default seller limit somewhere if a
      // user doesn't specify one in their preferences so as to avoid
      // crazy overflow of sellers -- also, we want to add some code
      // to throw out "bad" sellers with whom we have negotiated but
      // never downloaded anything from, so as to free up spots for
      // "good" sellers

      // determine if sellers maxed out
      if(sellerCount < prefs["sellerLimit"]->getInt32())
      {
         logDownloadStateMessage(
            "no suitable seller found, negotiating with a new one");

         // set negotiating flag
         mNegotiating = true;

         // schedule negotiator fiber
         Negotiator* n = new Negotiator(getNode(), getId());
         n->setUserId(BM_USER_ID(mDownloadState["userId"]));
         n->setDownloadState(mDownloadState);
         n->setMustFindSeller(must);
         getNode()->getFiberScheduler()->addFiber(n);
      }
      // determine if we *must* assign a seller but cannot
      else if(must)
      {
         MO_CAT_ERROR(BM_PURCHASE_CAT,
            "Cannot assign a seller, no suitable seller available, "
            "uid: %" PRIu64 ", dsid: %" PRIu64,
            BM_USER_ID(mDownloadState["userId"]),
            mDownloadState["id"]->getUInt64());

         ExceptionRef e = new Exception(
            "There are no sellers available that match the buyer's "
            "purchase preferences. A higher maximum price may be required to "
            "continue because a seller with a low enough price has gone "
            "offline.",
            EVENT_DOWNLOAD_STATE ".NoSellersAvailable");
         Exception::set(e);

         // issue an immediate seller pool timeout
         DynamicObject msg;
         msg["type"] = MsgPoolTimeout;
         messageSelf(msg);
      }
   }

   return rval;
}

bool DownloadManager::assignPiece(SellerData& sd)
{
   bool rval = false;

   // get working directory and download state ID
   UserId userId = BM_USER_ID(mDownloadState["userId"]);
   Config cfg = getConfig(mNode, userId);
   if(!cfg.isNull())
   {
      logDownloadStateMessage("assigning piece...");

      // get the file ID from the seller data's contract section
      ContractSection& cs = sd["section"];
      FileId fileId = BM_FILE_ID(cs["ware"]["fileInfos"][0]["id"]);
      DownloadStateId dsId = mDownloadState["id"]->getUInt64();
      string tmpPath = getConfig(mNode, userId)["temporaryPath"]->getString();

      // get the next available piece and assign it to the passed seller
      FilePiece fp(NULL);
      bool assigned = false;
      FileProgress& progress = mDownloadState["progress"][fileId];
      FileInfo& fi = progress["fileInfo"];
      FilePieceIterator i = progress["unassigned"].getIterator();
      if(i->hasNext())
      {
         // set true here to simplify checks below
         rval = true;
         fp = i->next();
         const char* csHash = cs["hash"]->getString();
         // check start date not set or initialized to empty string
         if(!mDownloadState->hasMember("startDate") ||
            strcmp(mDownloadState["startDate"]->getString(), "") == 0)
         {
            Date startDate;
            /*
            // Debug tool to adjust start date
            const char* toffset = getenv("BITMUNK_STARTDATE_TIME_OFFSET");
            if(toffset)
            {
               DynamicObject tmp;
               tmp = toffset;
               startDate.addSeconds(tmp->getInt32());
               //MO_CAT_DEBUG(BM_PURCHASE_CAT,
               printf("Start date time offset: %s", toffset);
               fflush(stdout);
            }
            */
            mDownloadState["startDate"] = startDate.getUtcDateTime().c_str();

            // update file progress
            rval = mPurchaseDatabase->updateDownloadStateFlags(mDownloadState);

            if(rval)
            {
               MO_CAT_DEBUG(BM_PURCHASE_CAT,
                  "First piece assigned, setting start date: "
                  "UserId:%" PRIu64 " DownloadState:%" PRIu64 " date:%s",
                  BM_USER_ID(mDownloadState["userId"]),
                  mDownloadState["id"]->getUInt64(),
                  mDownloadState["startDate"]->getString());
            }
         }
         if(rval)
         {
            progress["assigned"][csHash]->append(fp);
            i->remove();
            assigned = true;

            // add seller connection to active list
            string key = Tools::createSellerServerKey(sd["seller"]);
            uint32_t conns =
               mDownloadState["activeSellers"][key.c_str()]->getUInt32();
            mDownloadState["activeSellers"][key.c_str()] = conns + 1;
         }

         // get piece index and file extension
         uint32_t pieceIndex = fp["index"]->getUInt32();
         const char* extension = fi["extension"]->getString();

         // generate the filename for the file piece
         // first expand tmpPath
         rval = rval && getNode()->getConfigManager()->expandUserDataPath(
            tmpPath.c_str(), userId, tmpPath);
         if(rval)
         {
            fp["path"] = generateFilePieceFilename(
               tmpPath.c_str(), userId, dsId,
               fileId, extension, pieceIndex).c_str();

            // create database entry
            DynamicObject entries;
            DynamicObject& entry = entries->append();
            entry["fileId"] = fileId;
            entry["csHash"] = csHash;
            entry["status"] = "assigned";
            entry["piece"] = fp;

            // update file progress
            rval = mPurchaseDatabase->updateFileProgress(
               mDownloadState, entries);
         }

         if(rval)
         {
            // log piece downloader creation
            MO_CAT_DEBUG(BM_PURCHASE_CAT,
               "UserId %" PRIu64 ", DownloadState %" PRIu64 ": "
               "creating piece downloader for file ID %s, piece %u",
               BM_USER_ID(mDownloadState["userId"]),
               mDownloadState["id"]->getUInt64(),
               fileId, pieceIndex);

            // create and add piece downloader
            PieceDownloader* pd = new PieceDownloader(getNode(), getId());
            RateAverager* pa = new RateAverager(WINDOW_LENGTH);
            pd->initialize(
               ++mPieceDownloaderId,
               progress["sellerPool"]["pieceSize"]->getUInt32(),
               cs, fileId, fp, pa, &mDownloadRate);
            pd->setUserId(userId);
            pd->setDownloadState(mDownloadState);
            PieceDownloaderEntry pde;
            pde.fiberId = getNode()->getFiberScheduler()->addFiber(pd);
            pde.fileId = fileId;
            pde.pieceIndex = pieceIndex;
            pde.rateAverager = pa;
            mPieceDownloaders.insert(make_pair(mPieceDownloaderId, pde));
         }
      }

      if(assigned)
      {
         logDownloadStateMessage("another piece assigned.");

         // send piece assignment event
         Event e;
         e["type"] = EVENT_DOWNLOAD_STATE ".pieceAssigned";
         BM_ID_SET(e["details"]["fileId"], fileId);
         e["details"]["index"] = fp["index"]->getUInt32();
         sendDownloadStateEvent(e);
      }
   }

   return rval;
}

bool DownloadManager::pieceUpdate(DynamicObject& msg)
{
   bool rval = true;

   // get message details
   FileId fileId = BM_FILE_ID(msg["fileId"]);
   FilePiece fp = msg["piece"];
   FileProgress progress = mDownloadState["progress"][fileId];
   ContractSection cs = msg["section"];

   // get section hash
   const char* csHash = cs["hash"]->getString();

   // remove piece from assigned list
   bool removed = false;
   FilePieceIterator fpi = progress["assigned"][csHash].getIterator();
   while(!removed && fpi->hasNext())
   {
      FilePiece& next = fpi->next();
      if(next["index"] == fp["index"])
      {
         fpi->remove();
         removed = true;
      }
   }

   // clean up assigned entry for section hash if it is now empty
   if(progress["assigned"][csHash]->length() == 0)
   {
      progress["assigned"]->removeMember(csHash);
   }

   // remove seller connection from active list
   string key = Tools::createSellerServerKey(cs["seller"]);
   if(mDownloadState["activeSellers"]->hasMember(key.c_str()))
   {
      uint32_t conns =
         mDownloadState["activeSellers"][key.c_str()]->getUInt32();
      if(conns > 1)
      {
         // decrement entry
         mDownloadState["activeSellers"][key.c_str()] = --conns;
      }
      else
      {
         // remove entry entirely
         mDownloadState["activeSellers"]->removeMember(key.c_str());
      }
   }

   // create database entry
   DynamicObject entries;
   DynamicObject& dbe = entries->append();
   BM_ID_SET(dbe["fileId"], fileId);
   dbe["csHash"] = csHash;
   dbe["piece"] = fp;

   bool pieceReceived = msg->hasMember("pieceReceived");
   if(pieceReceived)
   {
      // add state to db entry
      dbe["status"] = "downloaded";

      // add piece to downloaded list
      progress["downloaded"][csHash]->append(fp);

      MO_CAT_DEBUG(BM_PURCHASE_CAT,
         "received piece %i of size %s, "
         "uid: %" PRIu64 ", dsid: %" PRIu64,
         fp["index"]->getInt32(),
         fp["size"]->getString(),
         BM_USER_ID(mDownloadState["userId"]),
         mDownloadState["id"]->getUInt64());

      // update bytes downloaded
      uint64_t tmp = progress["bytesDownloaded"]->getUInt64();
      progress["bytesDownloaded"] = tmp + fp["size"]->getUInt32();

      // decrement remaining pieces by 1
      mDownloadState["remainingPieces"] =
         mDownloadState["remainingPieces"]->getUInt32() - 1;

      // update download rate for seller
      SellerData& sd = progress["sellerData"][csHash];
      sd["downloadRate"] = msg["pieceRate"]->getDouble();
   }
   else
   {
      // add state to db entry
      dbe["status"] = "unassigned";

      // add piece to unassigned list
      progress["unassigned"]->append(fp);

      // download from seller failed, add to bad sellers list
      DynamicObject blacklistEntry;
      blacklistEntry["seller"] = cs["seller"].clone();
      blacklistEntry["time"] = System::getCurrentMilliseconds();
      mDownloadState["blacklist"][key.c_str()] = blacklistEntry;

      MO_CAT_DEBUG(BM_PURCHASE_CAT,
         "piece %i receive failed and will be re-assigned, "
         "uid: %" PRIu64 ", dsid: %" PRIu64,
         fp["index"]->getInt32(),
         BM_USER_ID(mDownloadState["userId"]),
         mDownloadState["id"]->getUInt64());
   }

   // insert entry into database and update download state flags
   rval =
      mPurchaseDatabase->updateFileProgress(mDownloadState, entries) &&
      mPurchaseDatabase->updateDownloadStateFlags(mDownloadState);

   // remove piece downloader with matching ID
   PieceDownloaderMap::iterator i = mPieceDownloaders.find(
      msg["pieceDownloaderId"]->getUInt32());
   if(i != mPieceDownloaders.end())
   {
      delete i->second.rateAverager;
      mPieceDownloaders.erase(i);
   }

   return rval;
}

bool DownloadManager::poolTimeout(DynamicObject& msg)
{
   // only bother updating pools if they aren't already being updated
   // and we aren't interrupted
   if(!mSellerPoolsUpdating && !mInterrupted)
   {
      // add seller pool for each incomplete file (file w/unassigned pieces)
      SellerPoolList pools;
      pools->setType(Array);
      FileProgressIterator fpi = mDownloadState["progress"].getIterator();
      while(fpi->hasNext())
      {
         FileProgress& fp = fpi->next();
         if(fp["unassigned"]->length() > 0)
         {
            FileInfo& fi = fp["sellerPool"]["fileInfo"];

            // create a seller pool to populate, try to retrieve up
            // to 100 sellers
            SellerPool sp;
            BM_ID_SET(sp["fileInfo"]["id"], BM_FILE_ID(fi["id"]));
            BM_ID_SET(sp["fileInfo"]["mediaId"], BM_MEDIA_ID(fi["mediaId"]));
            sp["sellerDataSet"]["start"] = 0;
            sp["sellerDataSet"]["num"] = 100;

            // add seller pool to list
            pools->append(sp);
         }
      }

      // only scheduler an update if there are pools to be updated and no
      // update is currently running
      if(pools->length() > 0)
      {
         // schedule a fiber to update seller pools
         SellerPoolUpdater* spu = new SellerPoolUpdater(getNode(), getId());
         spu->setUserId(getUserId());
         spu->setDownloadState(mDownloadState);
         spu->setSellerPools(pools);
         spu->initializeFileProgress(false);
         getNode()->getFiberScheduler()->addFiber(spu);
      }
   }

   return true;
}

bool DownloadManager::poolUpdate(DynamicObject& msg)
{
   // store completed seller pools in download state
   DynamicObject fileIds;
   fileIds->setType(Array);
   SellerPoolIterator i = msg["sellerPools"].getIterator();
   while(i->hasNext())
   {
      SellerPool& sp = i->next();
      FileId fileId = BM_FILE_ID(sp["fileInfo"]["id"]);
      fileIds->append() = fileId;

      // save old bfp ID to maintain consistency... all file pieces
      // must use the same bfp ID
      FileProgress& fp = mDownloadState["progress"][fileId];
      BfpId id = BM_BFP_ID(fp["sellerPool"]["bfpId"]);
      if(BM_BFP_ID_VALID(id))
      {
         BM_ID_SET(sp["bfpId"], id);
      }
      // FIXME: saving a reference to the old seller pool "fixes" a
      // race condition that isn't fully understood yet ... and hopefully
      // won't need to be understood with a rewrite of the purchase module
      // so that exactly these sorts of problems are simply avoided entirely
      // by design
      SellerPool hack = fp["sellerPool"];
      fp["sellerPool"] = sp;
   }

   // only keep "blacklist" entries that haven't expired
   DynamicObject blacklist;
   blacklist->setType(Map);
   DynamicObjectIterator bli = mDownloadState["blacklist"].getIterator();
   uint64_t now = System::getCurrentMilliseconds();
   while(bli->hasNext())
   {
      DynamicObject& blacklistEntry = bli->next();
      uint64_t diff = now - blacklistEntry["time"]->getUInt64();
      if(diff < DEFAULT_POOL_TIMEOUT)
      {
         blacklist[bli->getName()] = blacklistEntry;
      }
      else
      {
         MO_CAT_DEBUG(BM_PURCHASE_CAT,
            "Seller (%s) blacklist entry has expired, "
            "uid: %" PRIu64 ", dsid: %" PRIu64,
            bli->getName(),
            BM_USER_ID(mDownloadState["userId"]),
            mDownloadState["id"]->getUInt64());
      }
   }
   mDownloadState["blacklist"] = blacklist;

   // get seller pool timeout
   UserId userId = BM_USER_ID(mDownloadState["userId"]);
   uint64_t timeout = DEFAULT_POOL_TIMEOUT;
   Config cfg = getConfig(mNode, userId)["DownloadManager"];
   if(cfg->hasMember("sellerPoolTimeout"))
   {
      timeout = cfg["sellerPoolTimeout"]->getUInt32();
   }

   // seller pools now initialized (flag is forever set to false here, we only
   // need to initialize when we first start up, that flag simply blocks the
   // first piece from being assigned, pool updates will continue periodically)
   mSellerPoolsInitializing = false;
   mSellerPoolsUpdating = false;

   // schedule seller pools updated event
   {
      Event e;
      e["type"] = EVENT_POOLS_UPDATED;
      e["details"]["pools"] = fileIds;
      sendDownloadStateEvent(e);
   }

   // schedule event to indicate seller pool timeout
   {
      Event e;
      e["type"] = EVENT_POOL_TIMEOUT;
      e["parallel"] = true;
      e["details"]["fiberId"] = getId();
      getNode()->getEventDaemon()->add(e, timeout, 1);
   }

   return true;
}

bool DownloadManager::negotiationComplete(DynamicObject& msg)
{
   bool rval = true;

   // no longer negotiating
   mNegotiating = false;

   // see if there was a negotiation error and we *had* to find a seller
   if(msg["negotiationError"]->getBoolean() && msg["must"]->getBoolean())
   {
      // if there are no pieces being downloaded, then return false
      // because we can't find any more sellers
      ExceptionRef e = Exception::convertToException(msg["exception"]);
      Exception::set(e);
      sendErrorEvent();
      rval = false;

      MO_CAT_ERROR(BM_PURCHASE_CAT,
         "Cannot assign a seller, no seller found, "
         "uid: %" PRIu64 ", dsid: %" PRIu64,
         BM_USER_ID(mDownloadState["userId"]),
         mDownloadState["id"]->getUInt64());
   }

   return rval;
}

bool DownloadManager::pauseDownload(DynamicObject& msg)
{
   // confirm that download state and user IDs match, ignore otherwise
   if(mDownloadState["id"]->getUInt64() ==
      msg["downloadStateId"]->getUInt64() &&
      BM_USER_ID_EQUALS(
         BM_USER_ID(mDownloadState["userId"]), BM_USER_ID(msg["userId"])))
   {
      // pause download by interrupting self, save deleting flag
      mDownloadState["deleting"] = msg["deleting"]->getBoolean();
      getNode()->interruptFiber(getId());
   }

   return true;
}

bool DownloadManager::interrupt(DynamicObject& msg)
{
   logDownloadStateMessage("interrupted");

   // now interrupted
   mInterrupted = true;

   // write download state flags out
   mDownloadState["downloadPaused"] = true;
   mPurchaseDatabase->updateDownloadStateFlags(mDownloadState);

   // interrupt all piece downloaders
   for(PieceDownloaderMap::iterator i = mPieceDownloaders.begin();
       i != mPieceDownloaders.end(); ++i)
   {
      DynamicObject msg2;
      msg2["interrupt"] = true;
      msg2["pause"] = true;
      BM_ID_SET(msg2["userId"], BM_USER_ID(mDownloadState["userId"]));
      msg2["downloadStateId"] = mDownloadState["id"]->getUInt64();
      msg2["pieceDownloaderId"] = i->first;
      sendMessage(i->second.fiberId, msg2);
   }

   // send interrupting event
   Event e;
   e["type"] = EVENT_DOWNLOAD_STATE ".downloadInterrupting";
   sendDownloadStateEvent(e);

   return true;
}

bool DownloadManager::progressPolled(DynamicObject& msg)
{
   logDownloadStateMessage("download progress requested...");

   // create a download event with the current progress
   Event e;
   e["type"] = EVENT_DOWNLOAD_STATE ".progressUpdate";
   e["details"]["files"]->setType(Map);

   // add progress from current downloading pieces
   DynamicObject fpMap;
   fpMap->setType(Map);
   for(PieceDownloaderMap::iterator i = mPieceDownloaders.begin();
       i != mPieceDownloaders.end(); ++i)
   {
      FileId fileId = i->second.fileId.c_str();
      DynamicObject& entry = fpMap[fileId];

      // get piece size
      FileProgress& fp = mDownloadState["progress"][fileId];
      uint64_t size = fp["sellerPool"]["pieceSize"]->getUInt64();

      // get piece stats
      // calculate ETA (delta in seconds until download complete)
      uint64_t downloaded = i->second.rateAverager->getTotalItemCount();
      uint64_t remaining = (downloaded > size) ? 0 : (size - downloaded);
      double rate = i->second.rateAverager->getItemsPerSecond();
      double totalRate = i->second.rateAverager->getTotalItemsPerSecond();
      uint64_t eta = (remaining == 0) ?
         0 : (uint64_t)roundl(remaining / totalRate);

      // save piece stats
      DynamicObject& piece = entry["pieces"]->append();
      piece["index"] = i->second.pieceIndex;
      piece["downloaded"] = downloaded;
      piece["size"] = size;
      piece["eta"] = eta;
      piece["rate"] = rate;
      piece["totalRate"] = totalRate;

      // increment total file stats
      entry["downloaded"] = entry["downloaded"]->getUInt64() + downloaded;
      entry["rate"] = entry["rate"]->getDouble() + rate;
      entry["totalRate"] = entry["totalRate"]->getDouble() + totalRate;
   }

   // calculate download progress for all files
   uint64_t totalSize = 0;
   uint64_t totalDownloaded = 0;
   double rate = 0;
   double totalRate = 0;
   uint32_t totalNegotiatedSellers = 0;
   FileProgressIterator fpi = mDownloadState["progress"].getIterator();
   while(fpi->hasNext())
   {
      FileProgress& fp = fpi->next();
      FileId fileId = BM_FILE_ID(fp["fileInfo"]["id"]);
      DynamicObject& perFile = e["details"]["files"][fileId];

      // add indexes of pieces that are downloaded already
      DynamicObject& downloadedPieces = perFile["pieces"]["downloaded"];
      downloadedPieces->setType(Array);
      DynamicObjectIterator fpli = fp["downloaded"].getIterator();
      while(fpli->hasNext())
      {
         FilePieceList& fpl = fpli->next();
         FilePieceIterator fpi = fpl.getIterator();
         while(fpi->hasNext())
         {
            downloadedPieces->append() = fpi->next()["index"]->getUInt32();
         }
      }

      // get bytes downloaded so far and total file size
      uint64_t downloaded = fp["bytesDownloaded"]->getUInt64();
      uint64_t size = fp["fileInfo"]["contentSize"]->getUInt64();

      // add current piece information
      double fileRate = 0;
      double fileTotalRate = 0;
      if(fpMap->hasMember(fileId))
      {
         // add indexes of pieces that are in progress
         DynamicObject& entry = fpMap[fileId];
         perFile["pieces"]["assigned"] = entry["pieces"];

         // increment stats
         downloaded += entry["downloaded"]->getUInt64();
         fileRate += entry["rate"]->getDouble();
         fileTotalRate += entry["totalRate"]->getDouble();
      }
      else
      {
         // no pieces in progress or stats to update
         perFile["pieces"]["assigned"]->setType(Array);
      }

      // set per file stats
      // calculate ETA (delta in seconds until download complete)
      uint64_t remaining = (downloaded > size) ? 0 : (size - downloaded);
      perFile["eta"] = (remaining == 0) ?
         0 : (uint64_t)roundl(remaining / fileTotalRate);
      perFile["downloaded"] = downloaded;
      perFile["rate"] = fileRate;
      perFile["totalRate"] = fileTotalRate;
      perFile["size"] = size;
      perFile["pieces"]["size"] = fp["sellerPool"]["pieceSize"]->getUInt32();
      perFile["pieces"]["count"] = fp["sellerPool"]["pieceCount"]->getUInt32();
      totalSize += size;
      totalDownloaded += downloaded;
      rate += fileRate;
      totalRate += fileTotalRate;

      // seller info
      uint32_t sellerCount = fp["sellers"]->length();
      totalNegotiatedSellers += sellerCount;
      perFile["sellers"]["negotiated"] = sellerCount;
   }

   // calculate ETA (delta in seconds until download complete)
   uint64_t remaining = (totalDownloaded > totalSize) ?
      0 : totalSize - totalDownloaded;
   e["details"]["eta"] = (remaining == 0) ?
      0 : (uint64_t)roundl(remaining / totalRate);
   e["details"]["downloaded"] = totalDownloaded;
   e["details"]["rate"] = rate;
   e["details"]["totalRate"] = totalRate;
   e["details"]["size"] = totalSize;
   e["details"]["sellers"]["negotiated"] = totalNegotiatedSellers;
   e["details"]["sellers"]["active"] =
      mDownloadState["activeSellers"]->length();
   sendDownloadStateEvent(e);

   return true;
}

string DownloadManager::generateFilePieceFilename(
   const char* path, UserId userId, DownloadStateId dsId, FileId fileId,
   const char* extension, uint32_t index)
{
   // append filename
   int length = 20 + 20 + strlen(fileId) + strlen(extension) + 20 + 10;
   char filename[length];
   snprintf(
      filename, length, "%" PRIu64 "-%" PRIu64 "-%s-%s-%04u.fp",
      userId, dsId, fileId, extension, index);
   // join base directory and file piece name and return
   return File::join(path, filename);
}

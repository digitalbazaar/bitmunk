/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/purchase/DownloadStateInitializer.h"

#include "bitmunk/common/Logging.h"
#include "bitmunk/purchase/SellerPoolUpdater.h"
#include "monarch/config/ConfigManager.h"

using namespace bitmunk::common;
using namespace bitmunk::purchase;
using namespace bitmunk::node;
using namespace monarch::config;
using namespace monarch::rt;
using namespace monarch::event;
using namespace monarch::fiber;
using namespace std;

#define EVENT_DOWNLOAD_STATE "bitmunk.purchase.DownloadState"

DownloadStateInitializer::DownloadStateInitializer(Node* node) :
   NodeFiber(node),
   DownloadStateFiber(node, "DownloadStateInitializer", &mFiberExitData)
{
}

DownloadStateInitializer::~DownloadStateInitializer()
{
}

void DownloadStateInitializer::processMessages()
{
   // set processor ID on download state
   mPurchaseDatabase->setDownloadStateProcessorId(mDownloadState, 0, getId());

   // ensure download state ID is not invalid
   if(mDownloadState["id"]->getUInt64() == 0)
   {
      // invalid download state ID, send event
      ExceptionRef e = new Exception(
         "Could not initialize download. Invalid DownloadStateId of 0.",
         EVENT_DOWNLOAD_STATE ".InvalidId");
      Exception::set(e);
      sendErrorEvent();
   }
   // FIXME: add option in the future to re-initialize
   else if(mDownloadState["initialized"]->getBoolean())
   {
      // stop processing download state
      mPurchaseDatabase->stopProcessingDownloadState(mDownloadState, getId());

      // download state already initialized
      // send success event
      Event e;
      e["type"] = EVENT_DOWNLOAD_STATE ".initialized";
      sendDownloadStateEvent(e);
   }
   else
   {
      // insert the uninitialized pools into the database
      PurchaseDatabase* pd = PurchaseDatabase::getInstance(getNode());
      if(!pd->insertSellerPools(mDownloadState))
      {
         // send error event
         sendErrorEvent();
      }
      else
      {
         // run SellerPoolUpdater to initialize seller pools
         runSellerPoolUpdater();
      }
   }

   // stop processing download state
   mPurchaseDatabase->stopProcessingDownloadState(mDownloadState, getId());
}

void DownloadStateInitializer::runSellerPoolUpdater()
{
   // create list of all seller pools
   SellerPoolList pools;
   pools->setType(Array);
   FileInfoIterator fii = mDownloadState["ware"]["fileInfos"].getIterator();
   while(fii->hasNext())
   {
      FileInfo& fi = fii->next();

      // create a seller pool to populate, try to retrieve up
      // to 100 sellers
      SellerPool sp;
      sp["fileInfo"]["id"] = fi["id"]->getString();
      sp["fileInfo"]["mediaId"] = fi["mediaId"]->getUInt64();
      sp["sellerDataSet"]["start"] = 0;
      sp["sellerDataSet"]["num"] = 100;

      // add seller pool to list
      pools->append(sp);

      yield();
   }

   // start up SellerPoolUpdater fiber
   SellerPoolUpdater* spu = new SellerPoolUpdater(getNode(), getId());
   spu->setUserId(getUserId());
   spu->setDownloadState(mDownloadState);
   spu->setSellerPools(pools);
   spu->initializeFileProgress(true);
   mNode->getFiberScheduler()->addFiber(spu);

   // sleep until spu is done
   bool keepSleeping = true;
   while(keepSleeping)
   {
      sleep();

      // get messages
      FiberMessageQueue* msgs = getMessages();
      while(!msgs->empty())
      {
         DynamicObject msg = msgs->front();
         msgs->pop_front();

         if(msg->hasMember("interrupt"))
         {
            logDownloadStateMessage("interrupted");

            // send interrupted event
            Event e;
            e["type"] = EVENT_DOWNLOAD_STATE ".initializationInterrupted";
            sendDownloadStateEvent(e);
            keepSleeping = false;
         }
         // FIXME: is this intentionally not an elseif?
         if(msg->hasMember("sellerPoolUpdateComplete"))
         {
            // stop processing download state
            mPurchaseDatabase->stopProcessingDownloadState(
               mDownloadState, getId());

            // send success event
            Event e;
            e["type"] = EVENT_DOWNLOAD_STATE ".initialized";
            sendDownloadStateEvent(e);
            keepSleeping = false;
         }
         else if(msg->hasMember("sellerPoolUpdateError"))
         {
            // send error event
            Event e;
            e["type"] = EVENT_DOWNLOAD_STATE ".exception";
            e["details"]["exception"] = msg["exception"];
            sendDownloadStateEvent(e);
            keepSleeping = false;
         }
      }
   }
}

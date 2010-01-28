/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk/purchase/SellerPoolUpdater.h"

#include "bitmunk/purchase/FileCostCalculator.h"
#include "bitmunk/purchase/PurchaseDatabase.h"
#include "bitmunk/purchase/PurchaseModule.h"
#include "monarch/rt/DynamicObjectIterator.h"
#include "monarch/rt/RunnableDelegate.h"

using namespace std;
using namespace bitmunk::common;
using namespace bitmunk::purchase;
using namespace bitmunk::node;
using namespace monarch::config;
using namespace monarch::crypto;
using namespace monarch::event;
using namespace monarch::fiber;
using namespace monarch::modest;
using namespace monarch::net;
using namespace monarch::rt;
using namespace monarch::util;

SellerPoolUpdater::SellerPoolUpdater(Node* node, FiberId parentId) :
   NodeFiber(node, parentId),
   DownloadStateFiber(node, "SellerPoolUpdater", &mFiberExitData),
   mCurrent(NULL),
   mInitFileProgress(false)
{
   mUpdatedSellerPools->setType(Array);
}

SellerPoolUpdater::~SellerPoolUpdater()
{
}

void SellerPoolUpdater::setSellerPools(SellerPoolList& pools)
{
   // populate the seller pool stack
   SellerPoolIterator i = pools.getIterator();
   while(i->hasNext())
   {
      mNextStack.push_back(i->next());
   }
}

void SellerPoolUpdater::initializeFileProgress(bool yes)
{
   mInitFileProgress = yes;
}

void SellerPoolUpdater::fetchSellerPool()
{
   logDownloadStateMessage("updating seller pool...");

   // FIXME: if a pool fails to be updated, only report the error and exit
   // if we are initializing file progress, otherwise it is a non-fatal error
   bool error = false;

   // get pool using media ID and file ID, use existing seller data
   // set start/num
   Messenger* messenger = mNode->getMessenger();
   Url url;
   url.format("/api/3.0/catalog/sellerpools/%" PRIu64 "/%s?start=%s&num=%s",
      mCurrent["fileInfo"]["mediaId"]->getUInt64(),
      mCurrent["fileInfo"]["id"]->getString(),
      mCurrent["sellerDataSet"]["start"]->getString(),
      mCurrent["sellerDataSet"]["num"]->getString());
   SellerPool pool;
   error = !messenger->getSecureFromBitmunk(
      &url, pool, mDownloadState["userId"]->getUInt64());

   // create message regarding operation completion
   DynamicObject msg;
   msg["sellerPoolUpdated"] = !error;
   if(error)
   {
      msg["exception"] = Exception::getAsDynamicObject();
   }
   else
   {
      // update current pool
      mCurrent = pool;
   }

   // send message to self
   messageSelf(msg);
}

void SellerPoolUpdater::processMessages()
{
   // iterate over the seller pools in the stack
   bool success = true;
   while(success && !mNextStack.empty())
   {
      // pop the stack
      mCurrent = mNextStack.front();
      mNextStack.pop_front();

      // run a new fetch seller pool operation
      RunnableRef r = new RunnableDelegate<SellerPoolUpdater>(
         this, &SellerPoolUpdater::fetchSellerPool);
      Operation op = r;
      getNode()->runOperation(op);

      // wait for sellerPoolUpdated message
      const char* keys[] = {"sellerPoolUpdated", NULL};
      DynamicObject msg = waitForMessages(keys, &op)[0];
      if(!msg["sellerPoolUpdated"]->getBoolean())
      {
         logDownloadStateMessage("failed to update seller pool");

         // convert exception from dynamic object, send error event
         ExceptionRef e = Exception::convertToException(msg["exception"]);
         Exception::set(e);
         sendEvent(true);
         success = false;
      }
      else
      {
         logDownloadStateMessage("seller pool updated");

         // insert seller pool into database
         logDownloadStateMessage("inserting seller pool into database...");

         // add the seller pool to the list of updated pools
         mUpdatedSellerPools->append(mCurrent);

         // initialize file progress as necessary
         if(mNextStack.empty() && mInitFileProgress)
         {
            // file progress must be initialized
            success = initFileProgress();
         }
         else
         {
            success = true;
         }

         // Note: The updated pool is not set in the download state that is in
         // memory, it is only written to the database. This is done to protect
         // against race conditions. If an in-memory download state needs to be
         // updated, then another class can process either the event or the
         // fiber message sent by this class once all pools have been updated.
         PurchaseDatabase* pd = PurchaseDatabase::getInstance(getNode());
         success = success && pd->updateSellerPool(mDownloadState, mCurrent);
         if(!success)
         {
            // send error event
            sendEvent(true);
            success = false;
         }
      }
   }

   if(success)
   {
      // send event (and fiber message) that pools are finished updating
      sendEvent(false);
   }
}

void SellerPoolUpdater::sendEvent(bool error)
{
   Event e;
   DynamicObject msg;

   if(error)
   {
      // set failure event
      ExceptionRef ex = Exception::get();
      e["type"] = "bitmunk.purchase.SellerPoolUpdater.exception";
      e["details"]["exception"] = Exception::convertToDynamicObject(ex);
      msg["exception"] = e["details"]["exception"].clone();
      msg["sellerPoolUpdateError"] = true;

      // log exception
      logException(ex);
   }
   else
   {
      e["type"] = "bitmunk.purchase.SellerPoolUpdater.updateComplete";
      msg["sellerPoolUpdateComplete"] = true;
      msg["sellerPools"] = mUpdatedSellerPools;
   }

   // send event
   sendDownloadStateEvent(e);

   // send message to parent
   messageParent(msg);
}

bool SellerPoolUpdater::initFileProgress()
{
   bool rval;

   // iterate over all updated pools and init file progress
   uint32_t totalPieces = 0;
   SellerPoolIterator spi = mUpdatedSellerPools.getIterator();
   while(spi->hasNext())
   {
      SellerPool& sp = spi->next();
      FileId fileId = sp["fileInfo"]["id"]->getString();
      FileProgress& fp = mDownloadState["progress"][fileId];

      // initialize child data
      fp["fileInfo"] = sp["fileInfo"];
      fp["sellerPool"] = sp;
      fp["bytesDownloaded"] = 0;
      fp["budget"]->setType(String);
      fp["sellerData"]->setType(Map);
      fp["sellers"]->setType(Map);
      fp["unassigned"]->setType(Array);
      fp["assigned"]->setType(Map);
      fp["downloaded"]->setType(Map);

      // create all unassigned file pieces
      uint32_t size = sp["pieceSize"]->getUInt32();
      uint32_t count = sp["pieceCount"]->getUInt32();
      totalPieces += count;
      for(uint32_t n = 0; n < count; n++)
      {
         FilePiece& piece = fp["unassigned"]->append();
         piece["size"] = size;
         piece["index"] = n;
         piece["bfpId"] = sp["bfpId"]->getUInt32();
      }
   }

   // calculate file costs
   FileCostCalculator fcc;
   fcc.calculate(this, mDownloadState);

   // set piece counts, initialized state
   mDownloadState["remainingPieces"] = totalPieces;
   mDownloadState["totalPieceCount"] = totalPieces;
   mDownloadState["initialized"] = true;

   // print out total median price and remaining pieces
   MO_CAT_DEBUG(BM_PURCHASE_CAT,
      "initial total median price (data only): %s, remaining pieces: %u, "
      "uid: %" PRIu64 ", dsid: %" PRIu64 "",
      mDownloadState["totalMedPrice"]->getString(),
      mDownloadState["remainingPieces"]->getUInt32(),
      mDownloadState["userId"]->getUInt64(),
      mDownloadState["id"]->getUInt64());

   // write update state flags and file progress
   PurchaseDatabase* pd = PurchaseDatabase::getInstance(getNode());
   if(pd->updateDownloadStateFlags(mDownloadState))
   {
      rval = true;
   }
   else
   {
      // send error event
      sendEvent(true);
      rval = false;
   }

   return rval;
}

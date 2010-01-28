/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk/purchase/LicenseAcquirer.h"

#include "bitmunk/purchase/FileCostCalculator.h"
#include "bitmunk/purchase/PurchaseModule.h"
#include "monarch/crypto/BigDecimal.h"
#include "monarch/rt/RunnableDelegate.h"

using namespace std;
using namespace bitmunk::common;
using namespace bitmunk::purchase;
using namespace bitmunk::node;
using namespace monarch::crypto;
using namespace monarch::event;
using namespace monarch::fiber;
using namespace monarch::modest;
using namespace monarch::net;
using namespace monarch::rt;

LicenseAcquirer::LicenseAcquirer(Node* node) :
   NodeFiber(node, 0),
   DownloadStateFiber(node, "LicenseAcquirer", &mFiberExitData)
{
}

LicenseAcquirer::~LicenseAcquirer()
{
}

void LicenseAcquirer::acquireLicense()
{
   logDownloadStateMessage("acquiring license...");

   // contact sva's POST media service to create a signed media
   Url url;
   url.format("/api/3.0/sva/contracts/media/%" PRIu64,
      mDownloadState["contract"]["media"]["id"]->getUInt64());

   // post to media signing service
   Media& media = mDownloadState["contract"]["media"];
   bool success = getNode()->getMessenger()->postSecureToBitmunk(
      &url, NULL, &media, mDownloadState["userId"]->getUInt64());

   // create message regarding operation completion
   DynamicObject msg;
   msg["licenseAcquired"] = success;
   if(!success)
   {
      // failed to acquire license
      ExceptionRef e = new Exception(
         "Could not acquire license.",
         "bitmunk.purchase.LicenseAcquirer.LicenseFailure");
      Exception::push(e);
      msg["exception"] = Exception::convertToDynamicObject(e);
   }

   // send message to self
   messageSelf(msg);
}

void LicenseAcquirer::processMessages()
{
   // set processor ID on download state
   mPurchaseDatabase->setDownloadStateProcessorId(mDownloadState, 0, getId());

   // send event notifying that license acquisition has begun
   Event e;
   e["type"] = "bitmunk.purchase.DownloadState.licenseAcquisitionStarted";
   sendDownloadStateEvent(e);

   bool licenseAcquired = mDownloadState["licenseAcquired"]->getBoolean();
   if(licenseAcquired)
   {
      // FIXME: add option to re-acquire a new license in the future
#if 0
      // recalculate file costs
      FileCostCalculator fcc;
      fcc.calculate(this, mDownloadState);

      // print out total median price and remaining pieces
      MO_CAT_DEBUG(BM_PURCHASE_CAT,
         "removed old license total median price (data only): %s, "
         "total pieces: %u, "
         "removed license amount: %s, "
         "removed total micro payment cost: %s, "
         "uid: %" PRIu64 ", dsid: %" PRIu64,
         mDownloadState["totalMedPrice"]->getString(),
         mDownloadState["totalPieceCount"]->getUInt32(),
         mDownloadState["contract"]["media"]["licenseAmount"]->getString(),
         mDownloadState["totalMicroPaymentCost"]->getString(),
         mDownloadState["userId"]->getUInt64(),
         mDownloadState["id"]->getUInt64());
#endif
   }
   else
   {
      // FIXME: in future, possibly support the ability to check the
      // database for an existing or re-usable license

      // run a new acquire license operation
      RunnableRef r = new RunnableDelegate<LicenseAcquirer>(
         this, &LicenseAcquirer::acquireLicense);
      Operation op = r;
      getNode()->runOperation(op);

      // wait for licenseAcquired message
      const char* keys[] = {"licenseAcquired", NULL};
      DynamicObject msg = waitForMessages(keys, &op)[0];
      if(msg["licenseAcquired"]->getBoolean())
      {
         logDownloadStateMessage("license acquired");
         logDownloadStateMessage(
            "inserting acquired license into database...");

         // license now acquired
         mDownloadState["licenseAcquired"] = true;

         // recalculate file costs
         FileCostCalculator fcc;
         fcc.calculate(this, mDownloadState);

         // print out total median price and remaining pieces
         MO_CAT_DEBUG(BM_PURCHASE_CAT,
            "added license total median price (license+data): %s, "
            "total pieces: %u, "
            "license amount: %s, "
            "total micro payment cost: %s, "
            "uid: %" PRIu64 ", dsid: %" PRIu64,
            mDownloadState["totalMedPrice"]->getString(),
            mDownloadState["totalPieceCount"]->getUInt32(),
            mDownloadState["contract"]["media"]["licenseAmount"]->getString(),
            mDownloadState["totalMicroPaymentCost"]->getString(),
            mDownloadState["userId"]->getUInt64(),
            mDownloadState["id"]->getUInt64());

         // update contract in database and update flags
         licenseAcquired =
            mPurchaseDatabase->updateContract(mDownloadState) &&
            mPurchaseDatabase->updateDownloadStateFlags(mDownloadState);

         // update seller pools to update micro payment costs and budgets
         FileProgressIterator fpi = mDownloadState["progress"].getIterator();
         while(fpi->hasNext())
         {
            yield();
            FileProgress& fp = fpi->next();
            mPurchaseDatabase->updateSellerPool(
               mDownloadState, fp["sellerPool"]);
         }
      }
      else
      {
         // convert exception from dynamic object, send error event
         ExceptionRef e = Exception::convertToException(msg["exception"]);
         Exception::set(e);
      }
   }

   // stop processing download state
   mPurchaseDatabase->stopProcessingDownloadState(mDownloadState, getId());

   if(licenseAcquired)
   {
      // send success event
      e = Event();
      e["type"] = "bitmunk.purchase.DownloadState.licenseAcquired";
      sendDownloadStateEvent(e);
   }
   else
   {
      // send error event
      sendErrorEvent();
   }
}

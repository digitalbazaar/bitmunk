/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/purchase/Purchaser.h"

#include "bitmunk/common/Logging.h"
#include "bitmunk/common/Signer.h"
#include "monarch/rt/RunnableDelegate.h"

using namespace std;
using namespace bitmunk::common;
using namespace bitmunk::purchase;
using namespace bitmunk::node;
using namespace monarch::event;
using namespace monarch::modest;
using namespace monarch::net;
using namespace monarch::rt;
using namespace monarch::util;

#define EVENT_DOWNLOAD_STATE "bitmunk.purchase.DownloadState"
#define EXCEPTION_PURCHASER  "bitmunk.purchase.Purchaser"

Purchaser::Purchaser(Node* node) :
   NodeFiber(node),
   DownloadStateFiber(node, "Purchaser", &mFiberExitData)
{
   mDbEntries->setType(Array);
}

Purchaser::~Purchaser()
{
}

void Purchaser::runPurchaseLicenseOp()
{
   bool success = false;

   logDownloadStateMessage("purchasing license...");

   // do license purchase
   Contract& c = mDownloadState["contract"];
   Contract cIn;
   Url url("/api/3.0/sva/contracts/purchase/license");
   success = getNode()->getMessenger()->postSecureToBitmunk(
      &url, &c, &cIn, BM_USER_ID(mDownloadState["userId"]));
   if(success)
   {
      // replace contract
      c = cIn;
   }

   // create message regarding operation completion
   DynamicObject msg;
   msg["licensePurchased"] = success;
   if(!success)
   {
      // failed to purchase license
      ExceptionRef e = new Exception(
         "Could not purchase license.",
         EXCEPTION_PURCHASER ".PurchaseFailed");
      Exception::push(e);
      msg["exception"] = Exception::convertToDynamicObject(e);
   }

   // send message to self
   messageSelf(msg);
}

void Purchaser::runPurchaseDataOp()
{
   bool success = true;

   logDownloadStateMessage("purchasing data...");

   // perform the data purchase for each contract section
   FileProgressIterator pi = mDownloadState["progress"].getIterator();
   while(pi->hasNext())
   {
      FileProgress& progress = pi->next();
      SellerDataIterator sdi = progress["sellerData"].getIterator();
      while(sdi->hasNext())
      {
         SellerData& sd = sdi->next();
         ContractSection cs = sd["section"];

         // add downloaded file pieces to section, if any
         string csHash = cs["hash"]->getString();
         FileInfo fi = cs["ware"]["fileInfos"][0];
         if(!progress["downloaded"]->hasMember(csHash.c_str()))
         {
            // no file pieces downloaded for this file yet
            fi["pieces"]->setType(Array);
         }
         else
         {
            // clone pieces to clear paths for transport
            fi["pieces"] = progress["downloaded"][csHash.c_str()].clone();
            {
               FilePieceIterator i = fi["pieces"].getIterator();
               while(i->hasNext())
               {
                  FilePiece& piece = i->next();
                  piece->removeMember("path");
               }
            }

            // copy start date to file info
            fi["startDate"] = mDownloadState["startDate"]->getString();

            // set the contract ID for the contract section and the account
            // to purchase with
            BM_ID_SET(
               cs["contractId"],
               BM_TRANSACTION_ID(mDownloadState["contract"]["id"]));
            BM_ID_SET(
               cs["buyer"]["accountId"],
               BM_ACCOUNT_ID(mDownloadState["preferences"]["accountId"]));

            // setup CS wrapper with added current local date
            // date is used as offset for startDate in case local time is
            // out of sync with remote time
            DynamicObject out;
            out["section"] = cs;
            Date date;
            /*
            // Debug tool to adjust purchase time
            const char* toffset = getenv("BITMUNK_PURCHASE_TIME_OFFSET");
            if(toffset)
            {
               DynamicObject tmp;
               tmp = toffset;
               date.addSeconds(tmp->getInt32());
               //MO_CAT_DEBUG(BM_PURCHASE_CAT,
               printf(
                  "Purchase time offset: %s", toffset);
               fflush(stdout);
            }
            */
            out["date"] = date.getUtcDateTime().c_str();

            // pay for the data via the SVA
            Url url("/api/3.0/sva/contracts/purchase/data");
            success = getNode()->getMessenger()->postSecureToBitmunk(
               &url, &out, &cs, BM_USER_ID(mDownloadState["userId"]));

            if(success)
            {
               // update file progress with file infos' unlocked keys and
               // create database entries
               fi = cs["ware"]["fileInfos"][0];
               FileId fileId = BM_FILE_ID(fi["id"]);
               FileInfo& toUpdate = progress["fileInfo"];

               // set unlocked file pieces in progress file info
               FilePieceIterator fpi = fi["pieces"].getIterator();
               while(fpi->hasNext())
               {
                  FilePiece& fp = fpi->next();

                  // find piece to update
                  bool found = false;
                  uint32_t index = fp["index"]->getUInt32();
                  FilePieceIterator i =
                     progress["downloaded"][csHash.c_str()].getIterator();
                  while(!found && i->hasNext())
                  {
                     FilePiece& piece = i->next();
                     if(piece["index"]->getUInt32() == index)
                     {
                        found = true;

                        // restore path
                        fp["path"] = piece["path"]->getString();

                        // create database entry
                        DynamicObject& entry = mDbEntries->append();
                        BM_ID_SET(entry["fileId"], fileId);
                        entry["csHash"] = cs["hash"]->getString();
                        entry["status"] = "paid";
                        entry["piece"] = fp;

                        // update piece in file progress for later assembly
                        toUpdate["pieces"][index] = fp;
                     }
                  }
               }
            }
         }
      }
   }

   // create message regarding operation completion
   DynamicObject msg;
   msg["dataPurchased"] = success;
   if(!success)
   {
      ExceptionRef e = new Exception(
         "Could not purchase data.",
         EXCEPTION_PURCHASER ".PurchaseFailed");
      Exception::push(e);
      msg["exception"] = Exception::convertToDynamicObject(e);
   }

   // send message to self
   messageSelf(msg);
}

void Purchaser::processMessages()
{
   // set processor ID on download state
   mPurchaseDatabase->setDownloadStateProcessorId(mDownloadState, 0, getId());

   // send purchase started event
   Event e;
   e["type"] = EVENT_DOWNLOAD_STATE ".purchaseStarted";
   sendDownloadStateEvent(e);

   // FIXME: in the future, this will not be necessary, but it is
   // enforced for now... in the future payment could be collected
   // before all pieces are received
   // ensure there are no remaining pieces
   if(mDownloadState["remainingPieces"]->getUInt32() != 0)
   {
      // failed to purchase the license
      ExceptionRef e = new Exception(
         "Could not purchase license for the media. "
         "Not all pieces have been received yet.",
         EVENT_DOWNLOAD_STATE ".LicensePurchaseFailure");
      Exception::set(e);
   }
   else
   {
      // do license purchase, if not already done
      if(!mDownloadState["licensePurchased"]->getBoolean())
      {
         if(purchaseLicense())
         {
            // send event
            Event e;
            e["type"] = EVENT_DOWNLOAD_STATE ".licensePurchased";
            sendDownloadStateEvent(e);
         }
         else
         {
            // failed to purchase the license
            ExceptionRef e = new Exception(
               "Could not purchase license for the media.",
               EVENT_DOWNLOAD_STATE ".LicensePurchaseFailure");
            Exception::set(e);
         }
      }
   }

   // do data purchase if license was purchased and not already done
   if(mDownloadState["licensePurchased"]->getBoolean() &&
      !mDownloadState["dataPurchased"]->getBoolean())
   {
      if(purchaseData())
      {
         // send event
         Event e;
         e["type"] = EVENT_DOWNLOAD_STATE ".dataPurchased";
         sendDownloadStateEvent(e);
      }
      else
      {
         // failed to purchase the data
         ExceptionRef e = new Exception(
            "Could not purchase the contract data.",
            EVENT_DOWNLOAD_STATE ".DataPurchaseFailure");
         Exception::push(e);
      }
   }

   // stop processing download state
   mPurchaseDatabase->stopProcessingDownloadState(mDownloadState, getId());

   if(mDownloadState["licensePurchased"]->getBoolean() &&
      mDownloadState["dataPurchased"]->getBoolean())
   {
      // purchase complete, fire event and exit
      Event e;
      e["type"] = EVENT_DOWNLOAD_STATE ".purchaseCompleted";
      sendDownloadStateEvent(e);
   }
   else
   {
      // send error event
      sendErrorEvent();
   }
}

bool Purchaser::purchaseLicense()
{
   bool rval = false;

   // set the license purchased state
   mDownloadState["licensePurchased"] = false;

   // first sign license if necessary
   // get user's profile to use as signing delegate
   ProfileRef profile;
   UserId userId = BM_USER_ID(mDownloadState["userId"]);
   Contract& c = mDownloadState["contract"];

   if(mNode->getLoginData(userId, NULL, &profile))
   {
      rval = (c->hasMember("signature") ?
         true : Signer::signContract(c, profile));
   }

   if(rval)
   {
      // include buyer's account ID
      BM_ID_SET(
         mDownloadState["contract"]["buyer"]["accountId"],
         BM_ACCOUNT_ID(mDownloadState["preferences"]["accountId"]));

      // run a new purchase license operation
      RunnableRef r = new RunnableDelegate<Purchaser>(
         this, &Purchaser::runPurchaseLicenseOp);
      Operation op = r;
      getNode()->runOperation(op);

      // wait for licensePurchased message
      const char* keys[] = {"licensePurchased", NULL};
      DynamicObject msg = waitForMessages(keys, &op)[0];
      rval = msg["licensePurchased"]->getBoolean();
      if(rval)
      {
         logDownloadStateMessage("license purchased");
         logDownloadStateMessage("inserting license into database...");

         // FIXME: license may be purchased but fail to be written
         // to database -- how do we handle this? this is worst than
         // data not being purchased because there is a protection
         // against double-purchasing data -- this does not exist
         // with the license

         // insert purchased license into database
         rval = mPurchaseDatabase->updateContract(mDownloadState);
         if(rval)
         {
            // FIXME: handle flag setting database error?
            mDownloadState["licensePurchased"] = true;
            mPurchaseDatabase->updateDownloadStateFlags(mDownloadState);
         }
      }
      else
      {
         // convert exception from message
         ExceptionRef e = Exception::convertToException(msg["exception"]);
         Exception::set(e);
      }
   }

   return rval;
}

bool Purchaser::purchaseData()
{
   bool rval = false;

   // set the data purchased state
   mDownloadState["dataPurchased"] = false;

   // run a new purchase data operation
   RunnableRef r = new RunnableDelegate<Purchaser>(
      this, &Purchaser::runPurchaseDataOp);
   Operation op = r;
   getNode()->runOperation(op);

   // wait for dataPurchased message
   const char* keys[] = {"dataPurchased", NULL};
   DynamicObject msg = waitForMessages(keys, &op)[0];
   rval = msg["dataPurchased"]->getBoolean();
   if(rval)
   {
      logDownloadStateMessage("data purchased");
      logDownloadStateMessage("inserting data keys into database...");

      // insert decryption keys into database (update progress)
      rval = mPurchaseDatabase->updateFileProgress(mDownloadState, mDbEntries);
      if(rval)
      {
         // FIXME: handle flag setting database error?
         mDownloadState["dataPurchased"] = true;
         mPurchaseDatabase->updateDownloadStateFlags(mDownloadState);
      }
   }
   else
   {
      // convert exception from message
      ExceptionRef e = Exception::convertToException(msg["exception"]);
      Exception::set(e);
   }

   return rval;
}

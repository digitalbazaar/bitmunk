/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk/purchase/PieceDownloader.h"

#include "bitmunk/purchase/PurchaseModule.h"
#include "monarch/event/ObserverDelegate.h"
#include "monarch/io/FileOutputStream.h"
#include "monarch/rt/RunnableDelegate.h"
#include "monarch/util/Timer.h"

using namespace std;
using namespace bitmunk::common;
using namespace bitmunk::protocol;
using namespace bitmunk::purchase;
using namespace bitmunk::node;
using namespace monarch::event;
using namespace monarch::fiber;
using namespace monarch::http;
using namespace monarch::io;
using namespace monarch::modest;
using namespace monarch::net;
using namespace monarch::rt;
using namespace monarch::util;

#define EVENT_DOWNLOAD_STATE "bitmunk.purchase.DownloadState"

PieceDownloader::PieceDownloader(Node* node, FiberId parentId) :
   NodeFiber(node, parentId),
   DownloadStateFiber(node, "PieceDownloader", &mFiberExitData),
   mUniqueId(0),
   mPieceSize(0),
   mSection(NULL),
   mFileId(NULL),
   mFilePiece(NULL),
   mConnection(NULL),
   mRequest(NULL),
   mResponse(NULL),
   mPieceDownloadRate(NULL),
   mDownloadRate(NULL),
   mInterrupted(false)
{
}

PieceDownloader::~PieceDownloader()
{
   if(mFileId != NULL)
   {
      free(mFileId);
   }
}

void PieceDownloader::initialize(
   uint32_t id,
   uint32_t pieceSize,
   ContractSection& cs, FileId fileId, FilePiece& fp,
   RateAverager* pa, RateAverager* ra)
{
   mUniqueId = id;
   mPieceSize = pieceSize;
   mSection = cs.clone();
   mFileId = strdup(fileId);
   mFilePiece = fp.clone();
   mPieceDownloadRate = pa;
   mDownloadRate = ra;
}

void PieceDownloader::download()
{
   /* Algorithm:

   1. If download failed, send event, notify parent, exit.
   2. If download would block, register with IOWatcher.
   3. If download finished, send event, notify parent, exit.

   */

   bool error = false;

   // connect if not already connected
   if(mConnection == NULL)
   {
      if(connect())
      {
         // FIXME: set connection to asynchronous IO when writing optimization
         // to use IOMonitor (and maybe even move this out of an operation)

         // add bandwidth throttler to connection
         mConnection->setBandwidthThrottler(mDownloadThrottler, true);
      }
      else
      {
         error = true;
      }
   }

   if(!error)
   {
      logDownloadStateMessage("downloading piece from seller...");

      // send an event that the download has started
      sendEvent(false, EVENT_DOWNLOAD_STATE ".pieceStarted");

      // FIXME: when switching to IOMonitor this will be done in the fiber
      // with IO scheduling handled at the appropriate layer

      // keep receiving and writing data while success
      bool success = true;
      char b[2048];
      int numBytes;
      uint64_t start = System::getCurrentMilliseconds();
      while(success && (numBytes = mInputStream->read(b, 2048)) > 0)
      {
         mTotalDownloadRate->addItems(numBytes, start);
         mDownloadRate->addItems(numBytes, start);
         mPieceDownloadRate->addItems(numBytes, start);
         success = success && mOutputStream->write(b, numBytes);
         start = System::getCurrentMilliseconds();
      }

      if(success && numBytes == 0)
      {
         // check content security
         mInMessage.checkContentSecurity(
            mResponse->getHeader(), &(*mTrailer), &(*mSignature));
         if(mInMessage.getSecurityStatus() == BtpMessage::Breach)
         {
            // set exception
            ExceptionRef e = new Exception(
               "Message security breach.",
               "bitmunk.protocol.Security");
            e->getDetails()["resource"] = mUrl.toString().c_str();
            Exception::set(e);
            success = false;
         }
      }

      if(numBytes < 0 || !success)
      {
         ExceptionRef e = Exception::get();
         if(true)//!e->getDetails()->hasMember("wouldBlock"))
         {
            // download failed:
            error = true;

            // disconnect
            mConnection->close();

            // clean up
            delete mConnection;
            delete mRequest;
            delete mResponse;
            mOutputStream->close();
            mOutputStream.setNull();
         }
         else
         {
            // download would block, register with IOWatcher (which
            // will wakeup piece downloader to run step 2 again)
            // FIXME: use &PieceDownloader::fdUpdated;
         }
      }
      else
      {
         // disconnect
         mConnection->close();

         // clean up
         delete mConnection;
         delete mRequest;
         delete mResponse;
         mOutputStream->close();
         mOutputStream.setNull();

         // download finished
         logDownloadStateMessage("piece download finished");

         MO_CAT_INFO(BM_PURCHASE_CAT,
            "UserId %" PRIu64 ", DownloadState %" PRIu64 ": "
            "piece download finished, rate: %g bytes/s",
            BM_USER_ID(mDownloadState["userId"]),
            mDownloadState["id"]->getUInt64(),
            mPieceDownloadRate->getTotalItemsPerSecond());

         // get file piece trailers:
         string value;

         // actual size of file piece
         mTrailer->getField("Bitmunk-Piece-Size", value);
         mFilePiece["size"] = value.c_str();
         // convert
         mFilePiece["size"]->setType(UInt32);
         // FIXME: check errno

         // bfp signature on piece
         mTrailer->getField("Bitmunk-Bfp-Signature", value);
         mFilePiece["bfpSignature"] = value.c_str();

         // seller signature on piece + seller profile ID
         mTrailer->getField("Bitmunk-Seller-Signature", value);
         mFilePiece["sellerSignature"] = value.c_str();
         mTrailer->getField("Bitmunk-Seller-Profile-Id", value);
         BM_ID_SET(mFilePiece["sellerProfileId"], value.c_str());

         // open key information (to be sent to SVA to unlock piece key)
         mTrailer->getField("Bitmunk-Open-Key-Algorithm", value);
         mFilePiece["openKey"]["algorithm"] = value.c_str();
         mTrailer->getField("Bitmunk-Open-Key-Data", value);
         mFilePiece["openKey"]["data"] = value.c_str();
         mTrailer->getField("Bitmunk-Open-Key-Length", value);
         mFilePiece["openKey"]["length"] = value.c_str();
         // convert
         mFilePiece["openKey"]["length"]->setType(UInt32);
         // FIXME: check errno

         // encrypted piece key information
         // (to be unlocked with open key by SVA)
         mTrailer->getField("Bitmunk-Piece-Key-Algorithm", value);
         mFilePiece["pieceKey"]["algorithm"] = value.c_str();
         mTrailer->getField("Bitmunk-Piece-Key-Data", value);
         mFilePiece["pieceKey"]["data"] = value.c_str();
         mTrailer->getField("Bitmunk-Piece-Key-Length", value);
         mFilePiece["pieceKey"]["length"] = value.c_str();
         // convert
         mFilePiece["pieceKey"]["length"]->setType(UInt32);
         // FIXME: check errno

         // set piece to encrypted and ciphered
         mFilePiece["encrypted"] = true;
         mFilePiece["ciphered"] = true;
      }
   }

   // create message regarding operation completion
   DynamicObject msg;
   msg["pieceReceived"] = !error;
   if(error)
   {
      msg["exception"] = Exception::getAsDynamicObject();
   }

   // send message to self
   messageSelf(msg);
}

void PieceDownloader::fdUpdated(int fd, int events)
{
   // FIXME: this code must be changed to send a message to self like the
   // download operation does -- this ensures that interrupting this fiber
   // happens cleanly

   /*
   // FIXME: determine if error based on events
   bool error = false;

   if(error)
   {
      // create exception for given IO error event
      // FIXME: get specific event error
      ExceptionRef e = new Exception(
         "FIXME: I am an exception that occurred while waiting for IO "
         "in the purchase module's PieceDownloader class");
      sendEvent(true, NULL);
   }
   */
}

void PieceDownloader::processMessages()
{
   logDownloadStateMessage("starting...");

   // send message to parent that piece has been started
   DynamicObject msg;
   msg["pieceDownloaderId"] = mUniqueId;
   msg["downloadStateId"] = mDownloadState["id"]->getUInt64();
   BM_ID_SET(msg["userId"], BM_USER_ID(mDownloadState["userId"]));
   msg["pieceStarted"] = true;
   msg["piece"] = mFilePiece.clone();
   msg["section"] = mSection.clone();
   BM_ID_SET(msg["fileId"], mFileId);
   messageParent(msg);

   // run a new download operation
   RunnableRef r = new RunnableDelegate<PieceDownloader>(
      this, &PieceDownloader::download);
   Operation op = r;
   getNode()->runOperation(op);

   // wait for messages
   const char* keys[] =
      {"interrupt", "pieceReceived", "pieceFailed", NULL};
   bool pieceReceived = false;
   bool done = false;
   while(!done)
   {
      DynamicObject list = waitForMessages(keys);
      DynamicObjectIterator i = list.getIterator();
      while(i->hasNext())
      {
         DynamicObject& msg = i->next();

         // if interrupted, interrupt operation
         if(msg->hasMember("interrupt"))
         {
            // ensure unique piece downloader ID matches
            if(mUniqueId == msg["pieceDownloaderId"]->getUInt32())
            {
               // piece downloader interrupted
               mInterrupted = true;

               // send interruption event if pause is false
               if(!msg["pause"]->getBoolean())
               {
                  ExceptionRef e = new Exception(
                     "PieceDownloader interrupted.",
                     "bitmunk.purchase.PieceDownloader.interrupted");
                  Exception::set(e);
                  sendEvent(true, NULL);
               }

               // interrupt download operation
               op->interrupt();
            }
         }
         // message regards receiving piece
         else if(msg["pieceReceived"]->getBoolean())
         {
            sendEvent(false, EVENT_DOWNLOAD_STATE ".pieceFinished");
            pieceReceived = done = true;
         }
         // piece failed message
         else
         {
            // only send an error event if there was no interruption
            if(!mInterrupted)
            {
               // convert the exception from a dynamic object, send error event
               ExceptionRef e = Exception::convertToException(msg["exception"]);
               Exception::set(e);
               sendEvent(true, NULL);
            }

            done = true;
         }
      }
   }

   if(pieceReceived)
   {
      logDownloadStateMessage("finished with success");

      // send message to parent that piece has been received
      DynamicObject msg;
      msg["pieceDownloaderId"] = mUniqueId;
      msg["downloadStateId"] = mDownloadState["id"]->getUInt64();
      BM_ID_SET(msg["userId"], BM_USER_ID(mDownloadState["userId"]));
      msg["pieceReceived"] = true;
      msg["piece"] = mFilePiece;
      msg["section"] = mSection;
      BM_ID_SET(msg["fileId"], mFileId);
      msg["pieceRate"] = mPieceDownloadRate->getTotalItemsPerSecond();
      messageParent(msg);
   }
   else
   {
      logDownloadStateMessage("finished with error");

      // send message to parent that piece has failed
      DynamicObject msg;
      msg["pieceDownloaderId"] = mUniqueId;
      msg["downloadStateId"] = mDownloadState["id"]->getUInt64();
      BM_ID_SET(msg["userId"], BM_USER_ID(mDownloadState["userId"]));
      msg["pieceFailed"] = true;
      msg["piece"] = mFilePiece;
      msg["section"] = mSection;
      BM_ID_SET(msg["fileId"], mFileId);
      msg["pieceRate"] = mPieceDownloadRate->getTotalItemsPerSecond();
      messageParent(msg);
   }
}

bool PieceDownloader::connect()
{
   bool rval = false;

   // get buyer's profile
   ProfileRef profile;
   UserId userId = BM_USER_ID(mDownloadState["userId"]);
   if((rval = getNode()->getLoginData(userId, NULL, &profile)))
   {
      // find media ID for file ID
      MediaId mediaId = 0;
      FileInfoIterator fii = mSection["ware"]["fileInfos"].getIterator();
      while(mediaId == 0 && fii->hasNext())
      {
         FileInfo& fi = fii->next();
         if(BM_FILE_ID_EQUALS(BM_FILE_ID(fi["id"]), mFileId))
         {
            mediaId = BM_MEDIA_ID(fi["mediaId"]);
         }
      }

      // create piece request
      DynamicObject pieceRequest;
      pieceRequest["csHash"] = mSection["hash"]->getString();
      BM_ID_SET(pieceRequest["fileId"], mFileId);
      BM_ID_SET(pieceRequest["mediaId"], mediaId);
      pieceRequest["index"] = mFilePiece["index"]->getUInt32();
      // use full standard piece size, seller may truncate if last piece
      pieceRequest["size"] = mPieceSize;
      pieceRequest["peerbuyKey"] = mSection["peerbuyKey"]->getString();
      BM_ID_SET(
         pieceRequest["sellerProfileId"],
         BM_PROFILE_ID(mSection["seller"]["profileId"]));
      BM_ID_SET(pieceRequest["bfpId"], BM_BFP_ID(mFilePiece["bfpId"]));

      // setup btp messages
      mOutMessage.setDynamicObject(pieceRequest);
      mOutMessage.setType(BtpMessage::Post);
      mOutMessage.setUserId(userId);
      mOutMessage.setAgentProfile(profile);
      mInMessage.setPublicKeySource(getNode()->getPublicKeyCache());

      // create url to get file piece
      mUrl.format("%s/api/3.0/sales/contract/filepiece?nodeuser=%" PRIu64,
         mSection["seller"]["url"]->getString(),
         BM_USER_ID(mSection["seller"]["userId"]));

      MO_CAT_INFO(BM_PURCHASE_CAT,
         "UserId %" PRIu64 ", DownloadState %" PRIu64 ": "
         "connecting to seller (%" PRIu64 ":%u) @ %s",
         BM_USER_ID(mDownloadState["userId"]),
         mDownloadState["id"]->getUInt64(),
         BM_USER_ID(mSection["seller"]["userId"]),
         BM_SERVER_ID(mSection["seller"]["serverId"]),
         mUrl.toString().c_str());

      // connect to seller
      // bfp->startReading() may take more than 30 seconds to execute
      // on seller side, so clients should set their read timeouts to
      // something high like 2-5 minutes
      // FIXME: this may need to be done in the IOMonitor/IOWatcher, not
      // on the connection
      BtpClient* btpc = getNode()->getMessenger()->getBtpClient();
      mConnection = btpc->createConnection(
         BM_USER_ID(mSection["seller"]["userId"]), &mUrl, 120);
      if((rval = (mConnection != NULL)))
      {
         logDownloadStateMessage("connected to seller...");

         // create request and response
         mRequest = mConnection->createRequest();
         mResponse = mRequest->createResponse();

         // send message
         if((rval = btpc->sendMessage(
            &mUrl, &mOutMessage, mRequest, &mInMessage, mResponse)))
         {
            logDownloadStateMessage("preparing to download piece...");

            // get receive content stream
            mInMessage.getContentReceiveStream(
               mResponse, mInputStream, mTrailer, mSignature);

            // set up output stream
            File file(mFilePiece["path"]->getString());
            if(!(rval = file->mkdirs()))
            {
               ExceptionRef e = new Exception(
                  "Failed to create output directory for downloading file "
                  "pieces.",
                  "bitmunk.purchase.PieceDownloader.OutputFileWriteError");
               e->getDetails()["path"] = file->getPath();
               Exception::push(e);
            }
            else
            {
               mOutputStream = new FileOutputStream(file);
            }
         }
      }

      if(!rval)
      {
         // clean up
         if(mConnection != NULL)
         {
            mConnection->close();
            delete mConnection;
            delete mRequest;
            delete mResponse;
         }
      }
   }

   return rval;
}
// FIXME: clean these parameters up, its ugly
void PieceDownloader::sendEvent(bool error, const char* type)
{
   Event e;

   if(error)
   {
      // set failure event
      e["type"] = EVENT_DOWNLOAD_STATE ".exception";
      e["details"]["exception"] = Exception::getAsDynamicObject();
   }
   else
   {
      // send success event
      e["type"] = type;
   }

   // send event
   BM_ID_SET(e["details"]["fileId"], mFileId);
   e["details"]["piece"] = mFilePiece;
   e["details"]["downloaded"] = mPieceDownloadRate->getTotalItemCount();
   sendDownloadStateEvent(e);
}

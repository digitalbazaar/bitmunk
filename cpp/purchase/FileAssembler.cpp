/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk/purchase/FileAssembler.h"

#include "bitmunk/bfp/IBfpModule.h"
#include "bitmunk/common/Tools.h"
#include "bitmunk/common/TypeDefinitions.h"
#include "bitmunk/medialibrary/IMediaLibrary.h"
#include "bitmunk/purchase/FileAssembler.h"
#include "bitmunk/purchase/PurchaseModule.h"
#include "monarch/config/ConfigManager.h"
#include "monarch/io/FileInputStream.h"
#include "monarch/io/FileOutputStream.h"
#include "monarch/rt/RunnableDelegate.h"

using namespace std;
using namespace bitmunk::bfp;
using namespace bitmunk::common;
using namespace bitmunk::medialibrary;
using namespace bitmunk::purchase;
using namespace bitmunk::node;
using namespace monarch::config;
using namespace monarch::event;
using namespace monarch::io;
using namespace monarch::modest;
using namespace monarch::rt;

#define EVENT_DOWNLOAD_STATE   "bitmunk.purchase.DownloadState"
#define EXCEPTION_FILE_ASM     "bitmunk.purchase.FileAssembler"

#define BUFFER_SIZE 2048

FileAssembler::FileAssembler(Node* node) :
   NodeFiber(node),
   DownloadStateFiber(node, "FileAssembler", &mFiberExitData),
   mBfp(NULL),
   mFileInfo(NULL)
{
}

FileAssembler::~FileAssembler()
{
}

static IBfpModule* getIBfpInstance(Node* node)
{
   // get the BFP module interface
   IBfpModule* rval = dynamic_cast<IBfpModule*>(
      node->getModuleApi("bitmunk.bfp.Bfp"));
   if(rval == NULL)
   {
      ExceptionRef e = new Exception(
         "Could not get bfp module interface. No bfp module found.",
         "bitmunk.bfp.MissingBfpModule");
      Exception::set(e);
   }

   return rval;
}

void FileAssembler::acquireBfp()
{
   // get the BFP module interface
   IBfpModule* ibm = getIBfpInstance(mNode);
   if(ibm != NULL)
   {
      // create bfp
      mBfp = ibm->createBfp(mBfpId);
      if(mBfp != NULL)
      {
         // initialize the BFP for PeerBuy
         mBfp->initializePeerBuy(mDownloadState["contract"]);
      }
   }

   // send message to FileAssembler
   // set the success value based on if a BFP was successfully allocated
   DynamicObject msg;
   msg["bfpAcquired"] = (mBfp != NULL);
   if(mBfp == NULL)
   {
      // failed to acquire bfp
      ExceptionRef e = new Exception(
         "Could not acquire Bitmunk file processor (BFP).",
         EXCEPTION_FILE_ASM ".BfpRetrievalFailure");
      Exception::push(e);
      msg["exception"] = Exception::convertToDynamicObject(e);
   }
   messageSelf(msg);
}

void FileAssembler::checkFileId()
{
   // copy relevant file info data
   FileInfo fi;
   fi["extension"] = mFileInfo["extension"]->getString();
   fi["path"] = mFileInfo["path"]->getString();

   // create message to send to self
   DynamicObject msg;
   msg["fileVerified"] = false;

   // set file ID on new file info
   if(mBfp->setFileInfoId(fi))
   {
      // compare file IDs
      if(strcmp(fi["id"]->getString(), mFileInfo["id"]->getString()) == 0)
      {
         msg["fileVerified"] = true;
      }
      else
      {
         ExceptionRef e = new Exception(
            "Assembled file's ID does not match source file.",
            EXCEPTION_FILE_ASM ".VerifyFileFailure");
         e->getDetails()["validFileId"] = mFileInfo["id"]->getString();
         e->getDetails()["invalidFileId"] = fi["id"]->getString();
         Exception::set(e);
      }
   }

   if(!msg["fileVerified"]->getBoolean())
   {
      // failed to verify file
      ExceptionRef e = new Exception(
         "Could not verify assembled file.",
         EXCEPTION_FILE_ASM ".VerifyFileFailure");
      FileInfo tmp = mFileInfo.clone();
      tmp["pieces"]->clear();
      e->getDetails()["fileInfo"] = tmp;
      Exception::push(e);
      msg["exception"] = Exception::convertToDynamicObject(e);
   }

   // send message to FileAssembler
   messageSelf(msg);
}

void FileAssembler::processMessages()
{
   // set processor ID on download state
   mPurchaseDatabase->setDownloadStateProcessorId(mDownloadState, 0, getId());

   logDownloadStateMessage("file assembly started");

   // first ensure license and data have been purchased
   bool error = !checkPurchase();
   if(!error)
   {
      // send file assembly started event
      Event e;
      e["type"] = EVENT_DOWNLOAD_STATE ".assemblyStarted";
      sendDownloadStateEvent(e);

      // build file info list
      FileInfoList fil;
      fil->setType(Array);
      FileProgressIterator fpi = mDownloadState["progress"].getIterator();
      while(fpi->hasNext())
      {
         FileProgress& fp = fpi->next();
         fil->append(fp["fileInfo"]);
      }

      // get download path for files
      UserId userId = BM_USER_ID(mDownloadState["userId"]);
      string downloadPath = mNode->getConfigManager()->getModuleUserConfig(
         "bitmunk.purchase.Purchase", userId)["downloadPath"]->getString();
      if(!mNode->getConfigManager()->expandUserDataPath(
         downloadPath.c_str(), userId, downloadPath))
      {
         // failed to reassemble the files because of a download path error
         ExceptionRef e = new Exception(
            "Could not get download path for file assembly.",
            EXCEPTION_FILE_ASM ".DownloadPathError");
         e->getDetails()["downloadPath"] = downloadPath.c_str();
         e->getDetails()["userId"] = userId;
         Exception::set(e);
         error = true;
      }
      else
      {
         // iterate over all files
         FileInfoIterator fii = fil.getIterator();
         while(!error && fii->hasNext())
         {
            FileInfo& fi = fii->next();
            FileId fileId = fi["id"]->getString();
            SellerPool& sp = mDownloadState["progress"][fileId]["sellerPool"];
            mBfpId = BM_BFP_ID(sp["bfpId"]);

            // ensure bfp for file is loaded
            error = !loadBfp();
            if(!error)
            {
               // reassemble the next file
               error = !assembleFile(fi, downloadPath.c_str());
               yield();
            }

            if(!error)
            {
               // verify that the assembled file is valid
               error = !verifyFile(fi);
            }

            if(!error)
            {
               // add file to database
               error = !mPurchaseDatabase->insertAssembledFile(
                  mDownloadState, fi["id"]->getString(),
                  fi["path"]->getString());
            }

            if(!error)
            {
               // add file to media library if one is available
               IMediaLibrary* iml = dynamic_cast<IMediaLibrary*>(
                  mNode->getModuleApiByType("bitmunk.medialibrary"));
               if(iml != NULL)
               {
                  DynamicObject userData;
                  userData["source"] = "purchase";
                  userData["ware"] = mDownloadState["ware"];
                  iml->scanFile(
                     BM_USER_ID(mDownloadState["userId"]), fi, true,
                     &userData);
               }
            }

            if(!error)
            {
               // delete the temporary file pieces
               error = !cleanupFilePieces(fi);
            }

            // free the BFP, if one was loaded
            if(mBfp != NULL)
            {
               // get the BFP module interface
               IBfpModule* ibm = getIBfpInstance(mNode);
               ibm->freeBfp(mBfp);
               mBfp = NULL;
            }
         }
      }
   }

   if(error)
   {
      // send error event
      logDownloadStateMessage("file assembly failed");

      // stop processing download state
      mPurchaseDatabase->stopProcessingDownloadState(mDownloadState, getId());

      // failed to assemble file
      ExceptionRef e = new Exception(
         "Failed to assemble file.",
         EXCEPTION_FILE_ASM ".AssemblyFailed");
      Exception::push(e);
      sendErrorEvent();
   }
   else
   {
      logDownloadStateMessage("file assembly finished");

      // all files have been assembled, finish processing
      mDownloadState["filesAssembled"] = true;

      // update download state flags
      mPurchaseDatabase->updateDownloadStateFlags(mDownloadState);

      // FIXME: remove download state from database instead of updating flags?

      // stop processing download state
      mPurchaseDatabase->stopProcessingDownloadState(mDownloadState, getId());

      // send success event, include some more specific information because
      // the download state may be deleted in future code
      Event e;
      e["type"] = EVENT_DOWNLOAD_STATE ".assemblyCompleted";
      e["details"]["userId"] = mDownloadState["userId"];
      e["details"]["ware"] = mDownloadState["ware"].clone();
      sendDownloadStateEvent(e);
   }
}

bool FileAssembler::checkPurchase()
{
   bool rval = false;

   // check to make sure that the data and license were purchased
   // and that the list of file infos to download is ready.
   if(!mDownloadState["licensePurchased"]->getBoolean())
   {
      // failed to reassemble files because the license wasn't purchased
      ExceptionRef e = new Exception(
         "Could not assemble files. License has not been purchased.",
         EXCEPTION_FILE_ASM ".LicenseNotPurchased");
      Exception::set(e);
   }
   else if(!mDownloadState["dataPurchased"]->getBoolean())
   {
      // failed to reassemble files because the data wasn't purchased
      ExceptionRef e = new Exception(
         "Could not assemble files. Data has not been purchased.",
         EXCEPTION_FILE_ASM ".DataNotPurchased");
      Exception::set(e);
   }
   else
   {
      rval = true;
   }

   return rval;
}

bool FileAssembler::loadBfp()
{
   bool rval;

   // run a new acquire bfp operation
   RunnableRef r = new RunnableDelegate<FileAssembler>(
      this, &FileAssembler::acquireBfp);
   Operation op = r;
   getNode()->runOperation(op);

   // wait for bfpAcquired message
   const char* keys[] = {"bfpAcquired", NULL};
   DynamicObject msg = waitForMessages(keys, &op)[0];
   rval = msg["bfpAcquired"]->getBoolean();
   if(!rval)
   {
      // convert exception from message, send error event
      ExceptionRef e = Exception::convertToException(msg["exception"]);
      Exception::set(e);
   }

   return rval;
}

bool FileAssembler::assembleFile(FileInfo& fi, const char* downloadPath)
{
   bool rval = false;

   // populate media so a filename can be generated
   Media media;
   Media collection;
   if(!populateMediaAndCollectionDetails(fi, media, collection))
   {
      // media title information isn't available
      ExceptionRef e = new Exception(
         "Could not assemble file. The media title information isn't "
         "available for the file.",
         EXCEPTION_FILE_ASM ".MediaTitleMissing");
      BM_ID_SET(e->getDetails()["mediaId"], BM_MEDIA_ID(fi["mediaId"]));
      Exception::set(e);
   }
   else
   {
      // generate filename
      const char* extension = fi["extension"]->getString();
      string filename = downloadPath;
      filename.push_back('/');
      filename.append(Tools::createMediaFilename(
         collection, BM_MEDIA_ID(fi["mediaId"]), extension));
      fi["path"] = filename.c_str();

      // assemble pieces
      rval = assemblePieces(fi);
      if(rval)
      {
         // send success event
         Event e;
         e["type"] = EVENT_DOWNLOAD_STATE ".fileAssembled";
         sendDownloadStateEvent(e);
      }
   }

   return rval;
}

/**
 * A helper function to get a unique filename.
 *
 * @param file the file with a filename to make unique.
 */
static void getUniqueFilename(File& file)
{
   // check if filename already exists
   if(file->exists())
   {
      // file exists so loop to find a new name
      // FIXME: this code will find next available id starting from
      //        1.  a more user-friendly approach would be to find
      //        largest id file and add one
      const char* name = file->getAbsolutePath();
      string root;
      string ext;
      File::splitext(name, root, ext);
      bool done = false;
      uint64_t seqId = 0;
      while(!done)
      {
         // try "file.ext" => "file (#).ext"
         string::size_type nextLen = root.length() + 3 + 20 + ext.length();
         char nextPath[nextLen];
         snprintf(nextPath, nextLen, "%s (%" PRIu64 ")%s",
            root.c_str(), ++seqId, ext.c_str());
         File nextFile(nextPath);
         done = !nextFile->exists();
         if(done)
         {
            file = nextFile;
            file->create();
         }
      }
   }
}

bool FileAssembler::assemblePieces(FileInfo& fi)
{
   bool rval = true;

   File outputFile(fi["path"]->getString());
   if(!outputFile->mkdirs())
   {
      // stop processing if there was a mkdirs error
      ExceptionRef e = new Exception(
         "Failed to create output directory when assembling file.",
         EXCEPTION_FILE_ASM ".OutputFileDirectoryError");
      e->getDetails()["path"] = outputFile->getPath();
      Exception::push(e);
      rval = false;
   }
   else
   {
      // FIXME: move unique filename code into File class?
      getUniqueFilename(outputFile);
      fi["path"] = outputFile->getAbsolutePath();
      FileOutputStream* fos = new FileOutputStream(outputFile);

      // allocate the buffer
      char* buffer = (char*)malloc(BUFFER_SIZE);

      MO_CAT_DEBUG(BM_PURCHASE_CAT,
         "%s: UserId %" PRIu64 ", DownloadState %" PRIu64 ": "
         "preparing file '%s'",
         mName,
         BM_USER_ID(mDownloadState["userId"]),
         mDownloadState["id"]->getUInt64(),
         fi["id"]->getString());

      // prepare peerbuy file
      rval = mBfp->preparePeerBuyFile(fi);

      // setup the piece iterator
      FilePieceIterator fpi = fi["pieces"].getIterator();
      while(rval && fpi->hasNext())
      {
         // retrieve the next file piece to process
         FilePiece& fp = fpi->next();

         MO_CAT_DEBUG(BM_PURCHASE_CAT,
            "%s: UserId %" PRIu64 ", DownloadState %" PRIu64 ": "
            "preparing piece '%s'",
            mName,
            BM_USER_ID(mDownloadState["userId"]),
            mDownloadState["id"]->getUInt64(),
            fp["path"]->getString());

         // start reading the file piece
         if(!mBfp->startReading(fp))
         {
            ExceptionRef e = new Exception(
               "Failed to read input file when assembling file.",
               EXCEPTION_FILE_ASM ".ReadError");
            e->getDetails()["path"] = fi["path"]->getString();
            Exception::push(e);
            rval = false;
         }
         else
         {
            // read from the BFP and write it to the output stream
            int numBytes = -1;
            while(rval && (numBytes = mBfp->read(buffer, BUFFER_SIZE)) > 0)
            {
               // yield after reading
               yield();

               if(!fos->write(buffer, numBytes))
               {
                  // stop processing if there was a read error of some kind
                  ExceptionRef e = new Exception(
                     "Failed to write to output file when assembling file.",
                     EXCEPTION_FILE_ASM ".WriteError");
                  e->getDetails()["path"] = fi["path"]->getString();
                  Exception::push(e);
                  rval = false;
               }

               // yield after writing
               yield();
            }

            if(numBytes < 0)
            {
               // stop processing if there was a read error of some kind
               ExceptionRef e = new Exception(
                  "Failed to read from input file when assembling file.",
                  EXCEPTION_FILE_ASM ".ReadError");
               e->getDetails()["path"] = fp["path"]->getString();
               Exception::push(e);
               rval = false;
            }
         }

         if(rval)
         {
            // yield before next piece
            yield();
         }
      }

      // clean up buffer and output stream
      free(buffer);
      fos->close();
      delete fos;
   }

   return rval;
}

bool FileAssembler::verifyFile(FileInfo& fi)
{
   bool rval = false;

   // save the file info
   mFileInfo = fi;

   // run a new check file ID operation
   RunnableRef r = new RunnableDelegate<FileAssembler>(
      this, &FileAssembler::checkFileId);
   Operation op = r;
   getNode()->runOperation(op);

   // wait for fileVerified message
   const char* keys[] = {"fileVerified", NULL};
   DynamicObject msg = waitForMessages(keys, &op)[0];
   rval = msg["fileVerified"]->getBoolean();
   if(!rval)
   {
      // convert exception from message, send error event
      ExceptionRef e = Exception::convertToException(msg["exception"]);
      Exception::set(e);
   }
   else
   {
      // send verified event
      Event e;
      e["type"] = EVENT_DOWNLOAD_STATE ".fileVerified";
      sendDownloadStateEvent(e);
   }

   return rval;
}

bool FileAssembler::cleanupFilePieces(FileInfo& fi)
{
   bool rval = true;

   // iterate over the file pieces, removing all of them
   uint32_t pieceCount = 0;
   FilePieceIterator fpi = fi["pieces"].getIterator();
   while(rval && fpi->hasNext())
   {
      FilePiece& fp = fpi->next();

      MO_CAT_DEBUG(BM_PURCHASE_CAT,
         "%s: UserId %" PRIu64 ", DownloadState %" PRIu64 ": "
         "removing piece '%s'",
         mName,
         BM_USER_ID(mDownloadState["userId"]),
         mDownloadState["id"]->getUInt64(),
         fp["path"]->getString());

      // remove file piece
      File file(fp["path"]->getString());
      file->remove();
      pieceCount++;
      yield();
   }

   MO_CAT_INFO(BM_PURCHASE_CAT,
      "%s: UserId %" PRIu64 ", DownloadState %" PRIu64 ": "
      "cleaned up %u pieces for file ID '%s'",
      mName,
      BM_USER_ID(mDownloadState["userId"]),
      mDownloadState["id"]->getUInt64(),
      pieceCount,
      fi["id"]->getString());

   return rval;
}

bool FileAssembler::populateMediaAndCollectionDetails(
   FileInfo& fi, Media& media, Media& collection)
{
   bool rval = false;
   MediaId mediaId = BM_MEDIA_ID(fi["mediaId"]);

   // check to see if the included media is a collection or a single item
   if(strcmp(mDownloadState["contract"]["media"]["type"]->getString(),
      "collection") == 0)
   {
      collection = mDownloadState["contract"]["media"];
      // FIXME: extract the media from the collection using an iterator of
      //        some sort.

      // iterate over groups in contents
      DynamicObjectIterator gi = collection["contents"].getIterator();
      while(!rval && gi->hasNext())
      {
         // iterate over medias in group
         DynamicObject& group = gi->next();
         MediaIterator mi = group.getIterator();
         while(!rval && mi->hasNext())
         {
            Media& m = mi->next();
            if(BM_MEDIA_ID_EQUALS(mediaId, BM_MEDIA_ID(m["id"])))
            {
               media = m;
               rval = true;
            }
         }
      }
   }
   else if(BM_MEDIA_ID_EQUALS(
      mediaId, BM_MEDIA_ID(mDownloadState["contract"]["media"]["id"])))
   {
      media = mDownloadState["contract"]["media"];
      collection = media;
      rval = true;
   }

   return rval;
}

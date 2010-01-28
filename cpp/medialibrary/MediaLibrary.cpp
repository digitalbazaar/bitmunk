/*
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk/medialibrary/MediaLibrary.h"

#include "bitmunk/data/FormatDetectorInputStream.h"
#include "bitmunk/medialibrary/MediaLibraryModule.h"
#include "monarch/data/json/JsonWriter.h"
#include "monarch/event/ObserverDelegate.h"
#include "monarch/io/FileInputStream.h"
#include "monarch/rt/RunnableDelegate.h"
#include "bitmunk/bfp/IBfpModule.h"
#include "bitmunk/common/Logging.h"

using namespace std;
using namespace monarch::config;
using namespace monarch::data;
using namespace monarch::data::json;
using namespace monarch::event;
using namespace monarch::io;
using namespace monarch::logging;
using namespace monarch::modest;
using namespace monarch::rt;
using namespace monarch::sql;
using namespace bitmunk::bfp;
using namespace bitmunk::common;
using namespace bitmunk::data;
using namespace bitmunk::node;
using namespace bitmunk::medialibrary;

// FIXME: We should have another mechanism for retrieving the currently
// valid BFP_ID value from the SVA
#define BFP_ID 1

#define MEDIALIBRARY   "bitmunk.medialibrary"
#define MLDB_EXCEPTION MEDIALIBRARY ".MediaLibraryDatabase"

// events
#define ML_EVENT_FILE_SCANNED     MEDIALIBRARY ".File.scanned"
#define ML_EVENT_FILE_EXCEPTION   MEDIALIBRARY ".File.exception"

MediaLibrary::MediaLibrary(Node* node) :
   mNode(node),
   mMediaLibraryDatabase(NULL),
   mFileUpdatedObserver(NULL)
{
}

MediaLibrary::~MediaLibrary()
{
}

bool MediaLibrary::initialize()
{
   bool rval = false;

   // create media library database
   Config cfg = mNode->getConfigManager()->getModuleConfig(
      "bitmunk.medialibrary.MediaLibrary");
   if(!cfg.isNull())
   {
      mMediaLibraryDatabase = new MediaLibraryDatabase();
      rval = mMediaLibraryDatabase->initialize(mNode);
      if(!rval)
      {
         // failed to init media library database
         delete mMediaLibraryDatabase;
         mMediaLibraryDatabase = NULL;
      }
      else
      {
         // start handling file update events
         mFileUpdatedObserver = new ObserverDelegate<MediaLibrary>(
            this, &MediaLibrary::fileUpdated);
         mNode->getEventController()->registerObserver(
            &(*mFileUpdatedObserver), MEDIALIBRARY ".File.updated");
         MO_CAT_DEBUG(BM_MEDIALIBRARY_CAT,
            "MediaLibrary registered for " MEDIALIBRARY ".File.updated");
      }
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Could not initialize media library.",
         MEDIALIBRARY ".InitializeFailed");
      Exception::push(e);
   }

   return rval;
}

void MediaLibrary::cleanup()
{
   // clean up the database
   if(mMediaLibraryDatabase != NULL)
   {
      delete mMediaLibraryDatabase;
      mMediaLibraryDatabase = NULL;

      // unregister observer
      mNode->getEventController()->unregisterObserver(
         &(*mFileUpdatedObserver), MEDIALIBRARY ".File.updated");
      mFileUpdatedObserver.setNull();
      MO_CAT_DEBUG(BM_MEDIALIBRARY_CAT,
         "MediaLibrary unregistered for " MEDIALIBRARY ".File.updated");
   }
}

void MediaLibrary::addMediaLibraryExtension(
   IMediaLibraryExtension* extension)
{
   mMediaLibraryDatabase->addMediaLibraryExtension(extension);
}

void MediaLibrary::removeMediaLibraryExtension(
   IMediaLibraryExtension* extension)
{
   mMediaLibraryDatabase->removeMediaLibraryExtension(extension);
}

Connection* MediaLibrary::getConnection(UserId userId)
{
   return mMediaLibraryDatabase->getConnection(userId);
}

DatabaseClientRef MediaLibrary::getDatabaseClient(UserId userId)
{
   return mMediaLibraryDatabase->getDatabaseClient(userId);
}

// a helper function to run basic file checks on a path to ensure the
// path exists, is not a directory, and is readable
static bool _checkFile(File& file)
{
   bool rval = false;

   // the file must exist
   if(!file->exists())
   {
      ExceptionRef e = new Exception(
         "Could not update file in the media library. "
         "The file at the given path does not exist.",
         MEDIALIBRARY ".FileDoesNotExist");
      e->getDetails()["path"] = file->getAbsolutePath();
      Exception::set(e);
   }
   // the file must not be a directory
   else if(file->isDirectory())
   {
      ExceptionRef e = new Exception(
         "Could not update file in the media library. "
         "The file at the given path is a directory, not a file.",
         MEDIALIBRARY ".FileIsDirectory");
      e->getDetails()["path"] = file->getAbsolutePath();
      Exception::set(e);
   }
   // the file must be readable
   else if(!file->isReadable())
   {
      ExceptionRef e = new Exception(
         "Could not update file in the media library. "
         "The file at the given path is not readable.",
         MEDIALIBRARY ".FileAccessDenied");
      e->getDetails()["path"] = file->getAbsolutePath();
      Exception::set(e);
   }
   // the file extension must exist (extension includes "." character)
   else if(strlen(file->getExtension()) <= 1)
   {
      ExceptionRef e = new Exception(
         "Could not update file in the media library. "
         "The file does not have a valid file extension.",
         MEDIALIBRARY ".InvalidFileExtension");
      e->getDetails()["path"] = file->getAbsolutePath();
      Exception::set(e);
   }
   // basic file checks pass
   else
   {
      rval = true;
   }

   return rval;
}

void MediaLibrary::scanFile(
   UserId userId, FileInfo& fi, bool update, DynamicObject* userData)
{
   // run scan file operation asynchronously
   DynamicObject d;
   d["userId"] = userId;
   d["fileInfo"] = fi.clone();
   d["update"] = update;
   if(userData != NULL)
   {
      d["userData"] = *userData;
   }

   RunnableRef r = new RunnableDelegate<MediaLibrary>(
      this, &MediaLibrary::scanFile, d);
   Operation op = r;
   if(!mNode->runUserOperation(userId, op))
   {
      // failed to run user op, send event including last exception
      Event e;
      e["type"] = ML_EVENT_FILE_EXCEPTION;
      e["details"]["userId"] = userId;
      e["details"]["fileInfo"] = fi;
      e["details"]["exception"] = Exception::getAsDynamicObject();
      if(userData != NULL)
      {
         e["details"]["userData"] = *userData;
      }
      mNode->getEventController()->schedule(e);
   }
}

// a helper function to scan a file to get its format details and if its
// media ID is not set, its media ID
static bool _getFormatDetails(FileInfo& fi)
{
   bool rval = false;

   // get format details and try to find embedded media in file
   File file(fi["path"]->getString());
   FileInputStream* fis = new FileInputStream(file);
   InspectorInputStream* iis = new InspectorInputStream(fis, true);
   FormatDetectorInputStream* fdis =
      new FormatDetectorInputStream(iis, true);

   // fully inspect mp3 files
   fdis->getDataFormatInspector("bitmunk.data.MpegAudioDetector")->
      setKeepInspecting(true);

   // detect format details
   rval = fdis->detect();
   if(rval)
   {
      if(!fdis->isFormatRecognized())
      {
         ExceptionRef e = new Exception(
            "File format not recognized.",
            MEDIALIBRARY ".UnknownFileFormat");
         Exception::set(e);
         rval = false;
      }
      else
      {
         // FIXME: need a better system for determining which format
         // details to include in the file info ... or something.

         // get format details
         DynamicObject details = fdis->getFormatDetails();
         fi["formatDetails"] = details.first();
         fi["contentType"] = fi["formatDetails"]["contentType"]->getString();

         // search format details for media ID if one hasn't been set
         if(fi["mediaId"]->getUInt64() == 0)
         {
            bool mediaIdFound = false;
            DynamicObjectIterator i = details.getIterator();
            while(!mediaIdFound && i->hasNext())
            {
               DynamicObject& d = i->next();
               if(d->hasMember("media") && d["media"]["id"]->getUInt64() != 0)
               {
                  // media ID found
                  fi["mediaId"] = d["media"]["id"]->getUInt64();
                  mediaIdFound = true;
               }
            }
         }
      }
   }
   // clean up format detector stream
   fdis->close();
   delete fdis;

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Could not detect file format details.",
         MEDIALIBRARY ".FormatDetectionFailed");
      Exception::push(e);

      MO_CAT_DEBUG(BM_MEDIALIBRARY_CAT,
         "Failed to detect file format details for file ID %s: %s",
         fi["id"]->getString(),
         JsonWriter::writeToString(Exception::getAsDynamicObject()).c_str());
   }

   return rval;
}

bool MediaLibrary::updateFile(
   UserId userId, FileInfo& fi, DynamicObject* userData)
{
   bool rval = false;

   // keep track of whether file was new or not
   bool isNew = false;

   // check file information
   File file(fi["path"]->getString());
   rval = _checkFile(file);

   // basic file checks pass
   if(rval)
   {
      if(!fi->hasMember("id") || fi["id"]->length() == 0)
      {
         ExceptionRef e = new Exception(
            "Invalid file ID or no file ID specified."
            MEDIALIBRARY ".InvalidFileId");
         Exception::set(e);
         rval = false;
      }
      else
      {
         // update file extension and path (makes it absolute if it wasn't)
         fi["extension"] = file->getExtension() + 1;
         fi["path"] = file->getAbsolutePath();
         fi["size"] = (uint64_t)file->getLength();
      }
   }

   // file can be updated in media library
   MediaLibraryId mlId;
   if(rval)
   {
      rval = false;
      Connection* c = getConnection(userId);
      if(c != NULL)
      {
         if(c->begin())
         {
            // update file in a transaction
            rval = mMediaLibraryDatabase->updateFile(
               userId, fi, &mlId, &isNew, c);
            rval = rval ? c->commit() : c->rollback() && false;
         }

         c->close();
      }
   }

   // file NOT added to library
   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Could not update the file in the media library.",
         MEDIALIBRARY ".FileUpdateFailed");
      e->getDetails()["path"] = fi["path"]->getString();
      e->getDetails()["fileInfo"] = fi.clone();
      e->getDetails()["userId"] = userId;
      Exception::push(e);

      // schedule a media library file exception event
      Event ev;
      ev["type"] = ML_EVENT_FILE_EXCEPTION;
      ev["details"]["mediaLibraryId"] = mlId;
      ev["details"]["fileInfo"] = fi.clone();
      ev["details"]["userId"] = userId;
      ev["details"]["exception"] = Exception::getAsDynamicObject();
      if(userData != NULL)
      {
         ev["details"]["userData"] = *userData;
      }
      mNode->getEventController()->schedule(ev);
   }
   // file WAS added to library
   else
   {
      // schedule a media library file updated event
      Event e;
      e["type"] = MEDIALIBRARY ".File.updated";
      e["details"]["mediaLibraryId"] = mlId;
      e["details"]["fileInfo"] = fi.clone();
      e["details"]["userId"] = userId;
      e["details"]["isNew"] = isNew;
      if(userData != NULL)
      {
         e["details"]["userData"] = *userData;
      }
      mNode->getEventController()->schedule(e);
   }

   return rval;
}

bool MediaLibrary::removeFile(UserId userId, FileInfo& fi, bool erase)
{
   bool rval = false;

   // attempt to remove the file from the media library database, the
   // path will be updated in the file info if it was inaccurate
   rval = mMediaLibraryDatabase->removeFile(userId, fi);
   if(rval && erase)
   {
      File file(fi["path"]->getString());
      rval = file->remove();
   }

   return rval;
}

bool MediaLibrary::populateFile(
   UserId userId, FileInfo& fi, MediaLibraryId mlId, Connection* conn)
{
   return mMediaLibraryDatabase->populateFile(userId, fi, mlId, conn);
}

bool MediaLibrary::populateFileSet(
   UserId userId, DynamicObject& query, ResourceSet& fileSet, Connection* conn)
{
   return mMediaLibraryDatabase->populateFileSet(userId, query, fileSet, conn);
}

bool MediaLibrary::updateMedia(UserId userId, Media media)
{
   bool rval = false;

   Messenger* m = mNode->getMessenger();
   monarch::net::Url url;
   url.format("/api/3.0/media/%" PRIu64, media["id"]->getUInt64());
   if(m->getFromBitmunk(&url, media))
   {
      MO_CAT_DEBUG(BM_MEDIALIBRARY_CAT,
         "Retrieved media %" PRIu64 " from Bitmunk.",
         media["id"]->getUInt64());

      // update media in local media library database
      rval = mMediaLibraryDatabase->updateMedia(userId, media);
   }

   if(!rval)
   {
      DynamicObject dyno = Exception::getAsDynamicObject();
      MO_CAT_DEBUG(BM_MEDIALIBRARY_CAT,
         "Failed to update media %" PRIu64 " from Bitmunk: %s",
         media["id"]->getUInt64(),
         JsonWriter::writeToString(dyno).c_str());
   }
   else
   {
      // schedule a media library file addition event
      Event e;
      e["type"] = MEDIALIBRARY ".Media.updated";
      e["details"]["mediaId"] = media["id"];
      e["details"]["userId"] = userId;
      mNode->getEventController()->schedule(e);
   }

   return rval;
}

bool MediaLibrary::populateMedia(
   UserId userId, Media media,
   MediaLibraryId mlId, monarch::sql::Connection* conn)
{
   return mMediaLibraryDatabase->populateMedia(userId, media, mlId, conn);
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

/**
 * Acquires a BFP instance.
 *
 * @param bfpId the ID of the bfp to retrieve.
 *
 * @return the acquired BFP instance or NULL if an exception occurred.
 */
static Bfp* _acquireBfp(Node* node, BfpId bfpId)
{
   Bfp* rval = NULL;

   // get the BFP module interface
   IBfpModule* ibm = getIBfpInstance(node);
   if(ibm != NULL)
   {
      // create bfp
      rval = ibm->createBfp(bfpId);
   }

   if(rval == NULL)
   {
      // failed to acquire bfp
      ExceptionRef e = new Exception(
         "Failed to acquire Bitmunk File Processor (BFP).",
         MEDIALIBRARY ".BfpRetrievalFailure");
      Exception::push(e);
   }

   return rval;
}

/**
 * Frees the passed BFP instance.
 *
 * @param bfp the BFP instance to free.
 */
static void _releaseBfp(Node* node, Bfp* bfp)
{
   // get the BFP module interface
   IBfpModule* ibm = getIBfpInstance(node);
   ibm->freeBfp(bfp);
}

// a helper function to find the media ID associated with a file ID
static bool _findMediaId(Node* node, UserId userId, FileInfo& fi)
{
   bool rval = false;

   MO_CAT_DEBUG(BM_MEDIALIBRARY_CAT,
      "Detected unknown media associated with file, "
      "checking Bitmunk for media to associate...");

   // first hit bitmunk service to determine media ID
   Messenger* m = node->getMessenger();
   monarch::net::Url url;
   DynamicObject mediaIds;
   url.format("/api/3.0/catalog/sellerpools?fileId=%s&mediaIds=true",
      fi["id"]->getString());
   if(m->getSecureFromBitmunk(&url, mediaIds, userId))
   {
      MO_CAT_DEBUG(BM_MEDIALIBRARY_CAT,
         "Retrieved media IDs associated with file ID '%s' from Bitmunk:\n%s",
         fi["id"]->getString(),
         JsonWriter::writeToString(mediaIds, true).c_str());

      if(mediaIds->length() == 0)
      {
         ExceptionRef e = new Exception(
            "Found no media IDs associated with the given file ID on Bitmunk.",
            MLDB_EXCEPTION ".NoAssociatedMediaIds");
         Exception::set(e);
      }
      else
      {
         // just use first media ID -- there can be another interface where the
         // user can select from all of the possible media IDs if they don't like
         // the result
         fi["mediaId"] = mediaIds.first()->getUInt64();
         rval = true;
      }
   }

   if(!rval)
   {
      MO_CAT_DEBUG(BM_MEDIALIBRARY_CAT,
         "Failed to find media ID associated with file ID '%s': %s",
         fi["id"]->getString(),
         JsonWriter::writeToString(Exception::getAsDynamicObject()).c_str());
   }

   return rval;
}

void MediaLibrary::scanFile(DynamicObject& d)
{
   bool pass = false;

   UserId userId = d["userId"]->getUInt64();
   FileInfo& fi = d["fileInfo"];
   bool update = d["update"]->getBoolean();

   // run basic file checks
   File file(fi["path"]->getString());
   if(_checkFile(file))
   {
      // update file extension and path (makes it absolute if it wasn't)
      fi["extension"] = file->getExtension() + 1;
      fi["path"] = file->getAbsolutePath();
      fi["size"] = (uint64_t)file->getLength();

      // generate the file ID hash using the BFP
      // FIXME: get the latest bfp ID available somehow
      Bfp* bfp = _acquireBfp(mNode, BFP_ID);
      if(bfp != NULL)
      {
         pass = bfp->setFileInfoId(fi);
         _releaseBfp(mNode, bfp);
      }

      // if no media ID has been set on the file, try to find it via
      // bitmunk first, we trust it to be more accurate than embedded
      // media data which will be added when we get the format details
      // if we still haven't found a media ID by then
      if(pass && (!fi->hasMember("mediaId") || fi["mediaId"]->getUInt64() == 0))
      {
         // try to find media information
         _findMediaId(mNode, userId, fi);
      }

      // get format details
      pass = pass && _getFormatDetails(fi);
   }

   if(pass)
   {
      // send event with no exception
      Event e;
      e["type"] = ML_EVENT_FILE_SCANNED;
      e["details"]["userId"] = userId;
      e["details"]["fileInfo"] = fi;
      if(d->hasMember("userData"))
      {
         e["details"]["userData"] = d["userData"];
      }
      mNode->getEventController()->schedule(e);

      // update file in medialibrary if requested
      if(update)
      {
         DynamicObject userData(NULL);
         if(d->hasMember("userData"))
         {
            userData = d["userData"];
         }
         updateFile(userId, fi, userData.isNull() ? NULL : &userData);
      }
   }
   else
   {
      // send event including last exception
      Event e;
      e["type"] = ML_EVENT_FILE_EXCEPTION;
      e["details"]["userId"] = userId;
      e["details"]["fileInfo"] = fi;
      e["details"]["exception"] = Exception::getAsDynamicObject();
      if(d->hasMember("userData"))
      {
         e["details"]["userData"] = d["userData"];
      }
      mNode->getEventController()->schedule(e);
   }
}

void MediaLibrary::fileUpdated(Event& e)
{
   // if the media ID is zero, try to find the associated media ID via bitmunk
   UserId userId = e["details"]["userId"]->getUInt64();
   FileInfo fi = e["details"]["fileInfo"].clone();
   MediaId mediaId = fi["mediaId"]->getUInt64();
   if(mediaId == 0)
   {
      if(_findMediaId(mNode, userId, fi))
      {
         mediaId = fi["mediaId"]->getUInt64();
      }
   }

   // try to populate the media associated with the file (if one has been
   // associated, i.e. media ID is NOT zero)
   if(mediaId != 0)
   {
      Media media;
      media["id"] = mediaId;
      if(!mMediaLibraryDatabase->populateMedia(userId, media))
      {
         // if media not found, try to acquire it from bitmunk
         ExceptionRef e = Exception::get();
         ExceptionRef& cause = e->getCause();
         if(!cause.isNull() && cause->isType(MLDB_EXCEPTION ".NotFound"))
         {
            Exception::clear();
            MO_CAT_DEBUG(BM_MEDIALIBRARY_CAT,
               "Detected no local media data cached, "
               "checking Bitmunk for media data...");
            updateMedia(userId, media);
         }
      }
   }
}

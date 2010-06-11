/*
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/medialibrary/MediaLibraryService.h"

#include "bitmunk/node/BtpActionDelegate.h"
#include "bitmunk/node/RestResourceHandler.h"
#include "monarch/validation/Validation.h"

using namespace std;
using namespace monarch::rt;
using namespace bitmunk::common;
using namespace bitmunk::medialibrary;
using namespace bitmunk::protocol;
using namespace bitmunk::node;
namespace v = monarch::validation;

typedef BtpActionDelegate<MediaLibraryService> Handler;

#define MEDIALIBRARY   "bitmunk.medialibrary"
#define MLDB_EXCEPTION MEDIALIBRARY ".MediaLibraryDatabase"
#define MEDIALIBRARY_SERVICE MEDIALIBRARY ".MediaService"

MediaLibraryService::MediaLibraryService(
   Node* node, const char* path, MediaLibrary* ml) :
   NodeService(node, path),
   mLibrary(ml)
{
}

MediaLibraryService::~MediaLibraryService()
{
}

bool MediaLibraryService::initialize()
{
   bool rval = true;

   // files
   {
      RestResourceHandlerRef files = new RestResourceHandler();
      addResource("/files", files);

      // resource method handlers

      // Adds a new file to the media library
      // POST .../files
      {
         // a nodeuser must be specified when using a media library service,
         // and that user must be the same one that is using the service
         Handler* handler = new Handler(
            mNode, this, &MediaLibraryService::updateFile,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         v::ValidatorRef qValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         v::ValidatorRef validator = new v::All(
            new v::Map(
               "path", new v::Type(String),
               "mediaId", new v::Int(v::Int::NonNegative),
               NULL),
            new v::Any(
               // if the ID is provided, then the content type and
               // format details must also be provided
               new v::Map(
                  "id", new v::All(
                     new v::Type(String),
                     new v::Min(
                        40, "File IDs must be 40 hexadecimal characters."),
                     new v::Max(
                        40, "File IDs must be 40 hexadecimal characters."),
                     NULL),
                  "contentType", new v::Type(String),
                  "formatDetails", new v::Type(Map),
                  NULL),
               // if any of these fields is provided without the others, then
               // it will not pass the above validator or this one below
               new v::Map(
                  "id", new v::Optional(new v::NotValid(
                     "File ID, content type, and format details must all "
                     "be provided together or they must all be absent.")),
                  "contentType", new v::Optional(new v::NotValid(
                     "File ID, content type, and format details must all "
                     "be provided together or they must all be absent.")),
                  "formatDetails", new v::Optional(new v::NotValid(
                     "File ID, content type, and format details must all "
                     "be provided together or they must all be absent.")),
                  NULL),
               NULL),
            NULL);

         files->addHandler(h, BtpMessage::Post, 0, &qValidator, &validator);
      }

      // Get a file from the media library
      // GET .../files/<fileId>
      {
         // a nodeuser must be specified when using a media library service,
         // and that user must be the same one that is using the service
         Handler* handler = new Handler(
            mNode, this, &MediaLibraryService::getFile,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         v::ValidatorRef qValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         files->addHandler(h, BtpMessage::Get, 1, &qValidator);
      }

      // Get a list of files from the media library
      // GET .../files
      // GET .../files?[options]
      //        num=<num>
      //        start=<start>
      //        type=<type>
      //        extension=<ext>
      //        order=<field>
      //        dir=<asc|desc>
      //        media=<true|false>
      // GET .../files?id=<fileId>[&id=<fileId>...]&media=<true|false>
      {
         // a nodeuser must be specified when using a media library service,
         // and that user must be the same one that is using the service
         Handler* handler = new Handler(
            mNode, this, &MediaLibraryService::getFileSet,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         // FIXME: this is confusing as hell we shouldn't have to change
         // the validator (at least not for every single field) just
         // because we allow multiple entries for some of these fields
         v::ValidatorRef qValidator = new v::Map(
            "nodeuser", new v::Each(new v::Int(v::Int::Positive)),
            "start", new v::Optional(
               new v::Each(new v::Int(v::Int::NonNegative))),
            "num", new v::Optional(
               new v::Each(new v::Int(v::Int::NonNegative))),
            "type", new v::Optional(new v::Each(new v::Type(String))),
            "extension", new v::Optional(new v::Each(new v::Type(String))),
            "order", new v::Optional(new v::Each(
               new v::Regex("^(path|size|title|type)$"))),
            "dir", new v::Optional(new v::Each(new v::Regex("^(asc|desc)$"))),
            "media", new v::Optional(
               new v::Each(new v::Regex("^(true|false)$"))),
            "id", new v::Optional(new v::Each(new v::Type(String))),
            NULL);

         files->addHandler(
            h, BtpMessage::Get, 0, &qValidator, NULL,
            RestResourceHandler::ArrayQuery);
      }

      // Remove a file or files from the media library
      // DELETE .../files/<fileId>
      {
         // a nodeuser must be specified when using a media library service,
         // and that user must be the same one that is using the service
         Handler* handler = new Handler(
            mNode, this, &MediaLibraryService::removeFiles,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         v::ValidatorRef qValidator = new v::Map(
            "erase", new v::Optional(new v::Type(Boolean)),
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         files->addHandler(h, BtpMessage::Delete, 1, &qValidator);
      }
   }

   // media
   {
      RestResourceHandlerRef media = new RestResourceHandler();
      addResource("/media", media);

      // Updates a media in the media library using data from Bitmunk
      // POST .../media/<mediaId>
      {
         // a nodeuser must be specified when using a media library service,
         // and that user must be the same one that is using the service
         Handler* handler = new Handler(
            mNode, this, &MediaLibraryService::updateMedia,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         v::ValidatorRef qValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         media->addHandler(h, BtpMessage::Post, 0, &qValidator);
      }
   }

   return rval;
}

void MediaLibraryService::cleanup()
{
   // remove resources
   removeResource("/files");
   removeResource("/media");
}

bool MediaLibraryService::updateFile(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = false;

   UserId userId = action->getInMessage()->getUserId();

   // if no file ID is given, return a 202 Accepted and start an
   // asynchronous process to produce the file ID
   if(!in->hasMember("id") || in["id"]->length() == 0)
   {
      action->getResponse()->getHeader()->setStatus(202, "Accepted");
      out.setNull();
      rval = true;
      // send result before launching asynchronous process
      action->sendResult();
      mLibrary->scanFile(userId, in, true);
   }
   else
   {
      // update the file
      if((rval = mLibrary->updateFile(userId, in)))
      {
         // return file info
         out["fileInfo"] = in;
      }
   }

   if(!rval)
   {
      // failed to update the file in the media library database
      ExceptionRef e = new Exception(
         "The file info was not updated in the media library.",
         MEDIALIBRARY_SERVICE ".FileUpdateFailure");
      e->getDetails()["userId"] = userId;
      e->getDetails()["fileId"] = in["id"]->getString();
      Exception::push(e);
   }

   return rval;
}

bool MediaLibraryService::removeFiles(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = false;

   // get the user id of the caller
   UserId userId = action->getInMessage()->getUserId();

   // get the resource params
   DynamicObject params;
   action->getResourceParams(params);

   // get query params
   DynamicObject vars;
   action->getResourceQuery(vars);

   // get the file ID given in the URL
   FileInfo fi;
   fi["id"] = params[0]->getString();

   bool erase = false;
   if(vars->hasMember("erase"))
   {
      erase = vars["erase"]->getBoolean();
   }

   rval = mLibrary->removeFile(userId, fi, erase);

   return rval;
}

bool MediaLibraryService::getFile(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = false;

   UserId userId = action->getInMessage()->getUserId();

   // get the resource params
   DynamicObject params;
   action->getResourceParams(params);
   // get the file ID given in the URL
   FileId fileId = BM_FILE_ID(params[0]);

   FileInfo fi;
   BM_ID_SET(fi["id"], fileId);

   // populate the file object using the medialibrary
   if((rval = mLibrary->populateFile(userId, fi)))
   {
      // get query filters
      DynamicObject filters;
      action->getResourceQuery(filters);
      if(filters->hasMember("media") && filters["media"]->getBoolean())
      {
         // populate media
         Media media;
         BM_ID_SET(media["id"], BM_MEDIA_ID(fi["mediaId"]));
         rval = mLibrary->populateMedia(userId, media);
         if(!rval)
         {
            // if media not found, include a blank media
            ExceptionRef e = Exception::get();
            if(e->hasType(MLDB_EXCEPTION ".NotFound"))
            {
               Exception::clear();
               media["type"] = "";
               media["title"] = "";
               media["contributors"]->setType(Map);
               rval = true;
            }
         }

         if(rval)
         {
            // return the file info and the media
            out["fileInfo"] = fi;
            out["media"] = media;
         }
      }
      else
      {
         // return just the file
         out = fi;
      }
   }

   if(!rval)
   {
      // failed to retrieve file from the media library database
      ExceptionRef e = new Exception(
         "Failed to retrieve file from the media library database.",
         MEDIALIBRARY_SERVICE ".FileRetrievalFailure");
      BM_ID_SET(e->getDetails()["userId"], userId);
      BM_ID_SET(e->getDetails()["fileId"], fileId);
      Exception::push(e);
   }

   return rval;
}

bool MediaLibraryService::getFileSet(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = false;

   // get the user ID
   UserId userId = action->getInMessage()->getUserId();

   // get file set filters
   DynamicObject filters;
   action->getResourceQuery(filters, true);

   // get the query parameters from the set of filters that were passed in
   DynamicObject query;
   if(filters->hasMember("id"))
   {
      // ids are in a resource query array
      query["ids"] = filters["id"];
   }
   else
   {
      // defaults
      query["start"] = 0;
      query["num"] = 10;
      query["dir"] = "asc";
      // build query from filter params
      const char* params[] =
         {"start", "num", "extension", "order", "dir", NULL};
      for(int i = 0; params[i] != NULL; ++i)
      {
         const char* p = params[i];
         if(filters->hasMember(p))
         {
            query[p] = filters[p][filters[p]->length() - 1];
         }
      }
   }
   if(filters->hasMember("media"))
   {
      query["media"] =
         filters["media"][filters["media"]->length() - 1]->getBoolean();
   }

   // get the file set that corresponds with the given query filters
   ResourceSet fileSet;
   if((rval = mLibrary->populateFileSet(userId, query, fileSet)))
   {
      // return the set of files retrieved via the query
      out = fileSet;
   }

   return rval;
}

bool MediaLibraryService::updateMedia(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = false;

   UserId userId = action->getInMessage()->getUserId();

   // get the resource params
   DynamicObject params;
   action->getResourceParams(params);
   // get the media ID given in the URL
   MediaId mediaId = BM_MEDIA_ID(params[0]);

   // do the media update
   Media media;
   BM_ID_SET(media["id"], mediaId);
   if((rval = mLibrary->updateMedia(userId, media)))
   {
      out = media;
   }

   if(!rval)
   {
      // failed to update media in the media library database
      ExceptionRef e = new Exception(
         "Failed to update media in the media library database.",
         MEDIALIBRARY_SERVICE ".MediaUpdateFailure");
      BM_ID_SET(e->getDetails()["userId"], userId);
      BM_ID_SET(e->getDetails()["mediaId"], mediaId);
      Exception::push(e);
   }

   return rval;
}

bool MediaLibraryService::populateMedia(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = false;

   UserId userId = action->getInMessage()->getUserId();

   // get the resource params
   DynamicObject params;
   action->getResourceParams(params);
   // get the media ID given in the URL
   MediaId mediaId = BM_MEDIA_ID(params[0]);

   // get the media
   Media media;
   BM_ID_SET(media["id"], mediaId);
   if((rval = mLibrary->populateMedia(userId, media)))
   {
      out = media;
   }

   if(!rval)
   {
      // failed to retrieve media from the media library database
      ExceptionRef e = new Exception(
         "Failed to popoulate media in the media library database.",
         MEDIALIBRARY_SERVICE ".MediaPopulationFailure");
      BM_ID_SET(e->getDetails()["userId"], userId);
      BM_ID_SET(e->getDetails()["mediaId"], mediaId);
      Exception::push(e);
   }

   return rval;
}

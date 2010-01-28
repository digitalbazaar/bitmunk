/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/filebrowser/FileBrowserService.h"

#include "monarch/io/File.h"
#include "monarch/io/FileList.h"
#include "bitmunk/node/BtpActionDelegate.h"
#include "bitmunk/node/RestResourceHandler.h"

using namespace std;
using namespace monarch::rt;
using namespace monarch::io;
using namespace bitmunk::common;
using namespace bitmunk::filebrowser;
using namespace bitmunk::protocol;
using namespace bitmunk::node;
namespace v = monarch::validation;

typedef BtpActionDelegate<FileBrowserService> Handler;

FileBrowserService::FileBrowserService(
   Node* node, const char* path) :
   NodeService(node, path)
{
}

FileBrowserService::~FileBrowserService()
{
}

bool FileBrowserService::initialize()
{
   // filebrowser
   {
      RestResourceHandlerRef mappings = new RestResourceHandler();
      addResource("/files", mappings);

      /**
       * Gets the contents of a directory given a file path.
       *
       * @tags payswarm-api public-api
       * @qparam path the files that are contained in this path are returned.
       * @qparam nodeuser the user on the node that should perform the
       *                  requested action on the caller's behalf.
       *
       * @return an HTTP 200 code if successful, an exception if not.
       */
      {
         Handler* handler = new Handler(
            mNode, this, &FileBrowserService::getDirectoryContents,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         v::ValidatorRef qValidator = new v::Map(
            "path", new v::All(
               new v::Type(String),
               new v::Min(1,
                  "The path must be at least 1 character in length."),
               new v::Max(32767, "The path must be 32,767 characters or less."),
               NULL),
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         mappings->addHandler(h, BtpMessage::Get, 0, &qValidator, NULL);
      }
   }

   return true;
}

void FileBrowserService::cleanup()
{
   // remove resources
   removeResource("/files");
}

bool FileBrowserService::getDirectoryContents(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = false;

   // get path from the query parameters
   DynamicObject vars;
   action->getResourceQuery(vars);
   const char* path = vars["path"]->getString();

   // resolve relative path if necessary
   File dir((FileImpl*)NULL);
   if(!File::isPathAbsolute(path))
   {
      // expand relative user data path
      string str;
      if(mNode->getConfigManager()->expandUserDataPath(
         path, action->getInMessage()->getUserId(), str))
      {
         dir = File(str.c_str());
      }
   }
   if(dir.isNull())
   {
      dir = File(path);
   }

   // check to make sure the path exists
   if(!dir->exists())
   {
      ExceptionRef e = new Exception(
         "The specified path does not exist.",
         "bitmunk.filesystem.PathNotFound");
      e->getDetails()["path"] = path;
      Exception::set(e);
   }
   // check to make sure the path is readable
   else if(!dir->isReadable())
   {
      ExceptionRef e = new Exception(
         "Filesystem path permissions prevent reading the contents of "
         "the given path.",
         "bitmunk.filesystem.PathNotReadable");
      e->getDetails()["path"] = path;
      Exception::set(e);
   }
   // check to make sure the path is a directory
   else if(!dir->isDirectory())
   {
      ExceptionRef e = new Exception(
         "The specified path is not a directory.",
         "bitmunk.filesystem.PathNotDirectory");
      e->getDetails()["path"] = path;
      Exception::set(e);
   }
   // path exists, is readable, and is a directory
   else
   {
      rval = true;

      // build the list of files and directories
      FileList fileList;
      dir->listFiles(fileList);
      IteratorRef<File> fli = fileList->getIterator();

      // iterate through the entries and populate the output object
      out["path"] = dir->getAbsolutePath();
      out["contents"]->setType(Array);
      DynamicObject& contents = out["contents"];
      while(fli->hasNext())
      {
         File& f = fli->next();
         const char* basename = f->getBaseName();

         // check to make sure the path isn't . or ..
         if((strcmp(basename, ".") != 0) && (strcmp(basename, "..") != 0))
         {
            FilesystemEntry& entry = contents->append();
            entry["path"] = f->getAbsolutePath();
            entry["size"] = (uint64_t)f->getLength();
            entry["readable"] = f->isReadable();
            entry["type"] = f->isDirectory() ? "directory" : "file";
         }
      }
   }

   return rval;
}

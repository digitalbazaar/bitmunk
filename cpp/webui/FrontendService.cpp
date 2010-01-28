/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/webui/FrontendService.h"

#include "bitmunk/node/BtpActionDelegate.h"
#include "bitmunk/node/RestResourceHandler.h"
#include "bitmunk/webui/WebUiModule.h"
#include "monarch/data/json/JsonReader.h"
#include "monarch/io/File.h"
#include "monarch/io/FileList.h"
#include "monarch/io/FileInputStream.h"
#include "monarch/net/Url.h"
#include "monarch/http/HttpResponseHeader.h"
#include "monarch/logging/Logging.h"
#include "monarch/util/Date.h"
#include "monarch/validation/Validation.h"

using namespace std;
using namespace bitmunk::common;
using namespace bitmunk::protocol;
using namespace bitmunk::node;
using namespace bitmunk::webui;
using namespace monarch::data::json;
using namespace monarch::io;
using namespace monarch::http;
using namespace monarch::logging;
using namespace monarch::net;
using namespace monarch::rt;
using namespace monarch::util;
namespace v = monarch::validation;

#define MAIN_URL "/bitmunk"
#define MAIN_URL_SIZE 8
#define PLUGIN_URL MAIN_URL "/plugins"
#define PLUGIN_URL_SIZE (MAIN_URL_SIZE + 8)

typedef BtpActionDelegate<FrontendService> Handler;

FrontendService::FrontendService(
   SessionManager* sm,
   DynamicObject& pi, const char* mainPluginId,
   const char* path) :
   BtpService(path)
{
   mSessionManager = sm;
   mPluginInfo = pi;
   mMainPluginId = mainPluginId;
}

FrontendService::~FrontendService()
{
}

bool FrontendService::initialize()
{
   // common query validator with file type specific options
   v::ValidatorRef qValidator = new v::Map(
      "minjs", new v::Optional(new v::Regex("^(true|false)$")),
      NULL);

   // root
   {
      RestResourceHandlerRef root = new RestResourceHandler();
      addResource("/", root);

      // Get a resource
      // GET .../<r>[/<r2>[...]]
      {
         ResourceHandler h = new Handler(
            mSessionManager->getNode(),
            this, &FrontendService::serveMainContent,
            BtpAction::AuthOptional);
         root->addHandler(h, BtpMessage::Get, -1, &qValidator);
      }
   }

   // plugins
   {
      RestResourceHandlerRef plugins = new RestResourceHandler();
      addResource("/plugins", plugins);

      // Get a plugin resource
      // GET .../plugins/<r>[/<r2>[...]]
      {
         ResourceHandler h = new Handler(
            mSessionManager->getNode(),
            this, &FrontendService::servePluginContent,
            BtpAction::AuthOptional);
         plugins->addHandler(h, BtpMessage::Get, -1, &qValidator);
      }
   }

   // setup MIME content types
   // FIXME: should this be a static dyno?
   mContentTypes->setType(Map);
   mContentTypes->clear();
   mContentTypes[".css"] = "text/css";
   mContentTypes[".gif"] = "image/gif";
   mContentTypes[".html"] = "text/html";
   mContentTypes[".ico"] = "image/ico";
   mContentTypes[".jpg"] = "image/jpeg";
   mContentTypes[".jpeg"] = "image/jpeg";
   mContentTypes[".js"] = "application/javascript";
   mContentTypes[".json"] = "application/json";
   mContentTypes[".png"] = "image/png";
   mContentTypes[".svg"] = "image/svg+xml";
   mContentTypes[".swf"] = "application/x-shockwave-flash";
   mContentTypes[".txt"] = "text/plain";
   mContentTypes[".xml"] = "application/xml";

   return true;
}

void FrontendService::cleanup()
{
   // remove resources
   removeResource("/");
   removeResource("/plugins");
}

/**
 * Sets the MIME type for the given HttpResponseHeader to something sane
 * based on the file type.
 *
 * @parma contentTypes a map of file extensions to content types.
 * @param header the HTTP response header whose content type should be set
 *               based on the extension of the filename given.
 * @param filename the name of the file whose extension will be used to
 *                 determine the content type.
 */
static void setContentType(
   DynamicObject& contentTypes,
   HttpResponseHeader* header, const char* filename)
{
   string root;
   string ext;
   File::splitext(filename, root, ext);
   if(contentTypes->hasMember(ext.c_str()))
   {
      header->setField("Content-Type", contentTypes[ext.c_str()]->getString());
   }
   else
   {
      header->setField("Content-Type", "text/html");
   }
}

/**
 * Handles serving static content from disk and is used by the serve method
 * in this class.
 *
 * @param action the BtpAction.
 * @param file the file that should be read from disk and served to the
 *             requesting client.
 * @param contentTypes a map of file extension to content type.
 *
 * @return true if the file was served successfully, false if the file
 *         could not be served.
 */
static bool serveFile(
   BtpAction* action, File& file, DynamicObject& contentTypes)
{
   bool rval = true;

   // set the proper MIME type for the response
   setContentType(
      contentTypes,
      action->getResponse()->getHeader(), file->getAbsolutePath());

   // setup response header
   HttpResponseHeader* resHeader = action->getResponse()->getHeader();
   resHeader->setStatus(200, "OK");

   Date date = file->getModifiedDate();
   TimeZone gmt = TimeZone::getTimeZone("GMT");
   string str;
   date.format(str, HttpHeader::sDateFormat, &gmt);
   resHeader->setField("Last-Modified", str.c_str());

   // serve the file if it exists
   FileInputStream fis(file);
   rval = action->sendResult(&fis);
   fis.close();

   return rval;
}

/**
 * Handles concatenating and serving static content from disk and is used
 * by the serve method in this class.
 *
 * @param action the BtpAction.
 * @param fileList the list of files that should be read from disk,
 *                 concatenated, and served to the requesting client.
 * @param contentLength the content length for all of the files.
 * @param contentTypes a map of file extension to content type.
 *
 * @return true if the file list was served successfully, false if the file
 *         list could not be served.
 */
static bool serveFileList(
   BtpAction* action, FileList& fileList, int64_t contentLength,
   DynamicObject& contentTypes)
{
   bool rval = true;

   // iterate over files, concatenating them as they are sent out
   int length = 2048;
   char* buffer = (char*)malloc(length);
   OutputStreamRef os(NULL);
   HttpTrailerRef trailer(NULL);
   bool first = true;
   IteratorRef<File> i = fileList->getIterator();
   while(rval && i->hasNext())
   {
      File& file = i->next();

      if(first)
      {
         first = false;

         // set the proper MIME type for the response
         setContentType(
            contentTypes,
            action->getResponse()->getHeader(),
            file->getAbsolutePath());

         // setup response header
         HttpResponseHeader* resHeader = action->getResponse()->getHeader();
         resHeader->setStatus(200, "OK");

         // use compression and chunked encoding if supported
         HttpRequestHeader* reqHeader = action->getRequest()->getHeader();
         if(strcmp(reqHeader->getVersion(), "HTTP/1.1") == 0)
         {
            resHeader->setField("Transfer-Encoding", "chunked");
            action->setContentEncoding();
         }
         else
         {
            // use content length
            resHeader->setField("Content-Length", contentLength);
         }

         Date date = file->getModifiedDate();
         TimeZone gmt = TimeZone::getTimeZone("GMT");
         string str;
         date.format(str, HttpHeader::sDateFormat, &gmt);
         resHeader->setField("Last-Modified", str.c_str());

         // send header
         rval = action->getOutMessage()->sendHeader(
            action->getResponse()->getConnection(),
            resHeader, os, trailer);
      }

      if(rval)
      {
         // read in file, write out to body output stream
         FileInputStream fis(file);
         int numBytes;
         while(rval && (numBytes = fis.read(buffer, length)) > 0)
         {
            rval = os->write(buffer, numBytes);
         }
         fis.close();
      }
   }

   if(rval && !os.isNull())
   {
      rval = os->finish();
   }

   if(!os.isNull())
   {
      os->close();
   }

   // clean up buffer
   free(buffer);

   // result now sent
   action->setResultSent();

   return rval;
}

/**
 * Check if file is modified based on action headers.
 *
 * @param action the BtpAction.
 * @param file the file that should be checked for modification.
 *
 * @return true if the message was modified, false if not.
 */
static bool isModified(BtpAction* action, File& file)
{
   bool rval = true;

   HttpRequestHeader* reqHeader = action->getRequest()->getHeader();
   if(reqHeader->hasField("If-Modified-Since"))
   {
      string ims;
      reqHeader->getField("If-Modified-Since", ims);
      Date imsd(0);
      TimeZone tz = TimeZone::getTimeZone("GMT");
      if(imsd.parse(
         ims.c_str(), HttpHeader::sDateFormat, &tz))
      {
         time_t nows = Date().getSeconds();
         time_t imsds = imsd.getSeconds();
         time_t mods = file->getModifiedDate().getSeconds();
         rval = (mods > imsds) || (imsds > nows);
      }
   }

   return rval;
}

/**
 * Handles serving a resource not modified message.
 *
 * @param action the BtpAction.
 *
 * @return true if the message was served successfully, false if not.
 */
static bool serveNotModified(BtpAction* action)
{
   MO_CAT_DEBUG(BM_WEBUI_CAT,
      "Resource '%s' not modified.", action->getResource());

   HttpResponseHeader* resHeader = action->getResponse()->getHeader();
   resHeader->setStatus(304, "Not Modified");
   return action->sendResult();
}

/**
 * Handles serving a resource not found message.
 *
 * @param action the BtpAction.
 * @param mainPlugin the main plugin info.
 *
 * @return true if the error was served successfully, false if not.
 */
static bool serveNotFound(BtpAction* action, DynamicObject& mainPlugin)
{
   bool rval = true;

   MO_CAT_DEBUG(BM_WEBUI_CAT,
      "Resource '%s' not found.", action->getResource());

   // setup response header
   HttpResponseHeader* resHeader = NULL;
   resHeader = action->getResponse()->getHeader();
   resHeader->setStatus(404, "Not Found");

   // get main plugin
   if(mainPlugin->hasMember("content"))
   {
      // serve the 404 file
      resHeader->setField("Content-Type", "text/html");
      string ui404File(mainPlugin["content"]->getString());
      ui404File.append("/404.html");
      File file(ui404File.c_str());

      FileInputStream fis(file);
      rval = action->sendResult(&fis);
      fis.close();
   }
   else
   {
      action->sendResult();
   }

   return rval;
}

/**
 * Handles serving a resource forbidden message.
 *
 * @param action the BtpAction.
 * @param mainInf the main plugin info.
 *
 * @return true if the error was served successfully, false if not.
 */
static bool serveForbidden(BtpAction* action, DynamicObject& mainPlugin)
{
   bool rval = true;

   MO_CAT_DEBUG(BM_WEBUI_CAT,
      "Resource '%s' forbidden.", action->getResource());

   // setup response header
   HttpResponseHeader* resHeader = NULL;
   resHeader = action->getResponse()->getHeader();
   resHeader->setStatus(403, "Forbidden");

   // get main plugin
   if(mainPlugin->hasMember("content"))
   {
      // serve the 403 file
      resHeader->setField("Content-Type", "text/html");
      string ui403File(mainPlugin["content"]->getString());
      ui403File.append("/403.html");
      File file(ui403File.c_str());

      FileInputStream fis(file);
      rval = action->sendResult(&fis);
      fis.close();
   }
   else
   {
      action->sendResult();
   }

   return rval;
}

/**
 * Handles serving a method not allowed message.
 *
 * @param action the BtpAction.
 *
 * @return true if the error was served successfully, false if not.
 */
static bool serveMethodNotAllowed(BtpAction* action)
{
   bool rval = false;

   MO_CAT_DEBUG(BM_WEBUI_CAT,
      "Method '%s' with resource '%s' not allowed.",
      action->getRequest()->getHeader()->getMethod(),
      action->getResource());

   // 405
   action->getResponse()->getHeader()->setStatus(
      405, "Method Not Allowed");
   ExceptionRef e = new Exception(
      "Method not allowed.", "bitmunk.node.MethodNotAllowed", 405);
   e->getDetails()["invalidType"] =
      action->getRequest()->getHeader()->getMethod();

   // add valid types, build allow header
   const char* typeStr;
   e->getDetails()["validTypes"]->setType(Array);
   typeStr = BtpMessage::typeToString(BtpMessage::Get);
   e->getDetails()["validTypes"]->append() = typeStr;

   action->getResponse()->getHeader()->setField("Allow", typeStr);
   Exception::set(e);

   return rval;
}

bool FrontendService::serveMainContent(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = true;
   BtpMessage::Type type = action->getInMessage()->getType();

   // enable keep-alive if client supports it
   setKeepAlive(action);

   if(type == BtpMessage::Get)
   {
      // get the full path of the file that is requested
      // use Url to remove possible query and hash
      // strip off prefix
      HttpRequestHeader* reqHeader = action->getRequest()->getHeader();
      Url pathUrl(reqHeader->getPath());
      string relPath = pathUrl.getPath();
      relPath.erase(0, MAIN_URL_SIZE);

      // serve file
      rval = serve(action, in, out, mMainPluginId, relPath.c_str());
   }
   else
   {
      rval = serveMethodNotAllowed(action);
   }

   return rval;
}

bool FrontendService::servePluginContent(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = true;
   BtpMessage::Type type = action->getInMessage()->getType();

   // enable keep-alive if client supports it
   setKeepAlive(action);

   if(type == BtpMessage::Get)
   {
      DynamicObject params;
      action->getResourceParams(params);
      if(params->length() == 0)
      {
         // base plugins dir not servable
         out.setNull();
         rval = serveNotFound(action, mPluginInfo[mMainPluginId]);
      }
      else
      {
         // get the full path of the file that is requested
         // use Url to remove possible query and hash
         HttpRequestHeader* reqHeader = action->getRequest()->getHeader();
         Url pathUrl(reqHeader->getPath());
         string relPath = pathUrl.getPath();

         // get plugin id from path params
         const char* pluginId = params[0]->getString();

         // strip off prefix
         relPath.erase(0, PLUGIN_URL_SIZE + 1 + strlen(pluginId));

         // serve file
         rval = serve(action, in, out, pluginId, relPath.c_str());
      }
   }
   else
   {
      rval = serveMethodNotAllowed(action);
   }

   return rval;
}

bool FrontendService::serve(
   BtpAction* action, DynamicObject& in, DynamicObject& out,
   const char* plugin, const char* path)
{
   bool rval = true;

   if(!mPluginInfo->hasMember(plugin) ||
      !mPluginInfo[plugin]->hasMember("content"))
   {
      // plugin not found or no public content directory available
      rval = serveNotFound(action, mPluginInfo[mMainPluginId]);
   }
   else
   {
      // make sure that we don't generate a dynamic object as the output
      out.setNull();

      // get the specific plugin info
      DynamicObject& info = mPluginInfo[plugin];

      // get content directory
      File contentDirectory(info["content"]->getString());

      // see if the path is a virtual one, stored in the plugin info
      if(info->hasMember("virtual") && info["virtual"]->hasMember(path))
      {
         // get full virtual file path
         string fullVirtualPath = File::join(
            contentDirectory->getAbsolutePath(), path);

         // ensure each file in the list exists, is readable, and at least
         // one has been modified, keep track of total content length
         FileList fileList;
         bool exists = true;
         bool readable = true;
         bool modified = false;
         int64_t contentLength = 0;
         DynamicObjectIterator i = info["virtual"][path].getIterator();
         while(exists && readable && i->hasNext())
         {
            DynamicObject& filename = i->next();

            // FIXME: add ability to check for a minimized version of the
            // file as well (i guess based on file extension? added ".min"?)

            // concat plugin content dir and next file path
            File tmpFile(File::join(
               contentDirectory->getAbsolutePath(),
               filename->getString()).c_str());

            // ensure the file can be served
            if(!contentDirectory->contains(tmpFile))
            {
               MO_CAT_WARNING(BM_WEBUI_CAT,
                  "Attempt to fetch file outside of content directory: %s",
                  tmpFile->getAbsolutePath());
               readable = false;
            }
            else
            {
               string fullPath;
               if(tmpFile->exists() && tmpFile->isDirectory())
               {
                  // if file is a directory use index.html
                  fullPath = File::join(
                     tmpFile->getAbsolutePath(), "index.html");
               }
               else
               {
                  // append .html if unknown extension
                  fullPath.assign(tmpFile->getAbsolutePath());
                  if(!mContentTypes->hasMember(tmpFile->getExtension()))
                  {
                     fullPath.append(".html");
                  }
               }
               File file(fullPath.c_str());

               MO_CAT_INFO(BM_WEBUI_CAT,
                  "GET VIRTUAL CONCAT %s:%s => %s >> %s",
                  plugin, filename->getString(), fullPath.c_str(),
                  fullVirtualPath.c_str());

               // if any file does not exist or is not readable, then the
               // virtual file cannot be served
               if(!file->exists())
               {
                  exists = false;
               }
               else if(!file->isReadable())
               {
                  readable = false;
               }
               else
               {
                  // take note of file modification (if any is modified, then
                  // the virtual file is considered modified)
                  if(isModified(action, file))
                  {
                     modified = true;
                  }

                  // add content length
                  contentLength += file->getLength();

                  // add to the file list
                  fileList->add(file);
               }
            }
         }

         if(!exists)
         {
            rval = serveNotFound(action, mPluginInfo[mMainPluginId]);
         }
         else if(!readable)
         {
            rval = serveForbidden(action, mPluginInfo[mMainPluginId]);
         }
         else if(!modified)
         {
            rval = serveNotModified(action);
         }
         else
         {
            MO_CAT_INFO(BM_WEBUI_CAT, "GET VIRTUAL %s:%s => %s",
               plugin, path, fullVirtualPath.c_str());

            rval = serveFileList(
               action, fileList, contentLength, mContentTypes);
         }
      }
      // file must not be virtual
      else
      {
         // concat plugin content dir and path
         File tmpFile(File::join(
            contentDirectory->getAbsolutePath(), path).c_str());

         // serve the file if it is okay to do so...
         if(!contentDirectory->contains(tmpFile))
         {
            MO_CAT_WARNING(BM_WEBUI_CAT,
               "Attempt to fetch file outside of content directory: %s",
               tmpFile->getAbsolutePath());

            rval = serveForbidden(action, mPluginInfo[mMainPluginId]);
         }
         else
         {
            string fullPath;
            if(tmpFile->exists() && tmpFile->isDirectory())
            {
               // if file is a directory use index.html
               fullPath = File::join(
                  tmpFile->getAbsolutePath(), "index.html");
            }
            else
            {
               // append .html if unknown extension
               fullPath.assign(tmpFile->getAbsolutePath());
               if(!mContentTypes->hasMember(tmpFile->getExtension()))
               {
                  fullPath.append(".html");
               }
            }
            File file(fullPath.c_str());

            MO_CAT_INFO(BM_WEBUI_CAT, "GET %s:%s => %s",
               plugin, path, fullPath.c_str());

            if(!file->exists())
            {
               rval = serveNotFound(action, mPluginInfo[mMainPluginId]);
            }
            else if(!file->isReadable())
            {
               rval = serveForbidden(action, mPluginInfo[mMainPluginId]);
            }
            else if(!isModified(action, file))
            {
               rval = serveNotModified(action);
            }
            else
            {
               rval = serveFile(action, file, mContentTypes);
            }
         }
      }
   }

   return rval;
}

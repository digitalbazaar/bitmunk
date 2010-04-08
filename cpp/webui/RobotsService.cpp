/*
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/webui/RobotsService.h"

#include "bitmunk/node/BtpActionDelegate.h"
#include "bitmunk/node/RestResourceHandler.h"
#include "bitmunk/webui/WebUiModule.h"
#include "monarch/io/FileInputStream.h"

using namespace std;
using namespace monarch::config;
using namespace monarch::io;
using namespace monarch::logging;
using namespace monarch::net;
using namespace monarch::http;
using namespace monarch::rt;
using namespace monarch::util;
using namespace bitmunk::common;
using namespace bitmunk::protocol;
using namespace bitmunk::node;
using namespace bitmunk::webui;
namespace v = monarch::validation;

typedef BtpActionDelegate<RobotsService> Handler;

RobotsService::RobotsService(Node* node, const char* path) :
   NodeService(node, path)
{
}

RobotsService::~RobotsService()
{
}

bool RobotsService::initialize()
{
   // serve file
   {
      // FIXME: change later to serve up static files?
      RestResourceHandlerRef sf = new RestResourceHandler();
      addResource("/", sf);

      // GET .../
      {
         ResourceHandler h = new Handler(
            mNode, this, &RobotsService::getFile,
            BtpAction::AuthOptional);
         sf->addHandler(h, BtpMessage::Get);
      }
   }

   return true;
}

void RobotsService::cleanup()
{
   // remove resources
   removeResource("/");
}

/**
 * Handles serving static content from disk.
 *
 * @param action the BtpAction.
 * @param file the file that should be read from disk and served to the
 *             requesting client.
 *
 * @return true if the file was served successfully, false if the file
 *         could not be served.
 */
static bool serveFile(BtpAction* action, File& file)
{
   bool rval = false;

   // setup response header
   HttpResponseHeader* resHeader = action->getResponse()->getHeader();
   resHeader->setStatus(200, "OK");
   resHeader->setField("Content-Type", "text/plain");

   Date date = file->getModifiedDate();
   TimeZone gmt = TimeZone::getTimeZone("GMT");
   string str;
   date.format(str, HttpHeader::sDateFormat, &gmt);
   resHeader->setField("Last-Modified", str.c_str());

   // don't do any content-encoding (no gzip)
   action->getRequest()->getHeader()->removeField("Accept-Encoding");
   resHeader->setField("Content-Length", file->getLength());

   // serve the file if it exists
   FileInputStream fis(file);
   rval = action->sendResult(&fis);
   fis.close();

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
   HttpResponseHeader* resHeader = action->getResponse()->getHeader();
   resHeader->setStatus(304, "Not Modified");
   return action->sendResult();
}

/**
 * Handles serving a resource not found message.
 *
 * @param action the BtpAction.
 *
 * @return true if the error was served successfully, false if not.
 */
static bool serveNotFound(BtpAction* action)
{
   // setup response header
   HttpResponseHeader* resHeader = action->getResponse()->getHeader();
   resHeader->setStatus(404, "Not Found");
   return action->sendResult();
}

/**
 * Handles serving a resource forbidden message.
 *
 * @param action the BtpAction.
 *
 * @return true if the error was served successfully, false if not.
 */
static bool serveForbidden(BtpAction* action)
{
   // setup response header
   HttpResponseHeader* resHeader = NULL;
   resHeader = action->getResponse()->getHeader();
   resHeader->setStatus(403, "Forbidden");
   return action->sendResult();
}

bool RobotsService::getFile(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = true;

   // get the configuration
   bool notFound = true;
   Config cfg = mNode->getConfigManager()->getModuleConfig(
      "bitmunk.webui.Webui");
   if(!cfg.isNull() && cfg->hasMember("staticContentPath"))
   {
      string path = File::join(
         cfg["staticContentPath"]->getString(), "robots.txt");
      File file(path.c_str());

      MO_CAT_DEBUG(BM_WEBUI_CAT, "GET %s => %s",
         "/robots.txt", path.c_str());

      if(file->exists())
      {
         notFound = false;

         if(!file->isReadable())
         {
            rval = serveForbidden(action);
         }
         else if(!isModified(action, file))
         {
            rval = serveNotModified(action);
         }
         else
         {
            rval = serveFile(action, file);
         }
      }
   }

   if(notFound)
   {
      rval = serveNotFound(action);
   }

   return rval;
}

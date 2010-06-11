/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk/protocol/BtpService.h"

#include "bitmunk/common/Logging.h"
#include "monarch/util/Timer.h"

using namespace std;
using namespace bitmunk::common;
using namespace bitmunk::protocol;
using namespace monarch::http;
using namespace monarch::net;
using namespace monarch::rt;
using namespace monarch::util;

BtpService::BtpService(const char* path, bool dynamicResources) :
   HttpRequestServicer(path),
   mRequestModifier(NULL),
   mReadThrottler(NULL),
   mWriteThrottler(NULL),
   mDynamicResources(dynamicResources),
   mAllowHttp1(false)
{
}

BtpService::~BtpService()
{
   // clean up all action names
   for(HandlerMap::iterator i = mActionHandlers.begin();
       i != mActionHandlers.end(); ++i)
   {
      free((char*)i->first);
   }
}

void BtpService::setRequestModifier(HttpRequestModifier* hrm)
{
   mRequestModifier = hrm;
}

HttpRequestModifier* BtpService::getRequestModifier()
{
   return mRequestModifier;
}

void BtpService::addResource(const char* resource, BtpActionHandlerRef& handler)
{
   if(mDynamicResources)
   {
      mResourceLock.lockExclusive();
   }

   // append resource to btp service path
   int length = strlen(getPath()) + strlen(resource);
   char fullPath[length + 1];
   sprintf(fullPath, "%s%s", getPath(), resource);

   // normalize full path
   char* normalized = (char*)malloc(length + 2);
   HttpRequestServicer::normalizePath(fullPath, normalized);

   // add full path entry to map
   mActionHandlers.insert(make_pair(normalized, handler));

   if(mDynamicResources)
   {
      mResourceLock.unlockExclusive();
   }

   MO_CAT_DEBUG(BM_PROTOCOL_CAT,
      "Added BTP resource: %s", (const char*)normalized);
}

BtpActionHandlerRef BtpService::removeResource(const char* resource)
{
   BtpActionHandlerRef rval;

   if(mDynamicResources)
   {
      mResourceLock.lockExclusive();
   }

   // append resource to btp service path
   int length = strlen(getPath()) + strlen(resource);
   char fullPath[length + 1];
   sprintf(fullPath, "%s%s", getPath(), resource);

   // normalize full path
   char normalized[length + 2];
   HttpRequestServicer::normalizePath(fullPath, normalized);

   HandlerMap::iterator i = mActionHandlers.find(normalized);
   if(i != mActionHandlers.end())
   {
      free((char*)i->first);
      rval = i->second;
      mActionHandlers.erase(i);
   }

   if(mDynamicResources)
   {
      mResourceLock.unlockExclusive();
   }

   if(!rval.isNull())
   {
      MO_CAT_DEBUG(BM_PROTOCOL_CAT,
         "Removed BTP resource: %s", (const char*)normalized);
   }

   return rval;
}

void BtpService::findHandler(char* resource, BtpActionHandlerRef& h)
{
   h.setNull();

   // strip any query
   if(resource != NULL)
   {
      char* end = strrchr(resource, '?');
      if(end != NULL)
      {
         end[0] = 0;
      }
   }

   if(mDynamicResources)
   {
      mResourceLock.lockShared();
   }

   // try to find action handler for resource
   HandlerMap::iterator i;
   while(h.isNull() && resource != NULL)
   {
      i = mActionHandlers.find(resource);
      if(i != mActionHandlers.end())
      {
         h = i->second;
      }
      else if(strlen(resource) > 1)
      {
         // try to find handler for parent resource
         char* end = strrchr(resource, '/');
         if(end != NULL)
         {
            // if parent is root (end == resource), set resource to "/"
            // if parent is not root, clear last slash
            end[(end == resource) ? 1 : 0] = 0;
         }
         else
         {
            // no resource left to search
            resource = NULL;
         }
      }
      else
      {
         // no resource left to search
         resource = NULL;
      }
   }

   if(mDynamicResources)
   {
      mResourceLock.unlockShared();
   }
}

void BtpService::serviceRequest(
   HttpRequest* request, HttpResponse* response)
{
   // print out header
   MO_CAT_DEBUG(BM_PROTOCOL_CAT,
      "BtpService (%s) received header from %s:%i:\n%s",
      request->getConnection()->isSecure() ? "SSL" : "non-SSL",
      request->getConnection()->getRemoteAddress()->getAddress(),
      request->getConnection()->getRemoteAddress()->getPort(),
      request->getHeader()->toString().c_str());

   // set throttlers
   request->getConnection()->setBandwidthThrottler(
      getBandwidthThrottler(true), true);
   request->getConnection()->setBandwidthThrottler(
      getBandwidthThrottler(false), false);

   // set response server and connection fields
   response->getHeader()->setField("Server", "BtpServer/1.0");
   response->getHeader()->setField("Connection", "close");

   // do request modification
   if(mRequestModifier != NULL)
   {
      mRequestModifier->modifyRequest(request);
   }

   // check version (HTTP/1.1 or HTTP/1.0 if allowed)
   if(strcmp(request->getHeader()->getVersion(), "HTTP/1.1") == 0 ||
      (http1Allowed() &&
       strcmp(request->getHeader()->getVersion(), "HTTP/1.0") == 0))
   {
      // start timer
      Timer timer;
      timer.start();

      // set response version
      response->getHeader()->setVersion(request->getHeader()->getVersion());

      // FIXME: here or in another btp class like BtpAction or BtpMessage
      // the host field value in the request must be checked against a list
      // of valid hosts to ensure that the client doesn't spoof another client
      // that connected to a different host

      // create BtpAction
      BtpActionHandlerRef handler;
      BtpAction* action = createAction(request, handler);
      action->setResponse(response);
      if(!handler.isNull())
      {
         // handle action
         (*handler)(action);
      }
      else
      {
         // no action handler for resource
         response->getHeader()->setStatus(404, "Not Found");
         action->sendResult();
      }

      // print total service time
      MO_CAT_INFO(BM_PROTOCOL_CAT,
         "BtpService (%s) serviced resource '%s %s' for %s:%i "
         "in %" PRIu64 " ms",
         request->getConnection()->isSecure() ? "SSL" : "non-SSL",
         action->getRequest()->getHeader()->getMethod(),
         action->getResource(),
         request->getConnection()->getRemoteAddress()->getAddress(),
         request->getConnection()->getRemoteAddress()->getPort(),
         timer.getElapsedMilliseconds());

      // clean up action
      delete action;
   }
   else
   {
      // send 505 HTTP Version Not Supported
      response->getHeader()->setVersion("HTTP/1.1");
      response->getHeader()->setStatus(505, "HTTP Version Not Supported");
      response->getHeader()->setField("Content-Length", 0);
      response->sendHeader();

      // print out header
      MO_CAT_DEBUG(BM_PROTOCOL_CAT,
         "BtpService (%s) sent header to %s:%i:\n%s",
         request->getConnection()->isSecure() ? "SSL" : "non-SSL",
         request->getConnection()->getRemoteAddress()->getAddress(),
         request->getConnection()->getRemoteAddress()->getPort(),
         response->getHeader()->toString().c_str());
   }
}

inline void BtpService::setBandwidthThrottler(BandwidthThrottler* bt, bool read)
{
   read ? mReadThrottler = bt : mWriteThrottler = bt;
}

inline BandwidthThrottler* BtpService::getBandwidthThrottler(bool read)
{
   return (read ? mReadThrottler : mWriteThrottler);
}

inline void BtpService::setAllowHttp1(bool allow)
{
   mAllowHttp1 = allow;
}

inline bool BtpService::http1Allowed()
{
   return mAllowHttp1;
}

bool BtpService::setKeepAlive(BtpAction* action)
{
   // default to keep-alive for HTTP/1.1 and close for HTTP/1.0
   bool keepAlive =
      strcmp(action->getRequest()->getHeader()->getVersion(), "HTTP/1.1") == 0;

   string connection;
   if(action->getRequest()->getHeader()->getField("Connection", connection))
   {
      if(strcasecmp(connection.c_str(), "close") == 0)
      {
         keepAlive = false;
      }
      else if(strcasecmp(connection.c_str(), "keep-alive") == 0)
      {
         keepAlive = true;
      }
   }

   if(keepAlive)
   {
      action->getResponse()->getHeader()->setField("Connection", "keep-alive");
   }

   return keepAlive;
}

BtpAction* BtpService::createAction(
   HttpRequest* request, BtpActionHandlerRef& handler)
{
   BtpAction* rval = NULL;

   // normalize request resource and find handler
   const char* path = request->getHeader()->getPath();
   char resource[strlen(path) + 2];
   HttpRequestServicer::normalizePath(path, resource);
   char res[strlen(resource) + 1];
   strcpy(res, resource);
   findHandler(resource, handler);

   // create action
   rval = new BtpAction(res);

   // set base resource
   if(!handler.isNull())
   {
      rval->setBaseResourcePath(resource);
   }

   // set request in action
   rval->setRequest(request);

   return rval;
}

void BtpService::setResourceCreated(
   HttpResponse* response, const char* location)
{
   response->getHeader()->setField("Location", location);
   response->getHeader()->setStatus(201, "Created");
}

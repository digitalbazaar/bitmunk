/*
 * Copyright (c) 2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk/node/ProxyService.h"

#include "bitmunk/common/Logging.h"
#include "bitmunk/common/Tools.h"
#include "bitmunk/node/BtpActionDelegate.h"
#include "bitmunk/node/NodeModule.h"
#include "bitmunk/node/RestResourceHandler.h"
#include "monarch/net/SocketTools.h"

using namespace std;
using namespace monarch::config;
using namespace monarch::crypto;
using namespace monarch::http;
using namespace monarch::io;
using namespace monarch::net;
using namespace monarch::rt;
using namespace bitmunk::common;
using namespace bitmunk::protocol;
using namespace bitmunk::node;

typedef BtpActionDelegate<ProxyService> Handler;

#define PATH_WILDCARD   "*"

// Note: Current implementation doesn't lock on proxy map. It is assumed it
// will be set up before use and not changed thereafter.

ProxyService::ProxyService(Node* node, const char* path) :
   NodeService(node, path)
{
}

ProxyService::~ProxyService()
{
   for(ProxyMap::iterator i = mProxyMap.begin(); i != mProxyMap.end(); i++)
   {
      free((char*)i->first);
   }
}

bool ProxyService::initialize()
{
   // root
   {
      RestResourceHandlerRef proxyResource = new RestResourceHandler();
      addResource("/", proxyResource);

      // METHOD .../proxy
      ResourceHandler h = new Handler(
         mNode, this, &ProxyService::proxy,
         BtpAction::AuthOptional);

      proxyResource->addHandler(h, BtpMessage::Post, -1);
      proxyResource->addHandler(h, BtpMessage::Put, -1);
      proxyResource->addHandler(h, BtpMessage::Get, -1);
      proxyResource->addHandler(h, BtpMessage::Delete, -1);
      proxyResource->addHandler(h, BtpMessage::Head, -1);
      proxyResource->addHandler(h, BtpMessage::Options, -1);
   }

   return true;
}

void ProxyService::cleanup()
{
   // remove resources
   removeResource("/");
}

void ProxyService::addMapping(const char* path, const char* url)
{
   UrlRef u(NULL);

   // convert simple path into full local url if needed
   if(url[0] == '/')
   {
      // build full url
      string tmp = mNode->getMessenger()->getSelfUrl(false);
      tmp.append(url);
      u = new Url(tmp.c_str());
   }
   else
   {
      u = new Url(url);
   }

   mProxyMap[strdup(path)] = u;
}

/**
 * Proxies HTTP traffic coming from the "in" connection to the "out" connection.
 *
 * @param header the related HTTP header.
 * @param in the incoming HTTP connection.
 * @param out the outgoing HTTP connection.
 *
 * @return true if successful, false if an exception occurred.
 */
static bool _proxyHttp(
   HttpHeader* header, HttpConnection* in, HttpConnection* out)
{
   bool rval;

   // send header
   rval = out->sendHeader(header);

   // see if there is content to proxy
   if(rval && header->hasContent())
   {
      // proxy content
      HttpTrailer trailer;
      InputStream* is = in->getBodyInputStream(header, &trailer);
      rval = out->sendBody(header, is, &trailer);
      is->close();
      delete is;
   }

   return rval;
}

void ProxyService::proxy(BtpAction* action)
{
   bool pass = false;

   // get the URL to connect to, based on the resource
   ProxyMap::iterator i = mProxyMap.find(action->getResource());
   if(i == mProxyMap.end())
   {
      // specific path not found, use wildcard path
      i = mProxyMap.find(PATH_WILDCARD);
   }

   if(i == mProxyMap.end())
   {
      // send 404
      HttpResponseHeader* header = action->getResponse()->getHeader();
      header->setStatus(404, "Not Found");
      action->sendResult();
   }
   else
   {
      UrlRef url = i->second;

      // do proxy:
      MO_CAT_INFO(BM_NODE_CAT,
         "ProxyService proxying %s to: %s",
         action->getResource(), url->toString().c_str());

      // get a connection
      BtpClient* btpc = mNode->getMessenger()->getBtpClient();
      HttpConnection* conn = btpc->createConnection(false, &(*url));
      if(conn == NULL)
      {
         // send service unavailable
         HttpResponseHeader* header = action->getResponse()->getHeader();
         header->setStatus(503, "Service Unavailable");
         action->sendResult();
      }
      else
      {
         HttpRequest* req = action->getRequest();
         HttpResponse* res = action->getResponse();

         // add X-Forwarded headers
         HttpRequestHeader* reqHeader = req->getHeader();
         reqHeader->addField("X-Forwarded-For",
            req->getConnection()->getRemoteAddress()->toString(true).c_str());
         reqHeader->addField("X-Forwarded-Host",
            req->getHeader()->getFieldValue("Host").c_str());
         reqHeader->addField("X-Forwarded-Server",
            SocketTools::getHostname().c_str());

         // rewrite host
         req->getHeader()->setField("Host", url->getHostAndPort().c_str());

         // rewrite path if it does not match the URL path and is not
         // the wildcard path
         if(strcmp(action->getResource(), url->getPath().c_str()) != 0 &&
            strcmp(i->first, PATH_WILDCARD) != 0)
         {
            DynamicObject query;
            action->getResourceQuery(query, true);
            string path = url->getPath();
            path.append(Url::formEncode(query));
            req->getHeader()->setPath(path.c_str());
         }

         // proxy the client's request and server's response
         pass =
            _proxyHttp(req->getHeader(), req->getConnection(), conn) &&
            _proxyHttp(res->getHeader(), conn, req->getConnection());

         // close connection
         conn->close();

         // clean up
         delete conn;

         // result sent
         action->setResultSent(true);
      }
   }

   if(!pass && !action->isResultSent())
   {
      // send exception (client's fault if code < 500)
      ExceptionRef e = Exception::get();
      action->sendException(e, e->getCode() < 500);
   }
}

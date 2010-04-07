/*
 * Copyright (c) 2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk/node/ProxyService.h"

#include "bitmunk/common/Tools.h"
#include "bitmunk/node/BtpActionDelegate.h"
#include "bitmunk/node/RestResourceHandler.h"
#include "monarch/io/ByteArrayInputStream.h"
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

// Note: Current implementation doesn't lock on proxy map. It is assumed it
// will be set up before use and not changed thereafter.

// Note: ProxyService could be alternatively implemented as an extension to
// the HttpConnectionServicer class in the future.

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

      // METHOD .../
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

void ProxyService::addMapping(
   const char* host, const char* path, const char* url)
{
   // convert relative url into absolute url if needed
   string absUrl;
   if(url[0] == '/')
   {
      // build full url
      absUrl = mNode->getMessenger()->getSelfUrl(false);
   }
   else if(strncmp(url, "http://", 7) != 0)
   {
      absUrl = "http://";
   }
   absUrl.append(url);

   /* Build the absolute path (including host) that will be searched for in an
      HTTP request so that it can be mapped to the absolute url. Only append
      the proxy service path if it isn't the root path and only add the given
      path if it isn't a wildcard. The end result will be only the hostname
      for wildcards and the hostname concatenated with the proxy service path
      and relative path without having a leading double-slash when the proxy
      service is the root service.

      The code that looks for matches will first look for a matching host+path
      and if that fails, it will look for a host only (for wildcards).
   */
   string absPath = host;
   bool root = (strcmp(mPath, "/") == 0);
   bool wildcard = (strcmp(path, "*") == 0);
   if(!wildcard)
   {
      if(!root)
      {
         absPath.append(mPath);
      }
      absPath.append(path);
   }

   // remove duplicate entry
   ProxyMap::iterator i = mProxyMap.find(absPath.c_str());
   if(i != mProxyMap.end())
   {
      MO_CAT_INFO(BM_NODE_CAT,
         "ProxyService removed rule %s => %s",
         i->first, i->second->toString().c_str());

      free((char*)i->first);
      mProxyMap.erase(i);
   }

   // add new entry and log it
   mProxyMap[strdup(absPath.c_str())] = new Url(absUrl);
   MO_CAT_INFO(BM_NODE_CAT,
      "ProxyService added rule %s%s => %s%s",
      absPath.c_str(), wildcard ? "/*" : "",
      absUrl.c_str(), wildcard ? "/*" : "");
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
   // get request host
   HttpRequestHeader* hrh = action->getRequest()->getHeader();
   string host = hrh->getFieldValue("Host");

   // build proxy mapping (absolute path)
   string absPath = host;
   absPath.append(action->getResource());

   // get the URL to proxy to
   bool wildcard = false;
   ProxyMap::iterator i = mProxyMap.find(absPath);
   if(i == mProxyMap.end())
   {
      // specific path not found, check for wildcard using host only
      wildcard = true;
      i = mProxyMap.find(host.c_str());
   }

   if(i == mProxyMap.end())
   {
      // no proxy mapping found, fall back to rest resource handler
      RestResourceHandler::operator()(action);
   }
   else
   {
      UrlRef url = i->second;

      // do proxy:
      MO_CAT_INFO(BM_NODE_CAT,
         "ProxyService proxying %s => %s",
         absPath.c_str(), url->toString().c_str());

      // get a connection
      BtpClient* btpc = mNode->getMessenger()->getBtpClient();
      HttpConnection* conn = btpc->createConnection(false, &(*url));
      if(conn == NULL)
      {
         // send service unavailable
         HttpResponseHeader* header = action->getResponse()->getHeader();
         header->setStatus(503, "Service Unavailable");
         string content =
            "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n"
            "<html><head>\n"
            "<title>503 Service Unavailable</title>\n"
            "</head><body>\n"
            "<h1>Service Unavailable</h1>\n"
            "<p>The service was not available.</p>\n"
            "</body></html>";
         ByteBuffer b(content.length());
         b.put(content.c_str(), content.length(), false);
         ByteArrayInputStream bais(&b);
         action->sendResult(&bais);
      }
      else
      {
         HttpRequest* req = action->getRequest();
         HttpResponse* res = action->getResponse();

         // add X-Forwarded headers
         HttpRequestHeader* reqHeader = req->getHeader();
         reqHeader->appendFieldValue("X-Forwarded-For",
            req->getConnection()->getRemoteAddress()->toString(true).c_str());
         reqHeader->appendFieldValue("X-Forwarded-Host",
            req->getHeader()->getFieldValue("Host").c_str());
         reqHeader->appendFieldValue("X-Forwarded-Server",
            SocketTools::getHostname().c_str());

         // rewrite host
         req->getHeader()->setField("Host", url->getHostAndPort().c_str());

         // rewrite path if it does not match the URL path and is not
         // the wildcard path
         if(!wildcard &&
            strcmp(action->getResource(), url->getPath().c_str()) != 0)
         {
            DynamicObject query;
            action->getResourceQuery(query, true);
            string path = url->getPath();
            path.append(Url::formEncode(query));
            req->getHeader()->setPath(path.c_str());
         }

         // proxy the client's request and receive server's header
         if(_proxyHttp(req->getHeader(), req->getConnection(), conn) &&
            conn->receiveHeader(res->getHeader()))
         {
            // proxy the server's response, consider result sent
            _proxyHttp(res->getHeader(), conn, req->getConnection());
            action->setResultSent(true);
         }

         // close connection
         conn->close();

         // clean up
         delete conn;
      }

      if(!action->isResultSent())
      {
         // send exception (client's fault if code < 500)
         ExceptionRef e = Exception::get();
         action->sendException(e, e->getCode() < 500);
      }
   }
}

/*
 * Copyright (c) 2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk/node/ProxyService.h"

#include "bitmunk/common/Tools.h"
#include "bitmunk/node/BtpActionDelegate.h"
#include "bitmunk/nodemodule/NodeModule.h"
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

#define PATH_WILDCARD   "*"

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
   // convert simple path into full local url if needed
   string fullUrl;
   if(url[0] == '/')
   {
      // build full url
      fullUrl = mNode->getMessenger()->getSelfUrl(false);
   }
   else if(strncmp(url, "http://", 7) != 0)
   {
      fullUrl = "http://";
   }
   fullUrl.append(url);

   // prepend proxy service path to given path
   bool root = (strcmp(mPath, "/") == 0);
   bool wildcard = (strcmp(path, PATH_WILDCARD) == 0);
   string fullPath;
   if(!root && !wildcard)
   {
      fullPath = mPath;
      fullPath.append(path);
   }
   else
   {
      fullPath = path;
   }

   // remove duplicate entry
   ProxyMap::iterator i = mProxyMap.find(fullPath.c_str());
   if(i != mProxyMap.end())
   {
      MO_CAT_INFO(BM_NODE_CAT,
         "ProxyService removed rule %s => %s",
         i->first, i->second->toString().c_str());

      free((char*)i->first);
      mProxyMap.erase(i);
   }

   // add new entry
   mProxyMap[strdup(fullPath.c_str())] = new Url(fullUrl);

   MO_CAT_INFO(BM_NODE_CAT,
      "ProxyService added rule %s%s%s => %s%s",
      root ? "" : mPath, wildcard ? "/" : "",
      path, fullUrl.c_str(), wildcard ? "/" PATH_WILDCARD : "");
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
   bool wildcard = false;
   ProxyMap::iterator i = mProxyMap.find(action->getResource());
   if(i == mProxyMap.end())
   {
      // specific path not found, use wildcard path
      wildcard = true;
      i = mProxyMap.find(PATH_WILDCARD);
   }

   if(i == mProxyMap.end())
   {
      // send 404
      HttpResponseHeader* header = action->getResponse()->getHeader();
      header->setStatus(404, "Not Found");
      string content =
         "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n"
         "<html><head>\n"
         "<title>404 Not Found</title>\n"
         "</head><body>\n"
         "<h1>Not Found</h1>\n"
         "<p>The document was not found.</p>\n"
         "</body></html>";
      ByteBuffer b(content.length());
      b.put(content.c_str(), content.length(), false);
      ByteArrayInputStream bais(&b);
      action->sendResult(&bais);
   }
   else
   {
      UrlRef url = i->second;

      // do proxy:
      MO_CAT_INFO(BM_NODE_CAT,
         "ProxyService proxying %s => %s",
         action->getResource(), url->toString().c_str());

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

         // proxy the client's request and the server's response
         pass =
            _proxyHttp(req->getHeader(), req->getConnection(), conn) &&
            conn->receiveHeader(res->getHeader()) &&
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

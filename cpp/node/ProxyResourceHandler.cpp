/*
 * Copyright (c) 2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk/node/ProxyResourceHandler.h"

#include "bitmunk/node/BtpActionDelegate.h"
#include "monarch/io/ByteArrayInputStream.h"
#include "monarch/net/SocketTools.h"

using namespace std;
using namespace monarch::config;
using namespace monarch::http;
using namespace monarch::io;
using namespace monarch::net;
using namespace monarch::rt;
using namespace bitmunk::protocol;
using namespace bitmunk::node;

// Note: Current implementation doesn't lock on proxy map. It is assumed it
// will be set up before use and not changed thereafter.

ProxyResourceHandler::ProxyResourceHandler(Node* node, const char* path) :
   mNode(node),
   mPath(strdup(path))
{
}

ProxyResourceHandler::~ProxyResourceHandler()
{
   free(mPath);
   for(ProxyMap::iterator i = mProxyMap.begin(); i != mProxyMap.end(); i++)
   {
      PathToUrl* m = i->second;
      for(PathToUrl::iterator mi = m->begin(); mi != m->end(); mi++)
      {
         free((char*)mi->first);
      }
      delete m;
      free((char*)i->first);
   }
}

void ProxyResourceHandler::addMapping(
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

   /* Build the absolute path that will be searched for in an HTTP request so
      that it can be mapped to the absolute url. Only append the proxy service
      path if it isn't the root path and only add the given path if it isn't a
      wildcard. The end result will be either a wildcard or the proxy service
      path concatenated with the relative path without having a leading
      double-slash when the proxy service is the root service.

      The code that looks for matches will first look for the given host, then
      for a wilcard host if that fails, then for the absolute path within the
      found host mapping (if any), and then for a path wildcard if that fails.
   */
   string absPath;
   bool root = (strcmp(mPath, "/") == 0);
   bool wildcard = (strcmp(path, "*") == 0);
   if(wildcard)
   {
      absPath.push_back('*');
   }
   else
   {
      if(!root)
      {
         absPath.append(mPath);
      }
      absPath.append(path);
   }

   // find PathToUrl map to update
   PathToUrl* m;
   ProxyMap::iterator i = mProxyMap.find(host);
   if(i != mProxyMap.end())
   {
      // get existing map
      m = i->second;

      // remove any duplicate entry
      PathToUrl::iterator mi = m->find(absPath.c_str());
      if(mi != m->end())
      {
         MO_CAT_INFO(BM_NODE_CAT,
            "ProxyResourceHandler removed rule: %s%s/* => %s%s/*",
            host, (wildcard || strlen(mi->first) == 0) ? "" : mi->first,
            mi->second->getHostAndPort().c_str(),
            (mi->second->getPath().length() == 1) ?
               "" : mi->second->getPath().c_str());

         free((char*)mi->first);
         m->erase(mi);
      }
   }
   else
   {
      // add new map
      m = new PathToUrl;
      mProxyMap[strdup(host)] = m;
   }

   // add new entry and log it
   UrlRef u = new Url(absUrl);
   (*m)[strdup(absPath.c_str())] = u;
   MO_CAT_INFO(BM_NODE_CAT,
      "ProxyResourceHandler added rule: %s%s/* => %s%s/*",
      host, (wildcard || absPath.length() == 0) ? "" : absPath.c_str(),
      u->getHostAndPort().c_str(),
      (u->getPath().length() == 1) ? "" : u->getPath().c_str());
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

void ProxyResourceHandler::operator()(BtpAction* action)
{
   // first see if there is a resource handler for the action
   HandlerMap::iterator hmi = findHandler(action);
   if(hmi != mHandlers.end())
   {
      // handler found, delegate to rest resource handler
      RestResourceHandler::handleAction(action, hmi);
   }
   else
   {
      // look for a proxy mapping:

      // get request host
      HttpRequestHeader* hrh = action->getRequest()->getHeader();
      string host = hrh->getFieldValue("Host");

      // find an entry for the request host
      ProxyMap::iterator i = mProxyMap.find(host.c_str());
      if(i == mProxyMap.end())
      {
         // check for the any-host wildcard
         i = mProxyMap.find("*");
      }

      UrlRef url(NULL);
      string resource;
      bool wildcard = false;
      if(i != mProxyMap.end())
      {
         PathToUrl* m = i->second;

         // try to find the URL to proxy to based on the incoming absolute path
         string parent;
         resource = action->getResource();
         PathToUrl::iterator mi;
         do
         {
            mi = m->find(resource.c_str());
            if(mi == m->end())
            {
               string parent = Url::getParentPath(resource.c_str());
               if(strcmp(parent.c_str(), resource.c_str()) != 0)
               {
                  // haven't hit root path yet, keep checking
                  resource = parent;
               }
               else
               {
                  // path not found, check for wildcard path
                  wildcard = true;
                  mi = m->find("*");
               }
            }
         }
         while(mi == m->end() && !wildcard);

         if(mi != m->end())
         {
            // get URL to proxy to
            url = mi->second;
         }
      }

      if(url.isNull())
      {
         // no proxy mapping found, delegate to rest resource handler to
         // send 404
         RestResourceHandler::handleAction(action, hmi);
      }
      else
      {
         // get client-side request and response
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

         // rewrite the request path if it does not match the URL path to
         // proxy to
         string path = action->getResource();
         if(strcmp(action->getResource(), url->getPath().c_str()) != 0)
         {
            if(wildcard)
            {
               // since a wildcard is used, prepend the URL path to the
               // resource
               path.insert(0, url->getPath().c_str());
            }
            else
            {
               // replace the part of the resource that matched the proxy
               // mapping with the rewrite path from the proxy URL
               path.replace(0, resource.length(), url->getPath().c_str());
            }

            // add query and do rewrite
            DynamicObject query;
            action->getResourceQuery(query, true);
            path.append(Url::formEncode(query));
            req->getHeader()->setPath(path.c_str());
         }

         // do proxy:
         MO_CAT_INFO(BM_NODE_CAT,
            "ProxyResourceHandler proxying %s%s => %s%s",
            host.c_str(), action->getResource(),
            url->getHostAndPort().c_str(), path.c_str());

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
}

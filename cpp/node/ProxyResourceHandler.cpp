/*
 * Copyright (c) 2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk/node/ProxyResourceHandler.h"

#include "bitmunk/node/BtpActionDelegate.h"
#include "monarch/io/ByteArrayInputStream.h"
#include "monarch/net/SocketTools.h"
#include "monarch/util/StringTokenizer.h"
#include "monarch/util/StringTools.h"

#include <algorithm>

using namespace std;
using namespace monarch::config;
using namespace monarch::http;
using namespace monarch::io;
using namespace monarch::net;
using namespace monarch::rt;
using namespace monarch::util;
using namespace monarch::util::regex;
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
   ProxyResourceHandler::clearPermittedHosts();
   free(mPath);
   for(ProxyMap::iterator i = mProxyMap.begin(); i != mProxyMap.end(); i++)
   {
      PathToInfo* m = i->second;
      for(PathToInfo::iterator mi = m->begin(); mi != m->end(); mi++)
      {
         free((char*)mi->first);
      }
      delete m;
      free((char*)i->first);
   }
}

bool ProxyResourceHandler::addPermittedHost(const char* host)
{
   bool rval = true;

   // see if the host contains a wildcard
   if(strchr(host, '*') == NULL)
   {
      // not a wildcard
      mPermittedHosts.push_back(strdup(host));
      MO_CAT_INFO(BM_NODE_CAT,
         "ProxyResourceHandler added permitted host: %s", host);
   }
   else
   {
      // build regex, tokenizing on wildcards
      int paren = 0;
      string regex;
      StringTokenizer st(host, '*');
      while(st.hasNextToken())
      {
         // escape periods in token
         string token = st.nextToken();
         if(token.length() == 0 && regex.length() != 0)
         {
            regex.append(".*");
         }
         else if(token.length() > 0)
         {
            // handle ending without a colon or any port number
            if(token.at(token.length() - 1) == ':')
            {
               token.replace(token.length() - 1, 1, "($|:");
               paren++;
            }

            // start regex
            if(regex.length() == 0)
            {
               // handle starting with no period or with text then a period
               if(token.at(0) == '.')
               {
                  regex.append("(^|^.*\\.)");
                  token.erase(0, 1);
               }
               else
               {
                  regex.append("^.*");
               }
            }
            // continue regex
            else
            {
               regex.append(".*");
            }

            StringTools::replaceAll(token, ".", "\\.");
            regex.append(token);
         }
      }
      if(regex.length() == 0)
      {
         // handle the anything regex
         regex.append("^.*$");
      }
      else
      {
         // end regex
         regex.push_back('$');
         regex.append(paren, ')');
      }

      PatternRef pattern = Pattern::compile(regex.c_str(), true, false);
      if(pattern.isNull())
      {
         ExceptionRef e = new Exception(
            "Invalid permitted host wildcard format.",
            "bitmunk.node.ProxyResourceHandler.InvalidPermittedHost");
         e->getDetails()["host"] = host;
         e->getDetails()["regex"] = regex.c_str();
         Exception::push(e);
         rval = false;
      }
      else
      {
         mWildcardHosts.push_back(pattern);
         MO_CAT_INFO(BM_NODE_CAT,
            "ProxyResourceHandler added permitted host: %s (regex: '%s')",
            host, regex.c_str());
      }
   }

   return rval;
}

void ProxyResourceHandler::clearPermittedHosts()
{
   for(PermittedHosts::iterator i = mPermittedHosts.begin();
       i != mPermittedHosts.end(); i++)
   {
      free(*i);
   }
   mPermittedHosts.clear();
   mWildcardHosts.clear();
   MO_CAT_INFO(BM_NODE_CAT, "ProxyResourceHandler cleared permitted hosts");
}

void ProxyResourceHandler::addMapping(
   const char* host, const char* path, const char* url, bool rewriteHost)
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
      for a wildcard host if that fails, then for the absolute path within the
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

   // find PathToInfo map to update
   PathToInfo* m;
   ProxyMap::iterator i = mProxyMap.find(host);
   if(i != mProxyMap.end())
   {
      // get existing map
      m = i->second;

      // remove any duplicate entry
      PathToInfo::iterator mi = m->find(absPath.c_str());
      if(mi != m->end())
      {
         MO_CAT_INFO(BM_NODE_CAT,
            "ProxyResourceHandler removed rule: %s%s/* => %s%s/*",
            host, (wildcard || strlen(mi->first) == 0) ? "" : mi->first,
            mi->second.url->getHostAndPort().c_str(),
            (mi->second.url->getPath().length() == 1) ?
               "" : mi->second.url->getPath().c_str());

         free((char*)mi->first);
         m->erase(mi);
      }
   }
   else
   {
      // add new map
      m = new PathToInfo;
      mProxyMap[strdup(host)] = m;
   }

   // add new entry and log it
   MappingInfo info;
   info.url = new Url(absUrl);
   info.rewriteHost = rewriteHost;
   (*m)[strdup(absPath.c_str())] = info;
   MO_CAT_INFO(BM_NODE_CAT,
      "ProxyResourceHandler added rule: %s%s/* => %s%s/*",
      host, (wildcard || absPath.length() == 0) ? "" : absPath.c_str(),
      info.url->getHostAndPort().c_str(),
      (info.url->getPath().length() == 1) ? "" : info.url->getPath().c_str());
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
   // get request host
   HttpRequestHeader* hrh = action->getRequest()->getHeader();
   string host = hrh->getFieldValue("Host");
   // FIXME: add support for using X-Forwarded-Host if provided?

   // find a virtual host entry for the request host
   ProxyMap::iterator i = mProxyMap.find(host.c_str());
   if(i == mProxyMap.end())
   {
      // check for the any-host wildcard
      i = mProxyMap.find("*");
   }

   // find a proxy mapping
   MappingInfo info;
   string resource;
   bool wildcard = false;
   if(i != mProxyMap.end())
   {
      PathToInfo* m = i->second;

      // try to find the URL to proxy to based on the incoming absolute path
      string parent;
      resource = action->getResource();
      PathToInfo::iterator mi;
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
         // get MappingInfo
         info = mi->second;
      }
   }

   // if proxy mapping is a wildcard, try to get handler if host is permitted
   HandlerMap::iterator hmi = mHandlers.end();
   if(wildcard && isPermittedHost(host.c_str()))
   {
      // see if there is a resource handler for the action
      hmi = findHandler(action);
   }

   // if a handler was found or there is no proxy mapping then delegate
   // to rest resource handler
   if(hmi != mHandlers.end() || info.url.isNull())
   {
      RestResourceHandler::handleAction(action, hmi);
   }
   // proxy mapping found, do proxy
   else
   {
      // get URL to proxy to
      UrlRef url = info.url;

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

      // rewrite host if mapping info specifies it
      if(info.rewriteHost)
      {
         // handle 0.0.0.0 (any host)
         string urlHost = url->getHost();
         if(strcmp(urlHost.c_str(), "0.0.0.0") == 0)
         {
            urlHost = url->getHostAndPort().replace(
               0, 8, host.substr(0, host.find(':')).c_str());
         }
         else
         {
            urlHost = url->getHostAndPort();
         }
         req->getHeader()->setField("Host", urlHost.c_str());
      }

      // rewrite the request path if it does not match the URL path to
      // proxy to
      string path = reqHeader->getPath();
      if(strcmp(path.c_str(), url->getPath().c_str()) != 0)
      {
         if(wildcard)
         {
            // since a wildcard is used, prepend the URL path to the
            // resource (if the url path isn't '/')
            string urlPath = url->getPath();
            if(urlPath.length() > 1)
            {
               path.insert(0, url->getPath().c_str());
            }
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
      MO_CAT_DEBUG(BM_NODE_CAT,
         "ProxyResourceHandler request header for %s%s => %s%s:\n%s",
         host.c_str(), action->getResource(),
         url->getHostAndPort().c_str(), path.c_str(),
         req->getHeader()->toString().c_str());

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

bool ProxyResourceHandler::isPermittedHost(const char* host)
{
   bool rval = false;

   if(mPermittedHosts.empty() && mWildcardHosts.empty())
   {
      // all hosts are permitted
      rval = true;
   }
   else
   {
      // look for the specific host
      for(PermittedHosts::iterator i = mPermittedHosts.begin();
          !rval && i != mPermittedHosts.end(); i++)
      {
         if(strcasecmp(host, *i) == 0)
         {
            rval = true;
         }
      }

      if(!rval)
      {
         // check wildcards
         for(WildcardHosts::iterator i = mWildcardHosts.begin();
             !rval && i != mWildcardHosts.end(); i++)
         {
            if((*i)->match(host))
            {
               rval = true;
            }
         }
      }
   }

   return rval;
}

/*
 * Copyright (c) 2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_node_ProxyResourceHandler_H
#define bitmunk_node_ProxyResourceHandler_H

#include "bitmunk/node/Node.h"
#include "bitmunk/node/RestResourceHandler.h"

namespace bitmunk
{
namespace node
{

/**
 * A ProxyResourceHandler redirects HTTP traffic to another server.
 *
 * @author Dave Longley
 */
class ProxyResourceHandler : public bitmunk::node::RestResourceHandler
{
protected:
   /**
    * The related Bitmunk Node.
    */
   bitmunk::node::Node* mNode;

   /**
    * The resource path to this handler.
    */
   char* mPath;

   /**
    * A map of incoming path to new url.
    */
   typedef std::map<
      const char*, monarch::net::UrlRef, monarch::util::StringComparator>
      PathToUrl;

   /**
    * A map of host to PathToUrlMap.
    */
   typedef std::map<const char*, PathToUrl*, monarch::util::StringComparator>
      ProxyMap;
   ProxyMap mProxyMap;

public:
   /**
    * Creates a new ProxyResourceHandler.
    *
    * @param node the associated Bitmunk Node.
    * @param path the resource path to this handler.
    */
   ProxyResourceHandler(Node* node, const char* path);

   /**
    * Destructs this ProxyResourceHandler.
    */
   virtual ~ProxyResourceHandler();

   /**
    * Adds a proxy mapping. If the given host and path are received in an HTTP
    * request, that request will be proxied to the given URL and the proceeding
    * response will be proxied back to the client. A blank host value will
    * assume relative urls and a path value of '*' will proxy all paths for
    * the given host.
    *
    * Any sub-path of the given path will also be proxied.
    *
    * Note: The given path will be interpreted relative to the path of the
    * proxy handler. This means that if the proxy handler is for the resource:
    *
    * /path/to/handler
    *
    * Then a path of "/foo/bar" will look for "/path/to/handler/foo/bar" in
    * the HTTP request, regardless of the host value.
    *
    * @param host the incoming host, "*" for any host.
    * @param path the incoming path to map to another URL, relative to the
    *           proxy handler's resource path.
    * @param url the URL to map to, which may be relative or absolute and
    *           will, at proxy time, have any sub-paths appended to it.
    */
   virtual void addMapping(const char* host, const char* path, const char* url);

   /**
    * Proxies incoming HTTP traffic to another server.
    *
    * HTTP equivalent: METHOD .../
    *
    * @param action the BtpAction.
    */
   virtual void operator()(bitmunk::protocol::BtpAction* action);
};

} // end namespace webui
} // end namespace bitmunk
#endif

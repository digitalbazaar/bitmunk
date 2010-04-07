/*
 * Copyright (c) 2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_node_ProxyService_H
#define bitmunk_node_ProxyService_H

#include "bitmunk/node/NodeService.h"

namespace bitmunk
{
namespace node
{

/**
 * A ProxyService redirects HTTP traffic to another server.
 *
 * @author Dave Longley
 */
class ProxyService : public bitmunk::node::NodeService
{
protected:
   /**
    * A map of incoming path to new url.
    */
   typedef std::map<
      const char*, monarch::net::UrlRef, monarch::util::StringComparator>
      ProxyMap;
   ProxyMap mProxyMap;

public:
   /**
    * Creates a new ProxyService.
    *
    * @param node the associated Bitmunk Node.
    * @param path the path this servicer handles requests for.
    */
   ProxyService(Node* node, const char* path);

   /**
    * Destructs this ProxyService.
    */
   virtual ~ProxyService();

   /**
    * Initializes this BtpService.
    *
    * @return true if initialized, false if an Exception occurred.
    */
   virtual bool initialize();

   /**
    * Cleans up this BtpService.
    *
    * Must be called after initialize() regardless of its return value.
    */
   virtual void cleanup();

   /**
    * Adds a proxy mapping. If the given host and path are received in an HTTP
    * request, that request will be proxied to the given URL and the proceeding
    * response will be proxied back to the client. A blank host value will
    * assume relative urls and a path value of '*' will proxy all paths for
    * the given host.
    *
    * Keep in mind that the given path will be interpreted relative to the path
    * of the proxy service. This means that if the proxy service is on:
    *
    * /path/to/handler
    *
    * Then a path of "/foo/bar" will look for "/path/to/handler/foo/bar" in
    * the HTTP request, regardless of the host value.
    *
    * @param host the incoming host, "" for the default host.
    * @param path the incoming path to map to another URL, relative to the
    *           proxy service's path.
    * @param url the URL to map to, which may be relative or absolute.
    */
   virtual void addMapping(const char* host, const char* path, const char* url)

   /**
    * Proxies incoming HTTP traffic to another server.
    *
    * HTTP equivalent: METHOD .../
    *
    * @param action the BtpAction.
    */
   virtual void proxy(bitmunk::protocol::BtpAction* action);
};

} // end namespace webui
} // end namespace bitmunk
#endif

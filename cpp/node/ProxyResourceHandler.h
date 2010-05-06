/*
 * Copyright (c) 2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_node_ProxyResourceHandler_H
#define bitmunk_node_ProxyResourceHandler_H

#include "bitmunk/node/Node.h"
#include "bitmunk/node/RestResourceHandler.h"
#include "monarch/util/regex/Pattern.h"

namespace bitmunk
{
namespace node
{

/**
 * A ProxyResourceHandler redirects HTTP traffic to another server.
 *
 * An HTTP request (BTP action) is processed by this handler as follows:
 *
 * 1. Do proxy on specific paths.
 * 2. Do permitted hosts resource handling.
 * 3. Do proxy on wild-card paths.
 *
 * In pseudo-code:
 *
 * If there is a path-specific proxy mapping, do proxy.
 *    Done.
 * Else if the request host is permitted, find a handler to the resource.
 *    If found, delegate to RestResourceHandler.
 *       Done.
 * If not Done and found a wild-card proxy mapping, do proxy.
 *    Done.
 * Else delegate to RestResourceHandler.
 *    Done.
 *
 * Note: A request host is "permitted" if its related request has been
 * authorized to be handled by local resource handlers. Otherwise it can only
 * be handled by proxy mappings.
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
    * Data for a proxy mapping.
    */
   struct MappingInfo
   {
      monarch::net::UrlRef url;
      bool rewriteHost;
   };

   /**
    * A map of incoming path to mapping info.
    */
   typedef std::map<const char*, MappingInfo, monarch::util::StringComparator>
      PathToInfo;

   /**
    * A map of host to PathToInfo map.
    */
   typedef std::map<const char*, PathToInfo*, monarch::util::StringComparator>
      ProxyMap;
   ProxyMap mProxyMap;

   /**
    * A map of hosts to redirect permanently.
    */
   typedef std::map<const char*, const char*, monarch::util::StringComparator>
      RedirectHosts;
   RedirectHosts mRedirectHosts;

   /**
    * A list of permitted hosts.
    */
   typedef std::vector<char*> PermittedHosts;
   PermittedHosts mPermittedHosts;

   /**
    * A list of wildcard permitted hosts.
    */
   typedef std::vector<monarch::util::regex::PatternRef> WildcardHosts;
   WildcardHosts mWildcardHosts;

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
    * Adds a redirect host. This is a host to have redirected to another
    * host using the same URL path. For instance, requests with
    * www.mywebsite.com could be permanently redirected to mywebsite.com.
    *
    * @param host the host to redirect.
    * @param newHost the new host name.
    *
    * @return true if successful, false if not.
    */
   virtual bool addRedirectHost(const char* host, const char* newHost);

   /**
    * Clears all redirect hosts.
    */
   virtual void clearRedirectHosts();

   /**
    * Adds a permitted host. This method is for specifying hosts whose related
    * requests can be handled by a local resource handler. Only if no local
    * resource handler is found should the request be proxied. This provides
    * an easy way to indicate that all non-permitted hosts should not use
    * the local resource handlers and should instead be proxied immediately.
    *
    * If a request is made and its host matches the given host, then any added
    * resource handlers may be used, if found. If the request host is not in
    * this list, then only the proxy mappings will be used. If no hosts are
    * added via this method or clearPermittedHosts() is called, then resource
    * handlers may be used for any host.
    *
    * Wild cards like '*.mywebsite.com' or '*.mywebsite.com:*' are permitted.
    *
    * @param host the host to add.
    *
    * @return true if successful, false if not.
    */
   virtual bool addPermittedHost(const char* host);

   /**
    * Clears all permitted hosts. Unless addPermittedHost() is called again,
    * local resource handlers will be used with any host name.
    */
   virtual void clearPermittedHosts();

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
    * @param rewriteHost true to rewrite the host, false not to.
    */
   virtual void addMapping(
      const char* host, const char* path,
      const char* url, bool rewriteHost = true);

   /**
    * Proxies incoming HTTP traffic to another server.
    *
    * HTTP equivalent: METHOD .../
    *
    * @param action the BtpAction.
    */
   virtual void operator()(bitmunk::protocol::BtpAction* action);

protected:
   /**
    * Returns true if the given host is in the list of permitted hosts, false
    * if not.
    *
    * @param host the host to look for.
    *
    * @return true if the given host is in the list of permitted hosts, false
    *         if not.
    */
   virtual bool isPermittedHost(const char* host);
};

} // end namespace webui
} // end namespace bitmunk
#endif

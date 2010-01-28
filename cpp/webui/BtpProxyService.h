/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_webui_BtpProxyService_H
#define bitmunk_webui_BtpProxyService_H

#include "bitmunk/protocol/BtpService.h"
#include "bitmunk/webui/SessionManager.h"
#include "monarch/http/HttpConnectionPool.h"

namespace bitmunk
{
namespace webui
{

/**
 * A BtpProxyService allows a user to communicate over btp. This service acts
 * as a btp client for a user whose identity is authenticated using a method
 * other than btp, i.e. via a session.
 *
 * There are two ways to specify the btp service url that this service should
 * act as a proxy for:
 *
 * Method 1:
 *
 * A special set of headers can be provided by the user to forward traffic
 * to a btp service:
 *
 * Btp-Proxy-Url: <url>             - The btp url to hit.
 *
 * An optional header can be passed to specify the proxy timeout:
 *
 * Btp-Proxy-Timeout: <timeout>     - The timeout in seconds.
 *
 * Method 2:
 *
 * The query parameters can be provided to specify the btp service url:
 *
 * url=<url-encoded btp service url> - The absolute btp url to hit.
 * timeout=<timeout>                 - The timeout in seconds.
 *
 * The query parameters will take precedence over the headers, but a
 * combination of the two methods may also be used.
 *
 * The URL can either be a full URL or a URL with the leading scheme, host,
 * and port removed.  When the simpler form is used the proxy will default to
 * connecting to the local node and port.  The ssl option must be used in this
 * mode to specify if ssl should be used or not.
 *
 * The method used with this service will be the same method used to proxy. Any
 * message body that is sent to this service will be forwarded to the specified
 * btp service.
 *
 * If the content received from a btp service causes a btp security breach,
 * then the following trailer will be sent if trailers are permitted:
 *
 * Btp-Proxy-Content-Security-Breach: true
 *
 * @author Dave Longley
 */
class BtpProxyService : public bitmunk::protocol::BtpService
{
protected:
   /**
    * The session manager.
    */
   SessionManager* mSessionManager;

   /**
    * An http connection pool to reuse keep-alive connections.
    */
   monarch::http::HttpConnectionPoolRef mConnectionPool;

public:
   /**
    * Creates a new BtpProxyService.
    *
    * @param sm the SessionManager used to validate sessions.
    * @param path the path this servicer handles requests for.
    */
   BtpProxyService(SessionManager* sm, const char* path);

   /**
    * Destructs this BtpProxyService.
    */
   virtual ~BtpProxyService();

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
    * Acts as a proxy to a btp service.
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

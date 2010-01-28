/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_protocol_BtpService_H
#define bitmunk_protocol_BtpService_H

#include "monarch/rt/SharedLock.h"
#include "monarch/util/StringTools.h"
#include "monarch/http/HttpRequestServicer.h"
#include "bitmunk/common/PublicKeySource.h"
#include "bitmunk/protocol/BtpActionHandler.h"

namespace bitmunk
{
namespace protocol
{

/**
 * A BtpService is a web service that provides a set of resources that actions
 * can be taken on by a client via the BTP protocol.
 *
 * A client executes a BTP action by sending an HTTP request with an
 * appropriate method to one of this service's resources. This service finds
 * the resource's associated BtpActionHandler and uses it to receive the
 * action's content (from a BtpMessage) and, optionally, to respond with a
 * result or an exception, also via a BtpMessage. The BtpAction object provides
 * the required receive/send methods for communicating with a client.
 *
 * If a more complex response is required, the BtpAction provides access to its
 * internal BtpMessage, HttpRequest, and HttpResponse.
 *
 * @author Dave Longley
 */
class BtpService : public monarch::http::HttpRequestServicer
{
protected:
   /**
    * A map of action names to BtpActionHandlerRefs.
    */
   typedef std::map<
      const char*, BtpActionHandlerRef, monarch::util::StringComparator>
      HandlerMap;
   HandlerMap mActionHandlers;

   /**
    * The BandwidthThrottler used for reading content with this service.
    */
   monarch::net::BandwidthThrottler* mReadThrottler;

   /**
    * The BandwidthThrottler used for writing with this service.
    */
   monarch::net::BandwidthThrottler* mWriteThrottler;

   /**
    * A flag to allow dynamic adding/removing of resources.
    */
   bool mDynamicResources;

   /**
    * A lock for manipulating resources.
    */
   monarch::rt::SharedLock mResourceLock;

   /**
    * A flag to allow HTTP/1.0 non-secure requests.
    */
   bool mAllowHttp1;

public:
   /**
    * Creates a new BtpService that handles requests for the given path or
    * children of that path. The given path will be normalized such that
    * begins with a forward slash and does not end with one.
    *
    * Requests will be scanned for BtpActions according to the BTP protocol.
    *
    * @param path the path this servicer handles requests for.
    * @param dynamicResources true to allow dynamic adding/removing of
    *                         resources, false not to.
    */
   BtpService(const char* path, bool dynamicResources = false);

   /**
    * Destructs this BtpService.
    */
   virtual ~BtpService();

   /**
    * Initializes this BtpService.
    *
    * @return true if initialized, false if an Exception occurred.
    */
   virtual bool initialize() = 0;

   /**
    * Cleans up this BtpService.
    *
    * Must be called after initialize() regardless of its return value.
    */
   virtual void cleanup() = 0;

   /**
    * Adds a resource to this service. The resource name will be normalized
    * such that it begins with a forward slash and its path does not end
    * with one. Consecutive slashes will be normalized to a single slash. It
    * must be relative to the BtpService's path.
    *
    * @param resouce the name of the resource.
    * @param handler the action handler for the resource.
    */
   virtual void addResource(
      const char* resource, BtpActionHandlerRef& handler);

   /**
    * Removes a resource from this service. The resource name will be
    * normalized such that it begins with a forward slash and its path
    * does not end with one. Consecutive slashes will be normalized to a
    * single slash. It must be relative to the BtpService's path.
    *
    * @param resouce the name of the resource to remove.
    *
    * @return the removed BtpActionHandler, if any (NULL if none).
    */
   virtual BtpActionHandlerRef removeResource(const char* resource);

   /**
    * Gets the BtpActionHandler for the given resource, NULL if none
    * exists.
    *
    * @param resource the resource to action to get the handler for.
    * @param h the reference to update, set to NULL if none exists.
    */
   virtual void findHandler(char* resource, BtpActionHandlerRef& h);

   /**
    * Services the passed HttpRequest. The header for the request has already
    * been received, but the body has not. The HttpResponse object is used
    * to send an appropriate response, if necessary, according to the
    * servicer's specific implementation.
    *
    * @param request the HttpRequest to service.
    * @param response the HttpResponse to respond with.
    */
   virtual void serviceRequest(
      monarch::http::HttpRequest* request,
      monarch::http::HttpResponse* response);

   /**
    * Sets a BandwidthThrottler for this BtpService.
    *
    * @param bt the BandwidthThrottler to use with all new connections made
    *           with this service.
    * @param read true to use the BandwidthThrottler for reading, false
    *             to use it for writing.
    */
   virtual void setBandwidthThrottler(
      monarch::net::BandwidthThrottler* bt, bool read);

   /**
    * Gets the BandwidthThrottler for this BtpService.
    *
    * @param read true to get the BandwidthThrottler used for reading, false
    *             to get the BandwidthThrottler used for writing.
    *
    * @return the BandwidthThrottler for this BtpService.
    */
   virtual monarch::net::BandwidthThrottler* getBandwidthThrottler(bool read);

   /**
    * Sets whether or not this BtpService allows non-secure http/1.0
    * requests.
    *
    * @param allow true to allow HTTP/1.0, false not to.
    */
   virtual void setAllowHttp1(bool allow);

   /**
    * Gets whether or not this BtpService allows non-secure http/1.0
    * requests.
    *
    * @return true if HTTP/1.0 allowed, false if not.
    */
   virtual bool http1Allowed();

protected:
   /**
    * Sets the connection to keep-alive if the client supports/requested it.
    *
    * This must be called for each processed BtpAction if keep-alive is
    * desired. By default, keep-alive if off for all BtpServices.
    *
    * @param action the BtpAction to check/update.
    *
    * @return true if keep-alive is turned on, false if not.
    */
   virtual bool setKeepAlive(BtpAction* action);

   /**
    * Creates a BtpAction from an HttpRequest. The caller of this method
    * must free the action.
    *
    * @param request the HttpRequest to create the action from.
    * @param handler to store the BtpActionHandler for the action.
    *
    * @return the created BtpAction.
    */
   virtual BtpAction* createAction(
      monarch::http::HttpRequest* request, BtpActionHandlerRef& handler);

   /**
    * Sets the location for a created resource and automatically sets the
    * status to 201 "Created."
    *
    * @param response the HttpResponse to respond with.
    * @param location the location of the created resource.
    */
   virtual void setResourceCreated(
      monarch::http::HttpResponse* response, const char* location);
};

// type definition for a reference counted BtpService
typedef monarch::rt::Collectable<BtpService> BtpServiceRef;

} // end namespace protocol
} // end namespace bitmunk
#endif

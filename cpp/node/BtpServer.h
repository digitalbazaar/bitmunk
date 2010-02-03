/*
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_node_BtpServer_H
#define bitmunk_node_BtpServer_H

#include "bitmunk/node/Node.h"
#include "bitmunk/protocol/BtpService.h"
#include "monarch/http/HttpConnectionServicer.h"
#include "monarch/net/SocketDataPresenterList.h"
#include "monarch/net/SslContext.h"
#include "monarch/util/StringTools.h"

namespace bitmunk
{
namespace node
{

/**
 * A BtpServer is a container for the BtpServices for a Node.
 *
 * @author Dave Longley
 */
class BtpServer
{
protected:
   /**
    * The associated Node.
    */
   Node* mNode;

   /**
    * The server address for http traffic.
    */
   monarch::net::InternetAddressRef mHostAddress;

   /**
    * The SSL context for https traffic.
    */
   monarch::net::SslContextRef mSslContext;

   /**
    * The SocketDataPresenterList for handling the socket presentation layer.
    */
   monarch::net::SocketDataPresenterListRef mSocketDataPresenterList;

   /**
    * The HttpConnectionServicer for handling http traffic.
    */
   monarch::http::HttpConnectionServicer mHttpConnectionServicer;

   /**
    * The service ID for the HttpConnectionServicer.
    */
   monarch::net::Server::ServiceId mServiceId;

   /**
    * The secure and non-secure BtpServices.
    */
   typedef std::map<
      const char*, bitmunk::protocol::BtpServiceRef,
      monarch::util::StringComparator> BtpServiceMap;
   BtpServiceMap mSecureServices;
   BtpServiceMap mNonSecureServices;

   /**
    * A lock for manipulating the BtpServiceLists.
    */
   monarch::rt::ExclusiveLock mBtpServiceLock;

public:
   /**
    * Creates a new BtpServer.
    *
    * @param node the associated Node.
    */
   BtpServer(Node* node);

   /**
    * Destructs this BtpServer.
    */
   virtual ~BtpServer();

   /**
    * Initializes this BtpServer.
    *
    * @param cfg the configuration to use.
    *
    * @return true on success, false on failure with exception set.
    */
   virtual bool initialize(monarch::config::Config& cfg);

   /**
    * Cleans up this BtpServer.
    */
   virtual void cleanup();

   /**
    * Adds a BtpService. If initialize is true, then initialize() will be
    * called on the service, and the service will only be added if initialize()
    * returns true. If initialize is false, the service will be added without
    * any call to initalize().
    *
    * @param service the BtpService to add.
    * @param ssl the type of SSL security to use with the service.
    * @param initialize true to initialize the service, false not to.
    *
    * @return true if the service was added, false if not.
    */
   virtual bool addService(
      bitmunk::protocol::BtpServiceRef& service,
      Node::SslStatus ssl, bool initialize = true);

   /**
    * Removes a BtpService.
    *
    * @param service the BtpService to remove.
    * @param ssl to remove the service entirely, use SslAny, to remove it from
    *            http or https, pass the appropriate SslStatus.
    * @param cleanup true to call cleanup() on the service.
    */
   virtual void removeService(
      bitmunk::protocol::BtpServiceRef& service,
      Node::SslStatus ssl = Node::SslAny, bool cleanup = true);

   /**
    * Removes a BtpService by its path.
    *
    * @param path the path for the BtpService to remove.
    * @param ssl to remove the service entirely, use SslAny, to remove it from
    *            http or https, pass the appropriate SslStatus.
    * @param cleanup true to call cleanup() on the service.
    *
    * @return the BtpService that was removed, if there were two different
    *         services removed via SslAny, only the non-SSL service will be
    *         returned -- to avoid this, call this method twice, once with
    *         SslOn and once with SslOff.
    */
   virtual bitmunk::protocol::BtpServiceRef removeService(
      const char* path, Node::SslStatus ssl = Node::SslAny,
      bool cleanup = true);

   /**
    * Gets a BtpService by its path.
    *
    * @param path the path for the BtpService to get.
    * @param ssl to get a service regardless, use SslAny, to get a secure
    *            or non-secure one, pass the appropriate SslStatus.
    *
    * @return the BtpService that was fetched, if there were two different
    *         services and SslAny was provided, only the non-SSL service will
    *         be returned.
    */
   virtual bitmunk::protocol::BtpServiceRef getService(
      const char* path, Node::SslStatus ssl = Node::SslAny);

   /**
    * Gets this node's main host address.
    *
    * @return this node's main host address.
    */
   virtual monarch::net::InternetAddressRef getHostAddress();

   /**
    * Adds an virtual host entry for the given profile.
    *
    * @param profile the Profile to add the virtual host entry for.
    *
    * @return true if successful, false if not.
    */
   virtual bool addVirtualHost(bitmunk::common::ProfileRef& profile);

   /**
    * Removes the virtual host entry for the given user ID.
    *
    * @param userId the ID of the user to remove the virtual host entry for.
    * @param ctx set to the virtual host's SslContext, if NULL does nothing.
    */
   virtual void removeVirtualHost(
      bitmunk::common::UserId userId, monarch::net::SslContextRef* ctx = NULL);
};

} // end namespace node
} // end namespace bitmunk
#endif

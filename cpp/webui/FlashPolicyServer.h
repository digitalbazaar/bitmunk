/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_webui_FlashPolicyServer_H
#define bitmunk_webui_FlashPolicyServer_H

#include "bitmunk/node/Node.h"

namespace bitmunk
{
namespace webui
{

/**
 * A FlashPolicyServer serves up socket policy files so that flash can
 * connect using sockets.
 * 
 * @author Dave Longley
 */
class FlashPolicyServer : public monarch::net::ConnectionServicer
{
protected:
   /**
    * The node running this server.
    */
   bitmunk::node::Node* mNode;
   
   /**
    * The host address to bind to.
    */
   monarch::net::InternetAddressRef mHostAddress;
   
   /**
    * The service ID for this connection service.
    */
   monarch::net::Server::ServiceId mServiceId;
   
public:
   /**
    * Creates a new FlashPolicyServer.
    */
   FlashPolicyServer();
   
   /**
    * Destructs this FlashPolicyServer.
    */
   virtual ~FlashPolicyServer();
   
   /**
    * Initializes this FlashPolicyServer.
    * 
    * @param node the node to initialize the server with.
    * 
    * @return true if initialized, false if an Exception occurred.
    */
   virtual bool initialize(bitmunk::node::Node* node);
   
   /**
    * Cleans up this FlashPolicyServer.
    */
   virtual void cleanup();
   
   /**
    * Services the passed Connection. This method should end by closing the
    * passed Connection. After this method returns, the Connection will be
    * automatically closed, if necessary, and cleaned up.
    * 
    * @param c the Connection to service.
    */
   virtual void serviceConnection(monarch::net::Connection* c);
};

} // end namespace webui
} // end namespace bitmunk
#endif

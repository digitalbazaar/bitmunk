/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_system_RobotsService_H
#define bitmunk_system_RobotsService_H

#include "bitmunk/node/NodeService.h"

// FIXME: change this later to a service that serves up
// static files instead of just specifically serving up
// the robots file

namespace bitmunk
{
namespace system
{

/**
 * A RobotsService serves up the robots.txt file.
 *
 * @author Dave Longley
 */
class RobotsService : public bitmunk::node::NodeService
{
public:
   /**
    * Creates a new RobotsService.
    * 
    * @param node the associated Bitmunk Node.
    * @param path the path this servicer handles requests for.
    */
   RobotsService(bitmunk::node::Node* node, const char* path);
   
   /**
    * Destructs this RobotsService.
    */
   virtual ~RobotsService();
   
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
    * Serves the robots.txt file.
    * 
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool getFile(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);
};

} // end namespace system
} // end namespace bitmunk
#endif

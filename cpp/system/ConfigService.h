/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_system_ConfigService_H
#define bitmunk_system_ConfigService_H

#include "bitmunk/node/NodeService.h"

namespace bitmunk
{
namespace system
{

/**
 * A ConfigService provides access to a Node's configuration. It allows the
 * configuration to be retrieved and modified.
 * 
 * @author Dave Longley
 */
class ConfigService : public bitmunk::node::NodeService
{
public:
   /**
    * Creates a new ConfigService.
    * 
    * @param node the associated Bitmunk Node.
    * @param path the path this servicer handles requests for.
    */
   ConfigService(bitmunk::node::Node* node, const char* path);
   
   /**
    * Destructs this ConfigService.
    */
   virtual ~ConfigService();
   
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
    * Gets a current configuration value.
    * 
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool getConfig(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);
   
   /**
    * Updates a configuration value.
    * 
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool postConfig(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);
};

} // end namespace system
} // end namespace bitmunk
#endif

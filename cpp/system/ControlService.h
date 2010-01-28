/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_system_ControlService_H
#define bitmunk_system_ControlService_H

#include "bitmunk/node/NodeService.h"

namespace bitmunk
{
namespace system
{

/**
 * A ControlService services system control related btp actions.
 * 
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 */
class ControlService : public bitmunk::node::NodeService
{
public:
   /**
    * Creates a new ControlService.
    * 
    * @param node the associated Bitmunk Node.
    * @param path the path this servicer handles requests for.
    */
   ControlService(bitmunk::node::Node* node, const char* path);
   
   /**
    * Destructs this ControlService.
    */
   virtual ~ControlService();
   
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
    * Get the current and/or pending node state.
    * 
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool getState(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);
   
   /**
    * Request a change of node state such as shutting down.
    * 
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool postState(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);
};

} // end namespace system
} // end namespace bitmunk
#endif

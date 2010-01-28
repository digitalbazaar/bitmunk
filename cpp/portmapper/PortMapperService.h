/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_portmapper_PortMapperService_H
#define bitmunk_portmapper_PortMapperService_H

#include "bitmunk/portmapper/PortMapper.h"
#include "bitmunk/node/NodeService.h"

namespace bitmunk
{
namespace portmapper
{

/**
 * A PortMapperService services PortMapper-related btp actions.
 * 
 * @author Dave Longley
 */
class PortMapperService : public bitmunk::node::NodeService
{
protected:
   /**
    * The associated port mapper.
    */
   PortMapper* mPortMapper;
   
public:
   /**
    * Creates a new PortMapperService.
    * 
    * @param node the associated Bitmunk Node.
    * @param pm the associated PortMapper.
    * @param path the path this servicer handles requests for.
    */
   PortMapperService(
      bitmunk::node::Node* node, PortMapper* pm, const char* path);
   
   /**
    * Destructs this PortMapperService.
    */
   virtual ~PortMapperService();
   
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
    * Adds a new port mapping.
    * 
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool addPortMapping(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);
   
   /**
    * Gets an existing port mapping.
    * 
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool getPortMapping(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);
   
   /**
    * Deletes a port mapping.
    * 
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool deletePortMapping(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);
};

} // end namespace portmapper
} // end namespace bitmunk
#endif

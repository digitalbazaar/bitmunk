/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_portmapper_PortMapper_H
#define bitmunk_portmapper_PortMapper_H

#include "bitmunk/node/Node.h"
#include "bitmunk/portmapper/IPortMapper.h"

namespace bitmunk
{
namespace portmapper
{

/**
 * A PortMapper implements the IPortMapper interface.
 * 
 * @author Dave Longley
 */
class PortMapper : public IPortMapper
{
protected:
   /**
    * The associated Bitmunk node.
    */
   bitmunk::node::Node* mNode;
   
   /**
    * Cached discovered internet gateway devices.
    */
   monarch::upnp::DeviceList mGatewayCache;
   
   /**
    * A lock for accessing/updating the cache.
    */
   monarch::rt::SharedLock mCacheLock;
   
public:
   /**
    * Creates a new PortMapper.
    */
   PortMapper();
   
   /**
    * Destructs this PortMapper.
    */
   virtual ~PortMapper();
   
   /**
    * Initializes this PortMapper.
    * 
    * @param node the associated Bitmunk node.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool init(bitmunk::node::Node* node);
   
   /**
    * Cleans up this PortMapper.
    * 
    * @param node the associated Bitmunk node.
    */
   virtual void cleanup(bitmunk::node::Node* node);
   
   /**
    * Attempts to discover local internet gateway devices that can be used
    * to manipulate port mappings.
    * 
    * @param timeout the maximum amount of time to wait to discover devices,
    *                in milliseconds.
    * 
    * @return the number of devices discovered, -1 if an exception occurred.
    */
   virtual int discoverGateways(uint32_t timeout);
   
   /**
    * Gets the discovered internet gateway devices.
    * 
    * @return a list of discovered internet gateway devices.
    */
   virtual monarch::upnp::DeviceList getDiscoveredGateways();
   
   /**
    * Adds a port mapping to an internet gateway device.
    * 
    * @param igd the internet gateway device to use.
    * @param pm the port mapping to add.
    * 
    * @return true if successful, false if not.
    */
   virtual bool addPortMapping(
      monarch::upnp::Device& igd, monarch::upnp::PortMapping& pm);
   
   /**
    * Removes a port mapping from an internet gateway device.
    * 
    * @param igd the internet gateway device to use.
    * @param pm the port mapping to remove.
    * @param true to ignore an error that port mapping doesn't exist.
    * 
    * @return true if successful, false if not.
    */
   virtual bool removePortMapping(
      monarch::upnp::Device& igd, monarch::upnp::PortMapping& pm, bool ignoreDne);
};

} // end namespace portmapper
} // end namespace bitmunk
#endif

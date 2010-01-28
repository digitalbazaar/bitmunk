/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_portmapper_IPortMapper_H
#define bitmunk_portmapper_IPortMapper_H

#include "monarch/kernel/MicroKernelModuleApi.h"
#include "monarch/upnp/TypeDefinitions.h"

namespace bitmunk
{
namespace portmapper
{

/**
 * The IPortMapper interface provides a method for adding and/or removing port
 * mappings (aka port forwarding on firewalls).
 * 
 * @author Dave Longley
 */
class IPortMapper : public monarch::kernel::MicroKernelModuleApi
{
public:
   /**
    * Creates a new IPortMapper.
    */
   IPortMapper() {};
   
   /**
    * Destructs this IPortMapper.
    */
   virtual ~IPortMapper() {};
   
   /**
    * Attempts to discover local internet gateway devices that can be used
    * to manipulate port mappings.
    * 
    * @param timeout the maximum amount of time to wait to discover devices,
    *                in milliseconds.
    * 
    * @return the number of devices discovered, -1 if an exception occurred.
    */
   virtual int discoverGateways(uint32_t timeout) = 0;
   
   /**
    * Gets the discovered internet gateway devices.
    * 
    * @return a list of discovered internet gateway devices.
    */
   virtual monarch::upnp::DeviceList getDiscoveredGateways() = 0;
   
   /**
    * Adds a port mapping to an internet gateway device.
    * 
    * @param igd the internet gateway device to use.
    * @param pm the port mapping to add.
    * 
    * @return true if successful, false if not.
    */
   virtual bool addPortMapping(
      monarch::upnp::Device& igd, monarch::upnp::PortMapping& pm) = 0;
   
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
      monarch::upnp::Device& igd, monarch::upnp::PortMapping& pm, bool ignoreDne) = 0;
};

} // end namespace portmapper
} // end namespace bitmunk
#endif

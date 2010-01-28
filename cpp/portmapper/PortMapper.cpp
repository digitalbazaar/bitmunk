/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/portmapper/PortMapper.h"

#include "bitmunk/common/Logging.h"
#include "monarch/upnp/ControlPoint.h"
#include "monarch/upnp/DeviceDiscoverer.h"

using namespace monarch::config;
using namespace monarch::rt;
using namespace monarch::upnp;
using namespace bitmunk::common;
using namespace bitmunk::portmapper;
using namespace bitmunk::node;

PortMapper::PortMapper() :
   mNode(NULL)
{
   mGatewayCache->setType(Array);
}

PortMapper::~PortMapper()
{
}

bool PortMapper::init(Node* node)
{
   bool rval = false;
   
   mNode = node;
   rval = true;
   
   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Could not initialize port mapper.",
         "bitmunk.portmapper.InitializationError");
      Exception::push(e);
   }
   
   return rval;
}

void PortMapper::cleanup(Node* node)
{
   mCacheLock.lockExclusive();
   {
      // clear cache
      mGatewayCache->clear();
   }
   mCacheLock.unlockExclusive();
}

int PortMapper::discoverGateways(uint32_t timeout)
{
   int rval = -1;
   
   mCacheLock.lockExclusive();
   {
      // clear cache
      mGatewayCache->clear();
      
      // search for 1 internet gateway device
      DeviceDiscoverer dd;
      rval = dd.discover(mGatewayCache, UPNP_DEVICE_TYPE_IGD, timeout, 1);
      if(rval)
      {
         // default to error case
         rval = -1;
         
         // get device description
         Device igd = mGatewayCache.first();
         ControlPoint cp;
         if(cp.getDeviceDescription(igd))
         {
            // get wan IP connection service from device
            Service wipcs = cp.getWanIpConnectionService(igd);
            if(!wipcs.isNull())
            {
               // get service description
               if(cp.getServiceDescription(wipcs))
               {
                  // useful device found
                  rval = 1;
               }
            }
         }
      }
   }
   mCacheLock.unlockExclusive();
   
   return rval;
}

DeviceList PortMapper::getDiscoveredGateways()
{
   DeviceList rval(NULL);
   
   mCacheLock.lockShared();
   rval = mGatewayCache.clone();
   mCacheLock.unlockShared();
   
   return rval;
}

bool PortMapper::addPortMapping(Device& igd, PortMapping& pm)
{
   bool rval = false;
   
   // get wan IP connection service from device
   ControlPoint cp;
   Service wipcs = cp.getWanIpConnectionService(igd);
   if(!wipcs.isNull())
   {
      // add port mapping
      rval = cp.addPortMapping(pm, wipcs);
   }
   
   return rval;
}

bool PortMapper::removePortMapping(Device& igd, PortMapping& pm, bool ignoreDne)
{
   bool rval = false;
   
   // get wan IP connection service from device
   ControlPoint cp;
   Service wipcs = cp.getWanIpConnectionService(igd);
   if(!wipcs.isNull())
   {
      // remove port mapping
      bool dne = false;
      rval = cp.removePortMapping(pm, wipcs, &dne);
      
      // ignore does not exist errors if requested
      if(!rval && ignoreDne && dne)
      {
         Exception::clear();
         rval = true;
      }
   }
   
   return rval;
}

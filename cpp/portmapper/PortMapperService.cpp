/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/portmapper/PortMapperService.h"

#include "bitmunk/node/BtpActionDelegate.h"
#include "bitmunk/node/RestResourceHandler.h"

using namespace monarch::net;
using namespace monarch::rt;
using namespace monarch::upnp;
using namespace bitmunk::common;
using namespace bitmunk::portmapper;
using namespace bitmunk::protocol;
using namespace bitmunk::node;
namespace v = monarch::validation;

typedef BtpActionDelegate<PortMapperService> Handler;

// default gateway discovery timeout is 5 seconds
#define GATEWAY_DISCOVERY_TIMEOUT 5*1000

PortMapperService::PortMapperService(
   Node* node, PortMapper* pm, const char* path) :
   NodeService(node, path),
   mPortMapper(pm)
{
}

PortMapperService::~PortMapperService()
{
}

bool PortMapperService::initialize()
{
   // mappings
   {
      RestResourceHandlerRef mappings = new RestResourceHandler();
      addResource("/mappings", mappings);

      // POST .../mappings
      {
         Handler* handler = new Handler(
            mNode, this, &PortMapperService::addPortMapping,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         v::ValidatorRef qValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         // FIXME: implement IP address validator
         v::ValidatorRef validator = new v::Map(
            "internalAddress", new v::Type(String),
            "internalPort", new v::Int(1024, 65535, "Invalid port number."),
            "externalPort", new v::Int(1024, 65535, "Invalid port number."),
            "protocol", new v::Regex("^(UDP)|(TCP)$"),
            "description", new v::Type(String),
            NULL);

         mappings->addHandler(h, BtpMessage::Post, 0, &qValidator, &validator);
      }

      // GET .../mappings?internalAddress=<internalAddress>
      // FIXME: implement?
      /*
      {
         ResourceHandler h = new Handler(
            mNode, this, &PortMapperService::getPortMapping,
            BtpAction::AuthRequired);
         mappings->addHandler(h, BtpMessage::Get, 0);
      }
      */

      // DELETE .../mappings/<remotePort>
      {
         Handler* handler = new Handler(
            mNode, this, &PortMapperService::deletePortMapping,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         v::ValidatorRef qValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         mappings->addHandler(h, BtpMessage::Delete, 1, &qValidator);
      }
   }

   return true;
}

void PortMapperService::cleanup()
{
   // remove resources
   removeResource("/mappings");
}

bool PortMapperService::addPortMapping(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = true;

   // discover gateways if necessary
   DeviceList gateways = mPortMapper->getDiscoveredGateways();
   if(gateways->length() == 0)
   {
      int count = mPortMapper->discoverGateways(GATEWAY_DISCOVERY_TIMEOUT);
      if(count == 0)
      {
         ExceptionRef e = new Exception(
            "No UPnP-supported internet gateways detected.",
            "bitmunk.portmapper.NoUPnPInternetGateways");
         Exception::set(e);
         rval = false;
      }
   }

   if(rval)
   {
      // build port mapping
      PortMapping pm;
      pm["NewRemoteHost"] = "";
      pm["NewExternalPort"] = in["externalPort"];
      pm["NewProtocol"] = in["protocol"];
      pm["NewInternalPort"] = in["internalPort"];
      pm["NewInternalClient"] = in["internalAddress"];
      pm["NewEnabled"] = true;
      pm["NewPortMappingDescription"] = in["description"];
      pm["NewLeaseDuration"] = 0;

      // just use first gateway
      Device igd = mPortMapper->getDiscoveredGateways().first();
      rval = mPortMapper->addPortMapping(igd, pm);
      out.setNull();
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Could not add port mapping.",
         "bitmunk.portmapper.AddPortMappingFailed");
      Exception::push(e);
   }

   return rval;
}

bool PortMapperService::getPortMapping(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = true;

   // FIXME: This method is not implemented, do we need it? -- manu
   // FIXME: yes we do, just not for the 3.2 release -dlongley

   // discover gateways if necessary
   /**
   DeviceList gateways = mPortMapper->getDiscoveredGateways();
   if(gateways->length() == 0)
   {
      int count = mPortMapper->discoverGateways(GATEWAY_DISCOVERY_TIMEOUT);
      if(count == 0)
      {
         ExceptionRef e = new Exception(
            "No UPnP-supported internet gateways detected.",
            "bitmunk.portmapper.NoUPnPInternetGateways");
         Exception::set(e);
         rval = false;
      }
   }

   if(rval)
   {
      // build port mapping
      PortMapping pm;
      pm["NewRemoteHost"] = "";
      pm["NewExternalPort"] = in["remotePort"];
      pm["NewProtocol"] = in["protocol"];
      pm["NewInternalPort"] = in["localPort"];
      pm["NewInternalClient"] = in["localAddress"];
      pm["NewEnabled"] = true;
      pm["NewPortMappingDescription"] = in["description"];
      pm["NewLeaseDuration"] = 0;

      // just use first gateway
      Device igd = mPortMapper->getDiscoveredGateways().first();
      rval = mPortMapper->getPortMapping(igd, pm);
      out.setNull();
   }
   */

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Could not get port mapping.",
         "bitmunk.portmapper.GetPortMappingFailed");
      Exception::push(e);
   }

   return rval;
}

bool PortMapperService::deletePortMapping(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = true;

   // get parameters (get external port)
   DynamicObject params;
   action->getResourceParams(params);
   uint32_t externalPort = params[0]->getUInt32();

   // discover gateways if necessary
   DeviceList gateways = mPortMapper->getDiscoveredGateways();
   if(gateways->length() == 0)
   {
      int count = mPortMapper->discoverGateways(GATEWAY_DISCOVERY_TIMEOUT);
      if(count == 0)
      {
         ExceptionRef e = new Exception(
            "No UPnP-supported internet gateways detected.",
            "bitmunk.portmapper.NoUPnPInternetGateways");
         Exception::set(e);
         rval = false;
      }
   }

   if(rval)
   {
      // build port mapping
      PortMapping pm;
      pm["NewRemoteHost"] = "";
      pm["NewExternalPort"] = externalPort;
      pm["NewProtocol"] = in["protocol"];

      // just use first gateway
      Device igd = mPortMapper->getDiscoveredGateways().first();
      rval = mPortMapper->removePortMapping(igd, pm, false);
      out.setNull();
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Could not remove port mapping.",
         "bitmunk.portmapper.RemovePortMappingFailed");
      Exception::push(e);
   }

   return rval;
}

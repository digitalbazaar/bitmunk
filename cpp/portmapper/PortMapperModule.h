/*
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_portmapper_PortMapperModule_H
#define bitmunk_portmapper_PortMapperModule_H

#include "bitmunk/nodemodule/NodeModule.h"
#include "bitmunk/portmapper/PortMapper.h"

namespace bitmunk
{
namespace portmapper
{

/**
 * A PortMapperModule is a NodeModule that provides an interface for
 * mapping ports.
 *
 * @author Dave Longley
 */
class PortMapperModule : public bitmunk::node::NodeModule
{
protected:
   /**
    * The PortMapper.
    */
   PortMapper* mPortMapper;

public:
   /**
    * Creates a new PortMapperModule.
    */
   PortMapperModule();

   /**
    * Destructs this PortMapperModule.
    */
   virtual ~PortMapperModule();

   /**
    * Adds additional dependency information. This includes dependencies
    * beyond the Bitmunk Node module dependencies.
    *
    * @param depInfo the dependency information to update.
    */
   virtual void addDependencyInfo(monarch::rt::DynamicObject& depInfo);

   /**
    * Initializes this Module with the passed Node.
    *
    * @param node the Node.
    *
    * @return true if initialized, false if an Exception occurred.
    */
   virtual bool initialize(bitmunk::node::Node* node);

   /**
    * Cleans up this Module just prior to its unloading.
    *
    * @param node the Node.
    */
   virtual void cleanup(bitmunk::node::Node* node);

   /**
    * Gets the API for this NodeModule.
    *
    * @param node the Node.
    *
    * @return the API for this NodeModule.
    */
   virtual monarch::kernel::MicroKernelModuleApi* getApi(
      bitmunk::node::Node* node);
};

} // end namespace portmapper
} // end namespace bitmunk
#endif

/*
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_node_NodeModule_H
#define bitmunk_node_NodeModule_H

#include "bitmunk/node/Node.h"
#include "monarch/kernel/MicroKernelModule.h"
#include "monarch/modest/Module.h"

namespace bitmunk
{
namespace node
{

/**
 * The NodeModule provides access to a Bitmunk Node.
 *
 * @author Dave Longley
 */
class NodeModule : public monarch::kernel::MicroKernelModule
{
protected:
   /**
    * The Bitmunk Node.
    */
   Node* mNode;

public:
   /**
    * Creates a new NodeModule.
    */
   NodeModule();

   /**
    * Destructs this NodeModule.
    */
   virtual ~NodeModule();

   /**
    * Gets dependency information.
    *
    * @return the dependency information.
    */
   virtual monarch::rt::DynamicObject getDependencyInfo();

   /**
    * Initializes this Module with the passed MicroKernel.
    *
    * @param k the MicroKernel.
    *
    * @return true if initialized, false if an Exception occurred.
    */
   virtual bool initialize(monarch::kernel::MicroKernel* k);

   /**
    * Cleans up this Module just prior to its unloading.
    *
    * @param k the MicroKernel.
    */
   virtual void cleanup(monarch::kernel::MicroKernel* k);

   /**
    * Gets the API for this MicroKernelModule.
    *
    * @param k the MicroKernel that loaded this module.
    *
    * @return the API for this MicroKernelModule.
    */
   virtual monarch::kernel::MicroKernelModuleApi* getApi(
      monarch::kernel::MicroKernel* k);
};

} // end namespace kernel
} // end namespace monarch
#endif

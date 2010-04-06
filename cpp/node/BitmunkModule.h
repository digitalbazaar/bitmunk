/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_node_BitmunkModule_H
#define bitmunk_node_BitmunkModule_H

#include "bitmunk/node/Node.h"
#include "monarch/kernel/MicroKernelModule.h"
#include "monarch/logging/Logging.h"

namespace bitmunk
{
namespace node
{

/**
 * A BitmunkModule is a MicroKernelModule that depends on the Bitmunk Node
 * module. Any module that builds on top of the core Bitmunk functionality
 * and needs to access the Bitmunk Node should extend this class instead of
 * MicroKernelModule.
 *
 * @author Dave Longley
 */
class BitmunkModule : public monarch::kernel::MicroKernelModule
{
protected:
   /**
    * A reference to the Node this module has been initialized with.
    */
   bitmunk::node::Node* mNode;

public:
   /**
    * Creates a new BitmunkModule with the specified name and version.
    *
    * @param name the name for this BitmunkModule.
    * @param version the version for this BitmunkModule (major.minor).
    */
   BitmunkModule(const char* name, const char* version);

   /**
    * Destructs this BitmunkModule.
    */
   virtual ~BitmunkModule();

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

   /**
    * Adds additional dependency information. This includes dependencies
    * beyond the Bitmunk Node module dependencies.
    *
    * @param depInfo the dependency information to update.
    */
   virtual void addDependencyInfo(monarch::rt::DynamicObject& depInfo) = 0;

   /**
    * Initializes this Module with the passed Node.
    *
    * @param node the Node.
    *
    * @return true if initialized, false if an Exception occurred.
    */
   virtual bool initialize(Node* node) = 0;

   /**
    * Cleans up this Module just prior to its unloading.
    *
    * @param node the Node.
    */
   virtual void cleanup(Node* node) = 0;

   /**
    * Gets the API for this BitmunkModule.
    *
    * @param node the Node.
    *
    * @return the API for this BitmunkModule.
    */
   virtual monarch::kernel::MicroKernelModuleApi* getApi(Node* node) = 0;
};

} // end namespace node
} // end namespace bitmunk
#endif

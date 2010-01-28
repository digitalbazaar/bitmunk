/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_node_NodeModule_H
#define bitmunk_node_NodeModule_H

#include "bitmunk/node/Node.h"
#include "monarch/kernel/MicroKernelModule.h"
#include "monarch/logging/Logging.h"

// module logging category
extern monarch::logging::Category* BM_NODE_CAT;

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
class NodeModule : public monarch::kernel::MicroKernelModule
{
protected:
   /**
    * A reference to the Node this module has been initialized with.
    */
   bitmunk::node::Node* mNode;

public:
   /**
    * Creates a new NodeModule with the specified name and version.
    *
    * @param name the name for this NodeModule.
    * @param version the version for this NodeModule (major.minor).
    */
   NodeModule(const char* name, const char* version);

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
    * Performs whatever configuration (or other) validation is necessary
    * before initialize(MicroKernel*) is called on this Module. This will be
    * called before initialize(MicroKernel*) is called on any of this Module's
    * dependencies.
    *
    * The default method performs no validation.
    *
    * @param k the MicroKernel.
    *
    * @return true if validated, false if an Exception occurred.
    */
   virtual bool validate(monarch::kernel::MicroKernel* k);

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
    * Gets the API for this NodeModule.
    *
    * @param node the Node.
    *
    * @return the API for this NodeModule.
    */
   virtual monarch::kernel::MicroKernelModuleApi* getApi(Node* node) = 0;
};

} // end namespace node
} // end namespace bitmunk
#endif

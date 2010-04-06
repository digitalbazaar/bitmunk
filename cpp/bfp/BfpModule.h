/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_bfp_BfpModule_H
#define bitmunk_bfp_BfpModule_H

#include "bitmunk/nodemodule/NodeModule.h"
#include "bitmunk/bfp/IBfpModule.h"

// module logging category
extern monarch::logging::Category* BM_BFP_CAT;

namespace bitmunk
{
namespace bfp
{

/**
 * A BfpModule is a NodeModule that provides an interface for creating
 * and freeing Bfp objects. A Bfp object is used to assign IDs for Wares,
 * FileInfos, and FilePieces and for watermarking, encrypting, and decrypting
 * data transmitted on Bitmunk.
 *
 * @author Dave Longley
 */
class BfpModule : public bitmunk::node::NodeModule
{
protected:
   /**
    * Stores the interface instance.
    */
   IBfpModule* mInterface;

public:
   /**
    * Creates a new BfpModule.
    */
   BfpModule();

   /**
    * Destructs this BfpModule.
    */
   virtual ~BfpModule();

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

} // end namespace bfp
} // end namespace bitmunk
#endif

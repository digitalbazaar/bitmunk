/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_sell_SellModule_H
#define bitmunk_sell_SellModule_H

#include "bitmunk/nodemodule/NodeModule.h"

// module logging category
extern monarch::logging::Category* BM_SELL_CAT;

namespace bitmunk
{
namespace sell
{

/**
 * A SellModule is a NodeModule that provides an interface for
 * selling digital content on Bitmunk.
 *
 * @author Dave Longley
 */
class SellModule : public bitmunk::node::NodeModule
{
public:
   /**
    * Creates a new SellModule.
    */
   SellModule();

   /**
    * Destructs this SellModule.
    */
   virtual ~SellModule();

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

} // end namespace sell
} // end namespace bitmunk
#endif

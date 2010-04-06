/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_negotiate_NegotiateModule_H
#define bitmunk_negotiate_NegotiateModule_H

#include "bitmunk/negotiate/INegotiateModule.h"
#include "bitmunk/nodemodule/NodeModule.h"

namespace bitmunk
{
namespace negotiate
{

/**
 * A NegotiateModule is a NodeModule that provides an implementation for
 * negotiating a ContractSection.
 *
 * @author Dave Longley
 */
class NegotiateModule : public bitmunk::node::NodeModule
{
protected:
   /**
    * The interface for this module.
    */
   INegotiateModule* mInterface;

public:
   /**
    * Creates a new NegotiateModule.
    */
   NegotiateModule();

   /**
    * Destructs this NegotiateModule.
    */
   virtual ~NegotiateModule();

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

} // end namespace negotiate
} // end namespace bitmunk
#endif

/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_system_SystemModule_H
#define bitmunk_system_SystemModule_H

#include "bitmunk/node/BitmunkModule.h"

// module logging category
extern monarch::logging::Category* BM_SYSTEM_CAT;

namespace bitmunk
{
namespace system
{

/**
 * A SystemModule is a NodeModule that provides a web interface for
 * various system services.
 *
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 */
class SystemModule : public bitmunk::node::BitmunkModule
{
public:
   /**
    * Creates a new SystemModule.
    */
   SystemModule();

   /**
    * Destructs this SystemModule.
    */
   virtual ~SystemModule();

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

} // end namespace system
} // end namespace bitmunk
#endif

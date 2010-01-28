/*
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_peruserdb_PerUserDBModule_H
#define bitmunk_peruserdb_PerUserDBModule_H

#include "bitmunk/node/NodeModule.h"
#include "bitmunk/peruserdb/DatabaseHub.h"

// module logging category
extern monarch::logging::Category* BM_PERUSERDB_CAT;

namespace bitmunk
{
namespace peruserdb
{

/**
 * An PerUserDBModule is a NodeModule that provides an interface for
 * manipulating per-user databases, that is, databases that exist on a
 * per-user basis.
 * 
 * @author Dave Longley
 */
class PerUserDBModule : public bitmunk::node::NodeModule
{
protected:
   /**
    * The database hub.
    */
   DatabaseHub* mHub;

public:
   /**
    * Creates a new PerUserDBModule.
    */
   PerUserDBModule();
   
   /**
    * Destructs this PerUserDBModule.
    */
   virtual ~PerUserDBModule();
   
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

} // end namespace peruserdb
} // end namespace bitmunk
#endif

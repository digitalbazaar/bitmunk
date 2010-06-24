/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_customcatalog_CustomCatalogModule_H
#define bitmunk_customcatalog_CustomCatalogModule_H

#include "bitmunk/customcatalog/CustomCatalog.h"
#include "bitmunk/node/BitmunkModule.h"

// module logging category
extern monarch::logging::Category* BM_CUSTOMCATALOG_CAT;

namespace bitmunk
{
namespace customcatalog
{

/**
 * A CustomCatalogModule is a NodeModule that provides an interface for
 * a media catalog.
 *
 * @author Dave Longley
 */
class CustomCatalogModule : public bitmunk::node::BitmunkModule
{
protected:
   /**
    * The Catalog of Wares.
    */
   CustomCatalog* mCatalog;

public:
   /**
    * Creates a new CustomCatalogModule.
    */
   CustomCatalogModule();

   /**
    * Destructs this CustomCatalogModule.
    */
   virtual ~CustomCatalogModule();

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

} // end namespace customcatalog
} // end namespace bitmunk
#endif

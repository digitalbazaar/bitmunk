/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_purchase_PurchaseModule_H
#define bitmunk_purchase_PurchaseModule_H

#include "bitmunk/node/BitmunkModule.h"
#include "bitmunk/purchase/DownloadThrottlerMap.h"
#include "bitmunk/purchase/IPurchaseModule.h"
#include "bitmunk/purchase/PurchaseDatabase.h"

// module logging category
extern monarch::logging::Category* BM_PURCHASE_CAT;

namespace bitmunk
{
namespace purchase
{

/**
 * A PurchaseModule is a NodeModule that provides an interface for
 * purchasing digital content on Bitmunk.
 *
 * @author Dave Longley
 */
class PurchaseModule : public bitmunk::node::BitmunkModule
{
protected:
   /**
    * The PurchaseDatabase.
    */
   PurchaseDatabase* mDatabase;

   /**
    * A map of download throttlers for users.
    */
   DownloadThrottlerMap* mDownloadThrottlerMap;

   /**
    * The interface instance.
    */
   IPurchaseModule* mInterface;

public:
   /**
    * Creates a new PurchaseModule.
    */
   PurchaseModule();

   /**
    * Destructs this PurchaseModule.
    */
   virtual ~PurchaseModule();

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

} // end namespace purchase
} // end namespace bitmunk
#endif

/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_purchase_DownloadStateInitializer_H
#define bitmunk_purchase_DownloadStateInitializer_H

#include "bitmunk/purchase/DownloadStateFiber.h"
#include "bitmunk/node/NodeFiber.h"

namespace bitmunk
{
namespace purchase
{

/**
 * The DownloadStateInitializer fiber is in charge of initializing a
 * download state.
 *
 * At present, the only step required to initialize a download state is
 * to initialize its seller pools.
 *
 * FIXME: In the future, we will need support for requesting the ware
 * details from a particular seller's catalog for non-custom created wares
 * from the website.
 *
 * @author Dave Longley
 */
class DownloadStateInitializer :
public bitmunk::node::NodeFiber,
public DownloadStateFiber
{
public:
   /**
    * Creates a new DownloadStateInitializer.
    *
    * @param node the Node associated with this fiber.
    */
   DownloadStateInitializer(bitmunk::node::Node* node);

   /**
    * Destructs this DownloadStateInitializer.
    */
   virtual ~DownloadStateInitializer();

protected:
   /**
    * Processes messages, retrieved via getMessages(), and performs whatever
    * custom work is necessary.
    */
   virtual void processMessages();

   /**
    * Runs the SellerPoolUpdater fiber to initialize the seller pools.
    */
   virtual void runSellerPoolUpdater();
};

} // end namespace purchase
} // end namespace bitmunk
#endif

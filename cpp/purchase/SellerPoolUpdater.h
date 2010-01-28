/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_purchase_SellerPoolUpdater_H
#define bitmunk_purchase_SellerPoolUpdater_H

#include "bitmunk/purchase/DownloadStateFiber.h"
#include "bitmunk/node/NodeFiber.h"
#include "monarch/crypto/BigDecimal.h"

namespace bitmunk
{
namespace purchase
{

/**
 * A SellerPoolUpdater updates one or more SellerPools. Once the updates
 * have completed, it reports the results via an event and via a fiber message
 * to its parent, if it has one.
 *
 * @author Dave Longley
 */
class SellerPoolUpdater :
public bitmunk::node::NodeFiber,
public DownloadStateFiber
{
protected:
   /**
    * The stack containing the next seller pool to be updated.
    */
   std::list<monarch::rt::DynamicObject> mNextStack;

   /**
    * The current seller pool being updated.
    */
   bitmunk::common::SellerPool mCurrent;

   /**
    * A list of all of the updated seller pools.
    */
   bitmunk::common::SellerPoolList mUpdatedSellerPools;

   /**
    * True if this fiber should initialize FileProgresses in its associated
    * download state, false if not.
    */
   bool mInitFileProgress;

public:
   /**
    * Creates a new SellerPoolUpdater.
    *
    * @param node the Node associated with this fiber.
    * @param parentId the ID of the parent fiber.
    */
   SellerPoolUpdater(
      bitmunk::node::Node* node, monarch::fiber::FiberId parentId);

   /**
    * Destructs this SellerPoolUpdater.
    */
   virtual ~SellerPoolUpdater();

   /**
    * Sets the SellerPools to be updated. This must be called before starting
    * the fiber.
    *
    * @param pools a list of SellerPools to update.
    */
   virtual void setSellerPools(bitmunk::common::SellerPoolList& pools);

   /**
    * Sets whether or not this fiber should write out the piece totals and
    * create FileProgresses after it finishes updating SellerPools. This must
    * be called before starting the fiber.
    *
    * @param yes true to initialize piece totals and FileProgresses, false
    *            not to.
    */
   virtual void initializeFileProgress(bool yes);

   /**
    * Retrieves a seller pool.
    */
   virtual void fetchSellerPool();

protected:
   /**
    * Processes messages, retrieved via getMessages(), and performs whatever
    * custom work is necessary.
    */
   virtual void processMessages();

   /**
    * Sends an event about seller pool updating.
    *
    * @param error true if there was an error, false if not.
    */
   virtual void sendEvent(bool error);

   /**
    * Initializes FileProgresses for the associated download state.
    *
    * @return true if successful, false if not.
    */
   virtual bool initFileProgress();
};

} // end namespace purchase
} // end namespace bitmunk
#endif

/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_purchase_DownloadStateFiber_H
#define bitmunk_purchase_DownloadStateFiber_H

#include "bitmunk/purchase/PurchaseDatabase.h"
#include "bitmunk/purchase/TypeDefinitions.h"
#include "bitmunk/node/Node.h"
#include "monarch/event/Event.h"
#include "monarch/util/RateAverager.h"

namespace bitmunk
{
namespace purchase
{

/**
 * A DownloadStateFiber is a base class for fibers that manipulate a
 * DownloadState.
 *
 * @author Dave Longley
 */
class DownloadStateFiber
{
protected:
   /**
    * The node associated with this fiber.
    */
   bitmunk::node::Node* mBitmunkNode;

   /**
    * The name of this download state fiber.
    */
   char* mName;

   /**
    * Stores exit data to include in the event when this fiber exits.
    */
   monarch::rt::DynamicObject* mExitData;

   /**
    * A reference to the purchase database.
    */
   PurchaseDatabase* mPurchaseDatabase;

   /**
    * The DownloadState associated with this fiber.
    */
   DownloadState mDownloadState;

   /**
    * The purchase module's shared download rate averager.
    */
   monarch::util::RateAverager* mTotalDownloadRate;

   /**
    * The user's shared download throttler.
    */
   monarch::net::BandwidthThrottler* mDownloadThrottler;

public:
   /**
    * Creates a new DownloadStateFiber.
    *
    * @param node the associated bitmunk node.
    * @param name the name of this DownloadStateFiber.
    * @param exitData a pointer to the DynamicObject to store fiber exit
    *                 data in.
    */
   DownloadStateFiber(
      bitmunk::node::Node* node, const char* name,
      monarch::rt::DynamicObject* exitData);

   /**
    * Destructs this DownloadStateFiber.
    */
   virtual ~DownloadStateFiber();

   /**
    * Sets the DownloadState for this fiber to use. This should be called
    * before adding this fiber to the node's fiber scheduler.
    *
    * @param ds the DownloadState for this fiber to use.
    */
   virtual void setDownloadState(DownloadState& ds);

   /**
    * Gets the DownloadState for this fiber.
    *
    * @return the DownloadState for this fiber.
    */
   virtual DownloadState& getDownloadState();

   /**
    * Logs a message about a download state.
    *
    * @param msg the message to log.
    */
   virtual void logDownloadStateMessage(const char* msg);

   /**
    * Logs an exception about a download state.
    *
    * @param ex the exception to log.
    */
   virtual void logException(monarch::rt::ExceptionRef ex);

protected:
   /**
    * Send a download state event.
    *
    * This fiber's DownloadStateId and the DownloadState's user ID will be
    * included in the event's details.
    *
    * @param e the event to send.
    */
   virtual void sendDownloadStateEvent(monarch::event::Event& e);

   /**
    * Sends an error event using the last exception on the current thread.
    */
   virtual void sendErrorEvent();
};

} // end namespace purchase
} // end namespace bitmunk
#endif

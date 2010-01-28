/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_common_NodeMonitor_H
#define bitmunk_common_NodeMonitor_H

#include "bitmunk/common/SyncStateMonitor.h"
#include "monarch/rt/SharedLock.h"

namespace bitmunk
{
namespace node
{

/**
 * An asyncronous StateMonitor implementation for Nodes.
 *
 * This implementation allows state updates to be done asyncronously.
 * Update methods will queue their operations and return immediately.
 * This allows for lower latency updates if contention is an issue.
 * When getting values the current pending updates can be processed
 * first, potentially adding to latency, or pending updates can be
 * ignored giving a snapshot of the monitor state from some point in
 * the past.
 *
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 */
class NodeMonitor : public bitmunk::common::StateMonitor
{
protected:
   /**
    * Syncronous state.
    */
   bitmunk::common::SyncStateMonitor mState;

   /**
    * Currently queue for updates.
    */
   monarch::rt::DynamicObject mUpdateQueue;

   /**
    * Next queue for updates.
    */
   monarch::rt::DynamicObject mNextUpdateQueue;

public:
   /**
    * {@inheritDoc}
    */
   NodeMonitor();

   /**
    * {@inheritDoc}
    */
   virtual ~NodeMonitor();

   /**
    * {@inheritDoc}
    */
   virtual bool addStates(monarch::rt::DynamicObject& stateInfos);

   /**
    * {@inheritDoc}
    */
   virtual void removeAll();

   /**
    * {@inheritDoc}
    */
   virtual bool removeStates(monarch::rt::DynamicObject& states);

   /**
    * {@inheritDoc}
    */
   virtual void resetAll();

   /**
    * {@inheritDoc}
    */
   virtual bool resetStates(monarch::rt::DynamicObject& states);

   /**
    * {@inheritDoc}
    */
   virtual bool setStates(monarch::rt::DynamicObject& states, bool clone = false);

   /**
    * {@inheritDoc}
    */
   virtual monarch::rt::DynamicObject getAll();

   /**
    * {@inheritDoc}
    */
   virtual bool getStates(monarch::rt::DynamicObject& states);

   /**
    * {@inheritDoc}
    */
   virtual bool adjustStates(
      monarch::rt::DynamicObject& adjustments, bool clone = false);
};

} // end namespace node
} // end namespace bitmunk
#endif

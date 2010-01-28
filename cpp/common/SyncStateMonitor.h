/*
 * Copyright (c) 2009 Digital Bazaar, Inc.  All rights reserved.
 */
#ifndef bitmunk_common_SyncStateMonitor_H
#define bitmunk_common_SyncStateMonitor_H

#include "bitmunk/common/StateMonitor.h"
#include "monarch/rt/SharedLock.h"

namespace bitmunk
{
namespace common
{

/**
 * A syncronous StateMonitor implementation.
 * 
 * This implementation uses a single lock for all reads and writes to a single
 * DynamicObject for state storage.  
 * 
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 */
class SyncStateMonitor : public StateMonitor
{
protected:
   /**
    * State info. Map from state name to map of info and values.
    */
   monarch::rt::DynamicObject mStates;
   
   /**
    * Shared lock for all state reads and writes.
    */
   monarch::rt::SharedLock mLock;
   
public:
   /**
    * {@inheritDoc}
    */
   SyncStateMonitor();

   /**
    * {@inheritDoc}
    */
   virtual ~SyncStateMonitor();

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

} // end namespace common
} // end namespace bitmunk
#endif

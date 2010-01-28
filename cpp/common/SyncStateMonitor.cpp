/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/common/SyncStateMonitor.h"

#include "monarch/rt/DynamicObjectIterator.h"

using namespace monarch::rt;
using namespace bitmunk::common;

/*

FIXME: analyize/profile and optimize if needed

Higher level optimization if state monitoring not needed:
- Other modules check for stats interface on init.
- If present, can post stats messages.
- Perhaps a "null" monitor implementation to make clients easier to write at a
  cost of un-needed method calls.
- Perhaps both check if monitoring, then get interface.

Possible implementation optimization technique:
- Keep >=2 queues of state update messages.
- Stats module processes one queue while other has active writes.
- Swap queues between processing.
- Only process queues when overflow or periodic timer fires.

Messages can be dynos.  Use format which allows partial or full dyno reuse to
avoid mem allocs.  Use "type" field for always constant values.  Simple count:
{
  "type": {
    "id": "bitmunk.foo.bar.baz",
    "action": COUNT
  }
}
Adding values can share the type part between messages but has per-msg field:
{
  "type": {
    "id": "bitmunk.foo.bar.bif",
    "action": ADD
  }
  "value": 1234
}

*/

SyncStateMonitor::SyncStateMonitor()
{
   mStates->setType(Map);
}

SyncStateMonitor::~SyncStateMonitor()
{
}

bool SyncStateMonitor::addStates(monarch::rt::DynamicObject& stateInfos)
{
   bool rval = true;
   mLock.lockExclusive();
   {
      // check all valid
      DynamicObjectIterator i = stateInfos.getIterator();
      while(rval && i->hasNext())
      {
         DynamicObject& next = i->next();
         const char* state = i->getName();
         if(mStates->hasMember(state))
         {
            ExceptionRef e = new Exception(
               "Attempt to add duplicate StateMonitor state.",
               "bitmunk.common.DuplicateMonitorState");
            e->getDetails()["state"] = state;
            Exception::set(e);
            rval = false;
         }
         if(rval && !next->hasMember("init"))
         {
            ExceptionRef e = new Exception(
               "StateInfo does not have \"init\" value.",
               "bitmunk.common.BadMonitorStateInfo");
            e->getDetails()["state"] = state;
            Exception::set(e);
            rval = false;
         }
      }
      if(rval)
      {
         // all valid, process group
         DynamicObjectIterator i = stateInfos.getIterator();
         while(i->hasNext())
         {
            DynamicObject& stateInfo = i->next();
            const char* state = i->getName();
            mStates[state]["info"] = stateInfo;
         }
         resetStates(stateInfos);
      }
   }
   mLock.unlockExclusive();
   return rval;
}

void SyncStateMonitor::removeAll()
{
   mLock.lockExclusive();
   {
      // custom optimization
      mStates->clear();
   }
   mLock.unlockExclusive();
}

/**
 * Check if state is present.
 * 
 * @param state the state name
 * @param states the state storage
 * @param action the action name
 * 
 * @return true on success, false if an exception occured
 */
static bool _checkState(
   const char* state, DynamicObject& states, const char* action)
{
   bool rval = states->hasMember(state);
   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Attempt to access unknown StateMonitor state.",
         "bitmunk.common.UnknownMonitorState");
      e->getDetails()["state"] = state;
      e->getDetails()["action"] = action;
      Exception::set(e);
   }
   return rval;
}

bool SyncStateMonitor::removeStates(DynamicObject& states)
{
   bool rval = true;
   mLock.lockExclusive();
   {
      // check all valid
      DynamicObjectIterator i = states.getIterator();
      while(rval && i->hasNext())
      {
         i->next();
         rval = _checkState(i->getName(), mStates, "remove");
      }
      if(rval)
      {
         // all valid, process group
         DynamicObjectIterator i = states.getIterator();
         while(i->hasNext())
         {
            i->next();
            mStates->removeMember(i->getName());
         }
      }
   }
   mLock.unlockExclusive();
   return rval;
}

void SyncStateMonitor::resetAll()
{
   mLock.lockExclusive();
   {
      DynamicObjectIterator i = mStates.getIterator();
      while(i->hasNext())
      {
         DynamicObject& next = i->next();
         next["value"] = next["info"]["init"].clone();
      }
   }
   mLock.unlockExclusive();
}

bool SyncStateMonitor::resetStates(DynamicObject& states)
{
   bool rval = true;
   mLock.lockExclusive();
   {
      // check all valid
      DynamicObjectIterator i = states.getIterator();
      while(rval && i->hasNext())
      {
         i->next();
         rval = _checkState(i->getName(), mStates, "reset");
      }
      if(rval)
      {
         // all valid, process group
         DynamicObjectIterator i = states.getIterator();
         while(i->hasNext())
         {
            i->next();
            DynamicObject& s = mStates[i->getName()];
            s["value"] = s["info"]["init"].clone();
         }
      }
   }
   mLock.unlockExclusive();
   return rval;
}

bool SyncStateMonitor::setStates(DynamicObject& states, bool clone)
{
   bool rval = true;
   mLock.lockExclusive();
   {
      // skip state name check
      // FIXME: check types
      if(rval)
      {
         DynamicObjectIterator i = states.getIterator();
         while(rval && i->hasNext())
         {
            DynamicObject& v = i->next();
            mStates[i->getName()]["value"] = (clone ? v.clone() : v);
         }
      }
   }
   mLock.unlockExclusive();
   return rval;
}

DynamicObject SyncStateMonitor::getAll()
{
   DynamicObject rval;
   rval->setType(Map);
   mLock.lockShared();
   {
      DynamicObjectIterator i = mStates.getIterator();
      while(i->hasNext())
      {
         DynamicObject& next = i->next();
         DynamicObject& v = next["value"];
         rval[i->getName()] = v.clone();
      }
   }
   mLock.unlockShared();
   return rval;
}

bool SyncStateMonitor::getStates(DynamicObject& states)
{
   bool rval = true;
   mLock.lockShared();
   {
      // check all valid
      DynamicObjectIterator i = states.getIterator();
      while(rval && i->hasNext())
      {
         i->next();
         rval = _checkState(i->getName(), mStates, "get");
      }
      if(rval)
      {
         // all valid, process group
         DynamicObjectIterator i = states.getIterator();
         while(rval && i->hasNext())
         {
            i->next();
            const char* state = i->getName();
            DynamicObject& v = mStates[state]["value"];
            states[state] = v.clone();
         }
      }
   }
   mLock.unlockShared();
   return rval;
}

bool SyncStateMonitor::adjustStates(DynamicObject& adjustments, bool clone)
{
   bool rval = true;
   mLock.lockExclusive();
   {
      // check all valid
      DynamicObjectIterator i = adjustments.getIterator();
      while(rval && i->hasNext())
      {
         i->next();
         rval = _checkState(i->getName(), mStates, "adjust");
      }
      /*
      if(rval && (flags & CHECK_TYPES))
      {
         // check all valid
         DynamicObjectIterator i = adjustments.getIterator();
         while(rval && i->hasNext())
         {
            DynamicObject& adj = i->next();
            const char* state = i->getName();
            DynamicObject& v = mStates[state]["value"];
            switch(v->getType())
            {
               case Int64:
                  switch(adj->getType())
                  {
                     case Int64:
                     case UInt64:
                        break;
                     default:
                        ExceptionRef e = new Exception(
                           "Bad StateMonitor adjustment types.",
                           "bitmunk.common.BadAdjustmentTypes");
                        e->getDetails()["state"] = state;
                        e->getDetails()["valueType"] =
                           DynamicObject::descriptionForType(v->getType());
                        e->getDetails()["adjustmentType"] =
                           DynamicObject::descriptionForType(adj->getType());
                        Exception::set(e);
                        rval = false;
                        break;
                  }
                  break;
               case UInt64:
                  switch(adj->getType())
                  {
                     case Int64:
                     case UInt64:
                        break;
                     default:
                        ExceptionRef e = new Exception(
                           "Bad StateMonitor adjustment types.",
                           "bitmunk.common.BadAdjustmentTypes");
                        e->getDetails()["state"] = state;
                        e->getDetails()["valueType"] =
                           DynamicObject::descriptionForType(v->getType());
                        e->getDetails()["adjustmentType"] =
                           DynamicObject::descriptionForType(adj->getType());
                        Exception::set(e);
                        rval = false;
                        break;
                  }
                  break;
               case Double:
                  switch(adj->getType())
                  {
                     case Int64:
                     case UInt64:
                     case Double:
                        break;
                     default:
                        ExceptionRef e = new Exception(
                           "Bad StateMonitor adjustment types.",
                           "bitmunk.common.BadAdjustmentTypes");
                        e->getDetails()["state"] = state;
                        e->getDetails()["valueType"] =
                           DynamicObject::descriptionForType(v->getType());
                        e->getDetails()["adjustmentType"] =
                           DynamicObject::descriptionForType(adj->getType());
                        Exception::set(e);
                        rval = false;
                        break;
                  }
                  break;
               default:
                  break;
            }
         }
      }
      */
      // FIXME: check overflow/underflow
      if(rval)
      {
         // all valid, process group
         DynamicObjectIterator i = adjustments.getIterator();
         while(rval && i->hasNext())
         {
            DynamicObject& adj = i->next();
            const char* state = i->getName();
            DynamicObject& v = mStates[state]["value"];
            switch(v->getType())
            {
               case Int64:
                  switch(adj->getType())
                  {
                     case Int64:
                        v = v->getInt64() + adj->getInt64();
                        break;
                     case UInt64:
                        v = v->getInt64() + adj->getUInt64();
                        break;
                     default:
                        // type exceptions handled above
                        break;
                  }
                  break;
               case UInt64:
                  switch(adj->getType())
                  {
                     case Int64:
                        v = v->getUInt64() + adj->getInt64();
                        break;
                     case UInt64:
                        v = v->getUInt64() + adj->getUInt64();
                        break;
                     default:
                        // type exceptions handled above
                        break;
                  }
                  break;
               case Double:
                  switch(adj->getType())
                  {
                     case Int64:
                        v = v->getDouble() + adj->getInt64();
                        break;
                     case UInt64:
                        v = v->getDouble() + adj->getUInt64();
                        break;
                     case Double:
                        v = v->getDouble() + adj->getDouble();
                        break;
                     default:
                        // type exceptions handled above
                        break;
                  }
                  break;
               default:
                  mStates[i->getName()]["value"] = (clone ? v.clone() : v);
            }
         }
      }
   }
   mLock.unlockExclusive();
   return rval;
}

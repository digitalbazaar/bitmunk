/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/common/StateMonitor.h"

using namespace monarch::rt;
using namespace bitmunk::common;

StateMonitor::StateMonitor()
{
}

StateMonitor::~StateMonitor()
{
}

bool StateMonitor::addState(
   const char* state, monarch::rt::DynamicObject& stateInfo)
{
   DynamicObject stateInfos;
   stateInfos[state] = stateInfo;
   return addStates(stateInfos);
}

void StateMonitor::removeAll()
{
   DynamicObject all = getAll();
   removeStates(all);
}

bool StateMonitor::removeState(const char* state)
{
   DynamicObject states;
   states[state].setNull();
   return removeStates(states);
}

bool StateMonitor::resetState(const char* state)
{
   DynamicObject states;
   states[state].setNull();
   return resetStates(states);
}

bool StateMonitor::setState(const char* state, DynamicObject& value, bool clone)
{
   DynamicObject states;
   states[state] = value;
   return setStates(states, clone);
}

bool StateMonitor::setUInt64State(const char* state, uint64_t value)
{
   DynamicObject states;
   states[state] = value;
   return setStates(states);
}

bool StateMonitor::getState(const char* state, DynamicObject& target)
{
   bool rval;
   DynamicObject states;
   states[state]->setType(target->getType());
   rval = getStates(states);
   if(rval)
   {
      target = states[state];
   }
   return rval;
}

bool StateMonitor::getUInt64State(
   const char* state, uint64_t& target)
{
   bool rval;
   DynamicObject tmp;
   tmp->setType(UInt64);
   rval = getState(state, tmp);
   if(rval)
   {
      target = tmp->getUInt64();
   }
   return rval;
}

bool StateMonitor::adjustInt64State(
   const char* state, int64_t adjustment)
{
   DynamicObject states;
   states[state] = adjustment;
   return adjustStates(states);
}

bool StateMonitor::adjustUInt64State(
   const char* state, uint64_t adjustment)
{
   DynamicObject states;
   states[state] = adjustment;
   return adjustStates(states);
}

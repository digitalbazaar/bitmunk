/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/node/NodeMonitor.h"

#include "monarch/rt/DynamicObjectIterator.h"

using namespace monarch::rt;
using namespace bitmunk::common;
using namespace bitmunk::node;

/*
FIXME: make this asyncronous and use the queues.
FIXME: just proxy to a sync state monitor for now.
*/

NodeMonitor::NodeMonitor()
{
   mUpdateQueue->setType(Array);
   mNextUpdateQueue->setType(Array);
}

NodeMonitor::~NodeMonitor()
{
}

bool NodeMonitor::addStates(monarch::rt::DynamicObject& stateInfos)
{
   return mState.addStates(stateInfos);
}

void NodeMonitor::removeAll()
{
   mState.removeAll();
}

bool NodeMonitor::removeStates(DynamicObject& states)
{
   return mState.removeStates(states);
}

void NodeMonitor::resetAll()
{
   mState.resetAll();
}

bool NodeMonitor::resetStates(DynamicObject& states)
{
   return mState.resetStates(states);
}

bool NodeMonitor::setStates(DynamicObject& states, bool clone)
{
   return mState.setStates(states, clone);
}

DynamicObject NodeMonitor::getAll()
{
   return mState.getAll();
}

bool NodeMonitor::getStates(DynamicObject& states)
{
   return mState.getStates(states);
}

bool NodeMonitor::adjustStates(DynamicObject& adjustments, bool clone)
{
   return mState.adjustStates(adjustments, clone);
}

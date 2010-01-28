/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/eventreactor/EventReactorModule.h"

#include "bitmunk/eventreactor/EventReactorService.h"
#include "bitmunk/eventreactor/IEventReactorModule.h"
#include "bitmunk/node/BtpServer.h"

using namespace monarch::config;
using namespace monarch::kernel;
using namespace monarch::logging;
using namespace monarch::modest;
using namespace monarch::rt;
using namespace bitmunk::node;
using namespace bitmunk::protocol;
using namespace bitmunk::eventreactor;

// Logging category initialized during module initialization.
Category* BM_EVENTREACTOR_CAT;

EventReactorModule::EventReactorModule() :
   NodeModule("bitmunk.eventreactor.EventReactor", "1.0"),
   mInterface(NULL)
{
}

EventReactorModule::~EventReactorModule()
{
}

void EventReactorModule::addDependencyInfo(DynamicObject& depInfo)
{
   // no dependencies
}

bool EventReactorModule::initialize(Node* node)
{
   bool rval = true;

   BM_EVENTREACTOR_CAT = new Category(
      "BM_EVENTREACTOR",
      "Bitmunk Event Reactor",
      NULL);
   
   // create and event reactor service
   EventReactorService* ers =
      new EventReactorService(node, "/api/3.0/eventreactor");
   mInterface = ers;
   BtpServiceRef bs = ers;
   rval = node->getBtpServer()->addService(bs, Node::SslOn);
   
   return rval;
}

void EventReactorModule::cleanup(Node* node)
{
   // remove services
   node->getBtpServer()->removeService("/api/3.0/eventreactor");
   
   // interface now null
   mInterface = NULL;

   delete BM_EVENTREACTOR_CAT;
   BM_EVENTREACTOR_CAT = NULL;
}

MicroKernelModuleApi* EventReactorModule::getApi(Node* node)
{
   // return event reactor service
   return mInterface;
}

Module* createModestModule()
{
   return new EventReactorModule();
}

void freeModestModule(Module* m)
{
   delete m;
}

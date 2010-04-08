/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/system/SystemModule.h"

#include "bitmunk/node/BtpServer.h"
#include "bitmunk/system/ConfigService.h"
#include "bitmunk/system/ControlService.h"
#include "bitmunk/system/DirectiveService.h"
#include "bitmunk/system/EventService.h"
#include "bitmunk/system/StatisticsService.h"
#include "bitmunk/system/TestService.h"
#include "bitmunk/system/VersionService.h"

using namespace monarch::kernel;
using namespace monarch::logging;
using namespace monarch::modest;
using namespace monarch::rt;
using namespace bitmunk::node;
using namespace bitmunk::protocol;
using namespace bitmunk::system;

// Logging category initialized during module initialization.
Category* BM_SYSTEM_CAT;

SystemModule::SystemModule() :
   BitmunkModule("bitmunk.system.System", "1.0")
{
}

SystemModule::~SystemModule()
{
}

void SystemModule::addDependencyInfo(DynamicObject& depInfo)
{
   // no dependencies
}

bool SystemModule::initialize(Node* node)
{
   bool rval = true;

   BM_SYSTEM_CAT = new Category(
      "BM_SYSTEM",
      "Bitmunk System",
      NULL);

   BtpServiceRef bs;

   if(rval)
   {
      // create and add config server
      bs = new ConfigService(node, "/api/3.0/system/config");
      rval = node->getBtpServer()->addService(bs, Node::SslOn);
   }

   if(rval)
   {
      // create and add control service
      bs = new ControlService(node, "/api/3.0/system/control");
      rval = node->getBtpServer()->addService(bs, Node::SslOn);
   }

   if(rval)
   {
      // add events service
      bs = new EventService(node, "/api/3.0/system/events");
      rval = node->getBtpServer()->addService(bs, Node::SslOn);
   }

   if(rval)
   {
      // add directive service
      bs = new DirectiveService(node, "/api/3.0/system/directives");
      rval = node->getBtpServer()->addService(bs, Node::SslOn);
   }

   if(rval)
   {
      // create and add statistics service (SSL only)
      bs = new StatisticsService(node, "/api/3.0/system/statistics");
      rval = node->getBtpServer()->addService(bs, Node::SslOn);
   }

   if(rval)
   {
      // create and add test service
      bs = new TestService(node, "/api/3.0/system/test");
      bs->setAllowHttp1(true);
      rval = node->getBtpServer()->addService(bs, Node::SslAny);
   }

   if(rval)
   {
      // create and add version service
      bs = new VersionService(node, "/api/3.0/system/version");
      rval = node->getBtpServer()->addService(bs, Node::SslOn);
   }

   return rval;
}

void SystemModule::cleanup(Node* node)
{
   // remove services
   node->getBtpServer()->removeService("/api/3.0/system/config");
   node->getBtpServer()->removeService("/api/3.0/system/control");
   node->getBtpServer()->removeService("/api/3.0/system/events");
   node->getBtpServer()->removeService("/api/3.0/system/directives");
   node->getBtpServer()->removeService("/api/3.0/system/statistics");
   node->getBtpServer()->removeService("/api/3.0/system/test");
   node->getBtpServer()->removeService("/api/3.0/system/version");

   delete BM_SYSTEM_CAT;
   BM_SYSTEM_CAT = NULL;
}

MicroKernelModuleApi* SystemModule::getApi(Node* node)
{
   // no API
   return NULL;
}

Module* createModestModule()
{
   return new SystemModule();
}

void freeModestModule(Module* m)
{
   delete m;
}

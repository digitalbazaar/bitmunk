/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/sell/SellModule.h"

#include "bitmunk/node/BtpServer.h"
#include "bitmunk/sell/ContractService.h"
#include "bitmunk/sell/SampleService.h"

using namespace monarch::kernel;
using namespace monarch::logging;
using namespace monarch::modest;
using namespace monarch::rt;
using namespace bitmunk::node;
using namespace bitmunk::protocol;
using namespace bitmunk::sell;

// Logging category initialized during module initialization.
Category* BM_SELL_CAT;

SellModule::SellModule() :
   BitmunkModule("bitmunk.sell.Sell", "1.0")
{
}

SellModule::~SellModule()
{
}

void SellModule::addDependencyInfo(DynamicObject& depInfo)
{
   // set dependencies:

   // requires the bfp module, version 1.0
   {
      DynamicObject dep;
      dep["name"] = "bitmunk.bfp.Bfp";
      dep["version"] = "1.0";
      depInfo["dependencies"]->append(dep);
   }

   // requires any catalog module
   {
      DynamicObject dep;
      dep["type"] = "bitmunk.catalog";
      depInfo["dependencies"]->append(dep);
   }

   // requires any negotiate module
   {
      DynamicObject dep;
      dep["type"] = "bitmunk.negotiate";
      depInfo["dependencies"]->append(dep);
   }
}

bool SellModule::initialize(Node* node)
{
   bool rval = true;

   BM_SELL_CAT = new Category(
      "BM_SELL",
      "Bitmunk Sell",
      NULL);

   BtpServiceRef bs;

   if(rval)
   {
      // create and add contract service
      bs = new ContractService(node, "/api/3.0/sales/contract");
      rval = node->getBtpServer()->addService(bs, Node::SslOn);
   }

   if(rval)
   {
      // create and add sample service
      bs = new SampleService(node, "/api/3.0/sales/samples");
      // allow HTTP/1.0 for cached samples
      bs->setAllowHttp1(true);
      rval = node->getBtpServer()->addService(bs, Node::SslOff);
   }

   return rval;
}

void SellModule::cleanup(Node* node)
{
   // remove services
   node->getBtpServer()->removeService("/api/3.0/sales/contract");
   node->getBtpServer()->removeService("/api/3.0/sales/samples");

   delete BM_SELL_CAT;
   BM_SELL_CAT = NULL;
}

MicroKernelModuleApi* SellModule::getApi(Node* node)
{
   // no API
   return NULL;
}

Module* createModestModule()
{
   return new SellModule();
}

void freeModestModule(Module* m)
{
   delete m;
}

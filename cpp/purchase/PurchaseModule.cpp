/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/purchase/PurchaseModule.h"

#include "bitmunk/node/BtpServer.h"
#include "bitmunk/purchase/ContractService.h"
#include "bitmunk/purchase/IPurchaseModuleImpl.h"

using namespace monarch::config;
using namespace monarch::kernel;
using namespace monarch::logging;
using namespace monarch::modest;
using namespace monarch::rt;
using namespace bitmunk::node;
using namespace bitmunk::protocol;
using namespace bitmunk::purchase;

// Logging category initialized during module initialization.
Category* BM_PURCHASE_CAT;

PurchaseModule::PurchaseModule() :
   NodeModule("bitmunk.purchase.Purchase", "1.0"),
   mDatabase(NULL),
   mDownloadThrottlerMap(NULL),
   mInterface(NULL)
{
}

PurchaseModule::~PurchaseModule()
{
}

void PurchaseModule::addDependencyInfo(DynamicObject& depInfo)
{
   // set dependencies:
   
   // requires the bfp module, version 1.0
   {
      DynamicObject dep;
      dep["name"] = "bitmunk.bfp.Bfp";
      dep["version"] = "1.0";
      depInfo["dependencies"]->append(dep);
   }
   
   // requires any negotiate module
   {
      DynamicObject dep;
      dep["type"] = "bitmunk.negotiate";
      depInfo["dependencies"]->append(dep);
   }
   
   // requires the peruserdb module, version 1.0
   {
      DynamicObject dep;
      dep["name"] = "bitmunk.peruserdb.PerUserDB";
      dep["version"] = "1.0";
      depInfo["dependencies"]->append(dep);
   }
}

bool PurchaseModule::initialize(Node* node)
{
   bool rval = false;

   BM_PURCHASE_CAT = new Category(
      "BM_PURCHASE",
      "Bitmunk Purchase",
      NULL);
   
   // initialize download throttler
   Config cfg = node->getConfigManager()->getModuleConfig(
      "bitmunk.purchase.Purchase");
   if(!cfg.isNull())
   {
      // create and initialize purchase database
      mDatabase = new PurchaseDatabase();
      rval = mDatabase->initialize(node);
      if(rval)
      {
         // create and initialize download throttler map
         mDownloadThrottlerMap = new DownloadThrottlerMap();
         rval = mDownloadThrottlerMap->initialize(node);
      }
      
      if(rval)
      {
         // create interface
         mInterface = new IPurchaseModuleImpl(mDatabase, mDownloadThrottlerMap);

         // create and add contract service
         BtpServiceRef bs = new ContractService(
            node, "/api/3.0/purchase/contracts");
         rval = node->getBtpServer()->addService(bs, Node::SslOn);
      }
      else
      {
         // clean up purchase database
         if(mDatabase != NULL)
         {
            delete mDatabase;
            mDatabase = NULL;
         }
         
         // clean up download throttler
         if(mDownloadThrottlerMap != NULL)
         {
            delete mDownloadThrottlerMap;
            mDownloadThrottlerMap = NULL;
         }
      }
   }
   
   return rval;
}

void PurchaseModule::cleanup(Node* node)
{
   // remove services
   node->getBtpServer()->removeService("/api/3.0/purchase/contracts");
   
   // clean up interface
   if(mInterface != NULL)
   {
      delete mInterface;
      mInterface = NULL;
   }

   // clean up purchase database
   if(mDatabase != NULL)
   {
      delete mDatabase;
      mDatabase = NULL;
   }
   
   // clean up download throttler
   if(mDownloadThrottlerMap != NULL)
   {
      delete mDownloadThrottlerMap;
      mDownloadThrottlerMap = NULL;
   }

   delete BM_PURCHASE_CAT;
   BM_PURCHASE_CAT = NULL;
}

MicroKernelModuleApi* PurchaseModule::getApi(Node* node)
{
   return mInterface;
}

Module* createModestModule()
{
   return new PurchaseModule();
}

void freeModestModule(Module* m)
{
   delete m;
}

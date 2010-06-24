/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/customcatalog/CustomCatalogModule.h"

#include "bitmunk/customcatalog/CatalogService.h"
#include "bitmunk/node/BtpServer.h"

using namespace monarch::kernel;
using namespace monarch::logging;
using namespace monarch::modest;
using namespace monarch::rt;
using namespace bitmunk::node;
using namespace bitmunk::protocol;
using namespace bitmunk::customcatalog;

// Logging category initialized during module initialization.
Category* BM_CUSTOMCATALOG_CAT;

CustomCatalogModule::CustomCatalogModule() :
   BitmunkModule("bitmunk.catalog.CustomCatalog", "1.0"),
   mCatalog(NULL)
{
}

CustomCatalogModule::~CustomCatalogModule()
{
}

void CustomCatalogModule::addDependencyInfo(DynamicObject& depInfo)
{
   // set dependencies:

   {
      // requires any medialibrary module, it does not need
      // a specific one, just one that implements the IMediaLibrary
      // interface, which it must if its type is "bitmunk.medialibrary"
      DynamicObject dep;
      dep["type"] = "bitmunk.medialibrary";
      depInfo["dependencies"]->append(dep);
   }
}

bool CustomCatalogModule::initialize(Node* node)
{
   bool rval;

   BM_CUSTOMCATALOG_CAT = new Category(
      "BM_CUSTOMCATALOG",
      "Bitmunk Custom Catalog",
      NULL);

   // create and attempt to initialize catalog
   mCatalog = new CustomCatalog();
   if((rval = mCatalog->init(node)))
   {
      BtpServiceRef bs;

      {
         // create and add catalog service
         bs = new CatalogService(node, mCatalog, "/api/3.0/catalog");
         rval = node->getBtpServer()->addService(bs, Node::SslAny);
      }
   }

   if(!rval)
   {
      delete mCatalog;
      mCatalog = NULL;
   }

   return rval;
}

void CustomCatalogModule::cleanup(Node* node)
{
   // remove services
   node->getBtpServer()->removeService("/api/3.0/catalog");

   // clean up catalog
   if(mCatalog != NULL)
   {
      mCatalog->cleanup(node);
      delete mCatalog;
   }

   delete BM_CUSTOMCATALOG_CAT;
   BM_CUSTOMCATALOG_CAT = NULL;
}

MicroKernelModuleApi* CustomCatalogModule::getApi(Node* node)
{
   // return catalog
   return mCatalog;
}

Module* createModestModule()
{
   return new CustomCatalogModule();
}

void freeModestModule(Module* m)
{
   delete m;
}

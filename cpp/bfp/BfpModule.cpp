/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/bfp/BfpModule.h"

#include "bitmunk/bfp/IBfpModuleImpl.h"

using namespace monarch::kernel;
using namespace monarch::logging;
using namespace monarch::modest;
using namespace monarch::rt;
using namespace bitmunk::bfp;
using namespace bitmunk::node;

// Logging category initialized during module initialization.
Category* BM_BFP_CAT;

BfpModule::BfpModule() :
   BitmunkModule("bitmunk.bfp.Bfp", "1.0"),
   mInterface(NULL)
{
}

BfpModule::~BfpModule()
{
}

void BfpModule::addDependencyInfo(DynamicObject& depInfo)
{
   // no dependencies
}

bool BfpModule::initialize(Node* node)
{
   BM_BFP_CAT = new Category(
      "BM_BFP",
      "Bitmunk BFP",
      NULL);

   // create interface
   mInterface = new IBfpModuleImpl(node);
   return true;
}

void BfpModule::cleanup(Node* node)
{
   if(mInterface != NULL)
   {
      delete mInterface;
      mInterface = NULL;
   }

   delete BM_BFP_CAT;
   BM_BFP_CAT = NULL;
}

MicroKernelModuleApi* BfpModule::getApi(Node* node)
{
   return mInterface;
}

Module* createModestModule()
{
   return new BfpModule();
}

void freeModestModule(Module* m)
{
   delete m;
}

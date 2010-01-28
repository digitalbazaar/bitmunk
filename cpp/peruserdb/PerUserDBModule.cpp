/*
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/peruserdb/PerUserDBModule.h"

#include "bitmunk/peruserdb/IPerUserDBModule.h"

using namespace monarch::config;
using namespace monarch::kernel;
using namespace monarch::logging;
using namespace monarch::modest;
using namespace monarch::rt;
using namespace bitmunk::node;
using namespace bitmunk::peruserdb;

// Logging category initialized during module initialization.
Category* BM_PERUSERDB_CAT;

PerUserDBModule::PerUserDBModule() :
   NodeModule("bitmunk.peruserdb.PerUserDB", "1.0"),
   mHub(NULL)
{
}

PerUserDBModule::~PerUserDBModule()
{
}

void PerUserDBModule::addDependencyInfo(DynamicObject& depInfo)
{
   // no dependencies
}

bool PerUserDBModule::initialize(Node* node)
{
   BM_PERUSERDB_CAT = new Category(
      "BM_PERUSERDB",
      "Bitmunk Per-user Database",
      NULL);

   mHub = new DatabaseHub(node);
   return true;
}

void PerUserDBModule::cleanup(Node* node)
{
   // clean up database hub
   if(mHub != NULL)
   {
      delete mHub;
      mHub = NULL;
   }

   delete BM_PERUSERDB_CAT;
   BM_PERUSERDB_CAT = NULL;
}

MicroKernelModuleApi* PerUserDBModule::getApi(Node* node)
{
   return mHub;
}

Module* createModestModule()
{
   return new PerUserDBModule();
}

void freeModestModule(Module* m)
{
   delete m;
}

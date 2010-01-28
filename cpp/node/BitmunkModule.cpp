/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/node/BitmunkModule.h"

using namespace monarch::kernel;
using namespace monarch::modest;
using namespace monarch::rt;
using namespace bitmunk::node;

BitmunkModule::BitmunkModule() :
   MicroKernelModule("bitmunk.node.Node", "1.0"),
   mNode(NULL)
{
}

BitmunkModule::~BitmunkModule()
{
}

DynamicObject BitmunkModule::getDependencyInfo()
{
   DynamicObject rval;

   // set name, version, and type for this module
   rval["name"] = mId.name;
   rval["version"] = mId.version;
   rval["type"] = "bitmunk.node";

   // no dependencies
   rval["dependencies"]->setType(Array);

   return rval;
}

bool BitmunkModule::validate(MicroKernel* k)
{
   bool rval = true;

   // FIXME: get config manager can ensure command line options for
   // bitmunk node are valid

   return rval;
}

bool BitmunkModule::initialize(MicroKernel* k)
{
   bool rval = false;

   // create and start the bitmunk node
   mNode = new Node();
   rval = mNode->start(k);
   if(!rval)
   {
      // node failed to start
      delete mNode;
      mNode = NULL;
   }

   return rval;
}

void BitmunkModule::cleanup(MicroKernel* k)
{
   if(mNode != NULL)
   {
      // stop and clean up node
      mNode->stop();
      delete mNode;
      mNode = NULL;
   }
}

MicroKernelModuleApi* BitmunkModule::getApi(MicroKernel* k)
{
   return mNode;
}

Module* createModestModule()
{
   return new BitmunkModule();
}

void freeModestModule(Module* m)
{
   delete m;
}

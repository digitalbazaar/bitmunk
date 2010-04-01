/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/node/NodeModule.h"

using namespace monarch::kernel;
using namespace monarch::rt;
using namespace bitmunk::node;

NodeModule::NodeModule(const char* name, const char* version) :
   MicroKernelModule(name, version),
   mNode(NULL)
{
}

NodeModule::~NodeModule()
{
}

DynamicObject NodeModule::getDependencyInfo()
{
   DynamicObject rval;

   // set name, version, and type for this module
   rval["name"] = mId.name;
   rval["version"] = mId.version;

   // parse type as everything up to the last period
   const char* lastPeriod = strrchr(mId.name, '.');
   if(lastPeriod != NULL)
   {
      int len = lastPeriod - mId.name;
      char type[len + 1];
      type[len] = 0;
      strncpy(type, mId.name, len);
      rval["type"] = type;
   }

   // set default dependencies
   rval["dependencies"]->setType(Array);

   // depends on bitmunk.node v1.0
   {
      DynamicObject dep;
      dep["name"] = "bitmunk.node.Node";
      dep["version"] = "1.0";
      rval["dependencies"]->append(dep);
   }

   // add custom dependency info
   addDependencyInfo(rval);

   return rval;
}

bool NodeModule::initialize(MicroKernel* k)
{
   bool rval = false;

   // get bitmunk node and initialize with it
   mNode = dynamic_cast<Node*>(k->getModuleApi("bitmunk.node.Node"));
   rval = initialize(mNode);
   if(!rval)
   {
      mNode = NULL;
   }

   return rval;
}

void NodeModule::cleanup(MicroKernel* k)
{
   // get bitmunk node and clean up with it, if the node is NULL, then the
   // bitmunk module failed to initialize which means no node module would
   // have initialized ... so we don't need to call cleanup()
   Node* node = dynamic_cast<Node*>(k->getModuleApi("bitmunk.node.Node"));
   if(node != NULL)
   {
      cleanup(node);
   }
   mNode = NULL;
}

MicroKernelModuleApi* NodeModule::getApi(MicroKernel* k)
{
   // get node and return API
   Node* node = dynamic_cast<Node*>(k->getModuleApi("bitmunk.node.Node"));
   return getApi(node);
}

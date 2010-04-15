/*
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/nodemodule/NodeModule.h"

using namespace monarch::kernel;
using namespace monarch::logging;
using namespace monarch::modest;
using namespace monarch::rt;
using namespace bitmunk::node;

NodeModule::NodeModule() :
   MicroKernelModule("bitmunk.node.Node", "1.0"),
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
   rval["type"] = "bitmunk.node";

   // no dependencies
   rval["dependencies"]->setType(Array);

   return rval;
}

bool NodeModule::initialize(MicroKernel* k)
{
   bool rval = false;

   BM_NODE_CAT = new Category(
      "BM_NODE",
      "Bitmunk Node",
      NULL);
   BM_NODE_CAT->setAnsiEscapeCodes(
      MO_ANSI_CSI MO_ANSI_BOLD MO_ANSI_SEP MO_ANSI_FG_HI_RED MO_ANSI_SGR);

   // create and initialize the bitmunk node
   mNode = new Node(k);
   rval = mNode->initialize();
   if(!rval)
   {
      // node failed to initialize
      delete mNode;
      mNode = NULL;
   }

   return rval;
}

void NodeModule::cleanup(MicroKernel* k)
{
   if(mNode != NULL)
   {
      // clean up node
      mNode->cleanup();
      delete mNode;
      mNode = NULL;
   }

   delete BM_NODE_CAT;
   BM_NODE_CAT = NULL;
}

MicroKernelModuleApi* NodeModule::getApi(MicroKernel* k)
{
   return mNode;
}

Module* createModestModule()
{
   return new NodeModule();
}

void freeModestModule(Module* m)
{
   delete m;
}

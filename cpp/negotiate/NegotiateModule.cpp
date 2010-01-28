/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/negotiate/NegotiateModule.h"

using namespace monarch::kernel;
using namespace monarch::modest;
using namespace monarch::rt;
using namespace bitmunk::node;
using namespace bitmunk::negotiate;

NegotiateModule::NegotiateModule() :
   NodeModule("bitmunk.negotiate.Negotiate", "1.0"),
   mInterface(NULL)
{
}

NegotiateModule::~NegotiateModule()
{
}

void NegotiateModule::addDependencyInfo(DynamicObject& depInfo)
{
   // no dependencies
}

bool NegotiateModule::initialize(Node* node)
{
   mInterface = new INegotiateModule(node);
   return true;
}

void NegotiateModule::cleanup(Node* node)
{
   if(mInterface != NULL)
   {
      delete mInterface;
      mInterface = NULL;
   }
}

MicroKernelModuleApi* NegotiateModule::getApi(Node* node)
{
   return mInterface;
}

Module* createModestModule()
{
   return new NegotiateModule();
}

void freeModestModule(Module* m)
{
   delete m;
}

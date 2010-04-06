/*
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/portmapper/PortMapperModule.h"

#include "bitmunk/node/BtpServer.h"
#include "bitmunk/portmapper/PortMapperService.h"

using namespace monarch::kernel;
using namespace monarch::modest;
using namespace monarch::rt;
using namespace bitmunk::node;
using namespace bitmunk::portmapper;
using namespace bitmunk::protocol;

PortMapperModule::PortMapperModule() :
   BitmunkModule("bitmunk.portmapper.PortMapper", "1.0"),
   mPortMapper(NULL)
{
}

PortMapperModule::~PortMapperModule()
{
}

void PortMapperModule::addDependencyInfo(DynamicObject& depInfo)
{
   // no dependencies
}

bool PortMapperModule::initialize(Node* node)
{
   bool rval;

   // create and attempt to initialize port mapper
   mPortMapper = new PortMapper();
   if((rval = mPortMapper->init(node)))
   {
      BtpServiceRef bs;

      {
         // create and add port mapper service
         bs = new PortMapperService(node, mPortMapper, "/api/3.2/portmapper");
         rval = node->getBtpServer()->addService(bs, Node::SslOn);
      }
   }

   if(!rval)
   {
      delete mPortMapper;
      mPortMapper = NULL;
   }

   return rval;
}

void PortMapperModule::cleanup(Node* node)
{
   // remove services
   node->getBtpServer()->removeService("/api/3.2/portmapper");

   // clean up port mapper
   if(mPortMapper != NULL)
   {
      mPortMapper->cleanup(node);
      delete mPortMapper;
   }
}

MicroKernelModuleApi* PortMapperModule::getApi(Node* node)
{
   return mPortMapper;
}

Module* createModestModule()
{
   return new PortMapperModule();
}

void freeModestModule(Module* m)
{
   delete m;
}

/*
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/eventreactor/ErDownloadStateModule.h"

#include "bitmunk/eventreactor/DownloadStateEventReactor.h"
#include "bitmunk/eventreactor/IEventReactorModule.h"

using namespace monarch::config;
using namespace monarch::kernel;
using namespace monarch::logging;
using namespace monarch::modest;
using namespace monarch::rt;
using namespace bitmunk::node;
using namespace bitmunk::protocol;
using namespace bitmunk::eventreactor;

// Logging category initialized during module initialization.
Category* BM_EVENTREACTOR_DS_CAT;

ErDownloadStateModule::ErDownloadStateModule() :
   BitmunkModule("bitmunk.eventreactor.EventReactor.DownloadState", "1.0")
{
}

ErDownloadStateModule::~ErDownloadStateModule()
{
}

void ErDownloadStateModule::addDependencyInfo(DynamicObject& depInfo)
{
   // set special type for this module
   depInfo["type"] = "bitmunk.eventreactor.EventReactor";

   // set dependencies:

   // requires the event reactor module, version 1.0
   {
      DynamicObject dep;
      dep["name"] = "bitmunk.eventreactor.EventReactor";
      dep["version"] = "1.0";
      depInfo["dependencies"]->append(dep);
   }

   // requires the purchase module, version 1.0
   {
      DynamicObject dep;
      dep["name"] = "bitmunk.purchase.Purchase";
      dep["version"] = "1.0";
      depInfo["dependencies"]->append(dep);
   }
}

bool ErDownloadStateModule::initialize(Node* node)
{
   bool rval = true;

   BM_EVENTREACTOR_DS_CAT = new Category(
      "BM_EVENTREACTOR_DS",
      "Bitmunk Download State Event Reactor",
      NULL);

   // get event reactor module interface
   IEventReactorModule* inf = dynamic_cast<IEventReactorModule*>(
      node->getKernel()->getModuleApi("bitmunk.eventreactor.EventReactor"));
   if(inf == NULL)
   {
      ExceptionRef e = new Exception(
         "Could not get eventreactor module interface for "
         "eventreactor-downloadstate. Possible incompatible module.",
         "bitmunk.eventreactor.EventReactor.DownloadState");
      Exception::set(e);
      rval = false;
   }
   else
   {
      // set event reactor
      inf->setEventReactor(
         "downloadstate", new DownloadStateEventReactor(node));
   }

   delete BM_EVENTREACTOR_DS_CAT;
   BM_EVENTREACTOR_DS_CAT = NULL;

   return rval;
}

void ErDownloadStateModule::cleanup(Node* node)
{
   // get event reactor module interface
   IEventReactorModule* inf = dynamic_cast<IEventReactorModule*>(
      node->getKernel()->getModuleApi("bitmunk.eventreactor.EventReactor"));
   if(inf != NULL)
   {
      // remove event reactor
      inf->removeEventReactor("downloadstate");
   }
}

MicroKernelModuleApi* ErDownloadStateModule::getApi(Node* node)
{
   // no API
   return NULL;
}

Module* createModestModule()
{
   return new ErDownloadStateModule();
}

void freeModestModule(Module* m)
{
   delete m;
}

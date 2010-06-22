/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/test/Tester.h"

#include "bitmunk/common/Logging.h"
#include "monarch/config/ConfigManager.h"
#include "monarch/io/FileList.h"
#include "monarch/rt/Platform.h"
#include "monarch/test/Test.h"

using namespace std;
using namespace bitmunk::common;
using namespace bitmunk::node;
using namespace bitmunk::test;
using namespace monarch::app;
using namespace monarch::config;
using namespace monarch::io;
using namespace monarch::kernel;
using namespace monarch::rt;

Tester::Tester()
{
}

Tester::~Tester()
{
}

bool Tester::loadConfig(monarch::test::TestRunner& tr, Config* extraMerge)
{
   bool rval;

   ConfigManager* cm = tr.getApp()->getConfigManager();
   rval = cm->addConfigFile(
      tr.getApp()->getConfig()
         ["bitmunk.apps.tester.Tester"]["configPath"]->getString());
   assertNoException();
   if(rval)
   {
      Config cfg = cm->getConfig(BITMUNK_TESTER_CONFIG_ID, true);
      assertNoException();
      if(!cfg.isNull())
      {
         string tmp;
         assert(File::getTemporaryDirectory(tmp));
         cfg[ConfigManager::MERGE]["node"]["bitmunkHomePath"] =
            File::join(tmp.c_str(), "test").c_str();
         if(extraMerge)
         {
            cfg[ConfigManager::MERGE].merge(*extraMerge, false);
         }

         rval = cm->setConfig(cfg);
         assertNoException();
      }
   }

   return rval;
}

bool Tester::unloadConfig(monarch::test::TestRunner& tr)
{
   bool rval = true;

   ConfigManager* cm = tr.getApp()->getConfigManager();
   if(cm->hasConfig(BITMUNK_TESTER_CONFIG_ID))
   {
      rval = cm->removeConfig(BITMUNK_TESTER_CONFIG_ID);
      assertNoException();
   }

   return rval;
}

static FileList _getModulePaths(App* app)
{
   FileList rval;

   Config cfg = app->getConfig()["node"];
   ConfigIterator mpi = cfg["modulePath"].getIterator();
   while(mpi->hasNext())
   {
      const char* path = mpi->next()->getString();
      FileList pathList = File::parsePath(path);
      rval->concat(*pathList);
   }

   return rval;
}

Node* Tester::loadNode(monarch::test::TestRunner& tr)
{
   Node* rval = NULL;

   // get test config
   ConfigManager* cm = tr.getApp()->getConfigManager();
   Config cfg = cm->getConfig(BITMUNK_TESTER_CONFIG_ID);
   assertNoException();
   assert(!cfg.isNull());

   // load node module
   assert(cfg["test"]->hasMember("nodeModule"));
   const char* nodeModule = cfg["test"]["nodeModule"]->getString();
   MicroKernel* k = tr.getApp()->getKernel();
   MicroKernelModule* mod = k->loadMicroKernelModule(nodeModule);
   if(mod != NULL)
   {
      rval = dynamic_cast<Node*>(mod->getApi(k));
      if(rval == NULL)
      {
         // unload bad module
         k->unloadModule(&mod->getId());
         ExceptionRef e = new Exception(
            "Invalid node module.",
            "bitmunk.test.Tester.InvalidNodeModule");
         e->getDetails()["filename"] = nodeModule;
         Exception::set(e);
      }
      else
      {
         // get node and other related module paths
         FileList modulePaths = _getModulePaths(tr.getApp());
         // load all module paths at once
         if(!k->loadModules(modulePaths))
         {
            unloadNode(tr);
            rval = NULL;
         }
      }
   }

   return rval;
}

bool Tester::unloadNode(monarch::test::TestRunner& tr)
{
   MicroKernel* k = tr.getApp()->getKernel();
   return k->unloadModule("bitmunk.node.Node");
}

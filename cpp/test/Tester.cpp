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

#define BITMUNK_TESTER_CONFIG_ID   "bitmunk.test.Tester.config.base"

Tester::Tester()
{
}

Tester::~Tester()
{
}

Node* Tester::loadNode(monarch::test::TestRunner& tr, const char* configFile)
{
   Node* rval = NULL;

   // get app and config manager
   App* app = tr.getApp();
   ConfigManager* cm = app->getConfigManager();

   // save config manager state
   cm->saveState();

   // set TMP_DIR keyword, update BITMUNK_HOME
   string tmp;
   assert(File::getTemporaryDirectory(tmp));
   cm->setKeyword("TMP_DIR", tmp.c_str());
   cm->setKeyword("BITMUNK_HOME", File::join(tmp.c_str(), "test").c_str());

   // add test config
   cm->addConfigFile(app->getConfig()
      ["bitmunk.apps.tester.Tester"]["configPath"]->getString());
   assertNoException();

   // add optional unit test config
   if(configFile != NULL)
   {
      Config cfg = app->getConfig()["bitmunk.apps.tester.Tester"];
      string path = File::join(
         cfg["unitTestConfigPath"]->getString(), configFile);
      cm->addConfigFile(path.c_str());
      assertNoException();
   }

   // get the path to the node module
   Config cfg = app->getConfig();
   assertNoException();
   assert(!cfg.isNull());

   // load node module
   assert(cfg["test"]->hasMember("nodeModule"));
   const char* nodeModule = cfg["test"]["nodeModule"]->getString();
   MicroKernel* k = app->getKernel();
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

         // restore config manager state
         cm->restoreState();
      }
      else
      {
         // get all of the module paths
         FileList modulePaths;
         ConfigIterator mpi = cfg["node"]["modulePath"].getIterator();
         while(mpi->hasNext())
         {
            const char* path = mpi->next()->getString();
            FileList pathList = File::parsePath(path);
            modulePaths->concat(*pathList);
         }

         // load all module paths at once
         if(!k->loadModules(modulePaths))
         {
            unloadNode(tr);
            rval = NULL;
         }
      }
   }

   assertNoException();
   return rval;
}

bool Tester::unloadNode(monarch::test::TestRunner& tr)
{
   bool rval = true;

   // unload the node
   MicroKernel* k = tr.getApp()->getKernel();
   rval = k->unloadModule("bitmunk.node.Node");
   assertNoException();

   // restore the config manager state
   rval = tr.getApp()->getConfigManager()->restoreState();
   assertNoException();

   return rval;
}

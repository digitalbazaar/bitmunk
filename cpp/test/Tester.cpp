/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/test/Tester.h"

#include "bitmunk/common/Logging.h"
#include "monarch/config/ConfigManager.h"
#include "monarch/io/File.h"
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

bool Tester::loadConfig(monarch::test::TestRunner& tr, Config* extraMerge)
{
   bool rval;

   // FIXME: get config path, do not hard code
   ConfigManager* cm = tr.getApp()->getConfigManager();
   rval = cm->addConfigFile("configs/test.config");
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

Node* Tester::loadNode(monarch::test::TestRunner& tr)
{
   Node* rval = NULL;

   // get modules directory
   ConfigManager* cm = tr.getApp()->getConfigManager();
   Config cfg = cm->getConfig(BITMUNK_TESTER_CONFIG_ID);
   assertNoException();
   assert(!cfg.isNull());
   assert(cfg["node"]->hasMember("modulePath"));
   string path = cfg["node"]["modulePath"]->getString();
   assert(path.length() > 0);

   // load node module
   File file(File::join(path.c_str(), "libbmnode.so").c_str());
   MicroKernel* k = tr.getMicroKernel();
   MicroKernelModule* mod = k->loadMicroKernelModule(file->getAbsolutePath());
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
         e->getDetails()["filename"] = file->getAbsolutePath();
         Exception::set(e);
      }
   }

   return rval;
}

bool Tester::unloadNode(monarch::test::TestRunner& tr)
{
   MicroKernel* k = tr.getMicroKernel();
   return k->unloadModule("bitmunk.node.Node");
}

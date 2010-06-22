/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/apps/tester/Tester.h"

#include "bitmunk/common/Logging.h"
#include "monarch/app/AppFactory.h"
#include "monarch/config/ConfigManager.h"
#include "monarch/io/File.h"
#include "monarch/rt/Platform.h"
#include "monarch/test/Test.h"
#include "monarch/test/TestLoader.h"

using namespace std;
using namespace bitmunk::apps::tester;
using namespace bitmunk::common;
using namespace bitmunk::node;
using namespace monarch::app;
using namespace monarch::config;
using namespace monarch::io;
using namespace monarch::kernel;
using namespace monarch::modest;
using namespace monarch::rt;
using namespace monarch::test;

#define APP_NAME "bitmunk.apps.tester.Tester"

Tester::Tester()
{
}

Tester::~Tester()
{
}

bool Tester::initialize()
{
   bool rval = bitmunk::app::App::initialize();
   if(rval)
   {
      setName(APP_NAME);
      setVersion("1.0");
   }
   return rval;
}

bool Tester::initConfigs(Config& defaults)
{
   bool rval = bitmunk::app::App::initConfigs(defaults);
   if(rval)
   {
      // add test loader defaults
      TestLoader loader;
      rval =
         loader.initConfigs(defaults) &&
         getConfigManager()->addConfig(defaults);
   }
   return rval;
}

DynamicObject Tester::getCommandLineSpec(Config& cfg)
{
   // add test loader command line options
   TestLoader loader;
   return loader.getCommandLineSpec(cfg);
}

bool Tester::run()
{
   bool rval;

   // FIXME: get config path, do not hard code
   ConfigManager* cm = getConfigManager();
   rval = cm->addConfigFile("configs/test.config");
   if(rval)
   {
      Config cfg = cm->getConfig(BITMUNK_TESTER_CONFIG_ID, true);
      if(!cfg.isNull())
      {
         string tmp;
         rval = File::getTemporaryDirectory(tmp);
         if(rval)
         {
            cfg[ConfigManager::MERGE]["node"]["bitmunkHomePath"] =
               File::join(tmp.c_str(), "test").c_str();
            /*
            if(extraMerge)
            {
               cfg[ConfigManager::MERGE].merge(*extraMerge, false);
            }
            */
            rval = cm->setConfig(cfg);
         }
      }
   }

   // use test loader to run tests
   TestLoader loader;
   return loader.run(this);
}
/*
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
   }

   return rval;
}

bool Tester::unloadNode(monarch::test::TestRunner& tr)
{
   MicroKernel* k = tr.getApp()->getKernel();
   return k->unloadModule("bitmunk.node.Node");
}
*/
class TesterFactory : public AppFactory
{
public:
   TesterFactory() : AppFactory(APP_NAME, "1.0")
   {
   }

   virtual ~TesterFactory() {}

   virtual App* createApp()
   {
      return new Tester();
   }
};

Module* createModestModule()
{
   return new TesterFactory();
}

void freeModestModule(Module* m)
{
   delete m;
}

/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/apps/tester/Tester.h"

#include "bitmunk/common/Logging.h"
#include "monarch/app/AppFactory.h"
#include "monarch/config/ConfigManager.h"
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
   // initialize config
   Config& c = cfg[ConfigManager::MERGE][APP_NAME];
   c["dumpConfig"] = false;

   // add test loader command line options
   TestLoader loader;
   DynamicObject spec = loader.getCommandLineSpec(cfg);

   // add option to dump config when loading node
   string help = spec["help"]->getString();
   help.append(
"Bitmunk Test options:\n"
"      --dump-node-config Dump the node configuration before loading a node.\n"
"\n");
   spec["help"] = help.c_str();

   DynamicObject opt;

   opt = spec["options"]->append();
   opt["long"] = "--dump-node-config";
   opt["setTrue"]["root"] = c;
   opt["setTrue"]["path"] = "dumpConfig";

   return spec;
}

bool Tester::run()
{
   // use test loader to run tests
   TestLoader loader;
   return loader.run(this);
}

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

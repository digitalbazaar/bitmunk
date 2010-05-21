/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk.h"

#include "monarch/app/App.h"
#include "monarch/app/AppFactory.h"
#include "monarch/config/ConfigManager.h"
#include "monarch/data/json/JsonReader.h"
#include "monarch/data/json/JsonWriter.h"
#include "monarch/modest/Module.h"
#include "monarch/util/Macros.h"
#include "monarch/rt/DynamicObject.h"
#include "bitmunk/app/App.h"
#include "bitmunk/common/Logging.h"
#include "bitmunk/node/Node.h"

#include "config.h"

using namespace std;
using namespace monarch::app;
using namespace monarch::config;
using namespace monarch::data::json;
using namespace monarch::event;
using namespace monarch::fiber;
using namespace monarch::io;
using namespace monarch::kernel;
using namespace monarch::logging;
using namespace monarch::modest;
using namespace monarch::net;
using namespace monarch::rt;
using namespace bitmunk::apps;
using namespace bitmunk::common;
using namespace bitmunk::node;

#define APP_NAME "bitmunk.apps.Bitmunk"

#define BITMUNK_NODE_RESTART_EVENT "bitmunk.node.Node.restart"
#define BITMUNK_NODE_SHUTDOWN_EVENT "bitmunk.node.Node.shutdown"

#define BITMUNK_NAME "Bitmunk"

Bitmunk::Bitmunk()
{
}

Bitmunk::~Bitmunk()
{
}

bool Bitmunk::initialize()
{
   bool rval = bitmunk::app::App::initialize();
   if(rval)
   {
      setName(BITMUNK_NAME);
      setVersion(BITMUNK_VERSION);
   }
   return rval;
}

bool Bitmunk::initConfigs(Config& defaults)
{
   bool rval = bitmunk::app::App::initConfigs(defaults);
   if(rval)
   {
      // set defaults
      Config& cm = defaults[ConfigManager::MERGE];
      Config& cmp = defaults[ConfigManager::MERGE][APP_NAME];
      cm["node"]["handlers"]->setType(Map);
      cm["node"]["events"]->setType(Map);

      // all config info in one map
      Config& configs = cmp["configs"];
      configs->setType(Map);

      // setup package config loading
      {
         Config& cfg = configs["package"];
         cfg["id"] = APP_NAME ".configs.package";
         if(getenv("BITMUNK_PACKAGE_CONFIG") != NULL)
         {
            // use environment var and require
            cfg["path"] = getenv("BITMUNK_PACKAGE_CONFIG");
            cfg["optional"] = false;
         }
         else
         {
            // try to load optional default
            cfg["path"] = BITMUNK_PACKAGE_CONFIG;
            cfg["optional"] = true;
         }
         cfg["load"] = (cfg["path"]->length() > 0);
      }

      // setup system config loading
      {
         Config& cfg = configs["system"];
         cfg["id"] = APP_NAME ".configs.system";
         if(getenv("BITMUNK_SYSTEM_CONFIG") != NULL)
         {
            // use environment var and require
            cfg["path"] = getenv("BITMUNK_SYSTEM_CONFIG");
            cfg["optional"] = false;
         }
         else
         {
            // try to load optional default
            cfg["path"] = BITMUNK_SYSTEM_CONFIG;
            cfg["optional"] = true;
         }
         cfg["load"] = (cfg["path"]->length() > 0);
      }

      // setup system user config loading
      {
         Config& cfg = configs["systemUser"];
         cfg["id"] = APP_NAME ".configs.systemUser";
         if(getenv("BITMUNK_SYSTEM_USER_CONFIG") != NULL)
         {
            // use environment var and require
            cfg["path"] = getenv("BITMUNK_SYSTEM_USER_CONFIG");
            cfg["optional"] = false;
         }
         else
         {
            // try to load optional default
            cfg["path"] = BITMUNK_SYSTEM_USER_CONFIG;
            cfg["optional"] = true;
         }
         cfg["load"] = (cfg["path"]->length() > 0);
      }

      // add to config manager
      return getConfigManager()->addConfig(defaults);
   }

   return rval;
}

DynamicObject Bitmunk::getCommandLineSpec(Config& cfg)
{
   // initialize config
   Config& options = cfg[ConfigManager::MERGE];
   options[APP_NAME]->setType(Map);

   DynamicObject spec;
   spec["help"] =
"Bitmunk options\n"
"      --package-config FILE\n"
"                      The package configuration file to load.\n"
"                      Overrides default and BITMUNK_PACKAGE_CONFIG.\n"
"      --system-config FILE\n"
"                      The system configuration file to load.\n"
"                      Overrides default and BITMUNK_SYSTEM_CONFIG.\n"
"      --system-user-config FILE\n"
"                      The system user configuration file to load.\n"
"                      Overrides default and BITMUNK_SYSTEM_USER_CONFIG.\n"
"      --no-package-config\n"
"                      Do not load default package configuration file.\n"
"      --no-system-config\n"
"                      Do not load default system configuration file.\n"
"      --no-system-user-config\n"
"                      Do not load default system user configuration file.\n"
"      --bitmunk-module-path PATH\n"
"                      A colon separated list of Node modules or directories\n"
"                      where Node modules are stored. May be specified multiple\n"
"                      times. Appends to BITMUNK_MODULE_PATH.\n"
"      --profile-path PATH\n"
"                      The directory where profiles are stored.\n"
"  -p, --port PORT     The server port to use for all incoming connections.\n"
"  -a, --auto-login    Auto-login to the node.\n"
"  -U, --user USER     Node user for auto-login.\n"
"  -P, --password PASSWORD\n"
"                      Node password for auto-login.\n"
"\n";

   DynamicObject opt;
   Config& op = options[APP_NAME];
   op->setType(Map);

   opt = spec->append();
   opt["long"] = "--package-config";
   opt["include"]["config"] = getMetaConfig()["appOptions"];
   opt["include"]["params"] = op["configs"]["package"];
   opt["setTrue"]["root"] = op;
   opt["setTrue"]["path"] = "configs.package.load";
   opt["setFalse"]["root"] = op;
   opt["setFalse"]["path"] = "configs.package.optional";
   opt["argError"] = "No package config file specified.";

   opt = spec->append();
   opt["long"] = "--system-config";
   opt["include"]["config"] = getMetaConfig()["appOptions"];
   opt["include"]["params"] = op["configs"]["system"];
   opt["setTrue"]["root"] = op;
   opt["setTrue"]["path"] = "configs.system.load";
   opt["setFalse"]["root"] = op;
   opt["setFalse"]["path"] = "configs.system.optional";
   opt["argError"] = "No system config file specified.";

   opt = spec->append();
   opt["long"] = "--system-user-config";
   opt["include"]["config"] = getMetaConfig()["appOptions"];
   opt["include"]["params"] = op["configs"]["systemUser"];
   opt["setTrue"]["root"] = op;
   opt["setTrue"]["path"] = "configs.systemUser.load";
   opt["setFalse"]["root"] = op;
   opt["setFalse"]["path"] = "configs.systemUser.optional";
   opt["argError"] = "No system user config file specified.";

   opt = spec->append();
   opt["long"] = "--no-package-config";
   opt["setFalse"]["root"] = op;
   opt["setFalse"]["path"] = "configs.package.load";

   opt = spec->append();
   opt["long"] = "--no-system-config";
   opt["setFalse"]["root"] = op;
   opt["setFalse"]["path"] = "configs.system.load";

   opt = spec->append();
   opt["long"] = "--no-system-user-config";
   opt["setFalse"]["root"] = op;
   opt["setFalse"]["path"] = "configs.systemUser.load";

   opt = spec->append();
   opt["short"] = "-a";
   opt["long"] = "--auto-login";
   opt["setTrue"]["root"] = options;
   opt["setTrue"]["path"] = "node.login.auto";

   opt = spec->append();
   opt["short"] = "-p";
   opt["long"] = "--port";
   opt["arg"]["root"] = options;
   opt["arg"]["path"] = "node.port";
   opt["arg"]["type"] = (uint64_t)0;
   opt["argError"] = "No port specified.";

   opt = spec->append();
   opt["long"] = "--bitmunk-module-path";
   opt["append"]["root"] = options;
   opt["append"]["path"] = "node.modulePath";
   opt["argError"] = "No node module path specified.";

   opt = spec->append();
   opt["long"] = "--profile-dir";
   opt["arg"]["root"] = options;
   opt["arg"]["path"] = "node.profilePath";
   opt["argError"] = "No path specified.";

   opt = spec->append();
   opt["short"] = "-U";
   opt["long"] = "--user";
   opt["arg"]["root"] = options;
   opt["arg"]["path"] = "node.login.username";
   opt["argError"] = "No user specified.";

   opt = spec->append();
   opt["short"] = "-P";
   opt["long"] = "--password";
   opt["arg"]["root"] = options;
   opt["arg"]["path"] = "node.login.password";
   opt["argError"] = "No password specified.";

   return spec;
}

FileList Bitmunk::getModulePaths()
{
   FileList rval;

   Config cfg = getConfig()["node"];
   ConfigIterator mpi = cfg["modulePath"].getIterator();
   while(mpi->hasNext())
   {
      const char* path = mpi->next()->getString();
      FileList pathList = File::parsePath(path);
      rval->concat(*pathList);
   }

   return rval;
}

bool Bitmunk::startNode()
{
   bool rval;
   // setup app wait event
   // The bitmunk app run() call will finish normally but we must setup a
   // wait event so the kernel knows bitmunk is still running.
   //...

   // observe node events
   mNodeRestartObserver =
      new ObserverDelegate<Bitmunk>(this, &Bitmunk::restartNodeHandler);
   getKernel()->getEventController()->registerObserver(
      &(*mNodeRestartObserver), BITMUNK_NODE_RESTART_EVENT);
   MO_CAT_DEBUG(BM_APP_CAT,
      "Bitmunk registered for " BITMUNK_NODE_RESTART_EVENT);

   /*
   Currently shutdown event will cause kernel to stop everything so no need
   to handle it here.  At some point need node vs kernel shutdown handling.

   mNodeShutdownObserver =
      new ObserverDelegate<Bitmunk>(this, &Bitmunk::shutdownNodeHandler);
   getKernel()->getEventController()->registerObserver(
      &(*mNodeShutdownObserver), BITMUNK_NODE_SHUTDOWN_EVENT);
   MO_CAT_DEBUG(BM_APP_CAT,
      "Bitmunk registered for " BITMUNK_NODE_SHUTDOWN_EVENT);
   */

   // FIXME: change to use the builtin module path stuff? won't want to do
   // that if we want to be able to restart the node without restarting
   // the kernel...

   // get node and other related module paths
   FileList modulePaths = getModulePaths();
   // load all module paths at once
   rval = getKernel()->loadModules(modulePaths);
   Node* node = dynamic_cast<Node*>(
      getKernel()->getModuleApi("bitmunk.node.Node"));
   if(node == NULL)
   {
      ExceptionRef e = new Exception(
         "Could not load Node interface. No compatible Node module found.",
         APP_NAME ".InvalidNodeModule");
      Exception::set(e);
      rval = false;
   }
   else
   {
      rval = node->start();
   }

   // send ready event
   if(rval)
   {
      MO_CAT_INFO(BM_APP_CAT, "Ready.");
      Event e;
      e["type"] = "bitmunk.app.Bitmunk.ready";
      getKernel()->getEventController()->schedule(e);
   }

   return rval;
}

bool Bitmunk::stopNode(bool restarting)
{
   bool rval = true;

   // stop watching for node events
   getKernel()->getEventController()->unregisterObserver(
      &(*mNodeRestartObserver), BITMUNK_NODE_RESTART_EVENT);
   mNodeRestartObserver.setNull();

   /*
   No need to implement this until node can be shutdown without shutting down
   the kernel.

   if(!restarting)
   {
      undo app wait event
   }
   */

   Node* node = dynamic_cast<Node*>(
      getKernel()->getModuleApi("bitmunk.node.Node"));
   if(node != NULL)
   {
      node->stop();
      // unload node module and all modules depending on it
      rval = getKernel()->unloadModule("bitmunk.node.Node");
   }

   return rval;
}

bool Bitmunk::restartNode()
{
   return stopNode(true) && startNode();
}

void Bitmunk::restartNodeHandler(Event& e)
{
   MO_CAT_INFO(BM_APP_CAT, "Handling node restart event.");
   //bool success =
   restartNode();
   // FIXME: handle exceptions
}

void Bitmunk::shutdownNodeHandler(Event& e)
{
   MO_CAT_INFO(BM_APP_CAT, "Handling node shutdown event.");
   //bool success =
   stopNode();
   // FIXME: handle exceptions
}

DynamicObject Bitmunk::getWaitEvents()
{
   DynamicObject rval;
   DynamicObject event;
   event["id"] = APP_NAME;
   event["type"] = APP_NAME ".wait";
   rval->append(event);
   return rval;
}

bool Bitmunk::run()
{
   bool rval = true;

   // Dump preliminary start-up information
   MO_CAT_INFO(BM_APP_CAT, "Bitmunk");
   MO_CAT_INFO(BM_APP_CAT, "Version: %s", BITMUNK_VERSION);

   Config cfg = getConfig()["node"];

   MO_CAT_INFO(BM_APP_CAT, "Modules path: %s",
      JsonWriter::writeToString(cfg["modulePath"]).c_str());

   rval = startNode();

   return rval;
}

class BitmunkFactory : public AppFactory
{
public:
   BitmunkFactory() : AppFactory(APP_NAME, "1.0")
   {
   }

   virtual ~BitmunkFactory() {}

   virtual App* createApp()
   {
      return new Bitmunk();
   }
};

Module* createModestModule()
{
   return new BitmunkFactory();
}

void freeModestModule(Module* m)
{
   delete m;
}

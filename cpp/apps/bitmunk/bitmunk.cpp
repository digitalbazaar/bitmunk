/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "monarch/app/App.h"
#include "monarch/config/ConfigManager.h"
#include "monarch/data/json/JsonReader.h"
#include "monarch/data/json/JsonWriter.h"
#include "monarch/modest/Module.h"
#include "monarch/util/Macros.h"
#include "monarch/rt/DynamicObject.h"
#include "bitmunk/common/Logging.h"
#include "bitmunk/app/App.h"
#include "bitmunk/node/Node.h"

#include "bitmunk.h"
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

#define PLUGIN_NAME "bitmunk.apps.Bitmunk"
#define PLUGIN_CL_CFG_ID PLUGIN_NAME ".commandLine"

#define BITMUNK_NODE_RESTART_EVENT "bitmunk.node.Node.restart"
#define BITMUNK_NODE_SHUTDOWN_EVENT "bitmunk.node.Node.shutdown"

const char* BITMUNK_NAME = "Bitmunk";

Bitmunk::Bitmunk(MicroKernel* k) :
   mState(Stopped),
   mMicroKernel(k)
{
}

Bitmunk::~Bitmunk()
{
}

bool Bitmunk::didAddToApp(monarch::app::App* app)
{
   bool rval = AppPlugin::didAddToApp(app);
   if(rval)
   {
      getApp()->setName(BITMUNK_NAME);
      getApp()->setVersion(BITMUNK_VERSION);
   }
   return rval;
}

DynamicObject Bitmunk::getWaitEvents()
{
   DynamicObject rval = AppPlugin::getWaitEvents();
   DynamicObject event;
   event["id"] = PLUGIN_NAME;
   event["type"] = PLUGIN_NAME ".wait";
   rval->append(event);

   return rval;
}

bool Bitmunk::initMetaConfig(Config& meta)
{
   bool rval = monarch::app::AppPlugin::initMetaConfig(meta);

   // defaults added in willLoadConfigs()

   // command line options
   if(rval)
   {
      Config config =
         App::makeMetaConfig(
            meta, PLUGIN_CL_CFG_ID, "command line", "options");
      config[ConfigManager::VERSION] = BITMUNK_CONFIG_VERSION;
      Config& c = config[ConfigManager::MERGE];
      c[PLUGIN_NAME]->setType(Map);

      // setup default home path early so it can be used as a keyword in
      // command line configs
      // FIXME: Setting this here will make it always override any value in the
      // FIXME: config files. The default here is the same as the default in
      // FIXME: the system config but users will not be able to override it in
      // FIXME: the system user config.

      // try env var, else use default
      const char* home = getenv("BITMUNK_HOME");
      if(home == NULL)
      {
         home = BITMUNK_HOME;
      }
      c["node"]["bitmunkHomePath"] = home;
   }

   return rval;
}

DynamicObject Bitmunk::getCommandLineSpecs()
{
   DynamicObject spec;
   spec["help"] =
"Bitmunk options\n"
"      --home PATH     The directory path to use for storage and configs.\n"
"                      Overrides default and BITMUNK_HOME.\n"
"                      Available in configs as {BITMUNK_HOME}.\n"
"      --package-config FILE\n"
"                      The package configuration file to load.\n"
"                      Overrides default and BITMUNK_PACKAGE_CONFIG.\n"
"      --system-config FILE\n"
"                      The system configuration file to load.\n"
"                      Overrides default and BITMUNK_SYSTEM_CONFIG.\n"
"      --system-user-config FILE\n"
"                      The system user configuration file to load.\n"
"                      Overrides default and BITMUNK_SYSTEM_USER_CONFIG.\n"
"      --extra-config FILE\n"
"                      Add extra Bitmunk configuration files. May be specified\n"
"                      multiple times.\n"
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
   Config options = getApp()->getMetaConfig()
      ["options"][PLUGIN_CL_CFG_ID][ConfigManager::MERGE];
   Config& op = options[PLUGIN_NAME];
   op->setType(Map);

   opt = spec["options"]->append();
   opt["long"] = "--home";
   opt["arg"]["root"] = options;
   opt["arg"]["path"] = "node.bitmunkHomePath";
   opt["argError"] = "No home path specified.";

   opt = spec["options"]->append();
   opt["long"] = "--package-config";
   opt["arg"]["root"] = op;
   opt["arg"]["path"] = "configs.package.path";
   opt["argError"] = "No package config file specified.";
   opt["setTrue"]["root"] = op;
   opt["setTrue"]["path"] = "configs.package.load";
   opt["setFalse"]["root"] = op;
   opt["setFalse"]["path"] = "configs.package.optional";

   opt = spec["options"]->append();
   opt["long"] = "--system-config";
   opt["arg"]["root"] = op;
   opt["arg"]["path"] = "configs.system.path";
   opt["argError"] = "No system config file specified.";
   opt["setTrue"]["root"] = op;
   opt["setTrue"]["path"] = "configs.system.load";
   opt["setFalse"]["root"] = op;
   opt["setFalse"]["path"] = "configs.system.optional";

   opt = spec["options"]->append();
   opt["long"] = "--system-user-config";
   opt["arg"]["root"] = op;
   opt["arg"]["path"] = "configs.systemUser.path";
   opt["argError"] = "No system user config file specified.";
   opt["setTrue"]["root"] = op;
   opt["setTrue"]["path"] = "configs.systemUser.load";
   opt["setFalse"]["root"] = op;
   opt["setFalse"]["path"] = "configs.systemUser.optional";

   opt = spec["options"]->append();
   opt["long"] = "--extra-config";
   opt["append"]["root"] = op;
   opt["append"]["path"] = "configs.extra";
   opt["argError"] = "No extra config file specified.";

   opt = spec["options"]->append();
   opt["long"] = "--no-package-config";
   opt["setFalse"]["root"] = op;
   opt["setFalse"]["path"] = "configs.package.load";

   opt = spec["options"]->append();
   opt["long"] = "--no-system-config";
   opt["setFalse"]["root"] = op;
   opt["setFalse"]["path"] = "configs.system.load";

   opt = spec["options"]->append();
   opt["long"] = "--no-system-user-config";
   opt["setFalse"]["root"] = op;
   opt["setFalse"]["path"] = "configs.systemUser.load";

   opt = spec["options"]->append();
   opt["short"] = "-a";
   opt["long"] = "--auto-login";
   opt["setTrue"]["root"] = options;
   opt["setTrue"]["path"] = "app.login.auto";

   opt = spec["options"]->append();
   opt["short"] = "-p";
   opt["long"] = "--port";
   opt["arg"]["root"] = options;
   opt["arg"]["path"] = "node.port";
   opt["arg"]["type"] = (uint64_t)0;
   opt["argError"] = "No port specified.";

   opt = spec["options"]->append();
   opt["long"] = "--bitmunk-module-path";
   opt["append"]["root"] = options;
   opt["append"]["path"] = "node.modulePath";
   opt["argError"] = "No node module path specified.";

   opt = spec["options"]->append();
   opt["long"] = "--profile-dir";
   opt["arg"]["root"] = options;
   opt["arg"]["path"] = "node.profilePath";
   opt["argError"] = "No path specified.";

   opt = spec["options"]->append();
   opt["short"] = "-U";
   opt["long"] = "--user";
   opt["arg"]["root"] = options;
   opt["arg"]["path"] = "node.password";
   opt["argError"] = "No user specified.";

   opt = spec["options"]->append();
   opt["short"] = "-P";
   opt["long"] = "--password";
   opt["arg"]["root"] = options;
   opt["arg"]["path"] = "node.password";
   opt["argError"] = "No password specified.";

   DynamicObject specs = AppPlugin::getCommandLineSpecs();
   specs->append(spec);
   return specs;
}

bool Bitmunk::willLoadConfigs()
{
   bool rval = AppPlugin::willLoadConfigs();

   if(rval)
   {
      // setup keywords from options config
      // get config, but do not cache merged config to prevent config changes
      // from being tracked until absolutely necessary
      Config options = getApp()->getMetaConfig()
         ["options"][PLUGIN_CL_CFG_ID][ConfigManager::MERGE];
      // replace some vars while building standard config
      if(options->hasMember("node") &&
         options["node"]->hasMember("bitmunkHomePath"))
      {
         getApp()->getConfigManager()->setKeyword(
            "BITMUNK_HOME", options["node"]["bitmunkHomePath"]->getString());
      }
      if(options->hasMember("node") &&
         options["node"]->hasMember("resourcePath"))
      {
         getApp()->getConfigManager()->setKeyword(
            "RESOURCE_PATH", options["node"]["resourcePath"]->getString());
      }
   }

   // defaults
   // Note: Done here instead of in initMetaConfig so {BITMUNK_HOME} can be
   // used in defaults.
   if(rval)
   {
      Config meta = getApp()->getMetaConfig();
      Config c =
         App::makeMetaConfig(meta, PLUGIN_NAME ".defaults", "defaults");
      c[ConfigManager::VERSION] = BITMUNK_CONFIG_VERSION;
      Config& cm = c[ConfigManager::MERGE];
      Config& cmp = c[ConfigManager::MERGE][PLUGIN_NAME];
      cm["node"]["handlers"]->setType(Map);
      cm["node"]["events"]->setType(Map);

      // all config info in one map
      Config& configs = cmp["configs"];
      configs->setType(Map);

      // setup package config loading
      {
         Config& cfg = configs["package"];
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
      // Note: see didLoadConfigs() for special code that tries to find this
      // config and may prepend {BITMUNK_HOME} automatically.
      {
         Config& cfg = configs["systemUser"];
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

      // setup extra configs default
      configs["extra"]->setType(Array);

      // add to config manager
      rval = getApp()->getConfigManager()->addConfig(c);
   }

   return rval;
}

bool Bitmunk::didLoadConfigs()
{
   bool rval = AppPlugin::didLoadConfigs();

   // process and the config and load bitmunk configs

   // NOTE: The BITMUNK_HOME and RESOURCE_PATH keywords are set in a command
   // line config and will override any values that were just here.

   /*
   Configuration:

   The Bitmunk client uses a number of sources of configuration and manages
   them with a Nodes ConfigManager object.  The following are applied in order
   and may be controlled with command line options.  Configurations with
   __include__ directives will be processed and added to the ConfigManager in
   top-down order.  Only the main file is added as a Custom config.  All others
   are added as a Default.

   defaults - internal hard coded config
   package - package config
   system - default system config, BITMUNK_SYSTEM_CONFIG env, --system-config,
      or none if --no-system-config
   system user - default node config, BITMUNK_SYSTEM_USER_CONFIG env,
      --system-user-config, or none if --no-system-user-config
   cl - additional config files specified on the command line

   If a config file is specified then it must exist.  Defaults config files
   can be missing.
   */

   Config c = getApp()->getConfig()[PLUGIN_NAME];

   // app meta config
   Config meta = getApp()->getMetaConfig();

   // load system, node, and extra configs
   if(rval)
   {
      // fake a config with includes
      Config cfg;

      cfg[ConfigManager::ID] = PLUGIN_NAME ".includes";
      cfg[ConfigManager::GROUP] = meta["groups"]["main"]->getString();
      cfg[ConfigManager::VERSION] = BITMUNK_CONFIG_VERSION;

      // add standard configs
      cfg[ConfigManager::INCLUDE]->append() = c["configs"]["package"];
      cfg[ConfigManager::INCLUDE]->append() = c["configs"]["system"];
      cfg[ConfigManager::INCLUDE]->append() = c["configs"]["systemUser"];

      // add extra configs
      DynamicObjectIterator ei = c["configs"]["extra"].getIterator();
      while(ei->hasNext())
      {
         DynamicObject& next = ei->next();
         Config ecfg = cfg[ConfigManager::INCLUDE]->append();
         ecfg["path"] = next->getString();
         ecfg["load"] = true;
         ecfg["optional"] = false;
      }

      // load all configs
      rval = getApp()->getConfigManager()->addConfig(cfg);
   }

   return rval;
}

FileList Bitmunk::getModulePaths()
{
   FileList rval;

   Config cfg = getApp()->getConfigManager()->getConfig("main")["node"];
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
   // The bitmunk plugin run() call will finish normally but we must setup a
   // wait event so the kernel knows bitmunk is still running.
   //...

   // observe node events
   mNodeRestartObserver =
      new ObserverDelegate<Bitmunk>(this, &Bitmunk::restartNodeHandler);
   mMicroKernel->getEventController()->registerObserver(
      &(*mNodeRestartObserver), BITMUNK_NODE_RESTART_EVENT);
   MO_CAT_DEBUG(BM_APP_CAT,
      "Bitmunk registered for " BITMUNK_NODE_RESTART_EVENT);

   /*
   Currently shutdown event will cause kernel to stop everything so no need
   to handle it here.  At some point need node vs kernel shutdown handling.

   mNodeShutdownObserver =
      new ObserverDelegate<Bitmunk>(this, &Bitmunk::shutdownNodeHandler);
   mMicroKernel->getEventController()->registerObserver(
      &(*mNodeShutdownObserver), BITMUNK_NODE_SHUTDOWN_EVENT);
   MO_CAT_DEBUG(BM_APP_CAT,
      "Bitmunk registered for " BITMUNK_NODE_SHUTDOWN_EVENT);
   */

   // get node and other related module paths
   FileList modulePaths = getModulePaths();
   // load all module paths at once
   rval = mMicroKernel->loadModules(modulePaths);
   Node* node =
      dynamic_cast<Node*>(mMicroKernel->getModuleApi("bitmunk.node.Node"));
   if(node == NULL)
   {
      ExceptionRef e = new Exception(
         "Could not load Node interface. No compatible Node module found.",
         PLUGIN_NAME ".InvalidNodeModule");
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
      mMicroKernel->getEventController()->schedule(e);
   }

   return rval;
}

bool Bitmunk::stopNode(bool restarting)
{
   bool rval = true;

   // stop watching for node events
   mMicroKernel->getEventController()->unregisterObserver(
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

   Node* node =
      dynamic_cast<Node*>(mMicroKernel->getModuleApi("bitmunk.node.Node"));
   if(node != NULL)
   {
      node->stop();
      // unload node module and all modules depending on it
      rval = mMicroKernel->unloadModule("bitmunk.node.Node");
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

bool Bitmunk::run()
{
   bool rval = AppPlugin::run();

   // Dump preliminary start-up information
   MO_CAT_INFO(BM_APP_CAT, "Bitmunk");
   MO_CAT_INFO(BM_APP_CAT, "Version: %s", BITMUNK_VERSION);

   Config cfg = getApp()->getConfigManager()->getConfig("main")["node"];

   MO_CAT_INFO(BM_APP_CAT, "Modules path: %s",
      JsonWriter::writeToString(cfg["modulePath"]).c_str());

   rval = startNode();

   return rval;
}

class BitmunkFactory :
   public AppPluginFactory
{
public:
   BitmunkFactory() :
      AppPluginFactory(PLUGIN_NAME, "1.0")
   {
      addDependency("monarch.app.Config", "1.0");
      addDependency("monarch.app.Logging", "1.0");
      addDependency("bitmunk.app.App", "1.0");
   }

   virtual ~BitmunkFactory() {}

   virtual AppPluginRef createAppPlugin()
   {
      return new Bitmunk(mMicroKernel);
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

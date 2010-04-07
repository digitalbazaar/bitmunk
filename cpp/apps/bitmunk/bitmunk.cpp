/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "monarch/app/App.h"
#include "monarch/config/ConfigManager.h"
#include "monarch/data/json/JsonReader.h"
#include "monarch/data/json/JsonWriter.h"
#include "monarch/io/FileOutputStream.h"
#include "monarch/modest/Module.h"
#include "monarch/logging/OutputStreamLogger.h"
#include "monarch/util/Macros.h"
//#include "monarch/event/EventWaiter.h"
//#include "monarch/rt/Thread.h"
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
using namespace bitmunk::apps::bitmunk;
using namespace bitmunk::common;
using namespace bitmunk::node;

#define PLUGIN_NAME "bitmunk.apps.bitmunk.Bitmunk"
#define PLUGIN_CL_CFG_ID PLUGIN_NAME ".commandLine"

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

   // defaults
   if(rval)
   {
      Config c =
         App::makeMetaConfig(meta, PLUGIN_NAME ".defaults", "defaults");
      c[ConfigManager::VERSION] = BITMUNK_CONFIG_VERSION;
      Config& cm = c[ConfigManager::MERGE];
      Config& cmp = c[ConfigManager::MERGE][PLUGIN_NAME];
      cm["app"]["handlers"]->setType(Map);
      cm["app"]["events"]->setType(Map);

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
   }

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
   opt["setTrue"]["path"] = "configs.sysmteUser.load";
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
   system - default system config, BITMUNK_SYSTEM_CONFIG env, --system-config,
      or none if --no-system-config
   node - default node config, BITMUNK_SYSTEM_USER_CONFIG env, --node-config,
          or none if --no-node-config
   cl - additional config files specified on the command line

   If a config file is specified then it must exist.  Defaults config files
   can be missing.
   */

   // special config logger
   // used only during configuration
   FileOutputStream logStream(FileOutputStream::StdOut);
   OutputStreamLogger logger(&logStream, false);

   Logger::Level logLevel = Logger::Warning;
   Config appCfg = getApp()->getConfig();
   Config& c = appCfg[PLUGIN_NAME];
   if(appCfg["app"]["config"]["debug"]->getBoolean())
   {
      logLevel = Logger::Debug;
   }
   logger.setLevel(logLevel);
   Logger::addLogger(&logger);

   // app meta config
   Config meta = getApp()->getMetaConfig();

   // setup meta config
   if(rval)
   {
      /**
       * Meta config about configs.
       * For "system" and "node" types:
       *    cfg["app"]["config"][type]["load"] = true|false
       *    cfg["app"]["config"][type]["path"] = ...
       */
      Config config;
      config[ConfigManager::ID] = PLUGIN_NAME ".meta";
      config[ConfigManager::GROUP] = meta["groups"]["main"]->getString();
      config[ConfigManager::VERSION] = BITMUNK_CONFIG_VERSION;
      Config merge = config[ConfigManager::MERGE];
      merge["app"]["config"]["package"] = c["configs"]["package"];
      merge["app"]["config"]["system"] = c["configs"]["system"];
      merge["app"]["config"]["system user"] = c["configs"]["systemUser"];
      rval = getApp()->getConfigManager()->addConfig(config);
   }

   // load system, node, and extra configs
   if(rval)
   {
      // fake a config with includes
      Config cfg;
      int i = 0;

      cfg[ConfigManager::ID] = PLUGIN_NAME ".includes";
      cfg[ConfigManager::GROUP] = meta["groups"]["main"]->getString();
      cfg[ConfigManager::VERSION] = BITMUNK_CONFIG_VERSION;
      cfg[ConfigManager::INCLUDE][i++] = c["configs"]["package"];
      cfg[ConfigManager::INCLUDE][i++] = c["configs"]["system"];
      cfg[ConfigManager::INCLUDE][i++] = c["configs"]["systemUser"];
      DynamicObjectIterator eci =
         appCfg[PLUGIN_NAME]["configs"]["extra"].getIterator();
      while(eci->hasNext())
      {
         DynamicObject& next = eci->next();
         Config& ecfg = cfg[ConfigManager::INCLUDE][i++];
         ecfg["path"] = next->getString();
         ecfg["load"] = true;
         ecfg["optional"] = false;
      }
      rval = getApp()->getConfigManager()->addConfig(cfg);
   }

#if 0
   if(rval)
   {
      Config cfg = getApp()->getConfig();
      if(cfg["app"]["config"]["dump"]->getBoolean())
      {
         // array of maps with named configs
         // FIXME: put this or more comprehensive code in NodeConfigManager
         ConfigManager* cm = getConfigManager();
         /*
         DynamicObject dump;
         dump->setType(Array);
         // merged groups
         dump->append()["boot"] = cm->getConfig("boot");
         dump->append()["system"] = cm->getConfig("system");
         dump->append()["system user"] = cm->getConfig("system user");
         dump->append()["node"] = cm->getConfig("system");
         dump->append()["modules"] = cm->getConfig("system");
         dump->append()["command line"] = cm->getConfig("command line");
         dump->append()[meta["groups"]["main"]->getString()] =
            cm->getConfig(meta["groups"]["main"]->getString());
         dump->append()["users"] = cm->getConfig("users");
         dump->append()["meta"] = cm->getConfig("bitmunk.app meta");
         // even more debug
         dump->append()["debug"] = cm->getDebugInfo();
         JsonWriter::writeToStdOut(dump);
         */
         // all debug info
         JsonWriter::writeToStdOut(cm->getDebugInfo());
      }
   }
#endif

   Logger::removeLogger(&logger);

   return rval;
}

/*
bool Bitmunk::waitForNode()
{
   bool rval = true;

   MO_CAT_INFO(BM_APP_CAT, "Ready.");

   // send ready event
   {
      Event e;
      e["type"] = "bitmunk.app.Bitmunk.ready";
      mKernel->getEventController()->schedule(e);
   }

   EventWaiter waiter(mKernel->getEventController());
   waiter.start("bitmunk.node.Node.shutdown");
   waiter.start("bitmunk.node.Node.restart");
   waiter.waitForEvent();

   Event e = waiter.popEvent();
   if(strcmp("bitmunk.node.Node.shutdown", e["type"]->getString()) == 0)
   {
      mState = Stopping;
   }
   else if(strcmp("bitmunk.node.Node.restart", e["type"]->getString()) == 0)
   {
      mState = Restarting;
   }

   return rval;
}
*/

bool Bitmunk::run()
{
   bool rval = AppPlugin::run();

   // Dump preliminary start-up information
   MO_CAT_INFO(BM_APP_CAT, "Bitmunk");
   MO_CAT_INFO(BM_APP_CAT, "Version: %s", BITMUNK_VERSION);

   Config cfg = getApp()->getConfigManager()->getConfig("main")["node"];

   MO_CAT_INFO(BM_APP_CAT, "Modules path: %s",
      JsonWriter::writeToString(cfg["modulePath"]).c_str());

   // load modules
   // this will load the node and other modules
   // The bitmunk plugin will finish but has setup the kernel to wait for a
   // bitmunk wait event. The node can also signal shutdown and restart events
   // as needed.
   FileList modulePaths;
   ConfigIterator mpi = cfg["modulePath"].getIterator();
   while(rval && mpi->hasNext())
   {
      const char* path = mpi->next()->getString();
      FileList pathList = File::parsePath(path);
      modulePaths->concat(*pathList);
   }
   // load all module paths at once
   rval = mMicroKernel->loadModules(modulePaths);

   MO_CAT_INFO(BM_APP_CAT, "Ready.");

   // send ready event
   {
      Event e;
      e["type"] = "bitmunk.app.Bitmunk.ready";
      mMicroKernel->getEventController()->schedule(e);
   }

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

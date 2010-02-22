/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "monarch/app/App.h"
#include "monarch/config/ConfigManager.h"
#include "monarch/data/json/JsonReader.h"
#include "monarch/data/json/JsonWriter.h"
#include "monarch/io/FileOutputStream.h"
#include "monarch/util/Macros.h"
#include "monarch/event/EventWaiter.h"
#include "monarch/logging/OutputStreamLogger.h"
#include "monarch/rt/Thread.h"
#include "monarch/rt/DynamicObject.h"
#include "bitmunk/common/Logging.h"
#include "bitmunk/node/App.h"
#include "bitmunk/node/Node.h"

#include "bitmunk.h"
#include "config.h"

using namespace std;
using namespace monarch::app;
using namespace monarch::config;
using namespace monarch::data::json;
using namespace monarch::event;
using namespace monarch::fiber;
using namespace monarch::kernel;
using namespace monarch::logging;
using namespace monarch::io;
using namespace monarch::net;
using namespace monarch::rt;
using namespace bitmunk;
using namespace bitmunk::common;
using namespace bitmunk::node;

const char* BITMUNK_NAME = "Bitmunk";

BitmunkApp::BitmunkApp() :
   mState(Stopped)
{
   mInfo["id"] = "bitmunk.app.Bitmunk";
   mInfo["dependencies"]->append() = "bitmunk.app.App";
}

BitmunkApp::~BitmunkApp()
{
}

bool BitmunkApp::didAddToApp(monarch::app::App* app)
{
   bool rval = AppPlugin::didAddToApp(app);
   if(rval)
   {
      getApp()->setName(BITMUNK_NAME);
      getApp()->setVersion(BITMUNK_VERSION);
   }
   return rval;
}

ConfigManager* BitmunkApp::getConfigManager()
{
   return &mNodeConfigManager;
}

bool BitmunkApp::initMetaConfig(Config& meta)
{
   bool rval = monarch::app::AppPlugin::initMetaConfig(meta);

   // setup default home path early so it can be used in command line configs
   // command line option config
   if(rval)
   {
      const char* id = "bitmunk.app command line";
      Config& config = meta["options"][id];

      config[ConfigManager::GROUP] =
         meta["groups"]["command line"]->getString();
      config[ConfigManager::ID] = id;
      config[ConfigManager::VERSION] = NodeConfigManager::CONFIG_VERSION;

      Config& merge = config[ConfigManager::MERGE];

      // setup default home so it can be used as a keyword
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
      merge["node"]["bitmunkHomePath"] = home;
   }

   return rval;
}

DynamicObject BitmunkApp::getCommandLineSpecs()
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
"      --no-package-config\n"
"                      Do not load default package configuration file.\n"
"      --no-system-config\n"
"                      Do not load default system configuration file.\n"
"      --no-system-user-config\n"
"                      Do not load default system user configuration file.\n"
"  -c, --config FILE   Load a configuration file.  May be specified multiple\n"
"                      times.\n"
"  -m, --module-path PATH\n"
"                      A colon separated directory list where extension modules\n"
"                      are stored.\n"
"  -r, --resource-path PATH\n"
"                      The directory where the Bitmunk application resource\n"
"                      files were installed.\n"
"                      Available in configs as {RESOURCE_PATH}.\n"
"      --profile-path PATH\n"
"                      The directory where profiles are stored.\n"
"  -i, --interactive   Run in interactive mode.\n"
"  -p, --port PORT     The server port to use for all incoming connections.\n"
"  -a, --auto-login    Auto-login to the node.\n"
"  -U, --user USER     Node user for auto-login.\n"
"  -P, --password PASSWORD\n"
"                      Node password for auto-login.\n"
"\n";

   DynamicObject opt;
   Config options = getApp()->getMetaConfig()
      ["options"]["bitmunk.app command line"][ConfigManager::MERGE];

   opt = spec["options"]->append();
   opt["long"] = "--home";
   opt["arg"]["root"] = options;
   opt["arg"]["path"] = "node.bitmunkHomePath";
   opt["argError"] = "No home path specified.";

   opt = spec["options"]->append();
   opt["long"] = "--package-config";
   opt["arg"]["target"] = mPackageConfig["path"];
   opt["argError"] = "No file specified.";
   opt["setTrue"]["target"] = mPackageConfig["load"];
   opt["setFalse"]["target"] = mPackageConfig["optional"];

   opt = spec["options"]->append();
   opt["long"] = "--system-config";
   opt["arg"]["target"] = mSystemConfig["path"];
   opt["argError"] = "No file specified.";
   opt["setTrue"]["target"] = mSystemConfig["load"];
   opt["setFalse"]["target"] = mSystemConfig["optional"];

   opt = spec["options"]->append();
   opt["long"] = "--system-user-config";
   opt["arg"]["target"] = mSystemUserConfig["path"];
   opt["argError"] = "No file specified.";
   opt["setTrue"]["target"] = mSystemUserConfig["load"];
   opt["setFalse"]["target"] = mSystemUserConfig["optional"];

   opt = spec["options"]->append();
   opt["long"] = "--no-package-config";
   opt["setFalse"]["target"] = mPackageConfig["load"];

   opt = spec["options"]->append();
   opt["long"] = "--no-system-config";
   opt["setFalse"]["target"] = mSystemConfig["load"];

   opt = spec["options"]->append();
   opt["long"] = "--no-system-user-config";
   opt["setFalse"]["target"] = mSystemUserConfig["load"];

   opt = spec["options"]->append();
   opt["short"] = "-c";
   opt["long"] = "--config";
   opt["append"] = mExtraConfigs;
   opt["argError"] = "No file specified.";

   opt = spec["options"]->append();
   opt["short"] = "-i";
   opt["long"] = "--interactive";
   opt["setTrue"]["root"] = options;
   opt["setTrue"]["path"] = "app.interactive";

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
   opt["arg"]["type"] = (uint64_t)0;;
   opt["argError"] = "No port specified.";

   opt = spec["options"]->append();
   opt["short"] = "-m";
   opt["long"] = "--module-path";
   opt["arg"]["root"] = options;
   opt["arg"]["path"] = "node.modulePath";
   opt["argError"] = "No path specified.";

   opt = spec["options"]->append();
   opt["short"] = "-r";
   opt["long"] = "--resource-path";
   opt["arg"]["root"] = options;
   opt["arg"]["path"] = "node.resourcePath";
   opt["argError"] = "No resource path specified.";

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

bool BitmunkApp::willParseCommandLine(std::vector<const char*>* args)
{
   bool rval = AppPlugin::willParseCommandLine(args);

   // setup package config loading
   if(rval)
   {
      mPackageConfig[ConfigManager::VERSION] =
         NodeConfigManager::CONFIG_VERSION;
      if(getenv("BITMUNK_PACKAGE_CONFIG") != NULL)
      {
         // use environment var and require
         mPackageConfig["path"] = getenv("BITMUNK_PACKAGE_CONFIG");
         mPackageConfig["optional"] = false;
      }
      else
      {
         // try to load optional default
         mPackageConfig["path"] = BITMUNK_PACKAGE_CONFIG;
         mPackageConfig["optional"] = true;
      }
      mPackageConfig["load"] = (mPackageConfig["path"]->length() > 0);
   }

   // setup system config loading
   if(rval)
   {
      mSystemConfig[ConfigManager::VERSION] =
         NodeConfigManager::CONFIG_VERSION;
      if(getenv("BITMUNK_SYSTEM_CONFIG") != NULL)
      {
         // use environment var and require
         mSystemConfig["path"] = getenv("BITMUNK_SYSTEM_CONFIG");
         mSystemConfig["optional"] = false;
      }
      else
      {
         // try to load optional default
         mSystemConfig["path"] = BITMUNK_SYSTEM_CONFIG;
         mSystemConfig["optional"] = true;
      }
      mSystemConfig["load"] = (mSystemConfig["path"]->length() > 0);
   }

   // setup system user config loading
   if(rval)
   {
      mSystemUserConfig[ConfigManager::VERSION] =
         NodeConfigManager::CONFIG_VERSION;
      if(getenv("BITMUNK_SYSTEM_USER_CONFIG") != NULL)
      {
         // use environment var and require
         mSystemUserConfig["path"] = getenv("BITMUNK_SYSTEM_USER_CONFIG");
         mSystemUserConfig["optional"] = false;
      }
      else
      {
         // try to load optional default
         mSystemUserConfig["path"] = BITMUNK_SYSTEM_USER_CONFIG;
         mSystemUserConfig["optional"] = true;
      }
      mSystemUserConfig["load"] = (mSystemUserConfig["path"]->length() > 0);
   }

   if(rval)
   {
      mExtraConfigs->setType(Array);
   }

   return rval;
}

bool BitmunkApp::willLoadConfigs()
{
   bool rval = AppPlugin::willLoadConfigs();

   if(rval)
   {
      // setup keywords from options config
      // get config, but do not cache merged config to prevent config changes
      // from being tracked until absolutely necessary
      Config options = getApp()->getMetaConfig()
         ["options"]["bitmunk.app command line"][ConfigManager::MERGE];
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

bool BitmunkApp::didLoadConfigs()
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
      config[ConfigManager::ID] = "bitmunk.app meta";
      config[ConfigManager::GROUP] = meta["groups"]["main"]->getString();
      config[ConfigManager::VERSION] = NodeConfigManager::CONFIG_VERSION;
      Config merge = config[ConfigManager::MERGE];
      merge["app"]["config"]["package"] = mPackageConfig;
      merge["app"]["config"]["system"] = mSystemConfig;
      merge["app"]["config"]["system user"] = mSystemUserConfig;
      rval = getConfigManager()->addConfig(config);
   }

   Config defaults;
   if(rval)
   {
      defaults[ConfigManager::ID] = "bitmunk.app defaults";
      defaults[ConfigManager::GROUP] = meta["groups"]["main"]->getString();
      defaults[ConfigManager::VERSION] = NodeConfigManager::CONFIG_VERSION;
      // set defaults for optional items
      defaults[ConfigManager::MERGE]["app"]["handlers"]->setType(Map);
      defaults[ConfigManager::MERGE]["app"]["events"]->setType(Map);
      rval = getConfigManager()->addConfig(defaults);
   }

   // load system, node, and extra configs
   if(rval)
   {
      // fake a config with includes
      Config cfg;
      int i = 0;

      cfg[ConfigManager::ID] = "bitmunk.app includes";
      cfg[ConfigManager::GROUP] = meta["groups"]["main"]->getString();
      cfg[ConfigManager::VERSION] = NodeConfigManager::CONFIG_VERSION;
      cfg[ConfigManager::INCLUDE][i++] = mPackageConfig;
      cfg[ConfigManager::INCLUDE][i++] = mSystemConfig;
      cfg[ConfigManager::INCLUDE][i++] = mSystemUserConfig;
      DynamicObjectIterator eci = mExtraConfigs.getIterator();
      while(eci->hasNext())
      {
         DynamicObject& next = eci->next();
         Config& ecfg = cfg[ConfigManager::INCLUDE][i++];
         ecfg["path"] = next->getString();
         ecfg["load"] = true;
         ecfg["optional"] = false;
      }
      rval = getConfigManager()->addConfig(cfg);
   }

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

   Logger::removeLogger(&logger);

   return rval;
}

bool BitmunkApp::initConfigManager()
{
   getApp()->setConfigManager(&mNodeConfigManager, false);
   // set the config manager required version
   getApp()->getConfigManager()->addVersion(MO_DEFAULT_CONFIG_VERSION);
   getApp()->getConfigManager()->addVersion(NodeConfigManager::CONFIG_VERSION);

   return true;
}

void BitmunkApp::cleanupConfigManager()
{
   // no-op
}

bool BitmunkApp::waitForNode()
{
   bool rval = true;

   MO_CAT_INFO(BM_APP_CAT, "Ready.");

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

bool BitmunkApp::run()
{
   bool rval = AppPlugin::run();

   // Dump preliminary start-up information
   MO_CAT_INFO(BM_APP_CAT, "Bitmunk");
   MO_CAT_INFO(BM_APP_CAT, "Version: %s", BITMUNK_VERSION);

   // create kernel
   mKernel = new MicroKernel();
   mKernel->setConfigManager(&mNodeConfigManager, false);
   mKernel->setFiberScheduler(new FiberScheduler(), true);
   mKernel->setFiberMessageCenter(new FiberMessageCenter(), true);
   mKernel->setEventController(new EventController(), true);
   mKernel->setEventDaemon(new EventDaemon(), true);
   mKernel->setServer(new Server(), true);

   mState = Starting;
   while(mState == Starting || mState == Restarting)
   {
      // FIXME: figure out the correct config to use here
      Config cfg = mNodeConfigManager.getNodeConfig();

      // FIXME: show this somewhere
      MO_CAT_INFO(BM_APP_CAT,
         "Modules path: %s", cfg["modulePath"]->getString());

      // FIXME: actually restarting microkernel, not just node
      // [re]start the node
      MO_CAT_INFO(BM_APP_CAT,
         (mState == Restarting) ? "Restarting node..." : "Starting node...");
      rval = mKernel->start(cfg);
      if(rval)
      {
         // load modules
         rval = mKernel->loadModules(cfg["modulePath"]->getString());
         if(!rval)
         {
            mKernel->stop();
         }
      }
      mState = Running;

      if(rval)
      {
         MO_CAT_INFO(BM_APP_CAT, "Node started.");
         waitForNode();

         // FIXME: actually stopping microkernel, not just node
         // stop node
         MO_CAT_INFO(BM_APP_CAT,
            (mState == Restarting) ?
               "Stopping node for restart..." :
               "Stopping node...");
         mKernel->stop();
         MO_CAT_INFO(BM_APP_CAT, "Node stopped.");
         // set to stopped unless restarting
         mState = (mState == Stopping) ? Stopped : mState;
      }
      else
      {
         MO_CAT_ERROR(BM_APP_CAT, "Node start failed: %s",
            JsonWriter::writeToString(Exception::getAsDynamicObject()).c_str());
      }
   }

   MO_CAT_INFO(BM_APP_CAT, "Shutdown.");

   // clean up kernel
   delete mKernel;

   return rval;
}

/**
 * Top-level main()
 */
BM_APP_PLUGIN_MAIN(BitmunkApp)

/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/app/App.h"

#include "monarch/app/App.h"
#include "monarch/app/AppPluginFactory.h"
#include "monarch/modest/Module.h"
#include "bitmunk/common/Logging.h"
#include "bitmunk/node/NodeConfigManager.h"

#include <cstdio>

using namespace monarch::config;
using namespace monarch::modest;
using namespace monarch::rt;
using namespace bitmunk::app;
using namespace bitmunk::common;
using namespace bitmunk::node;

App::App()
{
}

App::~App()
{
}

bool App::initConfigManager()
{
   // add acceptable bitmunk version
   getApp()->getConfigManager()->addVersion(BITMUNK_CONFIG_VERSION);

   return true;
}

Config App::makeMetaConfig(Config& meta, const char* id, const char* groupId)
{
   Config rval = monarch::app::App::makeMetaConfig(meta, id, groupId);
   rval[ConfigManager::VERSION] = BITMUNK_CONFIG_VERSION;
   return rval;
}

bool App::willInitMetaConfig(Config& meta)
{
   bool rval = monarch::app::AppPlugin::willInitMetaConfig(meta);

   if(rval)
   {
      // insert Bitmunk specific config groups
      meta["groups"]["before system"] = "before system";
      meta["groups"]["system"] = "system";
      meta["groups"]["after system"] = "after system";

      meta["groups"]["before system user"] = "before system user";
      meta["groups"]["system user"] = "system user";
      meta["groups"]["after system user"] = "after system user";

      meta["groups"]["before users"] = "before users";
      meta["groups"]["users"] = "users";
      meta["groups"]["after users"] = "after users";

      // insert groups into hierarchy
      meta["parents"]["before system"] =
         meta["parents"]["command line"]->getString();
      meta["parents"]["system"] = "before system";
      meta["parents"]["after system"] = "system";
      meta["parents"]["before system user"] = "after system";
      meta["parents"]["system user"] = "before system user";
      meta["parents"]["after system user"] = "system user";
      meta["parents"]["command line"] = "after system user";
      // Note: "main" already has "command line" as a parent
      meta["parents"]["before users"] = "main";
      meta["parents"]["users"] = "before users";
      meta["parents"]["after users"] = "users";
   }

   return rval;
}

bool App::initMetaConfig(Config& meta)
{
   bool rval = monarch::app::AppPlugin::initMetaConfig(meta);

   if(rval)
   {
      // create the group hierarchy with empty configs

      // system
      makeMetaConfig(meta, "bitmunk.app.beforeSystem.empty", "before system");
      makeMetaConfig(meta, "bitmunk.app.system.empty", "system");
      makeMetaConfig(meta, "bitmunk.app.afterSystem.empty", "after system");

      // system user
      makeMetaConfig(meta,
         "bitmunk.app.beforeSystemUser.empty", "before system user");
      makeMetaConfig(meta,
         "bitmunk.app.systemUser.empty", "system user");
      makeMetaConfig(meta,
         "bitmunk.app.afterSystemUser.empty", "after system user");

      // command line
      makeMetaConfig(meta, "bitmunk.app.commandLine.empty", "command line");

      // users
      makeMetaConfig(meta, "bitmunk.app.beforeUsers.empty", "before users");
      makeMetaConfig(meta, "bitmunk.app.users.empty", "users");
      makeMetaConfig(meta, "bitmunk.app.afterUsers.empty", "after users");
   }

   return rval;
}

bool App::initializeLogging()
{
   bool rval = AppPlugin::initializeLogging();
   // setup logging system
   // no need to call superclass method here since logging will do the same
   bitmunk::common::Logging::initialize();
   return rval;
}

bool App::cleanupLogging()
{
   bool rval = AppPlugin::cleanupLogging();
   // cleanup logging system
   // no need to call superclass method here since logging will do the same
   bitmunk::common::Logging::cleanup();
   return rval;
}

class BitmunkAppFactory :
   public monarch::app::AppPluginFactory
{
public:
   BitmunkAppFactory() :
      AppPluginFactory("bitmunk.app.App", "1.0")
   {
      addDependency("monarch.app.Monarch", "1.0");
      addDependency("monarch.app.Logging", "1.0");
   }

   virtual ~BitmunkAppFactory() {}

   virtual monarch::app::AppPluginRef createAppPlugin()
   {
      return new App();
   }
};

Module* createModestModule()
{
   return new BitmunkAppFactory();
}

void freeModestModule(Module* m)
{
   delete m;
}

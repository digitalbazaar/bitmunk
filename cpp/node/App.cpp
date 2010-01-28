/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/node/App.h"

#include "monarch/app/App.h"
#include "bitmunk/common/Logging.h"
#include "bitmunk/node/NodeConfigManager.h"

using namespace monarch::config;
using namespace monarch::rt;
using namespace bitmunk::common;
using namespace bitmunk::node;

App::App()
{
   mInfo["id"] = "bitmunk.app.App";
   mInfo["dependencies"]->append() = "monarch.app.plugins.Common";
}

App::~App()
{
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

Config App::makeMetaConfig(Config& meta, const char* id, const char* groupId)
{
   Config rval = monarch::app::App::makeMetaConfig(meta, id, groupId);
   rval[ConfigManager::VERSION] = NodeConfigManager::CONFIG_VERSION;
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

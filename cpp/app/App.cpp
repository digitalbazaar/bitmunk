/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/app/App.h"

#include "monarch/app/App.h"
#include "bitmunk/common/Logging.h"

using namespace monarch::config;
using namespace monarch::rt;
using namespace bitmunk::app;
using namespace bitmunk::common;

#define BITMUNK_APP   "bitmunk.app"

App::App()
{
}

App::~App()
{
}

bool App::initialize()
{
   // Acceptable bitmunk config version
   // Note: As of 3.2.2 this is just for backwards compatibility. New configs
   // should use a Monarch config version (MO_DEFAULT_CONFIG_VERSION).
   getApp()->getConfigManager()->addVersion("Bitmunk 3.0");

   // setup bitmunk logging system
   bitmunk::common::Logging::initialize();
   return true;
}

void App::cleanup()
{
   // cleanup bitmunk logging system
   bitmunk::common::Logging::cleanup();
}

bool App::initConfigs(Config& defaults)
{
   bool rval = true;

   // insert Bitmunk specific config groups
   ConfigManager* cm = getApp()->getConfigManager();
   ConfigManager::ConfigId cmdLineParent = cm->getConfig(
      "command line", true, false)[ConfigManager::PARENT]->getString();
   rval =
      // insert configs before command line
      cm->addConfig(getApp()->makeConfig(
         BITMUNK_APP ".beforeSystem.empty", "before system",
         cmdLineParent)) &&
      cm->addConfig(getApp()->makeConfig(
         BITMUNK_APP ".system.empty", "system",
         "before system")) &&
      cm->addConfig(getApp()->makeConfig(
         BITMUNK_APP ".afterSystem.empty", "after system",
         "system")) &&
      cm->addConfig(getApp()->makeConfig(
         BITMUNK_APP ".beforeSystemUser.empty", "before system user",
         "after system")) &&
      cm->addConfig(getApp()->makeConfig(
         BITMUNK_APP ".systemUser.empty", "system user",
         "before system user")) &&
      cm->addConfig(getApp()->makeConfig(
         BITMUNK_APP ".afterSystemUser.empty", "after system user",
         "system user")) &&
      // change command line's parent to after system user
      cm->setParent("command line", "after system user");
      cm->addConfig(getApp()->makeConfig(
         BITMUNK_APP ".beforeUsers.empty", "before users",
         "main")) &&
      cm->addConfig(getApp()->makeConfig(
         BITMUNK_APP ".users.empty", "users",
         "before users")) &&
      cm->addConfig(getApp()->makeConfig(
         BITMUNK_APP ".afterUsers.empty", "after users",
         "users"));

   return rval;
}

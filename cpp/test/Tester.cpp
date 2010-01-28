/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/test/Tester.h"

#include <iostream>

// FIXME: bitmunk-unit-tests.h should be split between the bmtest library
//        and the tests.
#include "bitmunk/tests/bitmunk-unit-tests.h"
#include "bitmunk/common/Logging.h"
#include "monarch/config/ConfigManager.h"
#include "monarch/io/OStreamOutputStream.h"
#include "monarch/logging/Logging.h"
#include "monarch/logging/OutputStreamLogger.h"
#include "monarch/test/Test.h"

using namespace std;
using namespace bitmunk::common;
using namespace bitmunk::node;
using namespace bitmunk::test;
using namespace monarch::app;
using namespace monarch::config;
using namespace monarch::io;
using namespace monarch::logging;
using namespace monarch::rt;

Tester::Tester()
{
   mInfo["id"] = "monarch.test.Tester";
   mInfo["dependencies"]->append() = "monarch.app.plugins.Common";
}

Tester::~Tester()
{
}

void Tester::setup(monarch::test::TestRunner& tr)
{
   monarch::test::Tester::setup(tr);
   Config config;
   {
      Config meta = getApp()->getMetaConfig();
      const char* gid = meta["groups"]["main"]->getString();
      config[ConfigManager::ID] = "bitmunk.test.Tester config";
      config[ConfigManager::GROUP] = gid;
      config[ConfigManager::PARENT] = meta["parents"][gid]->getString();
   }
   Config& cfg = config[ConfigManager::MERGE];

   cfg["node"]["host"] = "localhost";
   cfg["node"]["port"] = 19101;
   cfg["node"]["bitmunkUrl"] = "http://localhost:19101";
   cfg["node"]["secureBitmunkUrl"] = "https://localhost:19101";
   cfg["node"]["modulePath"] = MODULES_DIRECTORY;
   cfg["node"]["bitmunkHomePath"] = "/tmp/test";
   cfg["node"]["profilePath"] = PROFILES_DIRECTORY;
   cfg["node"]["usersPath"] = "users";
   cfg["node"]["sslGenerate"] = true;
   cfg["node"]["sslCertificate"] = SSL_CERTIFICATE_FILE;
   cfg["node"]["sslPrivateKey"] = SSL_PRIVATE_KEY_FILE;
   cfg["node"]["sslCAFile"] = SSL_CA_FILE;
   cfg["node"]["maxConnectionCount"] = (uint64_t)100;
   cfg["node"]["maxThreadCount"] = (uint64_t)100;

   // cached bfp modules directory
   cfg["bitmunk.bfp.Bfp"]["cachePath"] = "bitmunk.bfp.Bfp/cache";

   // FIXME: this looks dated...
   // null webui config
   cfg["bitmunk.webui.WebUi"]["main"]->setType(String);
   cfg["bitmunk.webui.WebUi"]["plugins"]->setType(Map);
   //cfg["bitmunk.webui.WebUi"]["modules"]->setType(Array);
   //cfg["bitmunk.webui.WebUi"]["modulesPath"]->setType(Array);
   cfg["bitmunk.webui.WebUi"]["sessionDatabase"]["url"] =
      "sqlite3://bitmunk.webui.WebUi/sessiondb.db";
   cfg["bitmunk.webui.WebUi"]["sessionDatabase"]["connections"] = 1;

   // flash config info
   cfg["bitmunk.webui.WebUi"]["flash"]["policyServer"]["host"] = "0.0.0.0";
   cfg["bitmunk.webui.WebUi"]["flash"]["policyServer"]["port"] = 19844;

   // purchase module config
   cfg["bitmunk.purchase.Purchase"]["temporaryPath"] = "tmp";
   cfg["bitmunk.purchase.Purchase"]["downloadPath"] = "dl";
   cfg["bitmunk.purchase.Purchase"]["maxDownloadRate"] = 1024 * 1024;
   cfg["bitmunk.purchase.Purchase"]["maxExcessBandwidth"] = 1024 * 10;
   cfg["bitmunk.purchase.Purchase"]["maxPieces"] = 10;
   cfg["bitmunk.purchase.Purchase"]["database"]["url"] =
      "sqlite3://bitmunk.purchase.Purchase/purchase.db";
   cfg["bitmunk.purchase.Purchase"]["database"]["connections"] = 1;

   // media library config
   cfg["bitmunk.medialibrary.MediaLibrary"]["database"]["url"] =
      "sqlite3://bitmunk.medialibrary.MediaLibrary/medialibrary.db";
   cfg["bitmunk.medialibrary.MediaLibrary"]["database"]["connections"] = 1;

   // custom catalog config
   // Note: do updates every 10 seconds for testing
   cfg["bitmunk.catalog.CustomCatalog"]["listingSyncInterval"] = 10;
   cfg["bitmunk.catalog.CustomCatalog"]["uploadListings"] = false;

   // sell config
   cfg["bitmunk.sell.Sell"]["maxUploadRate"] = 0;

   // only load this config once
   ConfigManager* cm = getApp()->getConfigManager();
   if(!cm->hasConfig(config[ConfigManager::ID]->getString()))
   {
      cm->addConfig(config);
      assertNoException();
   }
}

bool Tester::setupNode(Node* node)
{
   bool rval = true;
   ConfigManager* cm = getApp()->getConfigManager();
   ConfigManager* ncm = node->getConfigManager();

   // copy test config as new node's boot config
   {
      Config c;
      c[ConfigManager::ID] = "boot";
      c[ConfigManager::MERGE] = cm->getConfig("main").clone();
      rval = ncm->addConfig(c);
   }

   // add empty main
   if(rval)
   {
      Config c;
      c[ConfigManager::ID] = "__main__";
      c[ConfigManager::PARENT] = "boot";
      c[ConfigManager::GROUP] = "main";
      rval = ncm->addConfig(c);
   }

   // copy raw users config
   if(rval)
   {
      Config c;
      c[ConfigManager::ID] = "bitmunk.test.Tester users";
      c[ConfigManager::GROUP] = "users";
      c[ConfigManager::MERGE] = cm->getConfig("users");
      rval = ncm->addConfig(c);
   }

   return rval;
}

bool Tester::setupPeerNode(Node* node, Config* extraMerge)
{
   bool rval;

   DynamicObject config;
   config[ConfigManager::ID] = "test peer node config";
   config[ConfigManager::GROUP] =
      getApp()->getMetaConfig()["groups"]["main"]->getString();
   Config merge = config[ConfigManager::MERGE];
   merge["node"]["port"] = 19200;
   merge["node"]["bitmunkUrl"] = "http://localhost:19100";
   merge["node"]["secureBitmunkUrl"] = "https://localhost:19100";
   if(extraMerge)
   {
      merge.merge(*extraMerge, false);
   }
   rval = node->getConfigManager()->addConfig(config);

   return rval;
}

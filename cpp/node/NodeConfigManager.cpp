/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk/node/NodeConfigManager.h"

#include <algorithm>
#include <vector>

#include "monarch/data/json/JsonReader.h"
#include "monarch/data/json/JsonWriter.h"
#include "monarch/io/BufferedOutputStream.h"
#include "monarch/io/File.h"
#include "monarch/io/FileInputStream.h"
#include "monarch/io/FileOutputStream.h"
#include "monarch/net/Url.h"
#include "monarch/util/StringTools.h"
#include "bitmunk/node/Node.h"
#include "bitmunk/node/NodeModule.h"

using namespace std;
using namespace bitmunk::common;
using namespace bitmunk::node;
using namespace monarch::config;
using namespace monarch::data;
using namespace monarch::data::json;
using namespace monarch::event;
using namespace monarch::io;
using namespace monarch::kernel;
using namespace monarch::net;
using namespace monarch::rt;
using namespace monarch::util;

// max size for user id string
#define ID_MAX (20 + 1)
// format of user id string
#define ID_FMT "%" PRIu64
// max size for user config id string
#define ID_CFG_MAX (5 + 20 + 1)
// format of user config id string
#define ID_CFG_FMT "user " ID_FMT
// common names
#define MAIN_ID "main"
#define NODE_ID "node"
#define USER_PARENT_ID "users"

#define BITMUNK_CM_EXCEPTION "bitmunk.node.NodeConfigManager"

NodeConfigManager::NodeConfigManager() :
   mMicroKernel(NULL)
{
}

NodeConfigManager::~NodeConfigManager()
{
}

void NodeConfigManager::setMicroKernel(MicroKernel* k)
{
   mMicroKernel = k;
}

MicroKernel* NodeConfigManager::getMicroKernel()
{
   return mMicroKernel;
}

ConfigManager* NodeConfigManager::getConfigManager()
{
   return (mMicroKernel != NULL) ? mMicroKernel->getConfigManager() : NULL;
}

Config NodeConfigManager::getNodeConfig(bool raw)
{
   Config rval(NULL);
   if(raw)
   {
      rval = getConfigManager()->getConfig(NODE_ID, raw);
   }
   else
   {
      Config tmp = getConfigManager()->getConfig(MAIN_ID, raw);
      if(!tmp.isNull() && tmp->hasMember(NODE_ID))
      {
         rval = tmp[NODE_ID];
      }
      else
      {
         ExceptionRef e = new Exception(
            "Could not get node config. Invalid config ID.",
            BITMUNK_CM_EXCEPTION ".InvalidConfigId");
         Exception::push(e);
      }
   }

   return rval;
}

// FIXME: This is a quick hack that uses knowledge of where the Bitmunk app is
// FIXME: storing meta information and how the configs are layered. The
// FIXME: bitmunk config code should be refactored into this class.
//
// FIXME: This code has an issue if the config file has a changed id or
// FIXME: changed contents since it was first loaded.  Just assuming this code
// FIXME: is the only code that manages the file for now.
bool NodeConfigManager::saveSystemUserConfig()
{

   bool rval;
   Config meta = getConfigManager()->getConfig("bitmunk.app meta");
   // get initial value
   const char* suPath =
      meta["app"]["config"]["system user"]["path"]->getString();
   string path;
   rval = expandBitmunkHomePath(suPath, path);
   if(rval)
   {
      // FIXME: do we need locking for load/save/get/remove?
      MO_CAT_DEBUG(BM_NODE_CAT,
         "NodeConfigManager: save system user config: path=%s",
         path.c_str());
      // read
      File file(path.c_str());
      // new config
      Config c;
      if(file->exists())
      {
         FileInputStream fis(file);
         JsonReader reader;
         reader.start(c);
         rval = reader.read(&fis) && reader.finish();
         fis.close();
      }
      else
      {
         c[ConfigManager::VERSION] = BITMUNK_CONFIG_VERSION;
         c[ConfigManager::ID] = "custom system user";
         c[ConfigManager::GROUP] = "system user";
      }
      // update
      if(rval &&
         getConfigManager()->hasConfig(c[ConfigManager::ID]->getString()))
      {
         // get old raw config
         Config active = getConfigManager()->getConfig(
            c[ConfigManager::ID]->getString(), true);
         c.merge(active, false);
      }
      // backup old file
      if(rval && file->exists())
      {
         string backupPath = path + ".bak";
         File backupFile(backupPath.c_str());
         rval = file->rename(backupFile);
      }
      // make sure dirs exist
      rval = rval && file->mkdirs();
      // write
      if(rval)
      {
         FileOutputStream fos(file);
         JsonWriter writer;
         writer.setCompact(false);
         rval = writer.write(c, &fos);
         fos.close();
      }
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Could not save system user config.",
         "bitmunk.node.NodeConfigManager.ConfigError");
      e->getDetails()["path"] = path.c_str();
      Exception::push(e);
   }

   return rval;


}

Config NodeConfigManager::getModuleConfig(const char* moduleName, bool raw)
{
   Config rval(NULL);
   if(raw)
   {
      rval = getConfigManager()->getConfig(moduleName, raw);
   }
   else
   {
      Config tmp = getConfigManager()->getConfig(MAIN_ID, raw);
      if(!tmp.isNull() && tmp->hasMember(moduleName))
      {
         rval = tmp[moduleName];
      }
      else
      {
         ExceptionRef e = new Exception(
            "Could not get module config. Invalid module name.",
            "bitmunk.node.NodeConfigManager.InvalidModuleName");
         e->getDetails()["moduleName"] = moduleName;
         Exception::push(e);
      }
   }

   return rval;
}

/**
 * Get the per-user config file path.  It is named bitmunk.config and found
 * under the path returned from getUserDataPath().
 *
 * @param cm the NodeConfigManager
 * @param userId the user id
 * @param path the output path to assign
 *
 * @return true on success, false if an exception occured.
 */
static bool getUserConfigPath(
   NodeConfigManager* cm, UserId userId, string& path)
{
   bool rval;
   rval = cm->getUserDataPath(userId, path);
   if(rval)
   {
      // append config file name
      path.assign(File::join(path.c_str(), "bitmunk.config"));
   }

   return rval;
}

bool NodeConfigManager::loadUserConfig(UserId userId)
{
   bool rval;
   string path;
   rval = getUserConfigPath(this, userId, path);
   if(rval)
   {
      MO_CAT_DEBUG(BM_NODE_CAT,
         "NodeConfigManager: load user config: id=%" PRIu64 ", path=%s",
         userId, path.c_str());
      // create a temporary loader id just to use include infrastructure
      char cfgId[ID_CFG_MAX];
      snprintf(cfgId, ID_CFG_MAX, ID_CFG_FMT, userId);
      const int loaderIdMax = ID_CFG_MAX + 9;
      char loaderId[loaderIdMax];
      snprintf(loaderId, loaderIdMax, "%s (loader)", cfgId);
      Config loader;
      loader[ConfigManager::ID] = loaderId;
      loader[ConfigManager::VERSION] = BITMUNK_CONFIG_VERSION;
      // optionally load
      Config inc = loader[ConfigManager::INCLUDE][0];

#ifdef WIN32
      if(rval)
      {
         // swap backslashes to standard if on windows
         StringTools::replaceAll(path, "\\", "/");
      }
#endif

      inc["path"] = path.c_str();
      inc["load"] = true;
      inc["optional"] = true;
      rval = getConfigManager()->addConfig(
         loader, true, File::dirname(path.c_str()).c_str());
      // remove temporary loader config
      rval = rval && getConfigManager()->removeConfig(loaderId);
      if(rval)
      {
         // check if anything was loaded, if not, create empty config
         if(!getConfigManager()->hasConfig(cfgId))
         {
            Config config;
            config[ConfigManager::ID] = cfgId;
            config[ConfigManager::PARENT] = USER_PARENT_ID;
            config[ConfigManager::VERSION] = BITMUNK_CONFIG_VERSION;
            rval = getConfigManager()->addConfig(config);
         }
      }
   }
   return rval;
}

bool NodeConfigManager::saveUserConfig(UserId userId)
{
   // FIXME: do we need locking for load/save/get/remove?
   bool rval;
   string path;
   rval = getUserConfigPath(this, userId, path);
   if(rval)
   {
      MO_CAT_DEBUG(BM_NODE_CAT,
         "NodeConfigManager: save user config: id=%" PRIu64 ", path=%s",
         userId, path.c_str());
      // FIXME: how to check if config is "empty" of useful data?
      Config c = getUserConfig(userId, true);
      File file(path.c_str());
      rval = file->mkdirs();
      if(rval)
      {
         FileOutputStream fos(file);
         JsonWriter writer;
         writer.setCompact(false);
         rval = writer.write(c, &fos);
         fos.close();
      }
   }
   return true;
}

Config NodeConfigManager::getUserConfig(UserId userId, bool raw)
{
   char cfgId[ID_CFG_MAX];
   snprintf(cfgId, ID_CFG_MAX, ID_CFG_FMT, userId);
   return getConfigManager()->getConfig(cfgId, raw);
}

bool NodeConfigManager::removeUserConfig(UserId userId)
{
   char cfgId[ID_CFG_MAX];
   snprintf(cfgId, ID_CFG_MAX, ID_CFG_FMT, userId);
   MO_CAT_DEBUG(BM_NODE_CAT,
      "NodeConfigManager: remove user config: id=%" PRIu64 "", userId);
   return getConfigManager()->removeConfig(cfgId);
}

bool NodeConfigManager::isUserConfig(Config& config, UserId userId)
{
   char cfgId[ID_CFG_MAX];
   snprintf(cfgId, ID_CFG_MAX, ID_CFG_FMT, userId);
   return !config.isNull() &&
      config->hasMember(ConfigManager::ID) &&
      strcmp(config[ConfigManager::ID]->getString(), cfgId) == 0 &&
      config->hasMember(ConfigManager::PARENT) &&
      strcmp(config[ConfigManager::PARENT]->getString(), USER_PARENT_ID) == 0;
}

bool NodeConfigManager::isSystemUserConfig(Config& config)
{
   return !config.isNull() &&
      config->hasMember(ConfigManager::GROUP) &&
      strcmp(config[ConfigManager::GROUP]->getString(), "system user") == 0;
}

Config NodeConfigManager::getModuleUserConfig(
   const char* moduleName, UserId userId)
{
   Config rval(NULL);
   Config user = getUserConfig(userId);
   if(!user.isNull() && user->hasMember(moduleName))
   {
      rval = user[moduleName];
   }
   return rval;
}

bool NodeConfigManager::getBitmunkHomePath(string& path)
{
   bool rval;

   Config c = getConfigManager()->getConfig(MAIN_ID);
   // get initial value
   const char* bitmunkHomePath = c["node"]["bitmunkHomePath"]->getString();
   // make sure bitmunkHomePath is user-expanded
   rval = File::expandUser(bitmunkHomePath, path);
   // make sure path is absolute
   if(!rval || !File::isPathAbsolute(path.c_str()))
   {
      ExceptionRef e = new Exception(
         "Could not get absolute bitmunk home path.",
         "bitmunk.node.NodeConfigManager.ConfigError");
      e->getDetails()["bitmunkHomePath"] = bitmunkHomePath;
      Exception::push(e);
      rval = false;
   }

#ifdef WIN32
   if(rval)
   {
      // swap backslashes to standard if on windows
      StringTools::replaceAll(path, "\\", "/");
   }
#endif

   return rval;
}

bool NodeConfigManager::expandBitmunkHomePath(
   const char* path, string& expandedPath)
{
   bool rval = true;

   expandedPath.assign(path);
   if(!File::isPathAbsolute(expandedPath.c_str()))
   {
      // not absolute - try user expansion
      rval = File::expandUser(expandedPath.c_str(), expandedPath);
      if(rval && !File::isPathAbsolute(expandedPath.c_str()))
      {
         // not absolute - prepend bitmunk home path
         string bitmunkHomePath;
         rval = getBitmunkHomePath(bitmunkHomePath);
         if(rval)
         {
            expandedPath.assign(
               File::join(bitmunkHomePath.c_str(), expandedPath.c_str()));
         }
      }
   }

#ifdef WIN32
   if(rval)
   {
      // swap backslashes to standard if on windows
      StringTools::replaceAll(expandedPath, "\\", "/");
   }
#endif

   return rval;
}

bool NodeConfigManager::getUserDataPath(UserId userId, string& path)
{
   bool rval;
   Config c = getConfigManager()->getConfig(MAIN_ID);
   // get initial value
   const char* usersPath = c["node"]["usersPath"]->getString();
   rval = expandBitmunkHomePath(usersPath, path);
   if(rval)
   {
      // append user id path
      char userIdStr[ID_MAX];
      snprintf(userIdStr, ID_MAX, ID_FMT, userId);
      path.assign(File::join(path.c_str(), userIdStr));

#ifdef WIN32
      // swap backslashes to standard if on windows
      StringTools::replaceAll(path, "\\", "/");
#endif
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Could not get absolute user data path.",
         "bitmunk.node.NodeConfigManager.ConfigError");
      e->getDetails()["usersPath"] = usersPath;
      Exception::push(e);
   }

   return rval;
}

bool NodeConfigManager::expandUserDataPath(
   const char* path, UserId userId, string& expandedPath)
{
   bool rval = true;

   expandedPath.assign(path);
   if(!File::isPathAbsolute(expandedPath.c_str()))
   {
      // not absolute - try user expansion
      rval = File::expandUser(expandedPath.c_str(), expandedPath);
      if(rval && !File::isPathAbsolute(expandedPath.c_str()))
      {
         // not absolute - prepend user data path
         string userDataPath;
         rval = getUserDataPath(userId, userDataPath);
         if(rval)
         {
            expandedPath.assign(
               File::join(userDataPath.c_str(), expandedPath.c_str()));
         }
      }
   }

#ifdef WIN32
   if(rval)
   {
      // swap backslashes to standard if on windows
      StringTools::replaceAll(expandedPath, "\\", "/");
   }
#endif

   return rval;
}

bool NodeConfigManager::translateUrlToUserFilePath(
   UserId userId, const char* url, string& filePath)
{
   bool rval = false;

   Url u(url);
   const string& path = u.getSchemeSpecificPart();

   // ensure url scheme is sqlite3
   if((strcmp(u.getScheme().c_str(), "sqlite3") != 0) &&
      (strcmp(u.getScheme().c_str(), "file") != 0))
   {
      ExceptionRef e = new Exception(
         "Could not translate URL, the scheme is not recognized.",
         BITMUNK_CM_EXCEPTION ".BadUrlScheme");
      e->getDetails()["url"] = url;
      e->getDetails()["supportedSchemes"]->append() = "sqlite3";
      e->getDetails()["supportedSchemes"]->append() = "file";
      Exception::set(e);
   }
   // check url format
   else if(path.length() <= 2 || path[0] != '/' || path[1] != '/')
   {
      ExceptionRef e = new Exception(
         "Could not translate URL, the path format is not recognized.",
         BITMUNK_CM_EXCEPTION ".BadUrlFormat");
      e->getDetails()["url"] = url;
      Exception::set(e);
   }
   else
   {
      // skip "//"
      const char* urlPath = path.c_str() + 2;
      // expand url path for user as necessary
      rval = expandUserDataPath(urlPath, userId, filePath);
   }

   return rval;
}

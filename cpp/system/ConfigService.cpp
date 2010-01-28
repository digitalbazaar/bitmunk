/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/system/ConfigService.h"

#include "bitmunk/common/Logging.h"
#include "bitmunk/node/BtpActionDelegate.h"
#include "bitmunk/node/RestResourceHandler.h"
#include "monarch/data/json/JsonWriter.h"
#include "monarch/io/FileOutputStream.h"
#include "monarch/validation/Validation.h"

using namespace std;
using namespace monarch::config;
using namespace monarch::data::json;
using namespace monarch::event;
using namespace monarch::io;
using namespace monarch::rt;
using namespace bitmunk::common;
using namespace bitmunk::protocol;
using namespace bitmunk::node;
using namespace bitmunk::system;
namespace v = monarch::validation;

typedef BtpActionDelegate<ConfigService> Handler;

ConfigService::ConfigService(Node* node, const char* path) :
   NodeService(node, path)
{
}

ConfigService::~ConfigService()
{
}

/**
 * Writes out the node port to a node port file if one is specified in the
 * system module config.
 *
 * @param cm the NodeConfigManager.
 *
 * @return true if no write was necessary or if the write was successful,
 *         false if the write failed.
 */
static bool _writeNodePortFile(NodeConfigManager* cm)
{
   bool rval = true;

   Config nodeCfg = cm->getNodeConfig();
   Config cfg = cm->getModuleConfig("bitmunk.system.System");
   if(!nodeCfg.isNull() && !cfg.isNull() && cfg->hasMember("nodePortFile"))
   {
      File file(cfg["nodePortFile"]->getString());

      MO_CAT_DEBUG(BM_PROTOCOL_CAT,
         "ConfigService writing to node port file: %s",
         file->getAbsolutePath());

      FileOutputStream fos(file);
      const char* port = nodeCfg["port"]->getString();
      rval = fos.write(port, strlen(port));
      fos.close();
   }

   return rval;
}

bool ConfigService::initialize()
{
   bool rval = true;

   // write out node port file if one is specified in the config
   {
      NodeConfigManager* cm = mNode->getConfigManager();
      rval = _writeNodePortFile(cm);
      if(!rval)
      {
         ExceptionRef e = new Exception(
            "Could not write out node port file.",
            "bitmunk.system.ConfigService.InvalidNodePortFile");
         Exception::push(e);
         rval = false;
      }
   }

   // config
   if(rval)
   {
      RestResourceHandlerRef config = new RestResourceHandler();
      addResource("/", config);

      // FIXME: add query param for user ID instead of just config id to
      // shortcut setting user configs?

      // Get a config or subset of a config. If no key path is given the top
      // level Map will be returned. If a key path is given, it represents a
      // traversal into the top level config. The returned value will be a Map
      // with the path value stored under a key with the last key value. This
      // is done to ensure that the top level returned value is a Map since
      // the final value may not be a valid top-level JSON value (Map or Array).
      // For instance, if the top level config is {'key1': {'key2': true}} a
      // request for '.../key1/key2' will return {'key2': true}.
      // GET .../<key1>/<key2>/<key3>?id=<configId>&raw=<true|false>
      {
         Handler* handler = new Handler(
            mNode, this, &ConfigService::getConfig,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         v::ValidatorRef queryValidator = new v::Map(
            "id", new v::Optional(new v::Type(String)),
            "raw", new v::Optional(new v::Regex("^(true|false)$")),
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         config->addHandler(h, BtpMessage::Get, -1, &queryValidator);
      }

      // POST .../?restart=<true|false>
      {
         Handler* handler = new Handler(
            mNode, this, &ConfigService::postConfig,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         v::ValidatorRef qValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            "restart", new v::Optional(new v::Regex("^(true|false)$")),
            NULL);

         // FIXME: better validation based on raw config format?
         // posted configuration must be a map
         v::ValidatorRef validator = new v::Type(Map);

         config->addHandler(h, BtpMessage::Post, 0, &qValidator, &validator);
      }
   }

   return rval;
}

void ConfigService::cleanup()
{
   // remove resources
   removeResource("/");
}

bool ConfigService::getConfig(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = true;

   // get configuration ID, raw setting
   DynamicObject vars;
   action->getResourceQuery(vars);
   bool raw = vars["raw"]->getBoolean();
   Config config(NULL);
   if(vars->hasMember("id"))
   {
      // user config ID from url query
      config = mNode->getConfigManager()->getConfig(
         vars["id"]->getString(), raw);
   }
   else
   {
      // default to user config
      config = mNode->getConfigManager()->getUserConfig(
         action->getInMessage()->getUserId(), raw);
   }

   // check configuration to ensure it is valid
   if(config.isNull())
   {
      // invalid config ID
      rval = false;
   }
   else
   {
      // check for specific config keys (only for raw=false)
      DynamicObject keys;
      if(action->getResourceParams(keys))
      {
         Config tmp(NULL);
         DynamicObjectIterator i = keys.getIterator();
         while(rval && i->hasNext())
         {
            DynamicObject& key = i->next();
            if(!config->hasMember(key->getString()))
            {
               // config value does not exist
               action->getResponse()->getHeader()->setStatus(404, "Not Found");
               ExceptionRef e = new Exception(
                  "Config key does not exist.",
                  "bitmunk.system.ConfigService.InvalidKey", 404);
               Exception::set(e);
               rval = false;
            }
            else if(!i->hasNext())
            {
               // return specific config
               out[key->getString()] = config[key->getString()];
            }
            else
            {
               // move to next config value
               tmp = config[key->getString()];
               config = tmp;
            }
         }
      }
      else
      {
         // return entire config
         out = config;
      }
   }

   return rval;
}

bool ConfigService::postConfig(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = true;

   // FIXME:
   // - post config changed event (diff for user vs other values?)

   // get resource variables
   DynamicObject vars;
   action->getResourceQuery(vars);
   bool restart = vars["restart"]->getBoolean();

   NodeConfigManager* cm = mNode->getConfigManager();

   // set or add config
   if(!in.isNull() && in->hasMember(ConfigManager::ID) &&
      cm->hasConfig(in[ConfigManager::ID]->getString()))
   {
      rval = cm->setConfig(in);
   }
   else
   {
      rval = cm->addConfig(in);
   }

   if(rval)
   {
      UserId userId = action->getInMessage()->getUserId();
      // save user and system user configs
      if(NodeConfigManager::isUserConfig(in, userId))
      {
         rval = cm->saveUserConfig(userId);
      }
      else if(NodeConfigManager::isSystemUserConfig(in))
      {
         rval = cm->saveSystemUserConfig();
         if(rval)
         {
            rval = _writeNodePortFile(cm);
            if(!rval)
            {
               ExceptionRef e = new Exception(
                  "System user config saved, but node port file could not "
                  "be updated.",
                  "bitmunk.system.ConfigService.InvalidNodePortFile");
               Exception::push(e);
            }
         }
      }

      // restart if necessary
      if(rval && restart)
      {
         // send result first, then restart
         action->sendResult();

         Event e;
         e["type"] = "bitmunk.node.Node.restart";
         mNode->getEventController()->schedule(e);
      }
   }
   out.setNull();

   return rval;
}

/*
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include "bitmunk/node/NodeEventHandler.h"

#include "bitmunk/nodemodule/NodeModule.h"
#include "monarch/data/json/JsonWriter.h"

using namespace std;
using namespace monarch::config;
using namespace monarch::data::json;
using namespace monarch::event;
using namespace monarch::logging;
using namespace monarch::rt;
using namespace bitmunk::node;

NodeEventHandler::NodeEventHandler(Node* node) :
   mNode(node)
{
}

NodeEventHandler::~NodeEventHandler()
{
}

/**
 * Registers or unregisters event handlers.
 *
 * @param neh the NodeEventHandler.
 * @param ec the EventController to use.
 * @param reg true to register, false to unregister.
 */
static bool _processEventHandlers(NodeEventHandler* neh, Node* node, bool reg)
{
   bool rval = true;

   EventController* ec = node->getEventController();
   Config cfg = node->getConfigManager()->getNodeConfig();
   Config& events = cfg["events"];

   if(reg)
   {
      // register to receive top-level event
      ec->registerObserver(neh, "*");
      MO_CAT_INFO(BM_NODE_CAT, "Registered event: *");
   }
   else
   {
      // unregister from top-level event
      ec->unregisterObserver(neh, "*");
   }

   // log events
   ConfigIterator i = events.getIterator();
   while(i->hasNext())
   {
      DynamicObject& handlers = i->next();
      const char* name = i->getName();

      // only print event registration if it has handlers
      if(handlers->length() > 0)
      {
         if(reg)
         {
            MO_CAT_INFO(BM_NODE_CAT, "Registered event: %s", name);
         }
         else
         {
            MO_CAT_INFO(BM_NODE_CAT, "Unregistered event: %s", name);
         }
      }
   }

   return rval;
}

bool NodeEventHandler::initialize()
{
   bool rval = false;

   // start listening for config changes
   mNode->getConfigManager()->getConfigManager()->setConfigChangeListener(this);

   // initialize event handlers
   MO_CAT_INFO(BM_NODE_CAT, "Initializing event handlers.");
   rval = _processEventHandlers(this, mNode, true);
   MO_CAT_INFO(BM_NODE_CAT, "Event handlers initialized.");

   return rval;
}

void NodeEventHandler::cleanup()
{
   // clean up event handlers
   MO_CAT_INFO(BM_NODE_CAT, "Cleaning up event handlers.");
   _processEventHandlers(this, mNode, false);
   MO_CAT_INFO(BM_NODE_CAT, "Event handlers cleaned up.");

   // stop listening for config changes
   mNode->getConfigManager()->getConfigManager()->setConfigChangeListener(NULL);
}

void NodeEventHandler::handleEvent(
   Event& e, const char* handler, Config* options)
{
   Config cfg = mNode->getConfigManager()->getNodeConfig();

   // get handler info and type
   Config handlerInfo;
   const char* hType = "";
   if(cfg["handlers"]->hasMember(handler))
   {
      handlerInfo = cfg["handlers"][handler];
      // merge in options if present
      if(options != NULL)
      {
         handlerInfo.merge(*options, true);
      }
      // handler must at least have a type
      if(handlerInfo->hasMember("type"))
      {
         hType = handlerInfo["type"]->getString();
      }
      else
      {
         MO_CAT_WARNING(BM_NODE_CAT, "No handler type found: %s", handler);
      }
   }
   else
   {
      MO_CAT_WARNING(BM_NODE_CAT, "No handler found: %s", handler);
   }

   // Array of sub-handlers
   if(strcmp(hType, "bitmunk.handler.multi") == 0)
   {
      ConfigIterator i = handlerInfo["handlers"].getIterator();
      while(i->hasNext())
      {
         const char* name = i->next()->getString();
         MO_CAT_DEBUG(BM_NODE_CAT, "multi handler: %s", name);
         handleEvent(e, name);
      }
   }
   // Log event
   else if(strcmp(hType, "bitmunk.handler.log") == 0)
   {
      MO_CAT_DEBUG(BM_NODE_CAT, "log handler");

      // default to info
      const char* levelStr = "info";
      if(handlerInfo->hasMember("level"))
      {
         levelStr = handlerInfo["level"]->getString();
      }

      // convert to Level
      Logger::Level level;
      if(!Logger::stringToLevel(levelStr, level))
      {
         // FIXME: throw exception instead of defaulting to info?
         level = Logger::Info;
      }

      const char* detail = handlerInfo["detail"]->getString();
      const char* sep = "";
      string details;
      if(strcmp(detail, "low") == 0)
      {
         // just the name, no details
      }
      else if(strcmp(detail, "medium") == 0)
      {
         // specific fields
         if(handlerInfo->hasMember("fields"))
         {
            ConfigIterator i = handlerInfo["fields"].getIterator();
            while(i->hasNext())
            {
               const char* name = i->next()->getString();
               if(e->hasMember(name))
               {
                  details.append(", ");
                  details.append(name);
                  details.append("=");
                  details.append(e[name]->getString());
               }
            }
         }
      }
      else if(strcmp(detail, "high") == 0)
      {
         // full event info
         sep = ", ";
         bool compact = false;
         if(handlerInfo->hasMember("compact"))
         {
            compact = handlerInfo["compact"]->getBoolean();
         }
         details = JsonWriter::writeToString(e, compact);
      }
      else
      {
         // FIXME: throw exception on unknown detail level?
      }

      MO_CAT_LEVEL_LOG(BM_NODE_CAT, level, "Event: %s%s%s",
         e["type"]->getString(), sep, details.c_str());
   }
   // Record event
   else if(strcmp(hType, "bitmunk.handler.stats") == 0)
   {
      MO_CAT_DEBUG(BM_NODE_CAT, "stats handler (unimplemented)");
   }
   // Auto-Login event
   else if(strcmp(hType, "bitmunk.handler.login") == 0)
   {
      MO_CAT_DEBUG(BM_NODE_CAT, "auto-login handler");

      if(cfg["login"]["auto"]->getBoolean())
      {
         MO_CAT_INFO(BM_NODE_CAT, "Auto-login enabled.");

         const char* user = cfg["login"]["username"]->getString();
         const char* password = cfg["login"]["password"]->getString();
         bool pass = mNode->login(user, password);
         if(pass)
         {
            MO_CAT_INFO(BM_NODE_CAT, "Auto-login complete: user=%s.", user);
         }
         else
         {
            MO_CAT_ERROR(BM_NODE_CAT,
               "Auto-login failed: user=%s, shutting down..., %s",
               user, JsonWriter::writeToString(
                  Exception::getAsDynamicObject()).c_str());

            // schedule shutdown event
            Event event;
            event["type"] = "bitmunk.node.Node.shutdown";
            mNode->getEventController()->schedule(event);
         }
      }
      else
      {
         MO_CAT_INFO(BM_NODE_CAT, "Auto-login disabled.");
      }
   }
   // Shutdown event
   else if(strcmp(hType, "bitmunk.handler.shutdown") == 0)
   {
      MO_CAT_DEBUG(BM_NODE_CAT, "shutdown handler");

      // stop node
      mNode->stop();

      // schedule shutdown event
      Event event;
      event["type"] = "monarch.kernel.MicroKernel.shutdown";
      mNode->getEventController()->schedule(event);
   }
   // Unknown event
   else if(strcmp(hType, "") != 0)
   {
      MO_CAT_WARNING(BM_NODE_CAT, "Unknown event handler type: %s", hType);
   }
}

void NodeEventHandler::eventOccurred(Event& e)
{
   const char* eType = e["type"]->getString();

   MO_CAT_DEBUG(BM_NODE_CAT, "Got event: %s", eType);

   Config cfg = mNode->getConfigManager()->getNodeConfig();
   cfg["events"][eType]->setType(Array);
   Config& eventHandlers = cfg["events"][eType];

   // loop through all handlers
   ConfigIterator i = eventHandlers.getIterator();
   while(i->hasNext())
   {
      Config& next = i->next();
      const char* name = "";
      Config* options = NULL;
      DynamicObjectType type = next->getType();
      bool valid = true;

      if(type == String)
      {
         // handler is simple string
         name = next->getString();
      }
      else if(type == Map)
      {
         // handler is map with "handler" and optional "options"
         if(next->hasMember("handler"))
         {
            name = next["handler"]->getString();
         }
         else
         {
            MO_CAT_WARNING(BM_NODE_CAT,
               "Invalid event handler (no name): %s", eType);
            valid = false;
         }

         if(next->hasMember("options"))
         {
            options = &next["options"];
            if((*options)->getType() != Map)
            {
               MO_CAT_WARNING(BM_NODE_CAT,
                  "Invalid event handler (options not a Map): %s", eType);
               valid = false;
            }
         }
      }
      else
      {
         MO_CAT_WARNING(BM_NODE_CAT,
            "Invalid event handler (bad type): %s", eType);
         valid = false;
      }

      if(valid)
      {
         MO_CAT_INFO(BM_NODE_CAT, "handler%s: %s",
            options != NULL ? " (+options)" : "",
            name);
         handleEvent(e, name, options);
      }
   }
}

void NodeEventHandler::configAdded(
   ConfigManager* cm, ConfigManager::ConfigId id)
{
   MO_CAT_DEBUG(BM_NODE_CAT, "config change, added '%s'", id);

   Event e;
   e["type"] = "monarch.config.Config.added";
   e["details"]["configId"] = id;
   // add user ID if the config ID is for a user
   if(strncmp(id, "user ", 5) == 0)
   {
      e["details"]["userId"] = strtoull(id + 5, NULL, 10);
   }
   mNode->getEventController()->schedule(e);
}

void NodeEventHandler::configChanged(
   ConfigManager* cm, ConfigManager::ConfigId id, Config& diff)
{
   MO_CAT_DEBUG(BM_NODE_CAT,
      "config change, changed '%s', diff: %s", id,
      JsonWriter::writeToString(diff).c_str());

   Event e;
   e["type"] = "monarch.config.Config.changed";
   e["details"]["configId"] = id;
   e["details"]["config"] = diff;
   // add user ID if the config ID is for a user
   if(strncmp(id, "user ", 5) == 0)
   {
      e["details"]["userId"] = strtoull(id + 5, NULL, 10);
   }
   mNode->getEventController()->schedule(e);
}

void NodeEventHandler::configRemoved(
   ConfigManager* cm, ConfigManager::ConfigId id)
{
   MO_CAT_DEBUG(BM_NODE_CAT, "config change, removed '%s'", id);

   Event e;
   e["type"] = "monarch.config.Config.removed";
   e["details"]["configId"] = id;
   // add user ID if the config ID is for a user
   if(strncmp(id, "user ", 5) == 0)
   {
      e["details"]["userId"] = strtoull(id + 5, NULL, 10);
   }
   mNode->getEventController()->schedule(e);
}

void NodeEventHandler::configCleared(ConfigManager* cm)
{
   MO_CAT_DEBUG(BM_NODE_CAT, "config change, configs cleared.");

   Event e;
   e["type"] = "monarch.config.Config.cleared";
   mNode->getEventController()->schedule(e);
}

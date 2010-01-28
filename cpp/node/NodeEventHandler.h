/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_node_NodeEventHandler_H
#define bitmunk_node_NodeEventHandler_H

#include "monarch/config/ConfigChangeListener.h"
#include "monarch/event/EventController.h"
#include "monarch/event/Observer.h"

namespace bitmunk
{
namespace node
{

class Node;

/**
 * The NodeEventHandler is responsible for receiving and handling generic
 * events and configuration changes.
 *
 * @author Dave Longley
 */
class NodeEventHandler :
public monarch::event::Observer,
public monarch::config::ConfigChangeListener
{
protected:
   /**
    * The associated Node.
    */
   Node* mNode;

public:
   /**
    * Creates a new NodeEventHandler.
    *
    * @param node the associated Node.
    */
   NodeEventHandler(Node* node);

   /**
    * Destructs this NodeEventHandler.
    */
   virtual ~NodeEventHandler();

   /**
    * Initializes event handlers.
    *
    * @return true on success, false and exception on failure.
    */
   virtual bool initialize();

   /**
    * Cleans up event handlers.
    */
   virtual void cleanup();

   /**
    * Handles an event.
    *
    * @param e the event.
    * @param handler the handler.
    * @param options handler options or NULL.
    */
   virtual void handleEvent(
      monarch::event::Event& e,
      const char* handler,
      monarch::config::Config* options = NULL);

   /**
    * Observer for handled events. Dispatches to handleEvent().
    *
    * @param e the event.
    */
   virtual void eventOccurred(monarch::event::Event& e);

   /**
    * Called by a ConfigManager when a config is added.
    *
    * @param cm the ConfigManager.
    * @param id the ID of the config.
    */
   virtual void configAdded(
      monarch::config::ConfigManager* cm,
      monarch::config::ConfigManager::ConfigId id);

   /**
    * Called by a ConfigManager when a config is changed.
    *
    * @param cm the ConfigManager.
    * @param id the ID of the config.
    * @param diff the diff between the old config and the new one.
    */
   virtual void configChanged(
      monarch::config::ConfigManager* cm,
      monarch::config::ConfigManager::ConfigId id,
      monarch::config::Config& diff);

   /**
    * Called by a ConfigManager when a config is removed.
    *
    * @param cm the ConfigManager.
    * @param id the ID of the config.
    */
   virtual void configRemoved(
      monarch::config::ConfigManager* cm,
      monarch::config::ConfigManager::ConfigId id);

   /**
    * Called by a ConfigManager when all configs are cleared.
    *
    * @param cm the ConfigManager.
    */
   virtual void configCleared(monarch::config::ConfigManager* cm);
};

} // end namespace node
} // end namespace bitmunk
#endif

/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_apps_Bitmunk_h
#define bitmunk_apps_Bitmunk_h

#include "monarch/app/AppPlugin.h"
#include "monarch/event/ObserverDelegate.h"
#include "monarch/kernel/MicroKernel.h"

#include <vector>

namespace bitmunk
{
namespace apps
{

/**
 * Top-level Bitmunk AppPlugin.
 *
 * Author: David I. Lehn
 */
class Bitmunk : public monarch::app::AppPlugin
{
protected:
   /**
    * The app state types.
    */
   enum State
   {
      // Node is stopped.
      Stopped,
      // In the process of starting the node.
      Starting,
      // Node has been started and is running.
      Running,
      // In the process of restarting the node.
      Restarting,
      // In the process of stopping the node.
      Stopping
   };

   /**
    * Current app state.
    */
   State mState;

   /**
    * Main application MicroKernel.
    */
   monarch::kernel::MicroKernel* mMicroKernel;

   /**
    * Observer for Node restart event.
    */
   monarch::event::ObserverRef mNodeRestartObserver;

   /**
    * Observer for Node shutdown event.
    */
   monarch::event::ObserverRef mNodeShutdownObserver;

public:
   /**
    * Create a Bitmunk instance.
    *
    * @param k MicroKernel that started this AppPlugin.
    */
   Bitmunk(monarch::kernel::MicroKernel* k);

   /**
    * Deconstruct this Bitmunk instance.
    */
   virtual ~Bitmunk();

   /**
    * {@inheritDoc}
    */
   virtual bool didAddToApp(monarch::app::App* app);

   /**
    * {@inheritDoc}
    */
   virtual monarch::rt::DynamicObject getWaitEvents();

   /**
    * Setup command line option config.
    *
    * @return true on success, false on failure and exception set
    */
   virtual bool initMetaConfig(monarch::config::Config& meta);

   /**
    * Get a specification of the command line paramters.
    *
    * @return the command line spec
    */
   virtual monarch::rt::DynamicObject getCommandLineSpecs();

   /**
    * Setup for config loading.
    *
    * @return true on success, false on failure and exception set
    */
   virtual bool willLoadConfigs();

   /**
    * Load Bitmunk specific configs.
    *
    * @return true on success, false on failure and exception set
    */
   virtual bool didLoadConfigs();

   /**
    * Get a list of module path the kernel should load to start a node.
    *
    * @return a list of modules to load.
    */
   virtual monarch::io::FileList getModulePaths();

   /**
    * Load node modules and setup related events.
    *
    * @return true on success, false on failure and exception set
    */
   virtual bool startNode();

   /**
    * Stop node, unload node modules, and teardown related events.
    *
    * @param restarting true if attempting a restart
    *
    * @return true on success, false on failure and exception set
    */
   virtual bool stopNode(bool restarting = false);

   /**
    * Restart a node with stopNode() and startNode().
    *
    * @return true on success, false on failure and exception set
    */
   virtual bool restartNode();

   /**
    * Event handler for restart events.
    */
   virtual void restartNodeHandler(monarch::event::Event& e);

   /**
    * Event handler for shutdown events.
    */
   virtual void shutdownNodeHandler(monarch::event::Event& e);

   /**
    * Run main application.
    */
   virtual bool run();
};

} // end namespace apps
} // end namespace bitmunk

#endif

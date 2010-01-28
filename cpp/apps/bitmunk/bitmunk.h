/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_H
#define bitmunk_H

#include "monarch/app/AppPlugin.h"
#include "monarch/event/Observer.h"
#include "monarch/kernel/MicroKernel.h"

#include <vector>

namespace bitmunk
{

/**
 * Bitmunk client.
 *
 * Author: David I. Lehn
 */
class BitmunkApp : public monarch::app::AppPlugin
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
   monarch::kernel::MicroKernel* mKernel;

   /**
    * The NodeConfigManager.
    */
   bitmunk::node::NodeConfigManager mNodeConfigManager;

   /**
    * Package config control.
    */
   monarch::config::Config mPackageConfig;

   /**
    * System config control.
    */
   monarch::config::Config mSystemConfig;

   /**
    * System user config control.
    */
   monarch::config::Config mSystemUserConfig;

   /**
    * List of extra config files to load.
    */
   monarch::rt::DynamicObject mExtraConfigs;

   /**
    * Init ConfigManager from mNode.
    *
    * @return true on succes, false and exception set on error
    */
   virtual bool initConfigManager();

   /**
    * No-op. NodeConfigManager is cleaned up in the destructor.
    */
   virtual void cleanupConfigManager();

   /**
    * Wait for a node completion event.
    *
    * @return true on success, false and exception on failure.
    */
   virtual bool waitForNode();

public:
   /**
    * Create a BitmunkApp instance.
    */
   BitmunkApp();

   /**
    * Deconstruct this BitmunkApp instance.
    */
   virtual ~BitmunkApp();

   /**
    * {@inheritDoc}
    */
   virtual bool didAddToApp(monarch::app::App* app);

   /**
    * Gets this app's ConfigManager.
    *
    * @return the ConfigManager for the node for this app.
    */
   virtual monarch::config::ConfigManager* getConfigManager();

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
    * Setup default command line options.
    *
    * @param args read-only vector of command line arguments.
    *
    * @return true on success, false on failure and exception set
    */
   virtual bool willParseCommandLine(std::vector<const char*>* args);

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
    * Run main application.
    */
   virtual bool run();
};

} // end namespace bitmunk

#endif

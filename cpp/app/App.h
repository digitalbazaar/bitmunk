/*
 * Copyright (c) 2007-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_node_App_H
#define bitmunk_node_App_H

#include "monarch/app/AppPlugin.h"

namespace bitmunk
{
namespace node
{

/**
 * monarch::app::AppPlugin with Bitmunk specific features.
 *
 * Author: David I. Lehn
 */
class App : public monarch::app::AppPlugin
{
protected:
   /**
    * Provide same functionality as monarch::app::App::makeMetaConfig() except
    * use the default Bitmunk version.
    *
    * @param meta the meta config.
    * @param id the config id.
    * @param groupId the group id.
    *
    * @return true on success, false and exception on failure.
    */
   static monarch::config::Config makeMetaConfig(
      monarch::config::Config& meta,
      const char* id = NULL,
      const char* groupId = NULL);

   /**
    * Prepare to initialize Bitmunk meta config.
    *
    * Subclasses should call the superclass method.
    *
    * @param meta the meta config.
    *
    * @return true on success, false and exception on failure.
    */

   virtual bool willInitMetaConfig(monarch::config::Config& meta);
   /**
    * Initialize Bitmunk meta config.
    *
    * Subclasses should call the superclass method.
    *
    * @param meta the meta config.
    *
    * @return true on success, false and exception on failure.
    */
   virtual bool initMetaConfig(monarch::config::Config& meta);

public:
   /**
    * Create an App instance.
    */
   App();

   /**
    * Deconstruct this App instance.
    */
   virtual ~App();

   /**
    * {@inheritDoc}
    */
   virtual bool initializeLogging();

   /**
    * {@inheritDoc}
    */
   virtual bool cleanupLogging();
};

/**
 * Macro to create a simple main() that creates an app, adds a plugin to it,
 * and calls App::main().
 *
 * @param appPluginClassName class name of an AppPlugin subclass.
 */
#define BM_APP_PLUGIN_MAIN(appPluginClassName)                     \
int main(int argc, const char* argv[])                             \
{                                                                  \
   monarch::app::App app;                                          \
   std::vector<monarch::app::AppPluginRef> plugins;                \
   monarch::app::AppPluginRef bitmunk = new bitmunk::node::App;    \
   plugins.push_back(bitmunk);                                     \
   monarch::app::AppPluginRef plugin = new appPluginClassName;     \
   plugins.push_back(plugin);                                      \
   return app.main(argc, argv, &plugins);                          \
}

} // end namespace node
} // end namespace bitmunk

#endif

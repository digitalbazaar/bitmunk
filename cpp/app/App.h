/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_app_App_H
#define bitmunk_app_App_H

#include "monarch/app/AppPlugin.h"
#include "monarch/app/AppPluginFactory.h"

namespace bitmunk
{
namespace app
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
    * {@inheritDoc}
    */
   virtual bool initConfigManager();

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

} // end namespace app
} // end namespace bitmunk

#endif

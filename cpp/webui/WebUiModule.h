/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_webui_WebUiModule_H
#define bitmunk_webui_WebUiModule_H

#include "bitmunk/node/BitmunkModule.h"
#include "bitmunk/webui/FlashPolicyServer.h"
#include "bitmunk/webui/SessionManager.h"

// module logging category
extern monarch::logging::Category* BM_WEBUI_CAT;

namespace bitmunk
{
namespace webui
{

/**
 * A WebUiModule provides a web-based user interface for a Bitmunk node.
 */
class WebUiModule : public bitmunk::node::BitmunkModule
{
protected:
   /**
    * The SessionManager used by this module.
    */
   SessionManager* mSessionManager;

   /**
    * The flash policy server.
    */
   FlashPolicyServer mFlashPolicyServer;

   /**
    * A map of web plugin ID to web plugin information.
    */
   monarch::rt::DynamicObject mPluginInfo;

   /**
    * The ID of main web plugin.
    */
   char* mMainPluginId;

public:
   /**
    * Creates a new WebUiModule.
    */
   WebUiModule();

   /**
    * Destructs this WebUiModule.
    */
   virtual ~WebUiModule();

   /**
    * Adds additional dependency information. This includes dependencies
    * beyond the Bitmunk Node module dependencies.
    *
    * @param depInfo the dependency information to update.
    */
   virtual void addDependencyInfo(monarch::rt::DynamicObject& depInfo);

   /**
    * Initializes this Module with the passed Node.
    *
    * @param node the Node.
    *
    * @return true if initialized, false if an Exception occurred.
    */
   virtual bool initialize(bitmunk::node::Node* node);

   /**
    * Cleans up this Module just prior to its unloading.
    *
    * @param node the Node.
    */
   virtual void cleanup(bitmunk::node::Node* node);

   /**
    * Gets the API for this NodeModule.
    *
    * @param node the Node.
    *
    * @return the API for this NodeModule.
    */
   virtual monarch::kernel::MicroKernelModuleApi* getApi(
      bitmunk::node::Node* node);

protected:
   /**
    * Load all web plugins based on global configuration information.
    *
    * @param node the Node.
    *
    * @return true if no error, false if an exception occurred.
    */
   virtual bool loadPlugins(bitmunk::node::Node* node);
};

} // end namespace webui
} // end namespace bitmunk
#endif

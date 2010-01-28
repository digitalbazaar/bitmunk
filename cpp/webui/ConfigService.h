/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_webui_ConfigService_H
#define bitmunk_webui_ConfigService_H

#include "bitmunk/protocol/BtpService.h"
#include "bitmunk/webui/SessionManager.h"
#include "bitmunk/webui/FrontendService.h"

namespace bitmunk
{
namespace webui
{

/**
 * ConfigService handles web ui configuration.
 * 
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 */
class ConfigService : public bitmunk::protocol::BtpService
{
protected:
   /**
    * The associated SessionManager.
    */
   bitmunk::webui::SessionManager* mSessionManager;
   
   /**
    * A map of web plugin ID to web plugin information.
    */
   monarch::rt::DynamicObject mPluginInfo;
   
public:
   /**
    * Creates a new ConfigService.
    * 
    * @param sm the associated SessionManager.
    * @param ii the web plugin information.
    * @param path the path this servicer handles requests for.
    */
   ConfigService(
      SessionManager* sm, monarch::rt::DynamicObject& pi, const char* path);
   
   /**
    * Destructs this ConfigService.
    */
   virtual ~ConfigService();
   
   /**
    * Initializes this BtpService.
    * 
    * @return true if initialized, false if an Exception occurred.
    */
   virtual bool initialize();
   
   /**
    * Cleans up this BtpService.
    * 
    * Must be called after initialize() regardless of its return value.
    */
   virtual void cleanup();
   
   /**
    * Get the plugins configuration.
    * 
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool getPlugins(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);
   
   /**
    * Get the flash configuration.
    * 
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool getFlashConfig(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);
};

} // end namespace webui
} // end namespace bitmunk
#endif

/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_webui_FrontendService_H
#define bitmunk_webui_FrontendService_H

#include "monarch/io/FileList.h"
#include "bitmunk/protocol/BtpService.h"
#include "bitmunk/webui/SessionManager.h"

namespace bitmunk
{
namespace webui
{

/**
 * The frontend service is responsible for serving up static page content from
 * disk. It is a very limited on-disk HTTP server.
 * 
 * @author Manu Sporny
 */
class FrontendService : public bitmunk::protocol::BtpService
{
protected:
   /**
    * The associated SessionManager.
    */
   bitmunk::webui::SessionManager* mSessionManager;
   
   /**
    * Map of file extensions to MIME content types.
    */
   monarch::rt::DynamicObject mContentTypes;
   
   /**
    * A map of web plugin ID to web plugin information.
    */
   monarch::rt::DynamicObject mPluginInfo;
   
   /**
    * The ID of main web plugin.
    */
   const char* mMainPluginId;
   
public:
   /**
    * Creates a new FrontendService.
    * 
    * @param sm the associated SessionManager.
    * @param pi the web plugin information.
    * @param mainPluginId the main plugin ID.
    * @param path the path this servicer handles requests for.
    */
   FrontendService(
      SessionManager* sm,
      monarch::rt::DynamicObject& pi, const char* mainPluginId,
      const char* path);
   
   /**
    * Destructs this FrontendService.
    */
   virtual ~FrontendService();
   
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
    * Handles serving static content from the main plugin.
    * 
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool serveMainContent(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);
   
   /**
    * Handles serving static content from an plugin.
    * 
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool servePluginContent(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);
   
protected:
   /**
    * Handles serving static content from an plugin.
    * 
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    * @param inf the plugin id.
    * @param path the relative resource path.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool serve(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out,
      const char* inf, const char* path);
};

} // end namespace webui
} // end namespace bitmunk
#endif

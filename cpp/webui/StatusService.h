/*
 * Copyright (c) 2007-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_webui_StatusService_H
#define bitmunk_webui_StatusService_H

#include "bitmunk/protocol/BtpService.h"
#include "bitmunk/webui/SessionManager.h"

namespace bitmunk
{
namespace webui
{

/**
 * The status service responds with the current state of the Bitmunk 
 * application. Typically, a controlling UI will query the application for it's 
 * state. If no response is received by the UI, it should be assumed that the 
 * bitmunk application is down. If the Bitmunk application is active, it will
 * return the current state of the application.
 * 
 * @author Manu Sporny
 */
class StatusService : public bitmunk::protocol::BtpService
{
protected:
   /**
    * The associated SessionManager.
    */
   bitmunk::webui::SessionManager* mSessionManager;
   
public:
   /**
    * Creates a new status service.
    * 
    * @param sm the associated SessionManager.
    * @param path the path this servicer handles requests for.
    */
   StatusService(SessionManager* sm, const char* path);
   
   /**
    * Destructs this status service.
    */
   virtual ~StatusService();
   
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
    * Handles a status check action. The returned DynamicObject has the
    * following fields:
    * 
    * online: true|false
    * 
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool status(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);
};

} // end namespace webui
} // end namespace bitmunk
#endif

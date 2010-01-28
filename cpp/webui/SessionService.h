/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_webui_SessionService_H
#define bitmunk_webui_SessionService_H

#include "bitmunk/protocol/BtpService.h"
#include "bitmunk/node/Node.h"
#include "bitmunk/webui/SessionManager.h"

namespace bitmunk
{
namespace webui
{

/**
 * A SessionService allows a user to start or end a session with a Bitmunk
 * node.
 * 
 * When a session is started, a session ID is returned to the user. This
 * session ID must be sent as a Cookie header when communicating with the
 * BtpProxyService. Most browsers will do this automatically.
 * 
 * @author Dave Longley
 */
class SessionService : public bitmunk::protocol::BtpService
{
protected:
   /**
    * The session manager.
    */
   SessionManager* mSessionManager;
   
public:
   /**
    * Creates a new SessionService.
    * 
    * @param sm the SessionManager used to manage sessions.
    * @param path the path this servicer handles requests for.
    */
   SessionService(SessionManager* sm, const char* path);
   
   /**
    * Destructs this SessionService.
    */
   virtual ~SessionService();
   
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
    * Starts a new session.
    * 
    * HTTP equivalent: POST .../login
    * 
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool createSession(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);
   
   /**
    * Ends an existing session.
    * 
    * HTTP equivalent: POST .../logout
    * 
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool deleteSession(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);
   
   /**
    * Sets the current session's userdata.
    * 
    * HTTP equivalent: POST .../session/userdata
    * 
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool setSessionUserdata(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);
   
   /**
    * Gets the current session's userdata.
    * 
    * HTTP equivalent: GET .../session/userdata
    * 
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool getSessionUserdata(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);
   
   /**
    * Gets the full access control list.
    * 
    * HTTP equivalent: GET .../session/access/users
    * 
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool getFullAccessControlList(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);
   
   /**
    * Gets the access control list for a given user ID.
    * 
    * HTTP equivalent: GET .../session/access/users/<userId>
    * 
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool getAccessControlList(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);
   
   /**
    * Grants a list of access control entries.
    * 
    * HTTP equivalent: POST .../session/access/grant
    * 
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool grantAccess(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);
   
   /**
    * Revokes a list of access control entries.
    * 
    * HTTP equivalent: POST .../session/access/revoke
    * 
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool revokeAccess(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);
};

} // end namespace webui
} // end namespace bitmunk
#endif

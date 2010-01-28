/*
 * Copyright (c) 2007-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_webui_SessionManager_H
#define bitmunk_webui_SessionManager_H

#include "bitmunk/node/Node.h"
#include "bitmunk/webui/SessionDatabase.h"
#include "monarch/rt/ExclusiveLock.h"
#include "monarch/util/StringTools.h"

namespace bitmunk
{
namespace webui
{

/**
 * The SessionManager stores and validates user sessions. A user session is
 * created when a user successfully validates their identity. A valid session
 * allows a user to modify the behavior of the bitmunk application.
 *
 * @author Dave Longley
 */
class SessionManager
{
protected:
   /**
    * The Bitmunk Node this SessionManager runs on.
    */
   bitmunk::node::Node* mNode;

   /**
    * SessionData contains the session's owner (a user ID), the IP address
    * of the owner, its creation time (in milliseconds), whether or not its
    * valid, and userdata.
    */
   struct SessionData
   {
      std::string id;
      bitmunk::common::UserId userId;
      std::string username;
      std::string ip;
      uint64_t time;
      bool valid;
      monarch::rt::DynamicObject userdata;
   };

   /**
    * A map of session ID to session data.
    */
   typedef std::map<const char*, SessionData, monarch::util::StringComparator>
      SessionMap;
   SessionMap mSessions;

   /**
    * A lock for modifying sessions.
    */
   monarch::rt::ExclusiveLock mSessionLock;

   /**
    * The session database.
    */
   SessionDatabase mSessionDatabase;

public:
   /**
    * Creates a new SessionManager that uses the passed Node.
    *
    * @param node the associated Node.
    */
   SessionManager(bitmunk::node::Node* node);

   /**
    * Destructs this SessionManager.
    */
   virtual ~SessionManager();

   /**
    * Initializes this SessionManager.
    *
    * @return true if successful, false if not.
    */
   virtual bool initialize();

   /**
    * Creates a new session, logging a user into the associated node if
    * necessary, and returns the session ID. Any existing session for the
    * given username and internet address will be overwritten.
    *
    * @param header the HttpHeader to set new cookies on.
    * @param username the username of the user to create a session for.
    * @param password the password of the user.
    * @param ip the internet address of the user.
    * @param session the variable to update with the new session ID.
    * @param userId a UserId to set to the session's owner.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool createSession(
      monarch::http::HttpHeader* header,
      const char* username, const char* password,
      monarch::net::InternetAddress* ip, std::string& session,
      bitmunk::common::UserId* userId = NULL);

   /**
    * Deletes any session in the passed BtpAction. If the session is valid
    * it will be removed from the session manager and any client cookies
    * will be deleted. If the session is not valid, only the client cookies
    * will be deleted, preventing a random user from deleting another user's
    * session.
    *
    * @param action the BtpAction possibly containing a session.
    */
   virtual void deleteSession(bitmunk::protocol::BtpAction* action);

   /**
    * Returns true if the session in the given BtpAction is valid, false if
    * not. If a UserId is passed, then it will be updated to the session's
    * owner if the session is valid.
    *
    * This call *will* update the cookie containing the session and ensure
    * the session's owner is logged in.
    *
    * @param action the BtpAction to check.
    * @param userId a UserId to set to the session's owner.
    *
    * @return true if the passed session is valid, false if not (exception set).
    */
   virtual bool isSessionValid(
      bitmunk::protocol::BtpAction* action,
      bitmunk::common::UserId* userId = NULL);

   /**
    * Refreshes the session cookies, if present. This is done automatically
    * if checking to see if a session is valid when passing a BtpAction.
    *
    * @param action the BtpAction with possible cookies to refresh.
    */
   virtual void refreshCookies(bitmunk::protocol::BtpAction* action);

   /**
    * Sets a session's userdata.
    *
    * @param action the BtpAction to get the session from.
    * @param userdata the new userdata to use.
    *
    * @return true if the session's userdata was set, false if not.
    */
   virtual bool setSessionUserdata(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& userdata);

   /**
    * Returns a session's userdata.
    *
    * @param action the BtpAction to get the session from.
    *
    * @return the session's userdata or NULL if none exists.
    */
   virtual monarch::rt::DynamicObject getSessionUserdata(
      bitmunk::protocol::BtpAction* action);

   /**
    * Grants access to the given user and IP.
    *
    * @param userId the ID of the user to grant access control to.
    * @param ip the IP address to grant access control to, NULL for any IP.
    *
    * @return true if successful, false on error.
    */
   virtual bool grantAccess(
      bitmunk::common::UserId userId, const char* ip);

   /**
    * Revokes access from the given user and IP.
    *
    * @param userId the ID of the user to revoke access control from.
    * @param ip the IP address to revoke access control from, NULL to revoke
    *           "any IP" access.
    *
    * @return true if successful, false on error.
    */
   virtual bool revokeAccess(
      bitmunk::common::UserId userId, const char* ip);

   /**
    * Gets an access control list. It contains entries with "userId" and
    * "ip" set.
    *
    * @param entries the access control list (array) to populate.
    * @param userId the ID of the user to get the access control entries for,
    *               NULL for all.
    *
    * @return true if successful, false on error.
    */
   virtual bool getAccessControlList(
      monarch::rt::DynamicObject& entries, bitmunk::common::UserId* userId = NULL);

   /**
    * Gets the associated Node.
    *
    * @return the associated node.
    */
   virtual bitmunk::node::Node* getNode();

protected:
   /**
    * Checks access control for the given username and IP.
    *
    * @param username the user's username.
    * @param ip the user's IP address.
    *
    * @return true if the user has access from the given IP, false if not.
    */
   virtual bool checkAccessControl(
      const char* username, monarch::net::InternetAddress* ip);

   /**
    * Sets the bitmunk-session, bitmunk-user-id, and bitmunk-username cookies.
    *
    * @param header the header to set.
    * @param session the session ID.
    * @param userId the session owner's user ID.
    * @param username the
    */
   virtual void setCookies(
      monarch::http::HttpHeader* header,
      const char* session, bitmunk::common::UserId userId,
      const char* username);

   /**
    * Clears the bitmunk-session and bitmunk-user-id cookies.
    *
    * @param header the header to update.
    */
   virtual void clearCookies(monarch::http::HttpHeader* header);

   /**
    * Gets a session ID and IP address from a BtpAction.
    *
    * @param action the BtpAction with a session.
    * @param session the string to fill with the session ID.
    * @param ip the InternetAddress to populate with the session IP.
    *
    * @return true if successful, false if no session found.
    */
   virtual bool getSessionFromAction(
      bitmunk::protocol::BtpAction* action,
      std::string& session, monarch::net::InternetAddress* ip);

   /**
    * Gets a pointer to session data from a BtpAction. This function assumes
    * that the session lock is engaged. Any valid session will be updated.
    *
    * @param action the BtpAction with a session.
    */
   virtual SessionData* getSessionData(bitmunk::protocol::BtpAction* action);

   /**
    * Gets a pointer to session data from a session ID and an IP address. This
    * function assumes that the session lock is engaged. Any valid session
    * will be updated.
    *
    * @param session the session to check.
    * @param ip the internet address that sent the session.
    * @param update true to update the session data (with a new time, etc.).
    */
   virtual SessionData* getSessionData(
      const char* session, monarch::net::InternetAddress* ip);
};

} // end namespace webui
} // end namespace bitmunk
#endif

/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_node_LoginManager_H
#define bitmunk_node_LoginManager_H

#include "bitmunk/common/Profile.h"
#include "bitmunk/common/TypeDefinitions.h"
#include "monarch/io/File.h"
#include "monarch/modest/OperationList.h"
#include "monarch/rt/SharedLock.h"

namespace bitmunk
{
namespace node
{

// forward declare Node
class Node;

/**
 * A LoginManager is used to log users in and out of a node.
 *
 * @author Dave Longley
 */
class LoginManager
{
protected:
   /**
    * The Node this LoginManager is for.
    */
   Node* mNode;

   /**
    * The login data for a particular user.
    */
   struct LoginData
   {
      bitmunk::common::User user;
      bitmunk::common::ProfileRef profile;
      monarch::modest::OperationList opList;
   };

   /**
    * A map of user ID to login data.
    */
   typedef std::map<bitmunk::common::UserId, LoginData*> UserIdMap;
   UserIdMap mUserIdMap;

   /**
    * A map of username to login data.
    */
   typedef std::map<std::string, LoginData*> UsernameMap;
   UsernameMap mUsernameMap;

   /**
    * A queue of the currently logged in users.
    */
   typedef std::list<bitmunk::common::UserId> UserIdList;
   UserIdList mUserIdQueue;

   /**
    * A lock used to synchronize getting/setting login data for users.
    */
   monarch::rt::SharedLock mLoginDataLock;

   /**
    * A list of the currently logging out users.
    */
   UserIdList mLogoutList;

   /**
    * Used to block all logins.
    */
   bool mBlockLogins;

   /**
    * A login lock used to synchronize logins, logouts, and the addition
    * of user operations.
    */
   monarch::rt::ExclusiveLock mLoginLock;

public:
   /**
    * Creates a new LoginManager.
    *
    * @param node the Node this LoginManager is for.
    */
   LoginManager(Node* node);

   /**
    * Destructs this LoginManager.
    */
   virtual ~LoginManager();

   /**
    * Logs a user into the associated node.
    *
    * @param username the user's username.
    * @param password the user's password.
    * @param userId to be set to the user's ID, NULL for don't care.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool loginUser(
      const char* username, const char* password,
      bitmunk::common::UserId* userId = NULL);

   /**
    * Logs a user out of the associated node.
    *
    * @param userId the user's ID.
    */
   virtual void logoutUser(bitmunk::common::UserId id);

   /**
    * Logs out all logged in users.
    */
   virtual void logoutAllUsers();

   /**
    * Adds the passed Operation to a particular user's OperationList,
    * updates the user ID to the default if 0 was passed, and retrieves
    * the user's profile.
    *
    * @param id the ID of the user or 0 for the default user.
    * @param op the Operation.
    * @param p will be set to the user's profile if not NULL.
    *
    * @return true if successful because the user was logged in, false
    *         if unsuccessful because the user was not logged in.
    */
   virtual bool addUserOperation(
      bitmunk::common::UserId& id,
      monarch::modest::Operation& op,
      bitmunk::common::ProfileRef* p);

   /**
    * Gets the User and/or Profile for a logged in user with the specified
    * user ID.
    *
    * @param id the user's ID or 0 for the default user (updated to user's ID).
    * @param user the User to populate or NULL if User isn't desired.
    * @param profile the Profile to populate or NULL if Profile isn't desired.
    *
    * @return true if the user was logged in, false if not.
    */
   virtual bool getLoginData(
      bitmunk::common::UserId& id,
      bitmunk::common::User* user,
      bitmunk::common::ProfileRef* profile);

   /**
    * Gets the User and/or Profile for a logged in user with the specified
    * username.
    *
    * @param username the user's username.
    * @param user the User to populate or NULL if User isn't desired.
    * @param profile the Profile to populate or NULL if Profile isn't desired.
    *
    * @return true if the user was logged in, false if not.
    */
   virtual bool getLoginData(
      const char* username,
      bitmunk::common::User* user,
      bitmunk::common::ProfileRef* profile);

protected:
   /**
    * Gets the LoginData for a particular user. This method assumes
    * the appropriate login data lock has been engaged and does no locking
    * itself.
    *
    * @param id the user's ID or 0 to get the default user's LoginData (which
    *           will update this ID to the default user's ID).
    * @param itr if not NULL, then the iterator into the UserIdMap will
    *            be set to this parameter.
    *
    * @return the user's LoginData or NULL if none exists.
    */
   virtual LoginData* getLoginDataUnlocked(
      bitmunk::common::UserId& id, UserIdMap::iterator* itr);

   /**
    * Gets the LoginData for a particular user. This method assumes
    * the appropriate login data lock has been engaged and does no locking
    * itself.
    *
    * @param username the user's username.
    * @param itr if not NULL, then the iterator into the UsernameMap will
    *            be set to this parameter.
    *
    * @return the user's LoginData or NULL if none exists.
    */
   virtual LoginData* getLoginDataUnlocked(
      const char* username, UsernameMap::iterator* itr);

   /**
    * Generates a user's profile.
    *
    * @param p the Profile object to generate with.
    * @param password the user's password.
    * @param os the OutputStream to save the profile to.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool generateProfile(
      bitmunk::common::ProfileRef& p, const char* password,
      monarch::io::OutputStream* os);

   /**
    * Loads a user's profile.
    *
    * @param p the Profile object to load with.
    * @param password the user's password.
    * @param file the File to load from.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool loadProfile(
      bitmunk::common::ProfileRef& p, const char* password, monarch::io::File& file);

   /**
    * Logs a user out of the associated node and does not block if mBlockLogins
    * is set.
    *
    * @param userId the user's ID.
    */
   virtual void logoutUserUnblocked(bitmunk::common::UserId id);
};

} // end namespace node
} // end namespace bitmunk
#endif

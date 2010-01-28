/*
 * Copyright (c) 2007-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_node_Node_H
#define bitmunk_node_Node_H

#include "monarch/kernel/MicroKernel.h"
#include "monarch/modest/OperationRunner.h"
#include "bitmunk/common/PublicKeyCache.h"
#include "bitmunk/protocol/BtpAction.h"
#include "bitmunk/node/LoginManager.h"
#include "bitmunk/node/Messenger.h"
#include "bitmunk/node/NodeConfigManager.h"
#include "bitmunk/node/NodeEventHandler.h"
#include "bitmunk/node/NodePublicKeySource.h"
#include "bitmunk/node/NodeMonitor.h"

namespace bitmunk
{
namespace node
{

class BtpServer;

/**
 * A Bitmunk Node is a single entity on the Bitmunk network.
 *
 * A node supports multiple Bitmunk users and uses the BTP (Bitmunk Transfer
 * Protocol) to communicate with other nodes.
 *
 * @author Dave Longley
 */
class Node :
public monarch::kernel::MicroKernelModuleApi,
public monarch::modest::OperationRunner
{
public:
   /**
    * An enum that determines the SSL status of a BTP service.
    */
   enum SslStatus
   {
      SslOn,    // service will only be found on https
      SslOff,   // service will only be found on http
      SslAny    // service will be found on both
   };

protected:
   /**
    * The MicroKernel this node runs on.
    */
   monarch::kernel::MicroKernel* mKernel;

   /**
    * The main event handler.
    */
   NodeEventHandler mEventHandler;

   /**
    * The Btp server.
    */
   BtpServer* mBtpServer;

   /**
    * This Node's LoginManager.
    */
   LoginManager mLoginManager;

   /**
    * The Messenger for this node.
    */
   MessengerRef mMessenger;

   /**
    * This Node's PublicKeyCache.
    */
   bitmunk::common::PublicKeyCache mPublicKeyCache;

   /**
    * The PublicKeySource for this node.
    */
   NodePublicKeySource mPublicKeySource;

   /**
    * The Monitor for this node.
    */
   NodeMonitor mMonitor;

   /**
    * Type definition for a list of user-owned fiber IDs.
    */
   typedef std::list<monarch::fiber::FiberId> UserFiberIdList;

   /**
    * Type definition and map of user ID to list of user-owned fiber IDs.
    */
   typedef std::map<bitmunk::common::UserId, UserFiberIdList*> UserFiberMap;
   UserFiberMap mUserFibers;

   /**
    * A lock for manipulating user fibers.
    */
   monarch::rt::ExclusiveLock mUserFiberLock;

   /**
    * An OperationList that manages all Operations started once a user
    * has logged in.
    */
   monarch::modest::OperationList mUserOperations;

public:
   /**
    * Creates a new Node.
    */
   Node();

   /**
    * Destructs this Node.
    */
   virtual ~Node();

   /**
    * Initializes and starts this Node and its services.
    *
    * @param k the associated MicroKernel.
    *
    * @return true on success, false on failure with exception set.
    */
   virtual bool start(monarch::kernel::MicroKernel* k);

   /**
    * Stops node services.
    */
   virtual void stop();

   /**
    * Logs a user with the given username and password into this Node.
    *
    * @param username the user's username.
    * @param password the user's password.
    * @param userId to be set to the user's ID, NULL for don't care.
    *
    * @return true if the user logged in, false if an Exception occurred.
    */
   virtual bool login(
      const char* username, const char* password,
      bitmunk::common::UserId* userId = NULL);

   /**
    * Logs the user with the given user ID out of this Node.
    *
    * @param id the user's ID.
    */
   virtual void logout(bitmunk::common::UserId id);

   /**
    * Logs out all users.
    */
   virtual void logoutAllUsers();

   /**
    * Queues the passed Operation with this Node's modest Engine.
    *
    * @param op the Operation to queue with this Node's modest Engine.
    */
   virtual void runOperation(monarch::modest::Operation& op);

   /**
    * Queues the passed Operation with this Node's modest Engine as a
    * user operation.
    *
    * @param id the ID of the user that owns the operation (0 for default).
    * @param op the Operation to queue with this Node's modest Engine.
    *
    * @return true if successful, false if the node wasn't running or the user
    *         wasn't logged in so the operation could not be run.
    */
   virtual bool runUserOperation(
      bitmunk::common::UserId& id, monarch::modest::Operation& op);

   /**
    * Gets the current thread's Operation. *DO NOT* call this if you
    * aren't sure the current thread is on an Operation, it may result
    * in memory corruption. It is safe to call this inside of a btp
    * service or an event handler.
    *
    * @return the current thread's Operation.
    */
   virtual monarch::modest::Operation currentOperation();

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
    * Adds a user fiber. This *must* only be called from a NodeFiber.
    *
    * @param id the ID of the user fiber.
    * @param userId the user ID.
    */
   virtual void addUserFiber(
      monarch::fiber::FiberId id, bitmunk::common::UserId userId);

   /**
    * Removes a user fiber. This *must* only be called from a NodeFiber.
    *
    * @param id the ID of the user fiber.
    * @param userId the user ID.
    */
   virtual void removeUserFiber(
      monarch::fiber::FiberId id, bitmunk::common::UserId userId);

   /**
    * Interrupts a particular NodeFiber.
    *
    * @param id the ID of the NodeFiber to interrupt.
    * @param sourceId the ID of the NodeFiber that is sending the message.
    */
   virtual void interruptFiber(
      monarch::fiber::FiberId id, monarch::fiber::FiberId sourceId = 0);

   /**
    * Interrupts all of a particular user's fibers.
    *
    * @param id the ID of the user.
    */
   virtual void interruptUserFibers(bitmunk::common::UserId id);

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

   /**
    * Checks to see if the user performing a btp action is logged in.
    *
    * @param action the action to check.
    * @param userId to be set to the user's ID (NULL if don't care).
    * @param user the User to populate or NULL if User isn't desired.
    * @param profile the Profile to populate or NULL if Profile isn't desired.
    *
    * @return true if the user is logged in, false if not (with exception set).
    */
   virtual bool checkLogin(
      bitmunk::protocol::BtpAction* action,
      bitmunk::common::UserId* userId = NULL,
      bitmunk::common::User* user = NULL,
      bitmunk::common::ProfileRef* profile = NULL);

   /**
    * Gets the default user's ID.
    *
    * @return the default user's ID.
    */
   virtual bitmunk::common::UserId getDefaultUserId();

   /**
    * Gets the default user's Profile.
    *
    * @param profile the Profile to populate.
    *
    * @return true if the Profile was available, false if not.
    */
   virtual bool getDefaultProfile(bitmunk::common::ProfileRef& profile);

   /**
    * Gets the interface to the Bitmunk NodeModule with the given name.
    *
    * @param name the name of the Bitmunk NodeModule to get the API for.
    *
    * @return the module's API, or NULL if the module does not exist
    *         or it has no API.
    */
   virtual monarch::kernel::MicroKernelModuleApi* getModuleApi(
      const char* name);

   /**
    * Gets the API to the first Bitmunk NodeModule with the given type.
    *
    * @param type the type of Bitmunk NodeModule to get the API for.
    *
    * @return the module's API, or NULL if the module does not exist
    *         or it has no API.
    */
   virtual monarch::kernel::MicroKernelModuleApi* getModuleApiByType(
      const char* type);

   /**
    * Gets the APIs for all the Bitmunk NodeModules with the given type.
    *
    * @param type the type of Bitmunk NodeModule to get the APIs for.
    * @param apiList the list to put the APIs into.
    */
   virtual void getModuleApisByType(
      const char* type,
      std::list<monarch::kernel::MicroKernelModuleApi*>& apiList);

   /**
    * Gets this node's MicroKernel.
    *
    * @return the MicroKernel for this node.
    */
   virtual monarch::kernel::MicroKernel* getKernel();

   /**
    * Gets this node's ConfigManager.
    *
    * @return the ConfigManager for this node.
    */
   virtual NodeConfigManager* getConfigManager();

   /**
    * Gets this node's FiberScheduler.
    *
    * @return the FiberScheduler for this node.
    */
   virtual monarch::fiber::FiberScheduler* getFiberScheduler();

   /**
    * Gets this node's FiberMessageCenter.
    *
    * @return the FiberMessageCenter for this node.
    */
   virtual monarch::fiber::FiberMessageCenter* getFiberMessageCenter();

   /**
    * Gets this node's EventController.
    *
    * @return the EventController for this node.
    */
   virtual monarch::event::EventController* getEventController();

   /**
    * Gets this node's EventDaemon.
    *
    * @return the EventDaemon for this node.
    */
   virtual monarch::event::EventDaemon* getEventDaemon();

   /**
    * Gets this node's Server.
    *
    * @return this node's Server.
    */
   virtual monarch::net::Server* getServer();

   /**
    * Gets this node's BtpServer.
    *
    * @return the BtpServer for this node.
    */
   virtual BtpServer* getBtpServer();

   /**
    * Gets this node's Messenger.
    *
    * @return the Messenger for this node.
    */
   virtual Messenger* getMessenger();

   /**
    * Gets this Node's PublicKeyCache.
    *
    * @return this Node's PublicKeyCache.
    */
   virtual bitmunk::common::PublicKeyCache* getPublicKeyCache();

   /**
    * Gets this node's Monitor.
    *
    * @return the Monitor for this node.
    */
   virtual NodeMonitor* getMonitor();
};

} // end namespace node
} // end namespace bitmunk
#endif

/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_peruserdb_DatabaseHub_H
#define bitmunk_peruserdb_DatabaseHub_H

#include "bitmunk/node/Node.h"
#include "bitmunk/peruserdb/IPerUserDBModule.h"
#include "monarch/net/Url.h"

#include <map>

namespace bitmunk
{
namespace peruserdb
{

/**
 * The DatabaseHub is the central location accessed by other modules to obtain
 * connections to per-user databases.
 * 
 * @author Dave Longley
 */
class DatabaseHub : public IPerUserDBModule
{
protected:
   /**
    * The associated Bitmunk Node.
    */
   bitmunk::node::Node* mNode;
   
   /**
    * A UserEntry contains a ConnectionPool and a DatabaseClient.
    */
   struct UserEntry
   {
      monarch::sql::ConnectionPoolRef pool;
      monarch::sql::DatabaseClientRef dbc;
   };
   
   /**
    * Map type for user ID to user entries.
    */
   typedef std::map<bitmunk::common::UserId, UserEntry> UserMap;
   
   /**
    * A ConnectionGroupEntry contains a UserMap and a PerUserDatabase interface.
    */
   struct ConnectionGroupEntry
   {
      UserMap* userMap;
      PerUserDatabase* db;
   };
   
   /**
    * Map of connection group ID to ConnectionGroupEntry.
    */
   typedef std::map<ConnectionGroupId, ConnectionGroupEntry> ConnectionMap;
   ConnectionMap mConnectionMap;
   
   /**
    * The last assigned connection group ID.
    */
   ConnectionGroupId mLastAssignedId;
   
   /**
    * Observer for the user logged out event.
    */
   monarch::event::ObserverRef mUserLoggedOutObserver;
   
   /**
    * A lock for manipulating the connection types and pools.
    */
   monarch::rt::SharedLock mMapLock;
   
public:
   /**
    * Creates a new DatabaseHub.
    * 
    * @param node the associated Bitmunk Node.
    */
   DatabaseHub(bitmunk::node::Node* node);
   
   /**
    * Destructs this DatabaseHub.
    */
   virtual ~DatabaseHub();
   
   /**
    * Registers a new group of database connections with this module. This
    * method will create a connection group ID that can be used to quickly
    * obtain per-user connections of a particular type (i.e. to a particular
    * database). The specific URL to the database can be different per-user
    * and must be provided via a class that has implemented the PerUserDatabase
    * interface.
    * 
    * The returned connection group ID must be passed back to this module to
    * unregister the connection group at a later time.
    * 
    * @param peruserDB the per-user database interface to use to initialize
    *                  a database for a particular user on the first call to
    *                  get a connection to the database for that user.
    * 
    * @return the connection group ID for the url or 0 if an exception occurred.
    */
   virtual ConnectionGroupId addConnectionGroup(PerUserDatabase* peruserDB);
   
   /**
    * Unregisters a database connection group that was previously registered
    * with this module. Once unregistered, no connections from that group can
    * be further obtained. The developer must ensure that no connection of the
    * unregistering group are currently in use before making this call.
    * 
    * @param id the ID of the connection group to unregister.
    * 
    * @return true if unregistered, false if an exception occurred.
    */
   virtual bool removeConnectionGroup(ConnectionGroupId id);
   
   /**
    * Gets a connection to a database for a particular user or blocks until
    * a connection is available. The returned connection must *not* be deleted,
    * but it *must* be closed when it is finished being used.
    * 
    * @param id the ID of the related connection group.
    * @param userId the ID of the user to get the connection for.
    * 
    * @return the connection or NULL if an exception occurred.
    */
   virtual monarch::sql::Connection* getConnection(
      ConnectionGroupId id, bitmunk::common::UserId userId);
   
   /**
    * Gets a database client for a database for a particular user.
    * 
    * @param id the ID of the related connection group.
    * @param userId the ID of the user to get the connection for.
    * 
    * @return the database client or NULL if an exception occurred.
    */
   virtual monarch::sql::DatabaseClientRef getDatabaseClient(
      ConnectionGroupId id, bitmunk::common::UserId userId);
   
   /**
    * Handles the user logged out event.
    * 
    * @param e the event.
    */
   virtual void userLoggedOut(monarch::event::Event& e);
   
protected:
   /**
    * Adds a user entry for a particular connection group ID if one does not
    * already exist.
    * 
    * Note: This method is called within the exclusive map lock.
    * 
    * @param id the connection group ID.
    * @param userId the userId.
    * 
    * @return the user's entry or NULL if an exception occurred.
    */
   virtual UserEntry* addUserEntry(
      ConnectionGroupId id, bitmunk::common::UserId userId);
};

} // end namespace peruserdb
} // end namespace bitmunk
#endif

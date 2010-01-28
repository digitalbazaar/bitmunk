/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_peruserdb_IPerUserDBModule_H
#define bitmunk_peruserdb_IPerUserDBModule_H

#include "bitmunk/common/TypeDefinitions.h"
#include "bitmunk/peruserdb/PerUserDatabase.h"
#include "monarch/kernel/MicroKernelModuleApi.h"
#include "monarch/sql/Connection.h"
#include "monarch/sql/DatabaseClient.h"

namespace bitmunk
{
namespace peruserdb
{

/**
 * An IPerUserDBModule provides the interface for the PerUserDBModule.
 * 
 * This interface allows developers to group particular kinds of connections
 * together for many users such that they can quickly obtain connections
 * for individual users and quickly clean up all connections for all users
 * when the time is appropriate.
 * 
 * The typical usage pattern for this interface is thus:
 * 
 * 1. Create a connection group.
 * 
 * This means an identifier will be assigned to empty group of per-user
 * connection pools. These connection pools may go to the same physical
 * database or they may go to the same "kind" of database (i.e. there may
 * be multiple databases that have the same schema, one per user). Each user
 * gets their own pool with their own limited number of concurrent connections.
 * 
 * 2. A user gets a connection.
 * 
 * When a user wants to get a connection to a database, they specify the
 * connection group (by ConnectionGroupId), and then they provide a PerUserDatabase
 * interface that can provide the user-specific url and maximum count of
 * permitted concurrent connections. Connection pool allocation is done
 * lazily -- only the first time a connection is requested will the
 * PerUserDatabase interface be invoked to obtain the url and max count for
 * a particular user as well as to do any initialization work (i.e. schema
 * creation). Each subsequent request for a connection will be fast, using
 * only the connection group ID and the user ID to find the user's particular
 * connection pool.
 * 
 * 3. Remove connection group.
 * 
 * When a developer is finished with a particular connection group, i.e.
 * they are unloading their own module that used a database, they call
 * removeConnectionGroup. This prevents any new connections from being
 * obtained from the previously registered group.
 * 
 * @author Dave Longley
 */
class IPerUserDBModule : public monarch::kernel::MicroKernelModuleApi
{
public:
   /**
    * Creates a new IPerUserDBModule.
    */
   IPerUserDBModule() {};
   
   /**
    * Destructs this IPerUserDBModule.
    */
   virtual ~IPerUserDBModule() {};
   
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
   virtual ConnectionGroupId addConnectionGroup(PerUserDatabase* peruserDB) = 0;
   
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
   virtual bool removeConnectionGroup(ConnectionGroupId id) = 0;
   
   /**
    * Gets a connection to a database for a particular user or blocks until
    * a connection is available. The returned connection must *not* be deleted,
    * but it *must* be closed when it is finished being used.
    * 
    * @param id the ID of the connection group to get.
    * @param userId the ID of the user to get the connection for.
    * 
    * @return the connection or NULL if an exception occurred.
    */
   virtual monarch::sql::Connection* getConnection(
      ConnectionGroupId id, bitmunk::common::UserId userId) = 0;
   
   /**
    * Gets a database client for a database for a particular user.
    * 
    * @param id the ID of the related connection group.
    * @param userId the ID of the user to get the connection for.
    * 
    * @return the database client or NULL if an exception occurred.
    */
   virtual monarch::sql::DatabaseClientRef getDatabaseClient(
      ConnectionGroupId id, bitmunk::common::UserId userId) = 0;
};

} // end namespace peruserdb
} // end namespace bitmunk
#endif

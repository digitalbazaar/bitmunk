/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_peruserdb_PerUserDatabase_H
#define bitmunk_peruserdb_PerUserDatabase_H

#include "bitmunk/common/TypeDefinitions.h"
#include "monarch/sql/DatabaseClient.h"

namespace bitmunk
{
namespace peruserdb
{

// connection group ID type
typedef uint32_t ConnectionGroupId;

/**
 * A PerUserDatabase is an interface that can be implemented by a database
 * class for a per-user database. It is used to synchronously initialize
 * databases on a per-user basis.
 * 
 * @author Dave Longley
 */
class PerUserDatabase
{
public:
   /**
    * Creates a new PerUserDatabase.
    */
   PerUserDatabase() {};
   
   /**
    * Destructs this PerUserDatabase.
    */
   virtual ~PerUserDatabase() {};
   
   /**
    * Gets the url and and maximum number of concurrent connections a
    * user can have to this database.
    * 
    * This will be called when the first connection is requested for a
    * particular connection group.
    * 
    * @param id the ID of the connection group.
    * @param userId the ID of the user.
    * @param url the url string to populate.
    * @param maxCount the maximum concurrent connection count to populate.
    * 
    * @return true if successful, false if an exception occurred.
    */
   virtual bool getConnectionParams(
      ConnectionGroupId id, bitmunk::common::UserId userId,
      std::string& url, uint32_t& maxCount) = 0;
   
   /**
    * Initializes a database for a user.
    * 
    * This will be called when the first connection is requested for a
    * particular connection group.
    * 
    * @param id the ID of the connection group.
    * @param userId the ID of the user.
    * @param conn a connection to use to initialize the database, *must*
    *             be closed before returning.
    * @param dbc a DatabaseClient to use -- but only the given connection must
    *            be used with it.
    * 
    * @return true if successful, false if an exception occurred.
    */
   virtual bool initializePerUserDatabase(
      ConnectionGroupId id, bitmunk::common::UserId userId,
      monarch::sql::Connection* conn, monarch::sql::DatabaseClientRef& dbc) = 0;
   
   /**
    * Cleans up a database for a user.
    * 
    * This will be called after a user has logged out and will no longer
    * have access to their database.
    * 
    * @param id the ID of the connection group.
    * @param userId the ID of the user.
    */
   virtual void cleanupPerUserDatabase(
      ConnectionGroupId id, bitmunk::common::UserId userId) {};
};

} // end namespace peruserdb
} // end namespace bitmunk
#endif

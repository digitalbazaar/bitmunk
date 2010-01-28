/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_webui_SessionDatabase_H
#define bitmunk_webui_SessionDatabase_H

#include "bitmunk/node/Node.h"
#include "monarch/sql/ConnectionPool.h"

namespace bitmunk
{
namespace webui
{

/**
 * The SessionDatabase stores access control information for the SessionManager.
 * 
 * @author Dave Longley
 */
class SessionDatabase
{
protected:
   /**
    * The node associated with this database.
    */
   bitmunk::node::Node* mNode;
   
   /**
    * The connection pool.
    */
   monarch::sql::ConnectionPoolRef mConnectionPool;
   
public:
   /**
    * Creates a new SessionDatabase.
    */
   SessionDatabase();
   
   /**
    * Destructs this SessionDatabase.
    */
   virtual ~SessionDatabase();
   
   /**
    * Initializes this database for use.
    * 
    * @param node the Node for this database.
    * 
    * @return true if successful, false if an exception occurred.
    */
   virtual bool initialize(bitmunk::node::Node* node);
   
   /**
    * Inserts an access control entry.
    * 
    * @param userId the user ID part of the entry.
    * @param ip the IP part of the entry.
    * 
    * @return true if successful, false if an exception occurred.
    */
   virtual bool insertAccessControlEntry(
      bitmunk::common::UserId userId, const char* ip);
   
   /**
    * Deletes an access control entry.
    * 
    * @param userId the user ID part of the entry.
    * @param ip the IP part of the entry.
    * 
    * @return true if successful, false if an exception occurred.
    */
   virtual bool deleteAccessControlEntry(
      bitmunk::common::UserId userId, const char* ip);
   
   /**
    * Gets a list of all access control entries for all users. The list
    * will be full of entries with "userId" and "ip" set.
    * 
    * @param entries the array to populate.
    * 
    * @return true if successful, false if an exception occurred.
    */
   virtual bool getAccessControlList(monarch::rt::DynamicObject& entries);
   
   /**
    * Gets a list of all access control entries for the given user ID. The list
    * will be full of entries with "userId" and "ip" set.
    * 
    * @param userId the user ID to get the access control entries for.
    * @param entries the array to populate.
    * 
    * @return true if successful, false if an exception occurred.
    */
   virtual bool getAccessControlList(
      bitmunk::common::UserId userId, monarch::rt::DynamicObject& entries);
   
   /**
    * Checks the database for the given access control entry, setting
    * an exception if it does not exist or there is an error.
    * 
    * @param userId the user ID part of the entry.
    * @param ip the IP part of the entry.
    * 
    * @return true if the entry exists, false if not or an error occurred.
    */
   virtual bool checkAccessControl(
      bitmunk::common::UserId userId, const char* ip);
   
protected:
   /**
    * Creates the connection pool.
    * 
    * @param url the url for the connection pool.
    * @param size the size (# of connections) of the pool.
    * 
    * @return the created connection pool or NULL if an exception occurred.
    */
   virtual monarch::sql::ConnectionPool* createConnectionPool(
      const char* url, unsigned int size);
};

} // end namespace webui
} // end namespace bitmunk
#endif

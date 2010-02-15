/*
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk/peruserdb/DatabaseHub.h"

#include "bitmunk/peruserdb/PerUserDBModule.h"
#include "monarch/event/ObserverDelegate.h"
#include "monarch/sql/sqlite3/Sqlite3ConnectionPool.h"
#include "monarch/sql/sqlite3/Sqlite3DatabaseClient.h"

#include <algorithm>

using namespace std;
using namespace monarch::event;
using namespace monarch::rt;
using namespace monarch::sql;
using namespace monarch::sql::sqlite3;
using namespace bitmunk::common;
using namespace bitmunk::node;
using namespace bitmunk::peruserdb;

#define PERUSERDB_EXCEPTION     "bitmunk.peruserdb"
#define EVENT_USER_LOGGED_OUT   "bitmunk.common.User.loggedOut"

DatabaseHub::DatabaseHub(Node* node) :
   mNode(node),
   mLastAssignedId(0),
   mUserLoggedOutObserver(NULL)
{
   // register observer to cleanup user data on logout
   mUserLoggedOutObserver = new ObserverDelegate<DatabaseHub>(
      this, &DatabaseHub::userLoggedOut);
   mNode->getEventController()->registerObserver(
      &(*mUserLoggedOutObserver), EVENT_USER_LOGGED_OUT);
   MO_CAT_DEBUG(BM_PERUSERDB_CAT,
      "DatabaseHub registered for " EVENT_USER_LOGGED_OUT);
}

DatabaseHub::~DatabaseHub()
{
   // unregister observer
   mNode->getEventController()->unregisterObserver(
      &(*mUserLoggedOutObserver), EVENT_USER_LOGGED_OUT);
   mUserLoggedOutObserver.setNull();
   MO_CAT_DEBUG(BM_PERUSERDB_CAT,
      "DatabaseHub unregistered for " EVENT_USER_LOGGED_OUT);
}

ConnectionGroupId DatabaseHub::addConnectionGroup(PerUserDatabase* peruserDB)
{
   ConnectionGroupId rval = 0;

   // insert user map
   mMapLock.lockExclusive();
   {
      rval = ++mLastAssignedId;
      ConnectionGroupEntry entry;
      entry.userMap = new UserMap();
      entry.db = peruserDB;
      mConnectionMap.insert(make_pair(rval, entry));
   }
   mMapLock.unlockExclusive();

   MO_CAT_INFO(BM_PERUSERDB_CAT, "Added connection group %u", rval);

   return rval;
}

bool DatabaseHub::removeConnectionGroup(ConnectionGroupId id)
{
   bool rval = false;

   mMapLock.lockExclusive();
   {
      // find connection group
      ConnectionMap::iterator ci = mConnectionMap.find(id);
      if(ci != mConnectionMap.end())
      {
         MO_CAT_DEBUG(BM_PERUSERDB_CAT,
            "DatabaseHub removing connection group %u", id);
         delete ci->second.userMap;
         mConnectionMap.erase(ci);
         rval = true;
      }
      else
      {
         ExceptionRef e = new Exception(
            "Connection group ID not found.",
            PERUSERDB_EXCEPTION ".InvalidConnectionGroupId");
         Exception::set(e);
      }
   }
   mMapLock.unlockExclusive();

   return rval;
}

Connection* DatabaseHub::getConnection(ConnectionGroupId id, UserId userId)
{
   Connection* rval = NULL;

   // lock to try to find the connection group
   mMapLock.lockShared();
   ConnectionMap::iterator ci = mConnectionMap.find(id);
   if(ci == mConnectionMap.end())
   {
      mMapLock.unlockShared();
      ExceptionRef e = new Exception(
         "Connection group ID not found.",
         PERUSERDB_EXCEPTION ".InvalidConnectionGroupId");
      Exception::set(e);
   }
   else
   {
      // find the connection pool for the user
      ConnectionPoolRef pool(NULL);
      UserMap::iterator ui = ci->second.userMap->find(userId);
      if(ui == ci->second.userMap->end())
      {
         // unlock shared and lock exclusive to create the user entry
         // for the user
         mMapLock.unlockShared();
         mMapLock.lockExclusive();
         {
            UserEntry* entry = addUserEntry(id, userId);
            if(entry != NULL)
            {
               pool = entry->pool;
            }
         }
         mMapLock.unlockExclusive();
      }
      else
      {
         // get a connection from the existing pool
         pool = ui->second.pool;
         mMapLock.unlockShared();
      }

      if(!pool.isNull())
      {
         // get a connection from the pool
         rval = pool->getConnection();
      }
   }

   return rval;
}

DatabaseClientRef DatabaseHub::getDatabaseClient(
   ConnectionGroupId id, UserId userId)
{
   DatabaseClientRef rval(NULL);

   // lock to try to find the connection group
   mMapLock.lockShared();
   ConnectionMap::iterator ci = mConnectionMap.find(id);
   if(ci == mConnectionMap.end())
   {
      mMapLock.unlockShared();
      ExceptionRef e = new Exception(
         "Connection group ID not found.",
         PERUSERDB_EXCEPTION ".InvalidConnectionGroupId");
      Exception::set(e);
   }
   else
   {
      // find the database client for the user
      UserMap::iterator ui = ci->second.userMap->find(userId);
      if(ui == ci->second.userMap->end())
      {
         // unlock shared and lock exclusive to create the user entry
         // for the user
         mMapLock.unlockShared();
         mMapLock.lockExclusive();
         {
            UserEntry* entry = addUserEntry(id, userId);
            if(entry != NULL)
            {
               rval = entry->dbc;
            }
         }
         mMapLock.unlockExclusive();
      }
      else
      {
         // get the database client from the existing pool
         rval = ui->second.dbc;
         mMapLock.unlockShared();
      }
   }

   return rval;
}

void DatabaseHub::userLoggedOut(Event& e)
{
   UserId userId = BM_USER_ID(e["details"]["userId"]);
   mMapLock.lockExclusive();
   {
      for(ConnectionMap::iterator ci = mConnectionMap.begin();
          ci != mConnectionMap.end(); ci++)
      {
         UserMap::iterator ui = ci->second.userMap->find(userId);
         if(ui != ci->second.userMap->end())
         {
            MO_CAT_DEBUG(BM_PERUSERDB_CAT,
               "DatabaseHub cleanup for connection group %u, user: %" PRIu64,
               ci->first, userId);

            // call custom clean up code
            ci->second.db->cleanupPerUserDatabase(ci->first, userId);

            // erase entry from user map
            ci->second.userMap->erase(ui);
         }
      }
   }
   mMapLock.unlockExclusive();
}

DatabaseHub::UserEntry* DatabaseHub::addUserEntry(
   ConnectionGroupId id, UserId userId)
{
   UserEntry* rval = NULL;

   // Note: This method is called inside of an exclusive lock, so no need
   // to lock again here.

   // find connection group
   bool createEntry = false;
   ConnectionMap::iterator ci = mConnectionMap.find(id);
   if(ci == mConnectionMap.end())
   {
      // connection group does not exist
      ExceptionRef e = new Exception(
         "Connection group ID not found.",
         PERUSERDB_EXCEPTION ".InvalidConnectionGroupId");
      Exception::set(e);
   }
   else
   {
      // see if an entry already exists for the user
      UserMap::iterator ui = ci->second.userMap->find(userId);
      if(ui != ci->second.userMap->end())
      {
         // return existing entry
         rval = &(ui->second);
      }
      else
      {
         // entry not found, we must create a new entry
         createEntry = true;
      }
   }

   if(createEntry)
   {
      // get the peruser database associated with the connection group
      PerUserDatabase* peruserDB = ci->second.db;

      // get url and max concurrent connection count
      string url;
      uint32_t maxCount;
      if(peruserDB->getConnectionParams(id, userId, url, maxCount))
      {
         // parse url, ensure it isn't malformed
         Exception::clear();
         monarch::net::Url dbUrl(url.c_str());
         if(Exception::isSet())
         {
            ExceptionRef e = new Exception(
               "Could not add connection group. Invalid database URL.",
               PERUSERDB_EXCEPTION ".BadUrl");
            e->getDetails()["url"] = url.c_str();
            Exception::set(e);
         }
         else
         {
            string expPath;
            if(mNode->getConfigManager()->translateUrlToUserFilePath(
               userId, url.c_str(), expPath))
            {
               size_t len = 10 /* sqlite3:// */ + expPath.length() + 1;
               char fullUrl[len];
               snprintf(fullUrl, len, "sqlite3://%s", expPath.c_str());
               MO_CAT_INFO(BM_PERUSERDB_CAT,
                  "Creating connection pool for userId %" PRIu64 ", url: '%s'",
                  userId, fullUrl);

               ConnectionPoolRef pool = new Sqlite3ConnectionPool(
                  fullUrl, maxCount);
               DatabaseClientRef dbc = new Sqlite3DatabaseClient();
               // FIXME: make logging a config option, it prints tons of stuff
               dbc->setDebugLogging(true);
               dbc->setReadConnectionPool(pool);
               dbc->setWriteConnectionPool(pool);
               dbc->initialize();

               // new pool created, run per-user db initialization
               Connection* conn = pool->getConnection();
               if(conn == NULL)
               {
                  // bogus pool
                  pool.setNull();
                  ExceptionRef e = new Exception(
                     "Could not get connection to initialize "
                     "per-user database.",
                     PERUSERDB_EXCEPTION ".InitializeError");
                  Exception::push(e);
               }
               // initialize database using given interface
               else if(!peruserDB->initializePerUserDatabase(
                  id, userId, conn, dbc))
               {
                  // failure to initialize
                  pool.setNull();
               }

               if(!pool.isNull())
               {
                  // create user entry
                  UserEntry entry;
                  entry.pool = pool;
                  entry.dbc = dbc;

                  // insert entry into map
                  pair<UserMap::iterator, bool> ret =
                     ci->second.userMap->insert(make_pair(userId, entry));
                  rval = &(ret.first->second);

                  MO_CAT_INFO(BM_PERUSERDB_CAT,
                     "Created connection pool for userId %" PRIu64 ", "
                     "url: '%s',"
                     " maximum connections: %u",
                     userId, fullUrl, maxCount);
               }
            }
         }
      }
   }

   return rval;
}

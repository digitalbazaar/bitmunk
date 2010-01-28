/*
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/webui/SessionDatabase.h"

#include "bitmunk/webui/WebUiModule.h"
#include "monarch/config/ConfigManager.h"
#include "monarch/sql/sqlite3/Sqlite3ConnectionPool.h"
#include "monarch/sql/Row.h"
#include "monarch/sql/Statement.h"

using namespace std;
using namespace bitmunk::common;
using namespace bitmunk::node;
using namespace bitmunk::webui;
using namespace monarch::config;
using namespace monarch::io;
using namespace monarch::rt;
using namespace monarch::sql;
using namespace monarch::sql::sqlite3;

#define TABLE_ACCESS_CONTROL "access_control"

SessionDatabase::SessionDatabase()
{
}

SessionDatabase::~SessionDatabase()
{
}

bool SessionDatabase::initialize(Node* node)
{
   bool rval = false;

   mNode = node;

   Config cfg = mNode->getConfigManager()->getModuleConfig(
      "bitmunk.webui.WebUi");
   const char* url = cfg["sessionDatabase"]["url"]->getString();
   uint32_t connections = cfg["sessionDatabase"]["connections"]->getUInt32();

   MO_CAT_DEBUG(BM_WEBUI_CAT,
      "Initializing session database connection pool, "
      "url:%s connections:%i", url, connections);

   // create connection pool
   ConnectionPool* pool = createConnectionPool(url, connections);
   if(pool != NULL)
   {
      mConnectionPool = pool;

      // create tables if they do not exist
      Connection* c = pool->getConnection();
      if((rval = (c != NULL) && c->begin()))
      {
         // Note: the statements here must only be prepared and executed
         // incrementally because they modify the database schema

         // FIXME: add a version table that can be used to update
         // schema for session database?

         // statement to create access control table
         {
            Statement* s = c->prepare(
               "CREATE TABLE IF NOT EXISTS " TABLE_ACCESS_CONTROL " ("
               "user_id BIGINT UNSIGNED,"
               "ip TEXT,"
               "PRIMARY KEY(user_id,ip))");
            if((rval = (s != NULL)))
            {
               rval = s->execute();
            }
         }

         if(rval)
         {
            rval = c->commit();
         }

         // close connection
         c->close();
      }
   }

   if(rval)
   {
      MO_CAT_DEBUG(BM_WEBUI_CAT,
         "Session database initialized, url:%s", url);
   }
   else
   {
      MO_CAT_ERROR(BM_WEBUI_CAT,
         "Session database failed to initialize, url:%s", url);
   }

   return rval;
}

bool SessionDatabase::insertAccessControlEntry(UserId userId, const char* ip)
{
   bool rval = false;

   // get database connection
   Connection* c = mConnectionPool->getConnection();
   if(c != NULL)
   {
      // prepare statement
      Statement* s = c->prepare(
         "REPLACE INTO " TABLE_ACCESS_CONTROL
         " (user_id,ip) VALUES (:userId,:ip)");
      if(s != NULL)
      {
         // set parameters and execute
         rval =
            s->setUInt64(":userId", userId) &&
            s->setText(":ip", ip) &&
            s->execute();
      }

      // close connection
      c->close();
   }

   return rval;
}

bool SessionDatabase::deleteAccessControlEntry(UserId userId, const char* ip)
{
   bool rval = false;

   // get database connection
   Connection* c = mConnectionPool->getConnection();
   if(c != NULL)
   {
      // prepare statement
      Statement* s = c->prepare(
         "DELETE FROM " TABLE_ACCESS_CONTROL
         " WHERE user_id=:userId AND ip=:ip");
      if(s != NULL)
      {
         // set parameters and execute
         rval =
            s->setUInt64(":userId", userId) &&
            s->setText(":ip", ip) &&
            s->execute();
      }

      // close connection
      c->close();
   }

   return rval;
}

bool SessionDatabase::getAccessControlList(DynamicObject& entries)
{
   bool rval = false;

   // initialize entries array
   entries->setType(Array);
   entries->clear();

   // get database connection
   Connection* c = mConnectionPool->getConnection();
   if(c != NULL)
   {
      // prepare statement
      Statement* s = c->prepare(
         "SELECT user_id,ip FROM " TABLE_ACCESS_CONTROL " LIMIT 1000");
      if(s != NULL)
      {
         // execute
         rval = s->execute();
         if(rval)
         {
            UserId userId;
            string ip;
            Row* row;
            while((row = s->fetch()) != NULL)
            {
               row->getUInt64("user_id", userId);
               row->getText("ip", ip);
               DynamicObject& entry = entries->append();
               entry["userId"] = userId;
               entry["ip"] = ip.c_str();
            }
         }
      }

      // close connection
      c->close();
   }

   return rval;
}

bool SessionDatabase::getAccessControlList(
   UserId userId, DynamicObject& entries)
{
   bool rval = false;

   // initialize entries array
   entries->setType(Array);
   entries->clear();

   // get database connection
   Connection* c = mConnectionPool->getConnection();
   if(c != NULL)
   {
      // prepare statement
      Statement* s = c->prepare(
         "SELECT ip FROM " TABLE_ACCESS_CONTROL
         " WHERE user_id=:userId LIMIT 1000");
      if(s != NULL)
      {
         // set parameters and execute
         rval =
            s->setUInt64(":userId", userId) &&
            s->execute();
         if(rval)
         {
            string ip;
            Row* row;
            while((row = s->fetch()) != NULL)
            {
               row->getText("ip", ip);
               DynamicObject& entry = entries->append();
               entry["userId"] = userId;
               entry["ip"] = ip.c_str();
            }
         }
      }

      // close connection
      c->close();
   }

   return rval;
}

bool SessionDatabase::checkAccessControl(UserId userId, const char* ip)
{
   bool rval = false;

   // get database connection
   Connection* c = mConnectionPool->getConnection();
   if(c != NULL)
   {
      // prepare statement
      Statement* s = c->prepare(
         "SELECT user_id FROM " TABLE_ACCESS_CONTROL
         " WHERE user_id=:userId AND (ip=:ip1 OR ip=:ip2) LIMIT 1");
      if(s != NULL)
      {
         // set parameters and execute
         rval =
            s->setUInt64(":userId", userId) &&
            s->setText(":ip1", ip) &&
            s->setText(":ip2", "*") &&
            s->execute();
         if(rval)
         {
            rval = (s->fetch() != NULL);
            if(!rval)
            {
               ExceptionRef e = new Exception(
                  "The user is denied access from the given IP address.",
                  "bitmunk.webui.SessionDatabase.AccessDeniedFromIP");
               e->getDetails()["userId"] = userId;
               e->getDetails()["ip"] = ip;
               Exception::set(e);
            }
         }
      }

      // close connection
      c->close();
   }

   return rval;
}

ConnectionPool* SessionDatabase::createConnectionPool(
   const char* url, unsigned int size)
{
   ConnectionPool* rval = NULL;

   // parse url, ensure it isn't malformed
   Exception::clear();
   monarch::net::Url dbUrl(url);
   if(!Exception::isSet())
   {
      if(strcmp(dbUrl.getScheme().c_str(), "sqlite3") == 0)
      {
         // create sqlite3 connection pool
         const string& path = dbUrl.getSchemeSpecificPart();
         if(path.length() <= 2 || path[0] != '/' || path[1] != '/')
         {
            ExceptionRef e = new Exception(
               "URL format not recognized.",
               "bitmunk.webui.SessionDatabase.BadUrlFormat");
            Exception::set(e);
         }
         else
         {
            string expPath;
            // skip "//"
            const char* urlPath = path.c_str() + 2;
            if(mNode->getConfigManager()->expandBitmunkHomePath(
               urlPath, expPath))
            {
               size_t len = 10 /* sqlite3:// */ + expPath.length() + 1;
               char fullUrl[len];
               snprintf(fullUrl, len, "sqlite3://%s", expPath.c_str());

               MO_CAT_DEBUG(BM_WEBUI_CAT,
                  "Creating session database connection pool, url:%s", fullUrl);
               rval = new Sqlite3ConnectionPool(fullUrl, size);
               MO_CAT_INFO(BM_WEBUI_CAT,
                  "Created session database connection pool, url:%s", fullUrl);
            }
         }
      }
      else
      {
         ExceptionRef e = new Exception(
            "URL scheme is not recognized.",
            "bitmunk.webui.SessionDatabase.BadUrlScheme");
         Exception::set(e);
      }
   }
   else
   {
      ExceptionRef e = new Exception(
         "Bad URL in configuration.",
         "bitmunk.webui.SessionDatabase.BadUrl");
      Exception::set(e);
   }

   if(rval == NULL)
   {
      ExceptionRef e = new Exception(
         "Could not create connection pool for session database.",
         "bitmunk.webui.SessionDatabase.Error");
      e->getDetails()["url"] = url;
      Exception::push(e);
   }

   return rval;
}

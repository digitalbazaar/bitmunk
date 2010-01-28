/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk/customcatalog/CatalogDatabase.h"

#include "bitmunk/customcatalog/CustomCatalogModule.h"
#include "monarch/data/json/JsonReader.h"
#include "monarch/net/Connection.h"
#include "monarch/rt/Exception.h"
#include "monarch/sql/Row.h"
#include "monarch/sql/Statement.h"
#include "monarch/util/StringTools.h"

#include "sqlite3.h"

using namespace bitmunk::common;
using namespace bitmunk::customcatalog;
using namespace bitmunk::medialibrary;
using namespace bitmunk::node;
using namespace monarch::data::json;
using namespace monarch::rt;
using namespace monarch::sql;
using namespace monarch::util;
using namespace std;

// tables
#define MLDB_TABLE_FILES        "bitmunk_medialibrary_files"
#define CC_TABLE_CONFIG         "bitmunk_customcatalog_config"
#define CC_TABLE_PAYEE_SCHEMES_PAYEES \
                                "bitmunk_customcatalog_payee_schemes_payees"
#define CC_TABLE_PROBLEMS       "bitmunk_customcatalog_problems"

// trigger errors for wares table
#define WARES_TRIGGER_ABORT_ERROR \
   "on table \"" CC_TABLE_WARES "\" violates foreign key constraint"
#define INSERT_WARES_TRIGGER_ABORT_ERROR "insert " WARES_TRIGGER_ABORT_ERROR

// limits
#define MAX_PAYEE_SCHEMES 255
#define MAX_ALLOCATION_ATTEMPTS MAX_PAYEE_SCHEMES
#define MAX_WARE_UPDATES_UINT64 100ULL
#define MAX_WARE_UPDATES "100"
#define MAX_PAYEE_UPDATES "100"

CatalogDatabase::CatalogDatabase()
{
}

CatalogDatabase::~CatalogDatabase()
{
}

bool CatalogDatabase::initialize(Connection* c)
{
   bool rval = true;

   /*
    Note: The statements here must only be prepared and executed
    incrementally because they modify the database schema.
    */

   /*
    Note: The wares table should really be using foreign keys (with
    the media_library_id) but this is not yet supported by sqlite's
    engine. Sqlite will parse foreign key syntax but triggers would
    have to be created (for insert/update/delete) to actually enforce
    foreign key restraints. This is attempted below.
    */

   // statement to create the configuration table
   if(rval)
   {
      Statement* s = c->prepare(
         "CREATE TABLE IF NOT EXISTS " CC_TABLE_CONFIG " ("
         "name TEXT PRIMARY KEY,"
         "value TEXT)");
      rval = (s != NULL) && s->execute();
   }

   // insert default values into the configuration table if they do not
   // already exist
   if(rval)
   {
      Statement* s = c->prepare(
         "INSERT OR IGNORE INTO " CC_TABLE_CONFIG
         "(name, value) "
         "VALUES (:name, :value)");

      // set the initial configuration values if they do not already exist
      rval = (s != NULL);
      if(rval)
      {
         rval =
            // set server ID
            s->setText(":name", "serverId") &&
            s->setText(":value", "0") &&
            s->execute() &&
            // set the server token
            s->setText(":name", "serverToken") &&
            s->setText(":value", "0") &&
            s->execute() &&
            // set the server url
            s->setText(":name", "serverUrl") &&
            s->setText(":value", "") &&
            s->execute() &&
            // set the current update ID
            s->setText(":name", "updateId") &&
            s->setText(":value", "0") &&
            s->execute();
      }
   }

   // statement to create the problems table
   if(rval)
   {
      Statement* s = c->prepare(
         "CREATE TABLE IF NOT EXISTS " CC_TABLE_PROBLEMS " ("
         "problem_id INTEGER PRIMARY KEY,"
         "text TEXT)");
      rval = (s != NULL) && s->execute();
   }

   // statement to create payee schemes table
   if(rval)
   {
      Statement* s = c->prepare(
         "CREATE TABLE IF NOT EXISTS " CC_TABLE_PAYEE_SCHEMES " ("
         "payee_scheme_id INTEGER UNSIGNED PRIMARY KEY,"
         "description TEXT,"
         "problem_id INTEGER UNSIGNED DEFAULT 0,"
         "dirty INTEGER UNSIGNED DEFAULT 1,"
         "updating INTEGER UNSIGNED DEFAULT 0,"
         "deleted INTEGER UNSIGNED DEFAULT 0)");
      rval = (s != NULL) && s->execute();
   }

   // statement to create payee schemes payees table
   if(rval)
   {
      Statement* s = c->prepare(
         "CREATE TABLE IF NOT EXISTS " CC_TABLE_PAYEE_SCHEMES_PAYEES " ("
         "payee_scheme_id INTEGER UNSIGNED NOT NULL,"
         "position INTEGER UNSIGNED NOT NULL,"
         "account_id BIGINT UNSIGNED,"
         "amount_type TEXT,"
         "description TEXT,"
         "amount TEXT,"
         "percentage TEXT,"
         "min TEXT,"
         "PRIMARY KEY(payee_scheme_id,position),"
         "FOREIGN KEY(payee_scheme_id) REFERENCES "
          CC_TABLE_PAYEE_SCHEMES " ON DELETE CASCADE)");
      rval = (s != NULL) && s->execute();
   }

   // statement to create wares table
   if(rval)
   {
      Statement* s = c->prepare(
         "CREATE TABLE IF NOT EXISTS " CC_TABLE_WARES " ("
         "media_library_id INTEGER UNSIGNED PRIMARY KEY,"
         "ware_id TEXT,"
         "description TEXT,"
         "payee_scheme_id INTEGER UNSIGNED DEFAULT 0,"
         "problem_id INTEGER UNSIGNED DEFAULT 0,"
         "dirty INTEGER UNSIGNED DEFAULT 1,"
         "updating INTEGER UNSIGNED DEFAULT 0,"
         "deleted INTEGER UNSIGNED DEFAULT 0,"
         "FOREIGN KEY(media_library_id) REFERENCES "
            MLDB_TABLE_FILES " ON DELETE CASCADE)");
      rval = (s != NULL) && s->execute();
   }

   // create triggers to handle foreign keys:

   // create trigger to prevent inserted into the ware table with an
   // invalid media library ID
   if(rval)
   {
      Statement* s = c->prepare(
         "CREATE TRIGGER IF NOT EXISTS fki_media_library_id "
         "BEFORE INSERT ON " CC_TABLE_WARES " FOR EACH ROW "
         "BEGIN "
          "SELECT RAISE(ABORT, '" INSERT_WARES_TRIGGER_ABORT_ERROR "') "
           "WHERE "
            "(SELECT media_library_id FROM " MLDB_TABLE_FILES
            " WHERE media_library_id=NEW.media_library_id) IS NULL;"
         "END;");
      rval = (s != NULL) && s->execute();
   }

   // create a trigger to update associated wares when a media library
   // file is updated
   if(rval)
   {
      Statement* s = c->prepare(
         "CREATE TRIGGER IF NOT EXISTS media_library_file_updated "
         "AFTER UPDATE ON " MLDB_TABLE_FILES " FOR EACH ROW "
         "BEGIN "
          "UPDATE " CC_TABLE_WARES " SET problem_id=0,dirty=1 "
          "WHERE media_library_id=NEW.media_library_id AND deleted=0;"
         "END;");
      rval = (s != NULL) && s->execute();
   }

   // create a trigger to mark wares as deleted when a media library file
   // is deleted
   if(rval)
   {
      // this trigger will cascade media library deletes to the wares table
      Statement* s = c->prepare(
         "CREATE TRIGGER IF NOT EXISTS fkd_media_library_id_files "
         "BEFORE DELETE ON " MLDB_TABLE_FILES " FOR EACH ROW "
         "BEGIN "
          "UPDATE " CC_TABLE_WARES " SET problem_id=0,dirty=1,deleted=1 "
          "WHERE media_library_id=OLD.media_library_id;"
         "END;");
      rval = (s != NULL) && s->execute();
   }

   // create a trigger to delete related payees when deleting a payee scheme
   if(rval)
   {
      // this trigger will cascade payee scheme deletes to the payee schemes
      // payees table
      Statement* s = c->prepare(
         "CREATE TRIGGER IF NOT EXISTS fkd_payee_scheme_id_payees "
         "BEFORE DELETE ON " CC_TABLE_PAYEE_SCHEMES " FOR EACH ROW "
         "BEGIN "
          "DELETE FROM " CC_TABLE_PAYEE_SCHEMES_PAYEES
          " WHERE payee_scheme_id=OLD.payee_scheme_id;"
         "END;");
      rval = (s != NULL) && s->execute();
   }

   return rval;
}

bool CatalogDatabase::setConfigValue(
   UserId userId, const char* name, const char* value, Connection* c)
{
   bool rval;

   MO_CAT_DEBUG(BM_CUSTOMCATALOG_CAT,
      "Setting config value '%s'='%s' for user ID: %" PRIu64,
      name, value, userId);

   Statement* s = c->prepare(
      "REPLACE INTO " CC_TABLE_CONFIG
      " (name,value) VALUES (:name,:value)");
   rval =
      (s != NULL) &&
      s->setText(":name", name) &&
      s->setText(":value", value) &&
      s->execute() && s->reset();

   return rval;
}

bool CatalogDatabase::getConfigValue(
   const char* name, string& value, Connection* c)
{
   bool rval;

   Statement* s = c->prepare(
      "SELECT value FROM " CC_TABLE_CONFIG
      " WHERE name=:name LIMIT 1");

   // retrieve the configuration value
   rval = (s != NULL) && s->setText(":name", name) && s->execute();

   if(rval)
   {
      Row* row = s->fetch();
      if(row == NULL)
      {
         // the value doesn't exist for some reason
         rval = false;
      }
      else
      {
         // the value was found, store it in the value variable
         row->getText("value", value);
         s->fetch();
      }
   }

   return rval;
}

bool CatalogDatabase::getConfigValue(
   const char* name, uint32_t& value, Connection* c)
{
   bool rval;

   Statement* s = c->prepare(
      "SELECT value FROM " CC_TABLE_CONFIG
      " WHERE name=:name LIMIT 1");

   // retrieve the configuration value
   rval = (s != NULL) && s->setText(":name", name) && s->execute();

   if(rval)
   {
      Row* row = s->fetch();
      if(row == NULL)
      {
         // the value doesn't exist for some reason
         rval = false;
      }
      else
      {
         // the value was found, store it in the value variable
         row->getUInt32("value", value);
         s->fetch();
      }
   }

   return rval;
}

bool CatalogDatabase::setTableFlags(
   const char* tableName, bool dirty, bool updating, Connection* c)
{
   bool rval = false;

   // build the query string
   string query = "UPDATE ";
   query.append(tableName);
   query.append(" SET dirty=:dirty,updating=:updating");

   // perform the update
   Statement* s = c->prepare(query.c_str());
   rval =
      (s != NULL) &&
      s->setUInt32(":dirty", dirty) &&
      s->setUInt32(":updating", updating) &&
      s->execute();

   return rval;
}

bool CatalogDatabase::purgeDeletedEntries(const char* tableName, Connection* c)
{
   bool rval = false;

   // build the delete string
   string query = "DELETE FROM ";
   query.append(tableName);
   query.append(" WHERE updating=1 AND deleted=1 AND dirty=0 AND problem_id=0");

   // perform the delete
   Statement* s = c->prepare(query.c_str());
   rval = (s != NULL) && s->execute();

   return rval;
}

bool CatalogDatabase::clearUpdatingFlags(const char* tableName, Connection* c)
{
   bool rval = false;

   // build the update string
   string query = "UPDATE ";
   query.append(tableName);
   query.append(" SET updating=0 WHERE updating=1");

   // perform the update
   Statement* s = c->prepare(query.c_str());
   rval = (s != NULL) && s->execute();

   return rval;
}

bool CatalogDatabase::populateSeller(
   UserId userId, Seller& seller, string& serverToken, Connection* c)
{
   bool rval = false;
   ServerId serverId;
   string serverUrl;

   rval =
      getConfigValue("serverId", serverId, c) &&
      getConfigValue("serverUrl", serverUrl, c) &&
      getConfigValue("serverToken", serverToken, c);

   if(rval)
   {
      seller["userId"] = userId;
      seller["serverId"] = serverId;
      seller["url"] = serverUrl.c_str();
   }

   return rval;
}

bool CatalogDatabase::isPayeeSchemeIdValid(PayeeSchemeId psId, Connection* c)
{
   bool rval = false;

   /*
    * Note: Unfortunately, we can't use triggers with payee scheme
    * IDs since they can be created at the same time as new wares.
    * This is because the trigger must check the payee scheme
    * table for an existing payee scheme... which won't be there
    * because we haven't committed our transaction yet. Therefore,
    * we only check for payee schemes here, when an ID has been
    * passed and we are therefore not creating a new scheme on
    * the fly.
    */
   Statement* s = c->prepare(
      "SELECT payee_scheme_id FROM " CC_TABLE_PAYEE_SCHEMES
      " WHERE payee_scheme_id=:psId AND deleted=0 LIMIT 1");

   // check to see if the payee scheme exists
   rval = (s != NULL) &&
      s->setUInt32(":psId", psId) &&
      s->execute();

   if(rval)
   {
      Row* row = s->fetch();
      if(row == NULL)
      {
         // payee scheme does not exist
         rval = false;
         ExceptionRef e = new Exception(
            "Payee scheme not found.",
            "bitmunk.catalog.InvalidPayeeSchemeId");
         e->getDetails()["payeeSchemeId"] = psId;
         Exception::set(e);
      }
      else
      {
         // payee scheme ID found, so finish result set
         s->fetch();
      }
   }

   return rval;
}

bool CatalogDatabase::allocateNewPayeeScheme(
   PayeeSchemeId& psId, const char* description, Connection* c)
{
   bool rval = true;

   // if the Payee scheme ID was not specified, generate the SQL to pick
   // an unused one.
   Statement* s = NULL;
   if(psId == 0)
   {
      // this crazy SQL statement picks the lowest available ID in a sparse
      // set of numbers. Therefore, if the set contains [1, 2, 3, 4, 6, 7, 8],
      // the call to the statement will return 5. If 5 becomes filled, the next
      // call to the statement will return 9.
      s = c->prepare(
         "SELECT payee_scheme_id FROM (SELECT 1 AS payee_scheme_id) q1 "
         "WHERE NOT EXISTS ("
         " SELECT 1 FROM " CC_TABLE_PAYEE_SCHEMES " WHERE payee_scheme_id = 1)"
         "UNION ALL "
         "SELECT * FROM ("
         " SELECT payee_scheme_id + 1 FROM " CC_TABLE_PAYEE_SCHEMES " t "
         "  WHERE NOT EXISTS ("
         "   SELECT 1 FROM " CC_TABLE_PAYEE_SCHEMES " ti "
         "    WHERE ti.payee_scheme_id = t.payee_scheme_id + 1)"
         " ORDER BY payee_scheme_id LIMIT 1) q2 "
         "ORDER BY payee_scheme_id LIMIT 1");

      rval = (s != NULL);
   }

   // prepare the payee scheme insert statement
   Statement* si = NULL;
   si = c->prepare(
      "INSERT INTO " CC_TABLE_PAYEE_SCHEMES
      " (payee_scheme_id,description) "
      "VALUES (:psId,:description)");
   rval = rval && (si != NULL);

   // repeatedly attempt to allocate the new payee scheme, or fail after 255
   // attempts.
   PayeeSchemeId allocatedPayeeSchemeId = 0;
   uint32_t allocationAttempts = 0;
   while(rval && (allocatedPayeeSchemeId == 0) &&
      !(allocationAttempts > MAX_ALLOCATION_ATTEMPTS))
   {
      if(psId == 0)
      {
         // find the lowest payee_scheme_id
         rval = s->execute();

         // get the new payee_scheme_id
         if(rval)
         {
            monarch::sql::Row* row = s->fetch();
            row->getUInt32("payee_scheme_id", allocatedPayeeSchemeId);
            // finish out the result set
            s->reset();
         }
      }
      else
      {
         allocatedPayeeSchemeId = psId;
      }

      // attempt to insert the payee scheme into the payee schemes table
      rval = (si != NULL) &&
         si->setUInt32(":psId", allocatedPayeeSchemeId) &&
         si->setText(":description", description) &&
         si->execute() && si->reset();

      if(!rval)
      {
         // if this attempt to allocate a payee_scheme_id failed because
         // there was a duplicate entry (due to another thread inserting
         // the entry into the database), modify the counters and try again
         ExceptionRef e = Exception::get();
         if(e->getCode() == SQLITE_CONSTRAINT)
         {
            allocatedPayeeSchemeId = 0;
            allocationAttempts++;
            rval = s->reset() && si->reset();
         }
      }
   }

   // if the allocation was successful, set psId, otherwise set the exceptions
   if(rval)
   {
      psId = allocatedPayeeSchemeId;
   }
   else if(allocationAttempts > MAX_ALLOCATION_ATTEMPTS)
   {
      ExceptionRef e = new Exception(
         "Exceeded the maximum number of payee scheme allocation attempts.",
         "bitmunk.catalog.database.PayeeSchemeIdAllocationLimitReached");
      Exception::set(e);
   }
   else
   {
      ExceptionRef e = new Exception(
         "Could not allocate a payee scheme for the given set of payees.",
         "bitmunk.catalog.database.PayeeSchemeIdAllocationFailed");
      Exception::push(e);
   }

   return rval;
}

bool CatalogDatabase::addPayeeScheme(
   Node* node, UserId userId,
   PayeeSchemeId& psId, const char* description, PayeeList& psList,
   Connection* c)
{
   bool rval = true;

   // allocate a new PayeeSchemeId
   rval = allocateNewPayeeScheme(psId, description, c);

   // insert all of the payees into the payee scheme payees table
   if(rval)
   {
      Statement* s = c->prepare(
         "INSERT INTO " CC_TABLE_PAYEE_SCHEMES_PAYEES
         " (payee_scheme_id,position,account_id,amount_type,"
         "description,amount,percentage,min) "
         "VALUES (:psId,:position,:accountId,:amountType,"
         ":description,:amount,:percentage,:min)");
      if((rval = (s != NULL)))
      {
         uint32_t position = 0;
         PayeeIterator pi = psList.getIterator();

         while(rval && pi->hasNext())
         {
            Payee& p = pi->next();
            rval =
               s->setUInt32(":psId", psId) &&
               s->setUInt32(":position", position++) &&
               s->setUInt64(":accountId",
                  p["id"]->getUInt64()) &&
               s->setText(":amountType",
                  p["amountType"]->getString()) &&
               s->setText(":description",
                  p["description"]->getString()) &&
               s->setText(":amount",
                  p["amount"]->getString()) &&
               s->setText(":percentage",
                  p["percentage"]->getString()) &&
               s->setText(":min",
                  p["min"]->getString()) &&
               s->execute() &&
               s->reset();

            // if the insert of the payee failed, note the failure in an
            // exception.
            if(!rval)
            {
               ExceptionRef e = new Exception(
                  "Failed to add a payee to the given payee scheme.",
                  "bitmunk.catalog.database.PayeeAdditionFailed");
               e->getDetails()["payee"] = p;
               e->getDetails()["payeeSchemeId"] = psId;
               Exception::push(e);
            }
         }
      }
   }

   return rval;
}

bool CatalogDatabase::updatePayeeScheme(PayeeScheme& ps, Connection* c)
{
   bool rval = false;

   // ensure payee scheme ID is valid
   PayeeSchemeId psId = ps["id"]->getUInt32();
   if(isPayeeSchemeIdValid(psId, c))
   {
      // update payee scheme description
      {
         Statement* s = c->prepare(
            "UPDATE " CC_TABLE_PAYEE_SCHEMES " SET "
            "description=:description,problem_id=0,dirty=1 "
            "WHERE payee_scheme_id=:psId");
         rval =
            (s != NULL) &&
            s->setText(":description", ps["description"]->getString()) &&
            s->setUInt32(":psId", psId) &&
            s->execute();
      }

      // delete old payees (but not entire payee scheme)
      if(rval)
      {
         Statement* s = c->prepare(
            "DELETE FROM " CC_TABLE_PAYEE_SCHEMES_PAYEES
            " WHERE payee_scheme_id=:psId");
         rval =
            (s != NULL) &&
            s->setUInt32(":psId", psId) &&
            s->execute();
      }

      // insert all of the payees into the payee scheme payees table
      if(rval)
      {
         Statement* s = c->prepare(
            "INSERT INTO " CC_TABLE_PAYEE_SCHEMES_PAYEES
            " (payee_scheme_id,position,account_id,amount_type,"
            "description,amount,percentage,min) "
            "VALUES (:psId,:position,:accountId,:amountType,"
            ":description,:amount,:percentage,:min)");
         if((rval = (s != NULL)))
         {
            uint32_t position = 0;
            PayeeIterator pi = ps["payees"].getIterator();
            while(rval && pi->hasNext())
            {
               Payee& p = pi->next();
               rval =
                  s->setUInt32(":psId", psId) &&
                  s->setUInt32(":position", position++) &&
                  s->setUInt64(":accountId",
                     p["id"]->getUInt64()) &&
                  s->setText(":amountType",
                     p["amountType"]->getString()) &&
                  s->setText(":description",
                     p["description"]->getString()) &&
                  s->setText(":amount",
                     p["amount"]->getString()) &&
                  s->setText(":percentage",
                     p["percentage"]->getString()) &&
                  s->setText(":min",
                     p["min"]->getString()) &&
                  s->execute();

               // if the insert of the payee failed, note the failure in an
               // exception.
               if(!rval)
               {
                  ExceptionRef e = new Exception(
                     "Failed to add a payee to the given payee scheme.",
                     "bitmunk.catalog.database.PayeeAdditionFailed");
                  e->getDetails()["payee"] = p;
                  e->getDetails()["payeeSchemeId"] = psId;
                  Exception::push(e);
               }
            }
         }
      }
   }

   return rval;
}

bool CatalogDatabase::populatePayeeSchemes(
   ResourceSet& rs, DynamicObject& filters, Connection* c)
{
   bool rval = false;

   // find all payee schemes that match the given filters
   // NOTE: A LIMIT statement is not used here because we must iterate through
   // all the items in the table in order to get a total count of the payee
   // schemes in the table. Two statements cannot be used due to concurrency
   // issues.
   Statement* sp = c->prepare(
      "SELECT ps.payee_scheme_id AS ps_id,ps.description AS ps_description,"
      "ps.dirty,ps.updating,ps.deleted,pb.problem_id,pb.text AS problem_text,"
      "p.position,"
      "p.account_id,p.amount_type,p.description,p.amount,p.percentage,p.min "
      "FROM " CC_TABLE_PAYEE_SCHEMES " ps "
      "LEFT JOIN " CC_TABLE_PAYEE_SCHEMES_PAYEES " p "
      "ON ps.payee_scheme_id=p.payee_scheme_id "
      "LEFT JOIN " CC_TABLE_PROBLEMS " pb "
      "ON ps.problem_id=pb.problem_id "
      "ORDER BY p.payee_scheme_id,p.position ASC");

   // create a statement to get the ware count for each payee scheme
   Statement* wareCount = c->prepare(
      "SELECT COUNT(*) AS ware_count FROM " CC_TABLE_WARES
      " WHERE payee_scheme_id=:psId");

   // initialize the result set
   rs["num"] = 0;
   rs["resources"]->setType(Array);
   rs["start"] = filters["start"]->getUInt32();

   if(sp != NULL)
   {
      rval = sp->execute();
      if(rval)
      {
         // populate all payee schemes and payees which are in payee,position
         // order via the SELECT statement above
         PayeeScheme ps(NULL);
         PayeeList payees(NULL);
         PayeeSchemeId lastPsId = 0;
         PayeeSchemeId psId = 0;
         AccountId accountId;
         string str;
         uint32_t currentPsOffset = 0;
         uint32_t startingPsOffset = filters["start"]->getUInt32();
         uint32_t psMax = filters->hasMember("num") ?
            filters["num"]->getUInt32() : MAX_PAYEE_SCHEMES;
         bool storePayees = false;

         // iterate through every payee in the system
         Row* row;
         while((row = sp->fetch()) != NULL)
         {
            // get the current payeeScheme ID
            row->getUInt32("ps_id", psId);

            // if the payee scheme ID doesn't match the last payee scheme ID
            // then we've started a new payee scheme
            if(psId != lastPsId)
            {
               // get the deleted status of the payee scheme, skipping it
               // if the payee scheme is deleted
               uint32_t deleted = 0;
               row->getUInt32("deleted", deleted);
               if(!deleted)
               {
                  // if the last payee scheme ID is not 0, then we're not on the
                  // first payee scheme and we've moved onto a new one ... which
                  // means we must increment the current payee scheme offset
                  if(lastPsId != 0)
                  {
                     currentPsOffset++;
                  }

                  // if the new offset is within the range of payee schemes
                  // specified by the filters, then create a new payee scheme
                  // to store the payees, otherwise we only step through the
                  // payees without storing them
                  if(currentPsOffset >= startingPsOffset &&
                     (uint32_t)rs["resources"]->length() < psMax)
                  {
                     // store payee scheme in resources list
                     ps = rs["resources"]->append();
                     ps["id"] = psId;
                     payees = PayeeList();
                     payees->setType(Array);
                     ps["payees"] = payees;

                     // get payee scheme description from row
                     row->getText("ps_description", str);
                     ps["description"] = str.c_str();

                     // include problem text if default filter is off
                     if(!filters["default"]->getBoolean())
                     {
                        uint64_t problemId;
                        row->getUInt64("problem_id", problemId);
                        if(problemId != 0)
                        {
                           string json;
                           row->getText("problem_text", json);
                           rval = rval && JsonReader::readFromString(
                              ps["exception"],
                              json.c_str(), json.length(), true);
                        }

                        // get dirty flag
                        uint32_t dirty = 0;
                        row->getUInt32("dirty", dirty);
                        ps["dirty"] = (dirty == 1);

                        // get updating flag
                        uint32_t updating = 0;
                        row->getUInt32("updating", updating);
                        ps["updating"] = (updating == 1);

                        // get related wares count
                        rval = rval &&
                           wareCount->setUInt32(":psId", psId) &&
                           wareCount->execute();
                        if(rval)
                        {
                           Row* row = wareCount->fetch();
                           if((rval = (row != NULL)))
                           {
                              uint64_t count = 0;
                              rval = row->getUInt64("ware_count", count);
                              if(rval)
                              {
                                 ps["wareCount"] = count;
                              }
                              wareCount->fetch();
                           }
                        }
                     }

                     storePayees = true;
                  }
                  else
                  {
                     storePayees = false;
                  }

                  // update last payee scheme ID
                  lastPsId = psId;
               }
            }

            // only store payees if applicable
            if(storePayees)
            {
               // create a payee to store current row's data
               Payee p;

               row->getUInt64("account_id", accountId);
               p["id"] = accountId;

               row->getText("amount_type", str);
               p["amountType"] = str.c_str();

               row->getText("description", str);
               p["description"] = str.c_str();

               row->getText("amount", str);
               if(str.length() > 0)
               {
                  p["amount"] = str.c_str();
               }

               row->getText("percentage", str);
               if(str.length() > 0)
               {
                  p["percentage"] = str.c_str();
               }

               row->getText("min", str);
               if(str.length() > 0)
               {
                  p["min"] = str.c_str();
               }

               // add payee to payee scheme's payee list
               payees->append(p);
            }
         }

         // update the number of resources and the total payee schemes in
         // the database
         rs["num"] = rs["resources"]->length();
         rs["total"] = currentPsOffset + 1;

         if(!rval)
         {
            ExceptionRef e = new Exception(
               "Could not populate payees. ",
               "bitmunk.catalog.database.FailedToPopulatePayeeSchemes");
            e->getDetails()["filters"] = filters;
            Exception::set(e);
            rval = false;
         }
      }
   }

   return rval;
}

bool CatalogDatabase::getFirstPayeeSchemeId(PayeeSchemeId& psId, Connection* c)
{
   bool rval = false;

   // no payee scheme yet
   psId = 0;
   Statement* s = c->prepare(
      "SELECT payee_scheme_id FROM " CC_TABLE_PAYEE_SCHEMES " LIMIT 1");
   rval = (s != NULL) && s->execute();
   if(rval)
   {
      Row* row = s->fetch();
      if(row != NULL)
      {
         rval = row->getUInt32("payee_scheme_id", psId);
         s->fetch();
      }
   }

   return rval;
}

bool CatalogDatabase::populatePayeeScheme(
   PayeeScheme& ps, DynamicObject& filters, Connection* c)
{
   bool rval = false;

   PayeeSchemeId psId = ps["id"]->getUInt32();

   // find all payees with the given payee scheme ID
   Statement* sp = c->prepare(
      "SELECT ps.description AS ps_description,ps.dirty,ps.updating,ps.deleted,"
      "pb.problem_id,pb.text AS problem_text,p.position,p.account_id,"
      "p.amount_type,p.description,p.amount,p.percentage,p.min "
      "FROM " CC_TABLE_PAYEE_SCHEMES " ps "
      "LEFT JOIN " CC_TABLE_PAYEE_SCHEMES_PAYEES " p "
      "ON ps.payee_scheme_id=p.payee_scheme_id "
      "LEFT JOIN " CC_TABLE_PROBLEMS " pb "
      "ON ps.problem_id=pb.problem_id "
      "WHERE ps.payee_scheme_id=:psId ORDER BY p.position ASC");

   if(sp != NULL)
   {
      rval = sp->setUInt32(":psId", psId) && sp->execute();
      if(rval)
      {
         // populate all payees (position-ordered by SELECT call above)
         PayeeList payees = ps["payees"];
         payees->setType(Array);
         payees->clear();
         AccountId accountId;
         string str;
         rval = false;
         Row* row;
         while((row = sp->fetch()) != NULL)
         {
            // if rval is false, it is the first time through this loop
            if(!rval)
            {
               // set the payeeScheme description
               row->getText("ps_description", str);
               ps["description"] = str.c_str();

               // include problem text and deleted status if appropriate
               if(!filters["default"]->getBoolean())
               {
                  uint64_t problemId;
                  rval = row->getUInt64("problem_id", problemId);
                  if(rval && problemId != 0)
                  {
                     string json;
                     row->getText("problem_text", json);
                     rval = rval && JsonReader::readFromString(
                        ps["exception"], json.c_str(), json.length(), true);
                  }

                  uint32_t updating = 0;
                  rval = rval && row->getUInt32("updating", updating);
                  if(rval)
                  {
                     ps["updating"] = (updating == 1);
                  }

                  uint32_t dirty = 0;
                  rval = rval && row->getUInt32("dirty", dirty);
                  if(rval)
                  {
                     ps["dirty"] = (dirty == 1);
                  }

                  uint32_t deleted = 0;
                  rval = rval && row->getUInt32("deleted", deleted);
                  if(rval)
                  {
                     ps["deleted"] = (deleted == 0) ? false : true;
                  }

                  // get related wares count
                  Statement* s = c->prepare(
                     "SELECT COUNT(*) AS ware_count FROM " CC_TABLE_WARES
                     " WHERE payee_scheme_id=:psId");
                  rval =
                     (s != NULL) &&
                     s->setUInt32(":psId", psId) &&
                     s->execute();
                  if(rval)
                  {
                     Row* row = s->fetch();
                     if((rval = (row != NULL)))
                     {
                        uint64_t count = 0;
                        rval = row->getUInt64("ware_count", count);
                        if(rval)
                        {
                           ps["wareCount"] = count;
                        }
                        s->fetch();
                     }
                  }
               }
            }
            rval = true;

            Payee p = payees->append();

            row->getUInt64("account_id", accountId);
            p["id"] = accountId;

            row->getText("amount_type", str);
            p["amountType"] = str.c_str();

            row->getText("description", str);
            p["description"] = str.c_str();

            row->getText("amount", str);
            if(str.length() > 0)
            {
               p["amount"] = str.c_str();
            }

            row->getText("percentage", str);
            if(str.length() > 0)
            {
               p["percentage"] = str.c_str();
            }

            row->getText("min", str);
            if(str.length() > 0)
            {
               p["min"] = str.c_str();
            }
         }

         if(!rval)
         {
            ExceptionRef e = new Exception(
               "Could not populate payees. Invalid payee scheme ID.",
               "bitmunk.catalog.database.InvalidPayeeSchemeId");
            e->getDetails()["payeeSchemeId"] = psId;
            Exception::set(e);
            rval = false;
         }
      }
   }

   return rval;
}

bool CatalogDatabase::deletePayeeScheme(uint32_t psId, Connection* c)
{
   bool rval = false;

   // Remove the payee scheme from the payee scheme table
   // WARNING: This should only be called when we know the payee scheme has
   // been removed from the SVA
   Statement* s = c->prepare(
      "DELETE FROM " CC_TABLE_PAYEE_SCHEMES
      " WHERE payee_scheme_id=:psId");
   rval =
      (s != NULL) &&
      s->setUInt32(":psId", psId) &&
      s->execute() && s->reset();

   return rval;
}

bool CatalogDatabase::ensurePayeeSchemeNotInUse(
   PayeeSchemeId psId, Connection* c)
{
   bool rval = false;

   Statement* s = c->prepare(
      "SELECT ware_id FROM " CC_TABLE_WARES
      " WHERE payee_scheme_id=:psId AND deleted=0 LIMIT 1");
   rval =
      (s != NULL) &&
      s->setUInt32(":psId", psId) &&
      s->execute();
   if(rval)
   {
      // if fetch is not null, then we found a ware that is
      // still using the payee scheme, which means that the payee scheme
      // is in use
      if(s->fetch() != NULL)
      {
         // finish fetching result set
         s->fetch();

         // payee scheme is in use
         ExceptionRef e = new Exception(
            "Payee scheme is in use by at least one ware.",
            "bitmunk.catalog.PayeeSchemeWareDependency");
         e->getDetails()["payeeSchemeId"] = psId;
         Exception::set(e);
         rval = false;
      }
   }

   return rval;
}

bool CatalogDatabase::removePayeeScheme(PayeeSchemeId psId, Connection* c)
{
   bool rval = false;

   // ensure the payee scheme is not in use
   if(ensurePayeeSchemeNotInUse(psId, c))
   {
      // mark the payee scheme for deletion. Doing this does not remove the
      // payee scheme from the table, but marks it for deletion
      Statement* s = c->prepare(
         "UPDATE " CC_TABLE_PAYEE_SCHEMES
         " SET problem_id=0,dirty=1,deleted=1 WHERE payee_scheme_id=:psId");
      rval =
         (s != NULL) &&
         s->setUInt32(":psId", psId) &&
         s->execute();
   }

   return rval;
}

bool CatalogDatabase::populateUpdatingPayeeSchemes(
   UserId userId, ServerId serverId, DynamicObject& payeeSchemes, Connection* c)
{
   bool rval = false;

   // FIXME: it seems like this population code needs to be done with a JOIN
   // so that the payee scheme result set is not corrupted while the individual
   // payees are being populated inside the loop (using the same connection)

   // build the list of payees that should be updated or removed
   // Note: We don't need to LIMIT the number of items that we select because
   //       the payee scheme marking code is guaranteed to mark with a
   //       LIMIT <= MAX_PAYEE_UPDATES.
   Statement* s = c->prepare(
      "SELECT payee_scheme_id,deleted FROM " CC_TABLE_PAYEE_SCHEMES
      " WHERE updating=1 AND dirty=0");
   rval = (s != NULL) && s->execute();
   if(rval)
   {
      // iterate through each payee scheme that is pending and add it to the
      // appropriate updates or removals list
      PayeeSchemeId psId;
      uint32_t deleted;
      Row* row = NULL;
      while(rval && (row = s->fetch()) != NULL)
      {
         row->getUInt32("payee_scheme_id", psId);
         row->getUInt32("deleted", deleted);

         // populate the PayeeScheme
         PayeeScheme ps;
         ps["id"] = psId;
         ps["userId"] = userId;
         ps["serverId"] = serverId;
         DynamicObject filters;
         filters["default"] = true;
         if((rval = populatePayeeScheme(ps, filters, c)))
         {
            // put listing into appropriate array based on whether or not
            // it has been deleted
            (deleted == 0) ? payeeSchemes["updates"]->append(ps) :
               payeeSchemes["removals"]->append(ps);
         }
         else
         {
            ExceptionRef e = new Exception(
               "Failed to populate payee scheme information for the given "
               "payee scheme ID.",
               "bitmunk.catalog.database.PayeeSchemeDoesNotExist");
            e->getDetails()["payeeSchemeId"] = psId;
            Exception::set(e);
         }
      }
   }

   return rval;
}

bool CatalogDatabase::updatePayeeSchemeProblemId(
   uint64_t problemId, PayeeSchemeId psId, Connection* c)
{
   bool rval = false;

   Statement* s = c->prepare(
      "UPDATE " CC_TABLE_PAYEE_SCHEMES
      " SET problem_id=:problemId "
      "WHERE payee_scheme_id=:psId AND dirty=0");
   rval =
      (s != NULL) &&
      s->setUInt64(":problemId", problemId) &&
      s->setUInt32(":psId", psId) &&
      s->execute();

   return rval;
}

bool CatalogDatabase::populateWare(
   UserId userId, Ware& ware, IMediaLibrary* mediaLibrary, Connection* c)
{
   bool rval = false;

   // select ware by its WareId
   Statement* s = c->prepare(
      "SELECT media_library_id,description,payee_scheme_id,deleted FROM "
      CC_TABLE_WARES " WHERE ware_id=:id LIMIT 1");
   if(s != NULL)
   {
      // perform the database query
      rval =
         s->setText(":id", ware["id"]->getString()) &&
         s->execute();
      if(rval)
      {
         uint32_t deleted = 0;
         monarch::sql::Row* row = s->fetch();
         if(row == NULL)
         {
            // if there are no results, the ware could not be found
            ExceptionRef e = new Exception(
               "Ware not found.",
               "bitmunk.catalog.InvalidWareId");
            e->getDetails()["wareId"] = ware["id"]->getString();
            Exception::set(e);
            rval = false;
         }
         else if((rval = row->getUInt32("deleted", deleted)) && deleted == 1)
         {
            // the ware has been deleted
            s->fetch();
            ExceptionRef e = new Exception(
               "Ware not found.",
               "bitmunk.catalog.InvalidWareId");
            e->getDetails()["wareId"] = ware["id"]->getString();
            Exception::set(e);
            rval = false;
         }
         else if(rval)
         {
            // if there are results, get the media library ID, payee scheme
            // ID and description and use those to populate the payee
            // scheme and file info
            MediaLibraryId mlId;
            PayeeSchemeId psId;
            string str;

            // retrieve data
            rval = rval &&
               row->getUInt32("media_library_id", mlId) &&
               row->getUInt32("payee_scheme_id", psId) &&
               row->getText("description", str);
            ware["mediaLibraryId"] = mlId;
            ware["payeeSchemeId"] = psId;
            ware["description"] = str.c_str();

            // finish out result set
            s->fetch();

            // populate file info and payees
            FileInfo& fi = ware["fileInfos"][0];
            PayeeScheme ps;
            ps["id"] = psId;
            DynamicObject filters;
            filters["default"] = true;
            rval = rval &&
               mediaLibrary->populateFile(userId, fi, mlId, c) &&
               populatePayeeScheme(ps, filters, c);
            if(rval)
            {
               // replace the ware media ID and payees with the information
               // in the database
               ware["mediaId"] = fi["mediaId"]->getUInt64();
               ware["payees"] = ps["payees"];
            }
         }
      }
   }

   return rval;
}

bool CatalogDatabase::populateWareSet(
   UserId userId, DynamicObject& query, ResourceSet& wareSet,
   IMediaLibrary* mediaLibrary, Connection* c)
{
   bool rval = false;

   // create map of unique ids to fetch
   DynamicObject ids;
   ids->setType(Map);
   // add ware ids
   if(query->hasMember("ids"))
   {
      DynamicObjectIterator i = query["ids"].getIterator();
      while(i->hasNext())
      {
         ids[i->next()->getString()] = true;
      }
   }

   // get wareIds from fileIds
   if(query->hasMember("fileIds"))
   {
      string sql =
         "SELECT"
         " w.ware_id"
         " FROM " CC_TABLE_WARES " w"
         " LEFT JOIN " MLDB_TABLE_FILES " f"
         " ON w.media_library_id=f.media_library_id"
         " WHERE f.file_id IN (";
      for(int i = 0, len = query["fileIds"]->length(); i < len; i++)
      {
         sql.push_back('?');
         if(i != (len - 1))
         {
            sql.push_back(',');
         }
      }
      sql.push_back(')');

      Statement* s = c->prepare(sql.c_str());
      if(s != NULL)
      {
         // perform the database query
         rval = true;
         // start at bind index 1
         int offset = 1;
         for(int i = 0, len = query["fileIds"]->length();
            rval && (i < len);
            i++)
         {
            rval = s->setText(i + offset, query["fileIds"][i]->getString());
         }
         rval = rval && s->execute();
         if(rval)
         {
            Row* row;
            while(rval && (row = s->fetch()) != NULL)
            {
               // Return the ware ID
               string wareId;

               // retrieve the ware id and add to ids to fetch
               rval = rval && row->getText("ware_id", wareId);
               if(rval)
               {
                  ids[wareId.c_str()] = true;
               }
            }
         }
      }
   }

   // init empty wareSet for request
   wareSet["resources"]->setType(Array);
   wareSet["resources"]->clear();
   wareSet["total"] = 0;
   wareSet["start"] = 0;
   wareSet["num"] =
      (query->hasMember("ids") ? query["ids"]->length() : 0) +
      (query->hasMember("fileIds") ? query["fileIds"]->length() : 0);

   if(query->hasMember("default") && query["default"]->getBoolean())
   {
      // get all details via populateWare
      DynamicObjectIterator i = ids.getIterator();
      rval = true;
      while(rval && i->hasNext())
      {
         DynamicObject& next = i->next();
         Ware ware;
         ware["id"] = next->getString();
         rval = populateWare(userId, ware, mediaLibrary, c);
         wareSet["resources"]->append(ware);
      }
   }
   else
   {
      // get basic info for list of wares
      string sql =
         "SELECT w.ware_id,w.media_library_id,w.description,w.payee_scheme_id,"
         "w.dirty,w.updating,w.deleted,pb.problem_id,pb.text AS problem_text "
         "FROM " CC_TABLE_WARES " w "
         "LEFT JOIN " CC_TABLE_PROBLEMS " pb "
         "ON w.problem_id=pb.problem_id "
         " WHERE ware_id IN (";
      DynamicObjectIterator mapi;
      mapi = ids.getIterator();
      int i = 0;
      while(mapi->hasNext())
      {
         mapi->next();
         if(i != 0)
         {
            sql.push_back(',');
         }
         sql.push_back('?');
         i++;
      }
      sql.push_back(')');

      Statement* s = c->prepare(sql.c_str());
      if(s != NULL)
      {
         // perform the database query
         rval = true;
         // start at bind index 1
         int offset = 1;
         mapi = ids.getIterator();
         while(rval && mapi->hasNext())
         {
            mapi->next();
            rval = s->setText(offset++, mapi->getName());
         }
         rval = rval && s->execute();
         if(rval)
         {
            Row* row;
            while(rval && (row = s->fetch()) != NULL)
            {
               // only populate non-deleted wares
               uint32_t deleted = 0;
               rval = row->getUInt32("deleted", deleted);
               if(rval && deleted == 0)
               {
                  // add to the ware list
                  Ware ware = wareSet["resources"]->append();

                  // Return the ware ID, media library ID, payee scheme
                  // ID and description.
                  string wareId;
                  MediaLibraryId mlId;
                  PayeeSchemeId psId;
                  string str;
                  uint64_t problemId = 0;
                  uint32_t dirty = 0;
                  uint32_t updating = 0;
                  string json;

                  // retrieve fields
                  rval = rval &&
                     row->getText("ware_id", wareId) &&
                     row->getUInt32("media_library_id", mlId) &&
                     row->getUInt32("payee_scheme_id", psId) &&
                     row->getText("description", str) &&
                     row->getUInt64("problem_id", problemId) &&
                     row->getUInt32("dirty", dirty) &&
                     row->getUInt32("updating", updating);
                  if(rval && problemId != 0)
                  {
                     rval = row->getText("problem_text", json);
                  }

                  if(rval)
                  {
                     ware["id"] = wareId.c_str();
                     ware["mediaLibraryId"] = mlId;
                     ware["payeeSchemeId"] = psId;
                     ware["description"] = str.c_str();
                     ware["dirty"] = (dirty == 1);
                     ware["updating"] = (updating == 1);
                     if(problemId != 0)
                     {
                        rval = JsonReader::readFromString(
                           ware["exception"],
                           json.c_str(), json.length(), true);
                     }
                  }
               }
            }
         }
      }
   }

   if(rval)
   {
      // fill out ResourceSet for specific requested set of ids
      // total in the number that were found
      wareSet["total"] = wareSet["resources"]->length();
   }
   else
   {
      // Fail on real errors but not being able to return results is not
      // considered an exception. Clients must check that the results match what
      // was requested.
      // set ex
      ExceptionRef e = new Exception(
         "Failed to retrieve ware set.",
         "bitmunk.catalog.WareSetRetrievalFailure");
      e->getDetails()["userId"] = userId;
      if(query->hasMember("ids"))
      {
         e->getDetails()["ids"] = query["ids"];
      }
      if(query->hasMember("fileIds"))
      {
         e->getDetails()["fileIds"] = query["fileIds"];
      }
      e->getDetails()["details"] =
         query->hasMember("details") ? query["details"]->getBoolean() : false;
      Exception::push(e);
   }

   return rval;
}

bool CatalogDatabase::populateWareFileInfo(
   UserId userId, Ware& ware, IMediaLibrary* mediaLibrary, Connection* c)
{
   bool rval = false;

   // get media library ID from wares table
   Statement* s = c->prepare(
      "SELECT media_library_id FROM " CC_TABLE_WARES
      " WHERE ware_id=:id LIMIT 1");
   if(s != NULL)
   {
      rval =
         s->setText(":id", ware["id"]->getString()) &&
         s->execute();
      if(rval)
      {
         Row* row = s->fetch();
         if(row == NULL)
         {
            ExceptionRef e = new Exception(
               "Ware not found.",
               "bitmunk.catalog.database.InvalidWareId");
            e->getDetails()["wareId"] = ware["id"]->getString();
            Exception::set(e);
            rval = false;
         }
         else
         {
            MediaLibraryId mlId;
            row->getUInt32("media_library_id", mlId);

            // finish out result set
            s->fetch();

            // populate FileInfo
            FileInfo& fi = ware["fileInfos"]->append();
            rval = mediaLibrary->populateFile(userId, fi, mlId, c);
         }
      }
   }

   return rval;
}

bool CatalogDatabase::removeWare(Ware& ware, Connection* c)
{
   bool rval = false;

   // mark ware as deleted, but do not actually delete it from the
   // database here, that will be done by a seller listing update
   Statement* s = c->prepare(
      "UPDATE " CC_TABLE_WARES
      " SET problem_id=0,dirty=1,deleted=1 WHERE ware_id=:id AND deleted=0");
   rval =
      (s != NULL) &&
      s->setText(":id", ware["id"]->getString()) &&
      s->execute();

   if(rval)
   {
      // check to make sure that the delete was successful
      uint64_t rows = 0;
      s->getRowsChanged(rows);
      if(rows == 0)
      {
         ExceptionRef e = new Exception(
            "Failed to remove a ware with an unknown WareId.",
            "bitmunk.catalog.database.WareNotFound");
         e->getDetails()["wareId"] = ware["id"]->getString();
         Exception::set(e);
         rval = false;
      }
      else
      {
         // populate the ware's payee scheme ID
         s = c->prepare(
            "SELECT payee_scheme_id FROM " CC_TABLE_WARES
            " WHERE ware_id=:id LIMIT 1");
         if(s != NULL)
         {
            // perform the database query
            rval =
               s->setText(":id", ware["id"]->getString()) &&
               s->execute();
            if(rval)
            {
               monarch::sql::Row* row = s->fetch();
               if(row != NULL)
               {
                  PayeeSchemeId psId;

                  // retrieve data
                  rval = rval && row->getUInt32("payee_scheme_id", psId);
                  ware["payeeSchemeId"] = psId;

                  // finish out result set
                  s->fetch();
               }
            }
         }
      }
   }

   return rval;
}

bool CatalogDatabase::updateWare(
   Ware& ware, PayeeSchemeId psId, Connection* c, bool* added)
{
   bool rval = false;

   // try to insert first
   bool inserted = false;
   {
      Statement* s = c->prepare(
         "INSERT OR IGNORE INTO " CC_TABLE_WARES " ("
         "media_library_id,ware_id,description,payee_scheme_id,dirty,deleted)"
         " VALUES ((SELECT media_library_id FROM " MLDB_TABLE_FILES
         " WHERE file_id=:fileId),"
         ":wareId,:description,:psId,1,0)");
      rval =
         (s != NULL) &&
         s->setText(":fileId",
            ware["fileInfos"][0]["id"]->getString()) &&
         s->setText(":wareId", ware["id"]->getString()) &&
         s->setText(":description", ware["description"]->getString()) &&
         s->setUInt32(":psId", psId) &&
         s->execute();
      if(rval)
      {
         uint64_t rows;
         s->getRowsChanged(rows);
         inserted = (rows == 1);
      }
   }

   // update if not inserted and no error
   if(rval && !inserted)
   {
      Statement* s = c->prepare(
         "UPDATE " CC_TABLE_WARES " SET "
         "ware_id=:wareId,"
         "description=:description,"
         "payee_scheme_id=:psId,"
         "problem_id=0,"
         "dirty=1,"
         "deleted=0 WHERE media_library_id="
            "(SELECT media_library_id FROM " MLDB_TABLE_FILES
            " WHERE file_id=:fileId)");
      rval =
         (s != NULL) &&
         s->setText(":fileId",
            ware["fileInfos"][0]["id"]->getString()) &&
         s->setText(":description", ware["description"]->getString()) &&
         s->setUInt32(":psId", psId) &&
         s->setText(":wareId", ware["id"]->getString()) &&
         s->execute();
   }

   if(rval && added != NULL)
   {
      *added = inserted;
   }

   return rval;
}

bool CatalogDatabase::updateWareProblemId(
   uint64_t problemId, FileId fileId, MediaId mediaId, Connection* c)
{
   bool rval = false;

   // this SQL will update the problem ID in the row in the wares
   // table where its dirty column is not set and where its
   // media library ID is equal to the media library ID in the
   // media library files row that has the given media ID
   // and file ID
   Statement* s = c->prepare(
      "UPDATE " CC_TABLE_WARES
      " SET problem_id=:problemId "
      "WHERE dirty=0 AND " CC_TABLE_WARES ".rowid IN ("
         "SELECT " CC_TABLE_WARES ".rowid FROM "
         CC_TABLE_WARES "," MLDB_TABLE_FILES " WHERE "
         CC_TABLE_WARES ".media_library_id="
         MLDB_TABLE_FILES ".media_library_id AND "
         MLDB_TABLE_FILES ".file_id=:fileId AND "
         MLDB_TABLE_FILES ".media_id=:mediaId LIMIT 1)");
   rval =
      (s != NULL) &&
      s->setUInt32(":problemId", problemId) &&
      s->setText(":fileId", fileId) &&
      s->setUInt64(":mediaId", mediaId) &&
      s->execute();

   return rval;
}

bool CatalogDatabase::populateUpdatingWares(
   UserId userId, IMediaLibrary* mediaLibrary, DynamicObject& wares,
   Connection* c)
{
   bool rval = false;

   // FIXME: it seems like this population code needs to be done with a JOIN
   // so that the ware result set is not corrupted while the files are being
   // populated inside the loop (using the same connection)

   // build the list of wares that should be included in the seller listing

   // Note: We don't need to limit this statement because there should always be
   //       less than or equal to MAX_WARE_UPDATES based on the way the
   //       ware marking code works.
   Statement* s = c->prepare(
      "SELECT media_library_id,ware_id,payee_scheme_id,deleted FROM "
      CC_TABLE_WARES " WHERE updating=1 AND dirty=0");
   rval = (s != NULL) && s->execute();
   if(rval)
   {
      // iterate through each ware that is pending and add it to the list of
      // wares to send in the seller listing update.
      MediaLibraryId mlId;
      PayeeSchemeId psId;
      uint32_t deleted = 0;
      Row* row = NULL;
      while(rval && (row = s->fetch()) != NULL)
      {
         row->getUInt32("deleted", deleted);

         if(deleted == 0)
         {
            // populate the SellerListing's file information
            row->getUInt32("media_library_id", mlId);
            row->getUInt32("payee_scheme_id", psId);
            FileInfo fi;
            if((rval = mediaLibrary->populateFile(userId, fi, mlId, c)))
            {
               // make sure to clear the path info before sending the object
               // to another node
               fi->removeMember("path");

               SellerListing sl;
               sl["fileInfo"] = fi;
               sl["payeeSchemeId"] = psId;
               wares["updates"]->append(sl);
            }
            else
            {
               ExceptionRef e = new Exception(
                  "Failed to populate file information for the given "
                  "media library ID.",
                  "bitmunk.catalog.database.FileDoesNotExist");
               e->getDetails()["mediaLibraryId"] = mlId;
               Exception::set(e);
            }
         }
         else
         {
            // populate the SellerListing's minimal information by using
            // the ware ID
            string wareId;
            row->getText("ware_id", wareId);
            DynamicObject tokens = StringTools::split(
               wareId.c_str(), "bitmunk:file:");
            DynamicObject ids = StringTools::split(
               tokens[1]->getString(), "-");
            SellerListing sl;
            sl["fileInfo"]["id"] = ids[1];
            sl["fileInfo"]["mediaId"] = ids[0];
            sl["fileInfo"]["mediaId"]->setType(UInt64);
            wares["removals"]->append(sl);
         }
      }
   }

   return rval;
}

bool CatalogDatabase::populateUpdatingSellerListings(
   UserId userId, ServerId serverId, IMediaLibrary* mediaLibrary,
   DynamicObject& wares, DynamicObject& payeeSchemes, Connection* c)
{
   bool rval = false;

   wares["updates"]->setType(Array);
   wares["removals"]->setType(Array);
   payeeSchemes["updates"]->setType(Array);
   payeeSchemes["removals"]->setType(Array);

   rval =
      // get the payee schemes that should be updated
      populateUpdatingPayeeSchemes(userId, serverId, payeeSchemes, c) &&
      // get the list of wares that should be updated
      populateUpdatingWares(userId, mediaLibrary, wares, c);

   return rval;
}

bool CatalogDatabase::markNextListingUpdate(Connection* c)
{
   bool rval = false;

   /* The algorithm for marking the next listings updates.
    *
    * This will work because an UPDATE in a transaction will lock the database:
    *
    *  1. Check to make sure there are no payees or wares that just have their
    *     updating flag set. If there are, select those for an update.
    *  2. Mark MAX_PAYEE_UPDATES that are dirty as updating and not dirty
    *     as long as an associated ware has not been deleted.
    *  3. Mark MAX_WARE_UPDATES that are deleted or (are dirty and whose
    *     payee schemes are not dirty/updating) as updating and not dirty.
    *  4. Select any updating and not dirty data.
    *
    *  Potential issues:
    *  1. If we crash after sending the update to the server but it ran
    *     the update ... we need to do the pending update again... but
    *     the user may have modified database entries.
    *
    *        Here are the possible changes that the user could have made
    *        to the things that are marked as updating:
    *
    *        1. The user deleted an updating=1 ware:
    *        If the user has deleted a ware that was going to be updated,
    *        then the ware will be deleted in the current update.
    *
    *        2. The user deleted an updating=0 payee scheme:
    *        If the user deleted a payee scheme, they could have only done
    *        so by deleting any associated updating=1 wares.
    *
    *        So, if we have a ware that was going to be updated that is in
    *        a payee scheme that was deleted... all that will happen is we will
    *        remove the ware in the update. Then in a subsequent update, we
    *        will remove the associated payee scheme. None of that should
    *        cause any significant problems.
    *
    *        3. The user updated an updating=1 ware or payee scheme:
    *        The ware or payee scheme will be marked as dirty=1 and not sent
    *        in the update. The dirty entry will be scheduled for a later
    *        update. The update we send to the server is slightly different but
    *        does not cause any significant, long term problems.
    */

   /* 1. Check to make sure that there are no payees or wares that just have
         their updating flag set. If there are, select those for an update.

    Note: We might have already marked some wares as updating ... that
    didn't complete because we failed to contact the server. Use those
    updates first, if they exist. We should never mark more than
    MAX_WARE_UPDATES/MAX_PAYEE_UPDATES because the code to process listing
    update responses assumes that there can never be more than
    MAX_WARE_UPDATES/MAX_PAYEE_UPDATES selected at a time.
    */
   bool usePending = false;
   {
      // check the ware table for updates
      Statement* ws = c->prepare(
         "SELECT ware_id FROM " CC_TABLE_WARES
         " WHERE updating=1 LIMIT 1");
      rval = (ws != NULL) && ws->execute();
      if(rval)
      {
         Row* row = ws->fetch();
         if(row != NULL)
         {
            usePending = true;
            ws->fetch();
         }
      }

      // check the payee table for updates
      if(rval && !usePending)
      {
         Statement* ps = c->prepare(
            "SELECT payee_scheme_id FROM " CC_TABLE_PAYEE_SCHEMES
            " WHERE updating=1 LIMIT 1");
         rval = (ps != NULL) && ps->execute();
         if(rval)
         {
            Row* row = ps->fetch();
            if(row != NULL)
            {
               usePending = true;
               ps->fetch();
            }
         }
      }
   }

   if(rval && !usePending)
   {
      /*
       Note: Using LIMIT in an UPDATE statement isn't always supported in
       sqlite3. It is only supported when the binary has been compiled with
       a special flag set... and the windows binary is not compiled with it.
       Therefore, we do an ugly sub-select that uses a LIMIT for portability.
       */

      // 2. Mark MAX_PAYEE_UPDATES that are dirty as updating and not dirty
      //    as long as an associated ware has not been deleted.
      Statement* ps = c->prepare(
         "UPDATE " CC_TABLE_PAYEE_SCHEMES " SET dirty=0,updating=1 "
         "WHERE payee_scheme_id IN ("
         "SELECT p.payee_scheme_id FROM " CC_TABLE_PAYEE_SCHEMES
         " p WHERE p.dirty=1 AND p.payee_scheme_id NOT IN ("
         "SELECT w.payee_scheme_id FROM " CC_TABLE_WARES
         " w WHERE w.payee_scheme_id=p.payee_scheme_id AND w.deleted=1) "
         "LIMIT " MAX_PAYEE_UPDATES ")");

      // 3. Mark MAX_WARE_UPDATES that are deleted or (are dirty and whose
      //    payee schemes are not dirty/updating) as updating and not dirty.

      // FIXME: In larger tests (such as peerbuy test) this will randomly cause
      // FIXME: segfaults in the sqlite sql parsing code. It is broken up into
      // FIXME: pieces below which appears to fix the problem.
      /*
      Statement* ws = c->prepare(
         "UPDATE " CC_TABLE_WARES " SET dirty=0,updating=1 "
         "WHERE ware_id IN ("
         "SELECT ware_id FROM " CC_TABLE_WARES " WHERE deleted=1 OR (dirty=1 "
         "AND payee_scheme_id NOT IN ("
         "SELECT payee_scheme_id FROM " CC_TABLE_PAYEE_SCHEMES
         " WHERE updating=1 OR dirty=1))"
         "LIMIT " MAX_WARE_UPDATES ")");
      */

      Statement* ws1 = c->prepare(
         "UPDATE " CC_TABLE_WARES " SET dirty=0,updating=1 "
         "WHERE ware_id IN ("
          "SELECT ware_id FROM " CC_TABLE_WARES
          " WHERE deleted=1 LIMIT " MAX_WARE_UPDATES ")");
      Statement* ws2 = c->prepare(
         "UPDATE " CC_TABLE_WARES " SET dirty=0,updating=1 "
         "WHERE ware_id IN ("
          "SELECT ware_id FROM " CC_TABLE_WARES
          " WHERE dirty=1 LIMIT ?)");
      // unmark wares that are not deleted where payee schemes are
      // updating or dirty
      Statement* ws3 = c->prepare(
         "UPDATE " CC_TABLE_WARES " SET dirty=1,updating=0 "
         "WHERE deleted=0 AND updating=1 AND payee_scheme_id IN ("
           "SELECT payee_scheme_id FROM " CC_TABLE_PAYEE_SCHEMES
           " WHERE updating=1 OR dirty=1)");
           /**/

      //rval = (ws != NULL) && (ps != NULL) && ps->execute() && ws->execute();
      rval = (ps != NULL) && ps->execute();
      rval = rval && (ws1 != NULL) && (ws2 != NULL) && (ws3 != NULL) &&
         ws1->execute();
      if(rval)
      {
         /*
          1. Get the number of deleted wares that are marked to be updated.
          2. Subtract that number from the maximum number of updates we
             are permitted to execute.
          3. If we are allowed to do more updates, then mark wares that are
             to be updated (rather than deleted) and unmark wares that are
             not deleted where their payee schemes are updating or dirty so
             that we will wait for their payee schemes to be updated first.
         */
         uint64_t rows;
         rval = ws1->getRowsChanged(rows);
         rows = MAX_WARE_UPDATES_UINT64 - rows;
         if(rval && rows > 0)
         {
            rval =
               ws2->setUInt64(1, rows) &&
               ws2->execute() &&
               ws3->execute();
         }
      }
   }

   return rval;
}

bool CatalogDatabase::getProblemId(
   const char* text, uint64_t& problemId, Connection* c)
{
   bool rval = false;

   // try to find the given text
   Statement* s = c->prepare(
      "SELECT problem_id FROM " CC_TABLE_PROBLEMS
      " WHERE text=:text LIMIT 1");
   rval =
      (s != NULL) &&
      s->setText(":text", text) &&
      s->execute();
   if(rval)
   {
      Row* row = s->fetch();
      if(row != NULL)
      {
         // get existing problem ID, finish out result set
         row->getUInt64("problem_id", problemId);
         s->fetch();
      }
      else
      {
         // add problem to table
         s = c->prepare(
            "INSERT INTO " CC_TABLE_PROBLEMS " (text) VALUES (:text)");
         rval =
            (s != NULL) &&
            s->setText(":text", text) &&
            s->execute();
         if(rval)
         {
            // get new problem ID
            problemId = s->getLastInsertRowId();
         }
      }
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Could not get problem ID.",
         "bitmunk.catalog.database.GetProblemIdFailed");
      Exception::push(e);
   }

   return rval;
}

bool CatalogDatabase::removeUnreferencedProblems(Connection* c)
{
   bool rval = false;

   // delete all problems in the problems table that do not exist in
   // the payee scheme table and also do not exist in the wares table
   Statement* s = c->prepare(
      "DELETE FROM " CC_TABLE_PROBLEMS " WHERE NOT EXISTS "
       "(SELECT problem_id FROM " CC_TABLE_PAYEE_SCHEMES " WHERE "
        CC_TABLE_PROBLEMS ".problem_id=" CC_TABLE_PAYEE_SCHEMES ".problem_id)"
       " AND NOT EXISTS "
       "(SELECT problem_id FROM " CC_TABLE_WARES " WHERE "
       CC_TABLE_PROBLEMS ".problem_id=" CC_TABLE_WARES ".problem_id)");
   rval = (s != NULL) && s->execute();

   return rval;
}

/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk/purchase/PurchaseDatabase.h"

#include "bitmunk/common/Tools.h"
#include "bitmunk/purchase/IPurchaseModule.h"
#include "bitmunk/purchase/PurchaseModule.h"
#include "monarch/data/json/JsonReader.h"
#include "monarch/data/json/JsonWriter.h"
#include "monarch/io/ByteArrayInputStream.h"
#include "monarch/sql/sqlite3/Sqlite3ConnectionPool.h"
#include "monarch/sql/Row.h"
#include "monarch/sql/Statement.h"
#include "monarch/util/StringTools.h"

using namespace std;
using namespace bitmunk::common;
using namespace bitmunk::peruserdb;
using namespace bitmunk::purchase;
using namespace bitmunk::node;
using namespace monarch::config;
using namespace monarch::data::json;
using namespace monarch::event;
using namespace monarch::io;
using namespace monarch::rt;
using namespace monarch::sql;
using namespace monarch::sql::sqlite3;
using namespace monarch::util;

#define PURCHASEDB_EXCEPTION "bitmunk.purchase.PurchaseDatabase"
#define PURCHASEDB_EXCEPTION "bitmunk.purchase.PurchaseDatabase"
#define PURCHASEDB_EXCEPTION_NOT_FOUND PURCHASEDB_EXCEPTION ".NotFound"

#define DOWNLOAD_STATES_V3_1_TABLE "download_states_v3_1"
#define SELLER_DATA_V3_1_TABLE     "seller_data_v3_1"
#define SELLER_POOLS_V3_1_TABLE    "seller_pools_v3_1"

PurchaseDatabase::PurchaseDatabase() :
   mNode(NULL),
   mPerUserDB(NULL),
   mConnectionGroupId(0)
{
}

PurchaseDatabase::~PurchaseDatabase()
{
   if(mConnectionGroupId != 0)
   {
      // unregister connection group
      mPerUserDB->removeConnectionGroup(mConnectionGroupId);
   }
}

PurchaseDatabase* PurchaseDatabase::getInstance(Node* node)
{
   PurchaseDatabase* rval = NULL;

   IPurchaseModule* ipm = dynamic_cast<IPurchaseModule*>(
      node->getModuleApi("bitmunk.purchase.Purchase"));
   if(ipm != NULL)
   {
      rval = ipm->getPurchaseDatabase();
   }
   else
   {
      ExceptionRef e = new Exception(
         "Could not get PurchaseDatabase. Purchase module not loaded.",
         "bitmunk.node.MissingModule");
      Exception::set(e);
   }

   return rval;
}

bool PurchaseDatabase::initialize(Node* node)
{
   bool rval = true;

   mNode = node;

   // get peruserdb module interface
   mPerUserDB = dynamic_cast<IPerUserDBModule*>(
      mNode->getModuleApi("bitmunk.peruserdb.PerUserDB"));
   if(mPerUserDB == NULL)
   {
      ExceptionRef e = new Exception(
         "Could not load interface for per-user database module dependency.",
         PURCHASEDB_EXCEPTION ".MissingDependency");
      Exception::set(e);
      rval = false;
   }
   else
   {
      // register connection group
      mConnectionGroupId = mPerUserDB->addConnectionGroup(this);
      if(mConnectionGroupId == 0)
      {
         ExceptionRef e = new Exception(
            "Could not register purchase database.",
            PURCHASEDB_EXCEPTION ".RegisterError");
         Exception::push(e);
         rval = false;
      }
      else
      {
         MO_CAT_INFO(BM_PURCHASE_CAT,
            "Added purchase database as "
            "connection group: %u", mConnectionGroupId);
      }
   }

   return rval;
}

bool PurchaseDatabase::getConnectionParams(
   ConnectionGroupId id, UserId userId, string& url, uint32_t& maxCount)
{
   bool rval = false;

   // get purchase database config
   Config userConfig = mNode->getConfigManager()->getModuleUserConfig(
      "bitmunk.purchase.Purchase", userId);
   if(!userConfig.isNull())
   {
      url = userConfig["database"]["url"]->getString();
      maxCount = userConfig["database"]["connections"]->getUInt32();
      rval = true;
   }
   else
   {
      ExceptionRef e = new Exception(
         "Could not get purchase database connection parameters.",
         PURCHASEDB_EXCEPTION ".ConfigError");
      Exception::push(e);
   }

   return rval;
}

/**
 * Create the meta information table if it doesn't exist.  The table is a used
 * for storage of single value subject-property-value relationships using
 * strings.
 *
 * Common use is to store database or table schema versions.  Any other data
 * can be stored though.
 *    ("database", "schema.version", "1.2.3")
 *    ("table.my_data", "schema.version", "1.0")
 *    ("database", "created", "2009-01-01 00:00:00")
 *
 * This method should be called inside a transaction and not call commit().
 *
 * @param conn an open connection to use to initialize the database which
 *           should not be closed.
 *
 * @return true if successful, false if an exception occurred.
 */
static bool _createMetaTable(Connection* conn)
{
   bool rval;

   // Create schema table if it doesn't exist.
   {
      Statement* s = conn->prepare(
         "CREATE TABLE IF NOT EXISTS bitmunk_meta ("
         " subject TEXT NOT NULL,"
         " property TEXT NOT NULL,"
         " value TEXT NOT NULL,"
         " PRIMARY KEY(subject, property))");
      if((rval = (s != NULL)))
      {
         rval = s->execute();
      }
   }

   return rval;
}

/**
 * Get the current database schema version.  This method will check if any
 * tables other than the meta table are present.  If not, then the assumption
 * is that this is a newly created database.  If other tables do exist then it
 * is assumed this is a pre-3.2 database.  The returned values will be either
 * "empty" for a new database, "< 3.2", or the current value of the
 * "schema.version" property for the "database" subject.
 *
 * This method should be called inside a transaction and not call commit().
 *
 * @param conn an open connection to use to initialize the database which
 *           should not be closed.
 * @param version string which will be assigned the version string.
 *
 * @return true if successful, false if an exception occurred.
 */
static bool _getSchemaVersion(Connection* conn, string& version)
{
   bool rval;

   // check if tables other than bitmunk_meta exist
   uint64_t tables;
   Statement* s = conn->prepare(
      "SELECT COUNT(*) FROM sqlite_master"
      " WHERE type=:type");
   rval =
      s != NULL &&
      s->setText(":type", "table") &&
      s->execute();
   if(rval)
   {
      Row* row = s->fetch();
      if(row != NULL)
      {
         rval = row->getUInt64("COUNT(*)", tables);
         s->fetch();
      }
   }
   else
   {
      ExceptionRef e = new Exception(
         "PurchaseDatabase could not get database version.",
         PURCHASEDB_EXCEPTION_NOT_FOUND);
      Exception::set(e);
      rval = false;
   }

   if(rval)
   {
      if(tables > 1)
      {
         // looks like non-empty pre-3.2 or greater
         Statement* s = conn->prepare(
            "SELECT value FROM bitmunk_meta"
            " WHERE subject=:subject AND property=:property");
         rval = s != NULL &&
            s->setText(":subject", "database") &&
            s->setText(":property", "schema.version") &&
            s->execute();
         if(rval)
         {
            Row* row = s->fetch();
            if(row != NULL)
            {
               // 3.2 or greater has version
               rval = row->getText("value", version);
               s->fetch();
            }
            else
            {
               // pre-3.2 database
               version.assign("< 3.2");
            }
         }
         else
         {
            ExceptionRef e = new Exception(
               "PurchaseDatabase could not get database version.",
               PURCHASEDB_EXCEPTION_NOT_FOUND);
            Exception::set(e);
            rval = false;
         }
      }
      else
      {
         // empty database
         version.assign("empty");
      }
   }

   return rval;
}

/**
 * Set the current database schema version.
 *
 * This method should be called inside a transaction and not call commit().
 *
 * @param conn an open connection to use to initialize the database which
 *           should not be closed.
 * @param fromVersion the old version.
 * @param toVersion the new version to set.
 *
 * @return true if successful, false if an exception occurred.
 */
static bool _setSchemaVersion(
   Connection* conn, const char* fromVersion, const char* toVersion)
{
   bool rval;

   // prepare statement
   Statement* s = conn->prepare(
      "REPLACE INTO bitmunk_meta"
      " (subject, property, value)"
      " VALUES (:subject, :property, :value)");
   rval =
      s != NULL &&
      s->setText(":subject", "database") &&
      s->setText(":property", "schema.version") &&
      s->setText(":value", toVersion) &&
      s->execute();
   if(!rval)
   {
      ExceptionRef e = new Exception(
         "PurchaseDatabase could not set database schema version.",
         PURCHASEDB_EXCEPTION ".Exception");
      e->getDetails()["fromVersion"] = fromVersion;
      e->getDetails()["toVersion"] = toVersion;
      Exception::push(e);
   }

   return rval;
}

/**
 * Create purchase tables in the latest format if they don't exist. This method
 * should be called inside a transaction and not call commit().
 *
 * @param conn an open connection to use to initialize the database which
 *           should not be closed.
 *
 * @return true if successful, false if an exception occurred.
 */
static bool _createTables(Connection* conn)
{
   bool rval = false;

   // create tables if they do not exist

   // Note: the statements here must only be prepared and executed
   // incrementally because they modify the database schema

   // FIXME: add a version table that can be used to update
   // schema for purchase database?

   // statement to create download states table
   {
      Statement* s = conn->prepare(
         "CREATE TABLE IF NOT EXISTS download_states ("
         "download_state_id INTEGER PRIMARY KEY AUTOINCREMENT,"
         "user_id BIGINT UNSIGNED,"
         "version VARCHAR(10),"
         "processing INTEGER UNSIGNED DEFAULT 0,"
         "processor_id INTEGER UNSIGNED DEFAULT 0,"
         "ware TEXT,"
         "total_min_price TEXT,"
         "total_med_price TEXT,"
         "total_max_price TEXT,"
         "total_piece_count INTEGER UNSIGNED,"
         "total_micro_payment_cost TEXT,"
         "preferences TEXT,"
         "start_date TEXT,"
         "remaining_pieces INTEGER UNSIGNED,"
         "initialized TINYINT UNSIGNED,"
         "license_acquired TINYINT UNSIGNED,"
         "download_started TINYINT UNSIGNED,"
         "download_paused TINYINT UNSIGNED,"
         "license_purchased TINYINT UNSIGNED,"
         "data_purchased TINYINT UNSIGNED,"
         "files_assembled TINYINT UNSIGNED)");
      if((rval = (s != NULL)))
      {
         rval = s->execute();
      }
   }

   // statement to create contracts table
   // Note: In the future, this table could be used to shared licenses
   // between differing downloads to prevent double paying for licenses.
   // contract ID is zero for unpurchased license, otherwise non-zero
   if(rval)
   {
      Statement* s = conn->prepare(
         "CREATE TABLE IF NOT EXISTS contracts ("
         "download_state_id INTEGER PRIMARY KEY,"
         "user_id BIGINT UNSIGNED,"
         "media_id BIGINT UNSIGNED,"
         "contract_id BIGINT UNSIGNED,"
         "contract TEXT)");
      if((rval = (s != NULL)))
      {
         rval = s->execute();
      }
   }

   // FIXME: create separate table to map file keys to file IDs
   // to allow sharing between seller_data, seller_pools, and
   // file_pieces tables

   // statement to create seller data table
   if(rval)
   {
      Statement* s = conn->prepare(
         "CREATE TABLE IF NOT EXISTS seller_data ("
         "download_state_id BIGINT UNSIGNED,"
         "user_id BIGINT UNSIGNED,"
         "file_id VARCHAR(40),"
         "price VARCHAR(20),"
         "section TEXT)");
      if((rval = (s != NULL)))
      {
         rval = s->execute();
      }
   }

   // statement to create index for seller data table
   if(rval)
   {
      Statement* s = conn->prepare(
         "CREATE INDEX IF NOT EXISTS seller_data_index ON seller_data ("
         "download_state_id,user_id,file_id)");
      if((rval = (s != NULL)))
      {
         rval = s->execute();
      }
   }

   // statement to create seller pools table
   if(rval)
   {
      // FIXME: in the future, these could be shared amongst multiple
      // download states
      Statement* s = conn->prepare(
         "CREATE TABLE IF NOT EXISTS seller_pools ("
         "download_state_id BIGINT UNSIGNED,"
         "user_id BIGINT UNSIGNED,"
         "file_id VARCHAR(40),"
         "seller_pool TEXT,"
         "micro_payment_cost TEXT,"
         "budget TEXT,"
         "PRIMARY KEY(download_state_id,user_id,file_id))");
      if((rval = (s != NULL)))
      {
         rval = s->execute();
      }
   }

   // statement to create file pieces table
   // Note: In the future, this table could be used to share file pieces
   // between differing contracts to prevent double paying for data.
   if(rval)
   {
      Statement* s = conn->prepare(
         "CREATE TABLE IF NOT EXISTS file_pieces ("
         "download_state_id BIGINT UNSIGNED,"
         "user_id BIGINT UNSIGNED,"
         "file_id VARCHAR(40),"
         "piece_index INTEGER UNSIGNED,"
         "valid TINYINT UNSIGNED,"
         "section_hash VARCHAR(40),"
         "status VARCHAR(12),"
         "file_piece TEXT)");
      if((rval = (s != NULL)))
      {
         rval = s->execute();
      }
   }

   // FIXME: also store "bad/blacklisted" sellers here or in another
   // table?
   // statement to create favorite sellers table
   if(rval)
   {
      Statement* s = conn->prepare(
         "CREATE TABLE IF NOT EXISTS sellers ("
         "user_id BIGINT UNSIGNED PRIMARY KEY,"
         "server_id INTEGER UNSIGNED,"
         "transfer_rate VARCHAR(20),"
         "seller TEXT)");
      if((rval = (s != NULL)))
      {
         rval = s->execute();
      }
   }

   // FIXME: The functionality provided by this table should move to the
   // FIXME: media library.
   // statement to create assemebled files table
   if(rval)
   {
      Statement* s = conn->prepare(
         "CREATE TABLE IF NOT EXISTS assembled_files ("
         "download_state_id BIGINT UNSIGNED,"
         "user_id BIGINT UNSIGNED,"
         "file_id VARCHAR(40),"
         "path TEXT,"
         "PRIMARY KEY(download_state_id,user_id,file_id))");
      if((rval = (s != NULL)))
      {
         rval = s->execute();
      }
   }

   return rval;
}

/**
 * Drop tables used for migration. This should only be called when it is safe
 * to drop migration tables. Due to locking issues this has to be called
 * outside of the migration transaction.
 */
static bool _dropMigrationTables(Connection* conn)
{
   bool rval = true;

   const char* tables[] = {
      DOWNLOAD_STATES_V3_1_TABLE,
      SELLER_DATA_V3_1_TABLE,
      SELLER_POOLS_V3_1_TABLE,
      NULL
   };

   DynamicObject sql;
   int i;
   for(i = 0; rval && tables[i] != NULL; i++)
   {
      sql->format("DROP TABLE IF EXISTS %s", tables[i]);
      Statement* s = conn->prepare(sql->getString());
      rval = (s != NULL) && s->execute();
      if(!rval)
      {
         ExceptionRef e = new Exception(
            "PurchaseDatabase could not drop migration table.",
            PURCHASEDB_EXCEPTION ".Exception");
         e->getDetails()["table"] = tables[i];
         Exception::push(e);
      }
   }

   return rval;
}

bool PurchaseDatabase::initializePerUserDatabase(
   ConnectionGroupId id, UserId userId, Connection* conn,
   DatabaseClientRef& dbc)
{
   bool rval;

   // safe to drop migration tables here
   rval = _dropMigrationTables(conn);

   // create tables if they do not exist
   if(rval && (rval = conn->begin()))
   {
      // Note: the statements here must only be prepared and executed
      // incrementally because they modify the database schema

      // Using per-database versioning for this database.
      // Known versions:
      //    "empty" if new
      //    "< 3.2" for pre-3.2
      //    "3.2" or greater

      // The version we are starting from.
      string fromVersion;
      // The current version we are initializing to.
      const char* toVersion = "3.2";

      rval =
         _createMetaTable(conn) &&
         _getSchemaVersion(conn, fromVersion);

      if(rval)
      {
         if(strcmp(fromVersion.c_str(), toVersion) == 0)
         {
            // database version is current
         }
         else if(strcmp(fromVersion.c_str(), "empty") == 0)
         {
            // create new tables
            MO_CAT_INFO(BM_PURCHASE_CAT, "Creating purchase database.");
            rval = _createTables(conn);
         }
         else
         {
            // Migrate in sequence from earliest to most recent version.
            // If needed could be optimized to jump steps.
            // current version while migrating
            const char* currentVersion = fromVersion.c_str();
            if(strcmp(currentVersion, "< 3.2") == 0)
            {
               // Upgrade download_states so id uses autoincrement mode.
               // This is done to avoid ids being reused.
               MO_CAT_INFO(BM_PURCHASE_CAT,
                  "Upgrading purchase database to version 3.2.");

               // database alteration section
               {
                  // rename old download state table
                  Statement* s = conn->prepare(
                     "ALTER TABLE download_states RENAME TO "
                     DOWNLOAD_STATES_V3_1_TABLE);
                  rval = (s != NULL) && s->execute();
               }

               // database alteration section
               if(rval)
               {
                  // rename old seller_data table
                  Statement* s = conn->prepare(
                     "ALTER TABLE seller_data RENAME TO "
                     SELLER_DATA_V3_1_TABLE);
                  rval = (s != NULL) && s->execute();
               }

               // database alteration section
               if(rval)
               {
                  // rename old seller_pools table
                  Statement* s = conn->prepare(
                     "ALTER TABLE seller_pools RENAME TO "
                     SELLER_POOLS_V3_1_TABLE);
                  rval = (s != NULL) && s->execute();
               }

               // create new table(s)
               rval = rval && _createTables(conn);

               // copy download_states data
               if(rval)
               {
                  // column changes require us to insert defaults
                  Statement* s = conn->prepare(
                     "INSERT INTO download_states "
                     "SELECT "
                     "download_state_id,"
                     "user_id,"
                     "version,"
                     "0," // processing (will be updated)
                     "0," // processor_id (will be updated
                     "ware,"
                     "total_med_price AS total_min_price,"
                     "total_med_price,"
                     "total_med_price AS total_max_price,"
                     "0,"      // total_piece_count (will be updated)
                     "'0.00'," // total micropayment cost (will be updated)
                     "preferences,"
                     "start_date,"
                     "remaining_pieces,"
                     "initialized,"
                     "license_acquired,"
                     "download_started,"
                     "download_paused,"
                     "license_purchased,"
                     "data_purchased,"
                     "files_assembled "
                     "FROM " DOWNLOAD_STATES_V3_1_TABLE);
                  rval = (s != NULL) && s->execute();
               }

               // copy seller_data data
               if(rval)
               {
                  // column changes are compatible, just copy over rows
                  Statement* s = conn->prepare(
                     "INSERT INTO seller_data "
                     "SELECT * FROM " SELLER_DATA_V3_1_TABLE);
                  rval = (s != NULL) && s->execute();
               }

               // copy seller_pools data
               if(rval)
               {
                  // column changes require us to insert defaults
                  Statement* s = conn->prepare(
                     "INSERT INTO seller_pools "
                     "SELECT "
                     "download_state_id,"
                     "user_id,"
                     "file_id,"
                     "seller_pool,"
                     "'0.00'," // micro_payment_cost (will be updated)
                     "'0.00' " // budget (will be updated)
                     "FROM " SELLER_POOLS_V3_1_TABLE);
                  rval = (s != NULL) && s->execute();
               }

               // Note: old table removed outside of transaction below
               currentVersion = "3.2";
            }
            // Further migrations can be done in sequence as:
            //if(rval && strcmp(currentVersion, "3.2") == 0)
            //{
            //   ...
            //   currentVersion = "3.x";
            //}
            //...
         }
      }

      // set schema version to latest if needed
      if(rval && strcmp(fromVersion.c_str(), toVersion) != 0)
      {
         rval = _setSchemaVersion(conn, fromVersion.c_str(), toVersion);
      }

      // commit transaction if everything worked
      rval = rval ? conn->commit() : conn->rollback() && false;

      // drop migration tables outside of transaction
      // FIXME: Drop is successful but tables still exist for unknown reason.
      // FIXME: However, they are successfully dropped before the above
      // FIXME: transaction.
      // FIXME: The next time it comes through in this code it's going to
      // drop them at the top ... but this may already be working and it
      // hasn't been checked it after we fixed the bug where we weren't
      // calling reset() or finalize() on sqlite statements
      rval = rval && _dropMigrationTables(conn);

      // clear all processing download states
      rval = rval && clearProcessingDownloadStates(userId, conn);
   }

   // close connection
   conn->close();

   return rval;
}

bool PurchaseDatabase::insertDownloadState(DownloadState& ds, Connection* conn)
{
   bool rval = false;

   UserId userId = BM_USER_ID(ds["userId"]);

   // get database connection
   Connection* c = (conn == NULL ? getConnection(userId) : conn);
   if(c != NULL)
   {
      // prepare statement to insert top-level state data
      Statement* s = c->prepare(
         "INSERT INTO download_states "
         "(user_id,version,ware,"
         "total_min_price,total_med_price,total_max_price,"
         "total_piece_count,total_micro_payment_cost,"
         "preferences,start_date,"
         "remaining_pieces,initialized,license_acquired,"
         "download_started,download_paused,"
         "license_purchased,data_purchased,files_assembled) "
         "VALUES "
         "(:userId,:version,:ware,"
         ":totalMinPrice,:totalMedPrice,:totalMaxPrice,"
         ":totalPieceCount,:totalMicroPaymentCost,"
         ":preferences,:startDate,"
         "0,0,0,0,0,0,0,0)");
      if(s != NULL && (rval = c->begin()))
      {
         string wareJson = JsonWriter::writeToString(ds["ware"], true);
         string prefsJson = JsonWriter::writeToString(ds["preferences"], true);

         // set parameters and execute statement
         rval =
            s->setUInt64(":userId", userId) &&
            s->setText(":version", "3.0") &&
            s->setText(":ware", wareJson.c_str()) &&
            s->setText(":totalMinPrice", "0.00") &&
            s->setText(":totalMedPrice", "0.00") &&
            s->setText(":totalMaxPrice", "0.00") &&
            s->setUInt32(":totalPieceCount", 0) &&
            s->setText(":totalMicroPaymentCost", "0.00") &&
            s->setText(":preferences", prefsJson.c_str()) &&
            s->setText(":startDate", ds["startDate"]->getString()) &&
            s->execute();

         if(rval)
         {
            // get autoincremented ID for download state
            ds["id"] = s->getLastInsertRowId();

            // insert contract and seller pools
            rval = insertContract(ds, c);
         }

         // commit transaction
         rval = rval ? c->commit() : c->rollback() && false;

         if(rval)
         {
            MO_CAT_INFO(BM_PURCHASE_CAT,
               "UserId %" PRIu64 ", DownloadState %" PRIu64 ": created",
               userId, ds["id"]->getUInt64());
         }
      }

      if(conn == NULL)
      {
         // close connection
         c->close();
      }
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Could not insert DownloadState.",
         PURCHASEDB_EXCEPTION ".Exception");
      BM_ID_SET(e->getDetails()["userId"], userId);
      Exception::push(e);
   }

   return rval;
}

bool PurchaseDatabase::deleteDownloadState(DownloadState& ds, Connection* conn)
{
   bool rval = false;

   UserId userId = BM_USER_ID(ds["userId"]);
   DownloadStateId dsId = ds["id"]->getUInt64();

   // get database connection
   Connection* c = (conn == NULL ? getConnection(userId) : conn);
   if(c != NULL)
   {
      // prepare statements to delete data from purchase database
      Statement* delFilePieces = c->prepare(
         "DELETE FROM file_pieces "
         "WHERE download_state_id=:downloadStateId");
      Statement* delSellerData = c->prepare(
         "DELETE FROM seller_data "
         "WHERE download_state_id=:downloadStateId");
      Statement* delContracts = c->prepare(
         "DELETE FROM contracts "
         "WHERE download_state_id=:downloadStateId");
      Statement* delStates = c->prepare(
         "DELETE FROM download_states "
         "WHERE download_state_id=:downloadStateId");
      Statement* delSellerPools = c->prepare(
         "DELETE FROM seller_pools "
         "WHERE download_state_id=:downloadStateId");
      Statement* delAssembledFiles = c->prepare(
         "DELETE FROM assembled_files "
         "WHERE download_state_id=:downloadStateId");

      if((delFilePieces != NULL) && (delSellerData != NULL) &&
         (delContracts != NULL) && (delStates != NULL) &&
         (delSellerPools != NULL) && (delAssembledFiles != NULL) &&
         c->begin())
      {
         // set parameters and execute statements to delete download state from
         // all tables
         rval =
            delFilePieces->setUInt64(":downloadStateId", dsId) &&
            delFilePieces->execute() &&
            delSellerData->setUInt64(":downloadStateId", dsId) &&
            delSellerData->execute() &&
            delContracts->setUInt64(":downloadStateId", dsId) &&
            delContracts->execute() &&
            delStates->setUInt64(":downloadStateId", dsId) &&
            delStates->execute() &&
            delSellerPools->setUInt64(":downloadStateId", dsId) &&
            delSellerPools->execute() &&
            delAssembledFiles->setUInt64(":downloadStateId", dsId) &&
            delAssembledFiles->execute();

         // commit transaction
         rval = rval ? c->commit() : c->rollback() && false;

         if(rval)
         {
            MO_CAT_INFO(BM_PURCHASE_CAT,
               "UserId %" PRIu64 ", DownloadState %" PRIu64 ": "
               "deleted", userId, dsId);
         }
      }

      if(conn == NULL)
      {
         // close connection
         c->close();
      }
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Could not delete DownloadState.",
         PURCHASEDB_EXCEPTION ".Exception");
      BM_ID_SET(e->getDetails()["userId"], userId);
      e->getDetails()["downloadStateId"] = dsId;
      Exception::push(e);
   }

   return rval;
}

bool PurchaseDatabase::startProcessingDownloadState(
   DownloadState& ds, uint32_t processorId, Connection* conn)
{
   bool rval = false;

   UserId userId = BM_USER_ID(ds["userId"]);
   DownloadStateId dsId = ds["id"]->getUInt64();

   // get database connection
   Connection* c = (conn == NULL ? getConnection(userId) : conn);
   if(c != NULL)
   {
      // do update in a database transaction to ensure download state exists
      // and to set more informative errors if it doesn't
      if((rval = c->begin()))
      {
         // ensure download state exists
         {
            Statement* s = c->prepare(
               "SELECT download_state_id FROM download_states WHERE "
               "user_id=:userId AND download_state_id=:dsId LIMIT 1");
            rval =
               (s != NULL) &&
               s->setUInt64(":userId", userId) &&
               s->setUInt64(":dsId", dsId) &&
               s->execute();
            if(rval)
            {
               Row* row = s->fetch();
               if(row == NULL)
               {
                  ExceptionRef e = new Exception(
                     "DownloadState does not exist.",
                     PURCHASEDB_EXCEPTION_NOT_FOUND);
                  Exception::set(e);
                  rval = false;
               }
               else
               {
                  s->fetch();
               }
            }
         }

         // mark download state as processing
         if(rval)
         {
            Statement* s = c->prepare(
               "UPDATE download_states SET "
               "processing=1,processor_id=:processorId "
               "WHERE user_id=:userId AND download_state_id=:dsId AND "
               "processing=0");
            rval =
               (s != NULL) &&
               s->setUInt64(":userId", userId) &&
               s->setUInt64(":dsId", dsId) &&
               s->setUInt32(":processorId", processorId) &&
               s->execute();
            if(rval)
            {
               uint64_t rows = 0;
               s->getRowsChanged(rows);
               if(rows == 0)
               {
                  ExceptionRef e = new Exception(
                     "DownloadState is already being processed.",
                     PURCHASEDB_EXCEPTION ".AlreadyProcessing");
                  Exception::set(e);
                  rval = false;
               }
            }
         }

         // finish database transaction
         rval = rval ? c->commit() : c->rollback() && false;
      }

      if(conn == NULL)
      {
         // close connection
         c->close();
      }
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Could not start processing DownloadState.",
         PURCHASEDB_EXCEPTION ".Exception");
      e->getDetails()["downloadStateId"] = dsId;
      BM_ID_SET(e->getDetails()["userId"], userId);
      Exception::push(e);
   }

   return rval;
}

bool PurchaseDatabase::setDownloadStateProcessorId(
   DownloadState& ds, uint32_t oldId, uint32_t newId, Connection* conn)
{
   bool rval = false;

   UserId userId = BM_USER_ID(ds["userId"]);
   DownloadStateId dsId = ds["id"]->getUInt64();

   // get database connection
   Connection* c = (conn == NULL ? getConnection(userId) : conn);
   if(c != NULL)
   {
      // do update in a database transaction to ensure download state exists
      // and to set more informative errors if it doesn't
      if((rval = c->begin()))
      {
         // ensure download state exists
         {
            Statement* s = c->prepare(
               "SELECT download_state_id FROM download_states WHERE "
               "user_id=:userId AND download_state_id=:dsId LIMIT 1");
            rval =
               (s != NULL) &&
               s->setUInt64(":userId", userId) &&
               s->setUInt64(":dsId", dsId) &&
               s->execute();
            if(rval)
            {
               Row* row = s->fetch();
               if(row == NULL)
               {
                  ExceptionRef e = new Exception(
                     "DownloadState does not exist.",
                     PURCHASEDB_EXCEPTION_NOT_FOUND);
                  Exception::set(e);
                  rval = false;
               }
               else
               {
                  s->fetch();
               }
            }
         }

         // mark download state as processing
         if(rval)
         {
            Statement* s = c->prepare(
               "UPDATE download_states SET "
               "processing=1,processor_id=:newId "
               "WHERE user_id=:userId AND download_state_id=:dsId AND "
               "processor_id=:oldId");
            rval =
               (s != NULL) &&
               s->setUInt64(":userId", userId) &&
               s->setUInt64(":dsId", dsId) &&
               s->setUInt32(":oldId", oldId) &&
               s->setUInt32(":newId", newId) &&
               s->execute();
            if(rval)
            {
               uint64_t rows = 0;
               s->getRowsChanged(rows);
               if(rows == 0)
               {
                  ExceptionRef e = new Exception(
                     "Existing DownloadState processor ID does not match.",
                     PURCHASEDB_EXCEPTION ".InvalidProcessorId");
                  Exception::set(e);
                  rval = false;
               }
            }
         }

         // finish database transaction
         rval = rval ? c->commit() : c->rollback() && false;
      }

      if(conn == NULL)
      {
         // close connection
         c->close();
      }
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Could not set DownloadState processor ID.",
         PURCHASEDB_EXCEPTION ".Exception");
      e->getDetails()["downloadStateId"] = dsId;
      BM_ID_SET(e->getDetails()["userId"], userId);
      Exception::push(e);
   }

   return rval;
}

bool PurchaseDatabase::stopProcessingDownloadState(
   DownloadState& ds, uint32_t processorId, Connection* conn)
{
   bool rval = false;

   UserId userId = BM_USER_ID(ds["userId"]);
   DownloadStateId dsId = ds["id"]->getUInt64();

   // get database connection
   Connection* c = (conn == NULL ? getConnection(userId) : conn);
   if(c != NULL)
   {
      // prepare statement
      Statement* s = c->prepare(
         "UPDATE download_states SET processing=0,processor_id=0 "
         "WHERE user_id=:userId AND download_state_id=:dsId AND "
         "processor_id=:processorId");
      rval =
         (s != NULL) &&
         s->setUInt64(":userId", userId) &&
         s->setUInt64(":dsId", dsId) &&
         s->setUInt32(":processorId", processorId) &&
         s->execute();

      if(conn == NULL)
      {
         // close connection
         c->close();
      }
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Could not stop processing DownloadState.",
         PURCHASEDB_EXCEPTION ".Exception");
      e->getDetails()["downloadStateId"] = dsId;
      BM_ID_SET(e->getDetails()["userId"], userId);
      Exception::push(e);
   }

   return rval;
}

bool PurchaseDatabase::populateProcessingInfo(
   DownloadState& ds, Connection* conn)
{
   bool rval = false;

   UserId userId = BM_USER_ID(ds["userId"]);
   DownloadStateId dsId = ds["id"]->getUInt64();

   // get database connection
   Connection* c = (conn == NULL ? getConnection(userId) : conn);
   if(c != NULL)
   {
      // prepare statement
      Statement* s = c->prepare(
         "SELECT processing,processor_id FROM download_states "
         "WHERE user_id=:userId AND download_state_id=:dsId LIMIT 1");
      rval =
         (s != NULL) &&
         s->setUInt64(":userId", userId) &&
         s->setUInt64(":dsId", dsId) &&
         s->execute();
      if(rval)
      {
         Row* row = s->fetch();
         if(row == NULL)
         {
            ExceptionRef e = new Exception(
               "DownloadState does not exist.",
               PURCHASEDB_EXCEPTION_NOT_FOUND);
            Exception::set(e);
            rval = false;
         }
         else
         {
            uint32_t num;
            rval = rval && row->getUInt32("processor_id", num);
            ds["processorId"] = num;
            rval = rval && row->getUInt32("processing", num);
            ds["processing"] = (num == 1);
            s->fetch();
         }
      }

      if(conn == NULL)
      {
         // close connection
         c->close();
      }
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Could not populate DownloadState processing information.",
         PURCHASEDB_EXCEPTION);
      e->getDetails()["downloadStateId"] = dsId;
      BM_ID_SET(e->getDetails()["userId"], userId);
      Exception::push(e);
   }

   return rval;
}

bool PurchaseDatabase::clearProcessingDownloadStates(
   UserId userId, Connection* conn)
{
   bool rval = false;

   // get database connection
   Connection* c = (conn == NULL ? getConnection(userId) : conn);
   if(c != NULL)
   {
      // prepare statement
      Statement* s = c->prepare(
         "UPDATE download_states SET processing=0,processor_id=0 "
         "WHERE user_id=:userId");
      rval =
         (s != NULL) &&
         s->setUInt64(":userId", userId) &&
         s->execute();

      if(conn == NULL)
      {
         // close connection
         c->close();
      }
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Could not clear processing DownloadStates.",
         PURCHASEDB_EXCEPTION ".Exception");
      BM_ID_SET(e->getDetails()["userId"], userId);
      Exception::push(e);
   }

   return rval;
}

bool PurchaseDatabase::updatePreferences(DownloadState& ds, Connection* conn)
{
   bool rval = false;

   UserId userId = BM_USER_ID(ds["userId"]);
   DownloadStateId dsId = ds["id"]->getUInt64();

   // get database connection
   Connection* c = (conn == NULL ? getConnection(userId) : conn);
   if(c != NULL)
   {
      // prepare statement
      Statement* s = c->prepare(
         "UPDATE download_states SET "
         "preferences=:preferences "
         "WHERE user_id=:userId AND download_state_id=:dsId");
      if(s != NULL)
      {
         string json = JsonWriter::writeToString(ds["preferences"], true);

         // set parameters and execute statement
         rval =
            s->setUInt64(":userId", userId) &&
            s->setUInt64(":dsId", dsId) &&
            s->setText(":preferences", json.c_str()) &&
            s->execute();

         if(rval)
         {
            uint64_t rows = 0;
            s->getRowsChanged(rows);
            if(rows == 0)
            {
               ExceptionRef e = new Exception(
                  "DownloadState does not exist.",
                  PURCHASEDB_EXCEPTION_NOT_FOUND);
               Exception::set(e);
               rval = false;
            }
         }
      }

      if(conn == NULL)
      {
         // close connection
         c->close();
      }
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Could not update DownloadState preferences.",
         PURCHASEDB_EXCEPTION ".Exception");
      e->getDetails()["downloadStateId"] = dsId;
      BM_ID_SET(e->getDetails()["userId"], userId);
      Exception::push(e);
   }

   return rval;
}

bool PurchaseDatabase::insertSellerPools(DownloadState& ds, Connection* conn)
{
   bool rval = false;

   UserId userId = BM_USER_ID(ds["userId"]);
   DownloadStateId dsId = ds["id"]->getUInt64();

   // get database connection
   Connection* c = (conn == NULL ? getConnection(userId) : conn);
   if(c != NULL)
   {
      // prepare statement
      Statement* s = c->prepare(
         "REPLACE INTO seller_pools "
         "(download_state_id,user_id,file_id,seller_pool,"
         "micro_payment_cost,budget) "
         "VALUES "
         "(:dsId,:userId,:fileId,:sellerPool,:microPaymentCost,:budget)");
      if((rval = (s != NULL)))
      {
         FileInfoIterator fii = ds["ware"]["fileInfos"].getIterator();
         while(rval && fii->hasNext())
         {
            FileInfo& fi = fii->next();
            FileId fileId = fi["id"]->getString();

            // produce seller pool json
            FileProgress& fp = ds["progress"][fileId];
            fp["fileInfo"] = fi;
            SellerPool& sp = fp["sellerPool"];
            sp["fileInfo"] = fi;
            sp["sellerDataSet"]["start"] = 0;
            sp["sellerDataSet"]["num"] = 0;
            sp["pieceSize"] = 0;
            sp["pieceCount"] = 0;
            sp["bfpId"] = 0;
            sp["stats"]["listingCount"] = 0;
            string json = JsonWriter::writeToString(sp, true);

            // set parameters, execute statement, reset for next execution
            rval =
               s->setUInt64(":dsId", dsId) &&
               s->setUInt64(":userId", userId) &&
               s->setText(":fileId", fileId) &&
               s->setText(":sellerPool", json.c_str()) &&
               s->setText(":microPaymentCost", "0.00") &&
               s->setText(":budget", "0.00") &&
               s->execute() &&
               s->reset();
         }
      }

      if(conn == NULL)
      {
         // close connection
         c->close();
      }
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Could not insert DownloadState seller pools.",
         PURCHASEDB_EXCEPTION);
      e->getDetails()["downloadStateId"] = dsId;
      BM_ID_SET(e->getDetails()["userId"], userId);
      Exception::push(e);
   }

   return rval;
}

bool PurchaseDatabase::updateSellerPool(
   DownloadState& ds, SellerPool& sp, Connection* conn)
{
   bool rval = false;

   UserId userId = BM_USER_ID(ds["userId"]);
   DownloadStateId dsId = ds["id"]->getUInt64();

   // get database connection
   Connection* c = (conn == NULL ? getConnection(userId) : conn);
   if(c != NULL)
   {
      // prepare statement
      Statement* s = c->prepare(
         "UPDATE seller_pools SET "
         "seller_pool=:sellerPool,"
         "micro_payment_cost=:microPaymentCost,"
         "budget=:budget "
         "WHERE user_id=:userId AND download_state_id=:dsId "
         "AND file_id=:fileId");
      if(s != NULL)
      {
         string json = JsonWriter::writeToString(sp, true);
         FileId fileId = sp["fileInfo"]["id"]->getString();
         FileProgress& fp = ds["progress"][fileId];

         // set parameters and execute statement
         rval =
            s->setUInt64(":userId", userId) &&
            s->setUInt64(":dsId", dsId) &&
            s->setText(":fileId", fileId) &&
            s->setText(":sellerPool", json.c_str()) &&
            s->setText(
               ":microPaymentCost", fp["microPaymentCost"]->getString()) &&
            s->setText(":budget", fp["budget"]->getString()) &&
            s->execute();

         if(rval)
         {
            uint64_t rows = 0;
            s->getRowsChanged(rows);
            if(rows == 0)
            {
               ExceptionRef e = new Exception(
                  "DownloadState or file ID does not exist.",
                  PURCHASEDB_EXCEPTION_NOT_FOUND);
               Exception::set(e);
               rval = false;
            }
         }
      }

      if(conn == NULL)
      {
         // close connection
         c->close();
      }
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Could not update DownloadState seller pool.",
         PURCHASEDB_EXCEPTION);
      e->getDetails()["downloadStateId"] = dsId;
      BM_ID_SET(e->getDetails()["userId"], userId);
      BM_ID_SET(e->getDetails()["fileId"], BM_FILE_ID(sp["fileInfo"]["id"]));
      Exception::push(e);
   }

   return rval;
}

bool PurchaseDatabase::populateSellerPools(DownloadState& ds, Connection* conn)
{
   bool rval = false;

   UserId userId = BM_USER_ID(ds["userId"]);
   DownloadStateId dsId = ds["id"]->getUInt64();

   // get database connection
   Connection* c = (conn == NULL ? getConnection(userId) : conn);
   if(c != NULL)
   {
      // prepare statement
      Statement* s = c->prepare(
         "SELECT file_id,seller_pool,micro_payment_cost,budget "
         "FROM seller_pools "
         "WHERE download_state_id=:dsId AND user_id=:userId");
      if(s != NULL)
      {
         // set parameters and execute statement
         rval =
            s->setUInt64(":dsId", dsId) &&
            s->setUInt64(":userId", userId) &&
            s->execute();

         if(rval)
         {
            string json;
            string fileId;
            string mpCost;
            string budget;
            JsonReader reader;
            Row* row;
            while(rval && (row = s->fetch()) != NULL)
            {
               row->getText("file_id", fileId);
               row->getText("seller_pool", json);
               row->getText("micro_payment_cost", mpCost);
               row->getText("budget", budget);

               // convert from json
               ByteArrayInputStream bais(json.c_str(), json.length());
               SellerPool sp;
               reader.start(sp);
               if((rval = reader.read(&bais) && reader.finish()))
               {
                  FileProgress& fp = ds["progress"][fileId.c_str()];
                  fp["sellerPool"] = sp;
                  fp["fileInfo"] = sp["fileInfo"];
                  fp["bytesDownloaded"] = 0;
                  fp["sellerData"]->setType(Map);
                  fp["sellers"]->setType(Map);
                  fp["unassigned"]->setType(Array);
                  fp["assigned"]->setType(Map);
                  fp["downloaded"]->setType(Map);
                  fp["microPaymentCost"] = mpCost.c_str();
                  fp["budget"] = budget.c_str();
               }
               else
               {
                  // bad database data, reset statement
                  s->reset();
               }
            }
         }
      }

      if(conn == NULL)
      {
         // close connection
         c->close();
      }
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Could not populate DownloadState seller pools.",
         PURCHASEDB_EXCEPTION ".Exception");
      e->getDetails()["downloadStateId"] = dsId;
      BM_ID_SET(e->getDetails()["userId"], userId);
      Exception::push(e);
   }

   return rval;
}

bool PurchaseDatabase::updateDownloadStateFlags(
   DownloadState& ds, Connection* conn)
{
   bool rval = false;

   UserId userId = BM_USER_ID(ds["userId"]);
   DownloadStateId dsId = ds["id"]->getUInt64();

   // get database connection
   Connection* c = (conn == NULL ? getConnection(userId) : conn);
   if(c != NULL)
   {
      // prepare statement
      Statement* s = c->prepare(
         "UPDATE download_states SET "
         "total_min_price=:totalMinPrice,"
         "total_med_price=:totalMedPrice,"
         "total_max_price=:totalMaxPrice,"
         "total_piece_count=:totalPieceCount,"
         "total_micro_payment_cost=:totalMicroPaymentCost,"
         "remaining_pieces=:remainingPieces,"
         "initialized=:initialized,"
         "license_acquired=:licenseAcquired,"
         "download_started=:downloadStarted,"
         "download_paused=:downloadPaused,"
         "license_purchased=:licensePurchased,"
         "data_purchased=:dataPurchased,"
         "files_assembled=:filesAssembled,"
         "start_date=:startDate "
         "WHERE user_id=:userId AND download_state_id=:dsId");
      if(s != NULL)
      {
         // set parameters and execute statement
         rval =
            s->setUInt64(":userId", userId) &&
            s->setUInt64(":dsId", dsId) &&
            s->setText(
               ":totalMinPrice",
               ds["totalMinPrice"]->getString()) &&
            s->setText(
               ":totalMedPrice",
               ds["totalMedPrice"]->getString()) &&
            s->setText(
               ":totalMaxPrice",
               ds["totalMaxPrice"]->getString()) &&
            s->setUInt32(
               ":totalPieceCount",
               ds["totalPieceCount"]->getUInt32()) &&
            s->setText(
               ":totalMicroPaymentCost",
               ds["totalMicroPaymentCost"]->getString()) &&
            s->setUInt32(
               ":remainingPieces",
               ds["remainingPieces"]->getUInt32()) &&
            s->setUInt32(
               ":initialized",
               ds["initialized"]->getBoolean() ? 1 : 0) &&
            s->setUInt32(
               ":licenseAcquired",
               ds["licenseAcquired"]->getBoolean() ? 1 : 0) &&
            s->setUInt32(
               ":downloadStarted",
               ds["downloadStarted"]->getBoolean() ? 1 : 0) &&
            s->setUInt32(
               ":downloadPaused",
               ds["downloadPaused"]->getBoolean() ? 1 : 0) &&
            s->setUInt32(
               ":licensePurchased",
               ds["licensePurchased"]->getBoolean() ? 1 : 0) &&
            s->setUInt32(
               ":dataPurchased",
               ds["dataPurchased"]->getBoolean() ? 1 : 0) &&
            s->setUInt32(
               ":filesAssembled",
               ds["filesAssembled"]->getBoolean() ? 1 : 0) &&
            s->setText(
               ":startDate",
               ds["startDate"]->getString()) &&
            s->execute();

         if(rval)
         {
            uint64_t rows = 0;
            s->getRowsChanged(rows);
            if(rows == 0)
            {
               ExceptionRef e = new Exception(
                  "DownloadState does not exist.",
                  PURCHASEDB_EXCEPTION_NOT_FOUND);
               Exception::set(e);
               rval = false;
            }
         }
      }

      if(conn == NULL)
      {
         // close connection
         c->close();
      }
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Could not update DownloadState flags.",
         PURCHASEDB_EXCEPTION ".Exception");
      e->getDetails()["downloadStateId"] = dsId;
      BM_ID_SET(e->getDetails()["userId"], userId);
      Exception::push(e);
   }

   return rval;
}

bool PurchaseDatabase::insertContract(DownloadState& ds, Connection* conn)
{
   bool rval = false;

   UserId userId = BM_USER_ID(ds["userId"]);
   DownloadStateId dsId = ds["id"]->getUInt64();

   // get database connection
   Connection* c = (conn == NULL ? getConnection(userId) : conn);
   if(c != NULL)
   {
      // prepare statement
      Statement* s = c->prepare(
         "INSERT INTO contracts (download_state_id,user_id,media_id,contract) "
         "VALUES (:dsId,:userId,:mediaId,:contract) ");
      if(s != NULL)
      {
         string json = JsonWriter::writeToString(ds["contract"], true);

         // set parameters and execute statement
         rval =
            s->setUInt64(":userId", userId) &&
            s->setUInt64(":dsId", dsId) &&
            s->setUInt64(
               ":mediaId", BM_MEDIA_ID(ds["contract"]["media"]["id"])) &&
            s->setText(":contract", json.c_str()) &&
            s->execute();
      }

      if(conn == NULL)
      {
         // close connection
         c->close();
      }
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Could not insert DownloadState contract.",
         PURCHASEDB_EXCEPTION ".Exception");
      e->getDetails()["downloadStateId"] = dsId;
      BM_ID_SET(e->getDetails()["userId"], userId);
      Exception::push(e);
   }

   return rval;
}

bool PurchaseDatabase::updateContract(DownloadState& ds, Connection* conn)
{
   bool rval = false;

   UserId userId = BM_USER_ID(ds["userId"]);
   DownloadStateId dsId = ds["id"]->getUInt64();

   // get database connection
   Connection* c = (conn == NULL ? getConnection(userId) : conn);
   if(c != NULL)
   {
      // prepare statement
      Statement* s = c->prepare(
         "UPDATE contracts SET contract=:contract "
         "WHERE user_id=:userId AND download_state_id=:dsId");
      if(s != NULL)
      {
         string json = JsonWriter::writeToString(ds["contract"], true);

         // set parameters and execute statement
         rval =
            s->setUInt64(":userId", userId) &&
            s->setUInt64(":dsId", dsId) &&
            s->setText(":contract", json.c_str()) &&
            s->execute();

         if(rval)
         {
            uint64_t rows = 0;
            s->getRowsChanged(rows);
            if(rows == 0)
            {
               ExceptionRef e = new Exception(
                  "DownloadState does not exist.",
                  PURCHASEDB_EXCEPTION_NOT_FOUND);
               Exception::set(e);
               rval = false;
            }
         }
      }

      if(conn == NULL)
      {
         // close connection
         c->close();
      }
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Could not update DownloadState contract.",
         PURCHASEDB_EXCEPTION ".Exception");
      e->getDetails()["downloadStateId"] = dsId;
      BM_ID_SET(e->getDetails()["userId"], userId);
      Exception::push(e);
   }

   return rval;
}

bool PurchaseDatabase::populateContract(DownloadState& ds, Connection* conn)
{
   bool rval = false;

   UserId userId = BM_USER_ID(ds["userId"]);
   DownloadStateId dsId = ds["id"]->getUInt64();

   // get database connection
   Connection* c = (conn == NULL ? getConnection(userId) : conn);
   if(c != NULL)
   {
      // prepare statement
      Statement* s = c->prepare(
         "SELECT contract FROM contracts "
         "WHERE download_state_id=:dsId AND user_id=:userId LIMIT 1");
      if(s != NULL)
      {
         // set parameters and execute statement
         rval =
            s->setUInt64(":dsId", dsId) &&
            s->setUInt64(":userId", userId) &&
            s->execute();
         if(rval)
         {
            Row* row;
            while(rval && (row = s->fetch()) != NULL)
            {
               string json;
               row->getText("contract", json);

               // convert from json
               JsonReader reader;
               ByteArrayInputStream bais(json.c_str(), json.length());
               Contract contract;
               reader.start(contract);
               if((rval = reader.read(&bais) && reader.finish()))
               {
                  ds["contract"] = contract;
               }
               else
               {
                  // bad database data, reset statement
                  s->reset();
               }
            }
         }
      }

      if(conn == NULL)
      {
         // close connection
         c->close();
      }
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Could not populate DownloadState contract.",
         PURCHASEDB_EXCEPTION);
      e->getDetails()["downloadStateId"] = dsId;
      BM_ID_SET(e->getDetails()["userId"], userId);
      Exception::push(e);
   }

   return rval;
}

bool PurchaseDatabase::updateFileProgress(
   DownloadState& ds, DynamicObject& entries, Connection* conn)
{
   bool rval = false;

   UserId userId = BM_USER_ID(ds["userId"]);
   DownloadStateId dsId = ds["id"]->getUInt64();

   // get database connection
   Connection* c = (conn == NULL ? getConnection(userId) : conn);
   if(c != NULL)
   {
      // begin db transaction for replacing file piece entries
      rval = c->begin();

      // prepare statement for deleting old file piece entries
      if(rval)
      {
         Statement* s = c->prepare(
            "DELETE FROM file_pieces WHERE "
            "download_state_id=:dsId AND "
            "user_id=:userId AND "
            "file_id=:fileId AND "
            "piece_index=:index AND "
            "section_hash=:csHash AND "
            "status<>:paid");
         rval = (s != NULL);

         // set parameters and execute each file piece update
         DynamicObjectIterator ei = entries.getIterator();
         while(rval && ei->hasNext())
         {
            DynamicObject& entry = ei->next();
            FileId fileId = entry["fileId"]->getString();
            uint32_t index = entry["piece"]["index"]->getUInt32();
            const char* hash = entry["csHash"]->getString();

            // set parameters, execute statement, reset statement
            rval =
               s->setUInt64(":dsId", dsId) &&
               s->setUInt64(":userId", userId) &&
               s->setText(":fileId", fileId) &&
               s->setUInt32(":index", index) &&
               s->setText(":csHash", hash) &&
               s->setText(":paid", "paid") &&
               s->execute() &&
               s->reset();
         }
      }

      // prepare statement for inserting file piece entries
      if(rval)
      {
         Statement* s = c->prepare(
            "INSERT INTO file_pieces "
            "(download_state_id,user_id,file_id,piece_index,"
            "valid,section_hash,status,file_piece) "
            "VALUES "
            "(:dsId,:userId,:fileId,:index,:valid,:csHash,"
            ":status,:filePiece)");
         rval = (s != NULL);

         // set parameters and execute each file piece update
         DynamicObjectIterator ei = entries.getIterator();
         while(rval && ei->hasNext())
         {
            DynamicObject& entry = ei->next();
            FileId fileId = entry["fileId"]->getString();
            const char* hash = entry["csHash"]->getString();
            FilePiece& fp = entry["piece"];
            string json = JsonWriter::writeToString(fp, true);
            uint32_t index = fp["index"]->getUInt32();
            const char* status = entry["status"]->getString();
            uint32_t valid = (strcmp(status, "unassigned") == 0) ? 0 : 1;

            // set parameters, execute statement, reset statement
            rval =
               s->setUInt64(":dsId", dsId) &&
               s->setUInt64(":userId", userId) &&
               s->setText(":fileId", fileId) &&
               s->setUInt32(":index", index) &&
               s->setUInt32(":valid", valid) &&
               s->setText(":csHash", hash) &&
               s->setText(":status", status) &&
               s->setText(":filePiece", json.c_str()) &&
               s->execute();
         }
      }

      // commit db transaction
      rval = rval ? c->commit() : c->rollback() && false;

      if(conn == NULL)
      {
         // close connection
         c->close();
      }
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Could not update DownloadState file progress.",
         PURCHASEDB_EXCEPTION);
      e->getDetails()["downloadStateId"] = dsId;
      BM_ID_SET(e->getDetails()["userId"], userId);
      Exception::push(e);
   }

   return rval;
}

bool PurchaseDatabase::populateFileProgress(DownloadState& ds, Connection* conn)
{
   bool rval = false;

   UserId userId = BM_USER_ID(ds["userId"]);
   DownloadStateId dsId = ds["id"]->getUInt64();

   // clear existing file progress
   ds["progress"]->setType(Map);
   ds["progress"]->clear();

   // get database connection
   Connection* c = (conn == NULL ? getConnection(userId) : conn);
   if((rval = (c != NULL)))
   {
      // populate download state's seller pools
      rval = populateSellerPools(ds, c);
   }

   if(rval)
   {
      // keep track of found pieces
      DynamicObject pieces;
      pieces->setType(Map);

      // prepare statement to load file pieces
      Statement* s = c->prepare(
         "SELECT * FROM file_pieces "
         "WHERE download_state_id=:dsId AND user_id=:userId");
      if((rval = (s != NULL)))
      {
         // set parameters and execute statement
         rval =
            s->setUInt64(":dsId", dsId) &&
            s->setUInt64(":userId", userId) &&
            s->execute();
      }

      // handle result set
      if(rval)
      {
         string csHash;
         string fileId;
         string status;
         uint32_t index;
         uint32_t valid;
         JsonReader reader;
         Row* row;
         while(rval && (row = s->fetch()) != NULL)
         {
            // get file piece json
            string str;
            row->getText("file_piece", str);

            // convert from json
            ByteArrayInputStream bais(str.c_str(), str.length());
            FilePiece piece;
            reader.start(piece);
            if((rval = reader.read(&bais) && reader.finish()))
            {
               // get file piece meta-data
               row->getText("file_id", fileId);
               row->getText("section_hash", csHash);
               row->getUInt32("piece_index", index);
               row->getText("status", status);
               row->getUInt32("valid", valid);

               // get appropriate file progress and initialize fields
               FileProgress& fp = ds["progress"][fileId.c_str()];
               fp["budget"]->setType(String);
               fp["sellerData"]->setType(Map);
               fp["sellers"]->setType(Map);
               fp["unassigned"]->setType(Array);
               fp["assigned"]->setType(Map);
               fp["downloaded"]->setType(Map);

               // update found piece map
               pieces[fileId.c_str()][index] = piece;

               if(strcmp(status.c_str(), "paid") == 0)
               {
                  fp["fileInfo"]["pieces"][index] = piece;
               }
               else if(strcmp(status.c_str(), "assigned") == 0)
               {
                  fp["assigned"][csHash.c_str()]->append(piece);
               }
               else if(strcmp(status.c_str(), "downloaded") == 0)
               {
                  fp["downloaded"][csHash.c_str()]->append(piece);
                  uint64_t tmp = fp["bytesDownloaded"]->getUInt64();
                  fp["bytesDownloaded"] = tmp + piece["size"]->getUInt32();
               }
               else if(strcmp(status.c_str(), "unassigned") == 0)
               {
                  // remove index member so that the piece will be updated
                  // as an unassigned piece, but consider the piece "found"
                  // so that its path information can be used to delete
                  // any temporary files
                  piece->removeMember("index");
               }
               else
               {
                  ExceptionRef e = new Exception(
                     "FilePiece status is invalid.",
                     PURCHASEDB_EXCEPTION ".InvalidStatus");
                  e->getDetails()["fileId"] = fileId.c_str();
                  e->getDetails()["index"] = index;
                  e->getDetails()["status"] = status.c_str();
                  Exception::set(e);
                  rval = false;
               }
            }

            if(!rval)
            {
               // bad database data, reset statement
               s->reset();
            }
         }
      }

      if(rval)
      {
         // append unassigned pieces to file progresses
         FileProgressIterator fpi = ds["progress"].getIterator();
         while(fpi->hasNext())
         {
            FileProgress& progress = fpi->next();
            SellerPool& sp = progress["sellerPool"];
            FileId fileId = progress["fileInfo"]["id"]->getString();
            DynamicObject& pieceArray = pieces[fileId];
            pieceArray->setType(Array);

            // ensure piece array covers all pieces
            int32_t pieceCount = sp["pieceCount"]->getInt32();
            if(pieceArray->length() < pieceCount)
            {
               // will fill piece array up to pieceCount size
               pieceArray[pieceCount - 1];
            }

            FilePieceIterator pi = pieceArray.getIterator();
            while(pi->hasNext())
            {
               FilePiece& fp = pi->next();

               // update any unassigned pieces
               if(!fp->hasMember("index"))
               {
                  // add unassigned piece
                  FilePiece& newPiece = progress["unassigned"]->append();
                  if(pi->hasNext())
                  {
                     // not last piece: all pieces same size
                     newPiece["size"] = sp["pieceSize"]->getUInt32();
                  }
                  else
                  {
                     // last piece: calculate remaining size (use actual
                     // content size for file, not arbitrary disk size)
                     uint64_t fileSize =
                        progress["fileInfo"]["contentSize"]->getUInt64();
                     newPiece["size"] = fileSize % sp["pieceSize"]->getUInt32();
                  }
                  newPiece["index"] = pi->getIndex();
                  BM_ID_SET(newPiece["bfpId"], BM_BFP_ID(sp["bfpId"]));

                  // persist old path so that, if necessary, temporary files
                  // that were created can be deleted
                  if(fp->hasMember("path"))
                  {
                     newPiece["path"] = fp["path"]->getString();
                  }
               }
            }
         }
      }
   }

   if(c != NULL && conn == NULL)
   {
      // close connection
      c->close();
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Could not populate DownloadState file progress.",
         PURCHASEDB_EXCEPTION ".Exception");
      e->getDetails()["downloadStateId"] = dsId;
      BM_ID_SET(e->getDetails()["userId"], userId);
      Exception::push(e);
   }

   return rval;
}

bool PurchaseDatabase::populateFilePaths(
   DownloadState& ds, Connection* conn)
{
   bool rval = false;

   UserId userId = BM_USER_ID(ds["userId"]);
   DownloadStateId dsId = ds["id"]->getUInt64();

   // get database connection
   Connection* c = (conn == NULL ? getConnection(userId) : conn);
   if(c != NULL)
   {
      // prepare statement to load file paths
      Statement* s = c->prepare(
         "SELECT * FROM assembled_files "
         "WHERE download_state_id=:dsId AND user_id=:userId");
      if((rval = (s != NULL)))
      {
         // set parameters and execute statement
         rval =
            s->setUInt64(":dsId", dsId) &&
            s->setUInt64(":userId", userId) &&
            s->execute();
      }

      // handle result set
      if(rval)
      {
         string fileId;
         string path;
         Row* row;
         while(rval && (row = s->fetch()) != NULL)
         {
            // get row data
            row->getText("file_id", fileId);
            row->getText("path", path);

            DynamicObject& progress = ds["progress"];
            // Check to make sure file progress exists.
            // Early calls to this method occur before it is populated.
            if(progress->hasMember(fileId.c_str()))
            {
               // set path and dir on file progress
               FileProgress& fp = progress[fileId.c_str()];
               fp["path"] = path.c_str();
               fp["directory"] = File::dirname(path.c_str()).c_str();
            }
            /*
            FIXME:
            else
            {
               ExceptionRef e = new Exception(
                  "FilePiece status is missing.",
                  PURCHASEDB_EXCEPTION ".InvalidStatus");
               e->getDetails()["fileId"] = fileId.c_str();
               Exception::set(e);
               rval = false;
            }
            */
         }
      }
   }

   if(c != NULL && conn == NULL)
   {
      // close connection
      c->close();
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Could not populate DownloadState file paths.",
         PURCHASEDB_EXCEPTION ".Exception");
      e->getDetails()["downloadStateId"] = dsId;
      BM_ID_SET(e->getDetails()["userId"], userId);
      Exception::push(e);
   }

   return rval;
}

bool PurchaseDatabase::insertSellerData(
   DownloadState& ds, FileId fileId, SellerData& sd, Connection* conn)
{
   bool rval = false;

   UserId userId = BM_USER_ID(ds["userId"]);
   DownloadStateId dsId = ds["id"]->getUInt64();

   // get database connection
   Connection* c = (conn == NULL ? getConnection(userId) : conn);
   if(c != NULL)
   {
      // prepare statement
      Statement* s = c->prepare(
         "INSERT INTO seller_data "
         "(download_state_id,user_id,file_id,price,section) "
         "VALUES (:dsId,:userId,:fileId,:price,:section) ");

      if(s != NULL)
      {
         string json = JsonWriter::writeToString(sd["section"], true);

         // set parameters and execute statement
         rval =
            s->setUInt64(":userId", userId) &&
            s->setUInt64(":dsId", dsId) &&
            s->setText(":fileId", fileId) &&
            s->setText(":price", sd["price"]->getString()) &&
            s->setText(":section", json.c_str()) &&
            s->execute();
      }

      if(conn == NULL)
      {
         // close connection
         c->close();
      }
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Could not insert DownloadState seller data.",
         PURCHASEDB_EXCEPTION ".Exception");
      e->getDetails()["downloadStateId"] = dsId;
      BM_ID_SET(e->getDetails()["userId"], userId);
      Exception::push(e);
   }

   return rval;
}

bool PurchaseDatabase::populateSellerData(DownloadState& ds, Connection* conn)
{
   bool rval = false;

   UserId userId = BM_USER_ID(ds["userId"]);
   DownloadStateId dsId = ds["id"]->getUInt64();

   // get database connection
   Connection* c = (conn == NULL ? getConnection(userId) : conn);
   if(c != NULL)
   {
      // prepare statement
      Statement* s = c->prepare(
         "SELECT file_id,price,section FROM seller_data "
         "WHERE download_state_id=:dsId AND user_id=:userId");
      if(s != NULL)
      {
         // set parameters and execute statement
         rval =
            s->setUInt64(":dsId", dsId) &&
            s->setUInt64(":userId", userId) &&
            s->execute();

         if(rval)
         {
            string fileId;
            string str;
            JsonReader reader;
            Row* row;
            while(rval && (row = s->fetch()) != NULL)
            {
               row->getText("file_id", fileId);
               row->getText("section", str);

               // convert from json
               ByteArrayInputStream bais(str.c_str(), str.length());
               ContractSection cs;
               reader.start(cs);
               if((rval = reader.read(&bais) && reader.finish()))
               {
                  // get price
                  row->getText("price", str);

                  // find associated file progress
                  FileProgress& fp = ds["progress"][fileId.c_str()];

                  // update seller data's section
                  const char* csHash = cs["hash"]->getString();
                  SellerData& sd = fp["sellerData"][csHash];
                  sd["section"] = cs;
                  sd["price"] = str.c_str();
                  sd["seller"] = cs["seller"].clone();

                  // update sellers map in file progress
                  string key = Tools::createSellerServerKey(cs["seller"]);
                  fp["sellers"][key.c_str()] = sd["seller"];
               }
               else
               {
                  // bad database data, reset statement
                  s->reset();
               }
            }
         }
      }

      if(conn == NULL)
      {
         // close connection
         c->close();
      }
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Could not populate DownloadState seller data.",
         PURCHASEDB_EXCEPTION);
      e->getDetails()["downloadStateId"] = dsId;
      BM_ID_SET(e->getDetails()["userId"], userId);
      Exception::push(e);
   }

   return rval;
}

bool PurchaseDatabase::populateDownloadState(
   DownloadState& ds, Connection* conn)
{
   bool rval = false;

   UserId userId = BM_USER_ID(ds["userId"]);
   DownloadStateId dsId = ds["id"]->getUInt64();

   // get database connection
   Connection* c = (conn == NULL ? getConnection(userId) : conn);
   if(c != NULL)
   {
      // prepare statement
      Statement* s = c->prepare(
         "SELECT * FROM download_states "
         "WHERE user_id=:userId AND download_state_id=:dsId LIMIT 1");
      if((rval = (s != NULL)))
      {
         // set parameters and execute
         rval =
            s->setUInt64(":userId", userId) &&
            s->setUInt64(":dsId", dsId) &&
            s->execute();
         if(rval)
         {
            bool found = false;
            Row* row;
            while((row = s->fetch()) != NULL)
            {
               found = true;

               // convert ware from json
               JsonReader reader;
               Ware ware;
               {
                  string json;
                  row->getText("ware", json);
                  ByteArrayInputStream bais(json.c_str(), json.length());
                  reader.start(ware);
                  rval = reader.read(&bais) && reader.finish();
                  if(!rval)
                  {
                     ExceptionRef e = new Exception(
                        "Could not populate DownloadState ware.",
                        PURCHASEDB_EXCEPTION);
                     e->getDetails()["downloadStateId"] = dsId;
                     BM_ID_SET(e->getDetails()["userId"], userId);
                     Exception::push(e);
                  }
               }

               // convert preferences from json
               PurchasePreferences prefs;
               if(rval)
               {
                  string json;
                  row->getText("preferences", json);
                  ByteArrayInputStream bais(json.c_str(), json.length());
                  reader.start(prefs);
                  rval = reader.read(&bais) && reader.finish();
                  if(!rval)
                  {
                     ExceptionRef e = new Exception(
                        "Could not populate DownloadState preferences.",
                        PURCHASEDB_EXCEPTION);
                     e->getDetails()["downloadStateId"] = dsId;
                     BM_ID_SET(e->getDetails()["userId"], userId);
                     Exception::push(e);
                  }
               }

               if(rval)
               {
                  string str;
                  uint32_t num;

                  // set ware and purchase preferences
                  ds["ware"] = ware;
                  ds["preferences"] = prefs;

                  // set total min price
                  row->getText("total_min_price", str);
                  ds["totalMinPrice"] = str.c_str();

                  // set total median price
                  row->getText("total_med_price", str);
                  ds["totalMedPrice"] = str.c_str();

                  // set total max price
                  row->getText("total_max_price", str);
                  ds["totalMaxPrice"] = str.c_str();

                  // set total piece count
                  row->getUInt32("total_piece_count", num);
                  ds["totalPieceCount"] = num;

                  // set total micro payment cost
                  row->getText("total_micro_payment_cost", str);
                  ds["totalMicroPaymentCost"] = str.c_str();

                  // set version
                  row->getText("version", str);
                  ds["version"] = str.c_str();

                  // set start date
                  row->getText("start_date", str);
                  ds["startDate"] = str.c_str();

                  // set flags
                  row->getUInt32("remaining_pieces", num);
                  ds["remainingPieces"] = num;

                  row->getUInt32("initialized", num);
                  ds["initialized"] = (num == 1);

                  row->getUInt32("license_acquired", num);
                  ds["licenseAcquired"] = (num == 1);

                  row->getUInt32("download_started", num);
                  ds["downloadStarted"] = (num == 1);

                  row->getUInt32("download_paused", num);
                  ds["downloadPaused"] = (num == 1);

                  row->getUInt32("license_purchased", num);
                  ds["licensePurchased"] = (num == 1);

                  row->getUInt32("data_purchased", num);
                  ds["dataPurchased"] = (num == 1);

                  row->getUInt32("files_assembled", num);
                  ds["filesAssembled"] = (num == 1);

                  // initialize blacklist & active sellers
                  ds["blacklist"]->setType(Map);
                  ds["activeSellers"]->setType(Map);

                  row->getUInt32("processor_id", num);
                  ds["processorId"] = num;

                  row->getUInt32("processing", num);
                  ds["processing"] = (num == 1);
               }
            }

            if(!found)
            {
               ExceptionRef e = new Exception(
                  "DownloadState does not exist.",
                  PURCHASEDB_EXCEPTION_NOT_FOUND);
               Exception::set(e);
               rval = false;
            }
         }
      }

      // populate contracts, file progress, and seller data
      rval = rval &&
         populateContract(ds, c) &&
         populateFileProgress(ds, c) &&
         populateFilePaths(ds, c) &&
         populateSellerData(ds, c);

      if(conn == NULL)
      {
         // close connection
         c->close();
      }
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Could not populate DownloadState.",
         PURCHASEDB_EXCEPTION);
      e->getDetails()["downloadStateId"] = dsId;
      BM_ID_SET(e->getDetails()["userId"], userId);
      Exception::push(e);
   }

   return rval;
}

bool PurchaseDatabase::populateDownloadStates(
   UserId userId, DownloadStateList& dsList, Connection* conn)
{
   bool rval = false;

   // get database connection
   Connection* c = (conn == NULL ? getConnection(userId) : conn);
   if(c != NULL && dsList->length() > 0)
   {
      // populate download states, contracts, file progress, and seller data
      rval = true;
      DownloadStateIterator dsi = dsList.getIterator();
      while(rval && dsi->hasNext())
      {
         DownloadState& ds = dsi->next();
         rval = populateDownloadState(ds, c);
      }

      if(conn == NULL)
      {
         // close connection
         c->close();
      }
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Could not populate DownloadStates.",
         PURCHASEDB_EXCEPTION);
      BM_ID_SET(e->getDetails()["userId"], userId);
      Exception::push(e);
   }

   return rval;
}

bool PurchaseDatabase::getIncompleteDownloadStates(
   UserId userId, DownloadStateList& dsList, Connection* conn)
{
   bool rval = false;

   // ensure dsList is an array
   dsList->setType(Array);

   // get database connection
   Connection* c = (conn == NULL ? getConnection(userId) : conn);
   if(c != NULL)
   {
      // prepare statement to get download state IDs
      Statement* s = c->prepare(
         "SELECT download_state_id FROM download_states WHERE "
         "user_id=:userId AND files_assembled=0");
      if(s != NULL)
      {
         // set parameters and execute statement
         rval =
            s->setUInt64(":userId", userId) &&
            s->execute();

         if(rval)
         {
            // obtain download state IDs
            Row* row;
            while((row = s->fetch()) != NULL)
            {
               uint64_t dsId;
               row->getUInt64("download_state_id", dsId);
               DownloadState& ds = dsList->append();
               ds["id"] = dsId;
               BM_ID_SET(ds["userId"], userId);
            }

            // populate all download states in list
            if(dsList->length() > 0)
            {
               rval = populateDownloadStates(userId, dsList, c);
            }
         }
      }

      if(conn == NULL)
      {
         // close connection
         c->close();
      }
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Could not populate incomplete DownloadStates.",
         PURCHASEDB_EXCEPTION ".Exception");
      BM_ID_SET(e->getDetails()["userId"], userId);
      Exception::push(e);
   }

   return rval;
}

bool PurchaseDatabase::getUnpurchasedDownloadStates(
   UserId userId, DownloadStateList& dsList, Connection* conn)
{
   bool rval = false;

   // ensure dsList is an array
   dsList->setType(Array);

   // get database connection
   Connection* c = (conn == NULL ? getConnection(userId) : conn);
   if(c != NULL)
   {
      // prepare statement
      Statement* s = c->prepare(
         "SELECT download_state_id FROM download_states WHERE "
         "user_id=:userId AND data_purchased=0");
      if(s != NULL)
      {
         // set parameters and execute statement
         rval =
            s->setUInt64(":userId", userId) &&
            s->execute();

         if(rval)
         {
            // obtain download state IDs
            Row* row;
            while((row = s->fetch()) != NULL)
            {
               uint64_t dsId;
               row->getUInt64("download_state_id", dsId);
               DownloadState& ds = dsList->append();
               ds["id"] = dsId;
               BM_ID_SET(ds["userId"], userId);
            }

            // populate all download states in list
            if(dsList->length() > 0)
            {
               rval = populateDownloadStates(userId, dsList, c);
            }
         }
      }

      if(conn == NULL)
      {
         // close connection
         c->close();
      }
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Could not populate unpurchased DownloadStates.",
         PURCHASEDB_EXCEPTION ".Exception");
      BM_ID_SET(e->getDetails()["userId"], userId);
      Exception::push(e);
   }

   return rval;
}

bool PurchaseDatabase::insertAssembledFile(
   DownloadState& ds, bitmunk::common::FileId fileId,
   const char* path, monarch::sql::Connection* conn)
{
   bool rval = false;

   UserId userId = BM_USER_ID(ds["userId"]);
   DownloadStateId dsId = ds["id"]->getUInt64();

   // get database connection
   Connection* c = (conn == NULL ? getConnection(userId) : conn);
   if(c != NULL)
   {
      // prepare statement
      Statement* s = c->prepare(
         "INSERT INTO assembled_files "
         "(download_state_id,user_id,file_id,path) "
         "VALUES "
         "(:dsId,:userId,:fileId,:path)");
      if((rval = (s != NULL)))
      {
         // set parameters, execute statement, reset for next execution
         rval =
            s->setUInt64(":dsId", dsId) &&
            s->setUInt64(":userId", userId) &&
            s->setText(":fileId", fileId) &&
            s->setText(":path", path) &&
            s->execute() &&
            s->reset();
      }

      if(conn == NULL)
      {
         // close connection
         c->close();
      }
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Could not insert assembled file into database.",
         PURCHASEDB_EXCEPTION);
      e->getDetails()["downloadStateId"] = dsId;
      BM_ID_SET(e->getDetails()["userId"], userId);
      e->getDetails()["fileId"] = fileId;
      e->getDetails()["path"] = path;
      Exception::push(e);
   }

   return rval;
}

Connection* PurchaseDatabase::getConnection(UserId userId)
{
   return mPerUserDB->getConnection(mConnectionGroupId, userId);
}

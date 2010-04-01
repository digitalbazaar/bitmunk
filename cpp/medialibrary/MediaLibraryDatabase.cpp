/*
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/medialibrary/MediaLibraryDatabase.h"

#include "bitmunk/medialibrary/IMediaLibrary.h"
#include "bitmunk/medialibrary/MediaLibraryModule.h"
#include "bitmunk/peruserdb/PerUserDatabase.h"
#include "monarch/config/ConfigManager.h"
#include "monarch/data/json/JsonReader.h"
#include "monarch/data/json/JsonWriter.h"
#include "monarch/sql/Row.h"
#include "monarch/sql/Statement.h"

#include <algorithm>

using namespace std;
using namespace bitmunk::common;
using namespace bitmunk::medialibrary;
using namespace bitmunk::peruserdb;
using namespace bitmunk::node;
using namespace monarch::config;
using namespace monarch::data::json;
using namespace monarch::io;
using namespace monarch::rt;
using namespace monarch::sql;

#define MLDB_TABLE_FILES         "bitmunk_medialibrary_files"
#define MLDB_TABLE_MEDIA         "bitmunk_medialibrary_media"
#define MLDB_TABLE_CONTRIBUTORS  "bitmunk_medialibrary_contributors"
#define MLDB_EXCEPTION           "bitmunk.medialibrary.MediaLibraryDatabase"
#define MLDB_EXCEPTION_NOT_FOUND MLDB_EXCEPTION ".NotFound"

// trigger errors
#define TRIGGER_ABORT_ERROR \
   "on table \"" MLDB_TABLE_CONTRIBUTORS "\" violates foreign key constraint"
#define INSERT_TRIGGER_ABORT_ERROR "insert " TRIGGER_ABORT_ERROR
#define UPDATE_TRIGGER_ABORT_ERROR "update " TRIGGER_ABORT_ERROR

MediaLibraryDatabase::MediaLibraryDatabase() :
   mNode(NULL),
   mPerUserDB(NULL),
   mConnectionGroupId(0)
{
}

MediaLibraryDatabase::~MediaLibraryDatabase()
{
   if(mConnectionGroupId != 0)
   {
      // unregister connection group
      mPerUserDB->removeConnectionGroup(mConnectionGroupId);
   }

   // clear any existing media library extensions
   mExtensions.clear();
}

bool MediaLibraryDatabase::initialize(Node* node)
{
   bool rval = false;
   mNode = node;

   // get peruserdb module interface
   mPerUserDB = dynamic_cast<IPerUserDBModule*>(
      mNode->getModuleApi("bitmunk.peruserdb.PerUserDB"));
   if(mPerUserDB == NULL)
   {
      ExceptionRef e = new Exception(
         "Could not load interface for per-user database module dependency.",
         MLDB_EXCEPTION ".MissingDependency");
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
            "Could not register media library database.",
            MLDB_EXCEPTION ".RegisterError");
         Exception::push(e);
         rval = false;
      }
      else
      {
         MO_CAT_INFO(BM_MEDIALIBRARY_CAT,
            "Added media library database as "
            "connection group: %u", mConnectionGroupId);
      }
   }

   return true;
}

bool MediaLibraryDatabase::getConnectionParams(
   ConnectionGroupId id, UserId userId, string& url, uint32_t& maxCount)
{
   bool rval = false;

   // get media library database config
   Config config = mNode->getConfigManager()->getModuleUserConfig(
      "bitmunk.medialibrary.MediaLibrary", userId);
   if(!config.isNull())
   {
      // get the database url and maximum number of active connections
      url = config["database"]["url"]->getString();
      maxCount = config["database"]["connections"]->getUInt32();
      rval = true;
   }
   else
   {
      ExceptionRef e = new Exception(
         "Could not get media library database connection parameters.",
         MLDB_EXCEPTION ".ConfigError");
      Exception::push(e);
   }

   return rval;
}

bool MediaLibraryDatabase::initializePerUserDatabase(
   ConnectionGroupId id, UserId userId, Connection* conn,
   DatabaseClientRef& dbc)
{
   bool rval = false;

   // define table schemas:

   // base media library files table
   {
      SchemaObject schema;
      schema["table"] = MLDB_TABLE_FILES;
      schema["indices"]->append() = "UNIQUE(file_id)";

      DatabaseClient::addSchemaColumn(schema,
         "media_library_id", "INTEGER PRIMARY KEY", "mediaLibraryId", UInt64);
      DatabaseClient::addSchemaColumn(schema,
         "file_id", "TEXT", "id", String);
      DatabaseClient::addSchemaColumn(schema,
         "media_id", "BIGINT UNSIGNED", "mediaId", UInt64);
      DatabaseClient::addSchemaColumn(schema,
         "path", "TEXT", "path", String);
      DatabaseClient::addSchemaColumn(schema,
         "content_type", "TEXT", "contentType", String);
      DatabaseClient::addSchemaColumn(schema,
         "content_size", "BIGINT UNSIGNED", "contentSize", UInt64);
      DatabaseClient::addSchemaColumn(schema,
         "size", "BIGINT UNSIGNED", "size", UInt64);
      DatabaseClient::addSchemaColumn(schema,
         "format_details", "TEXT", "formatDetails", String);

      rval = dbc->define(schema);
   }

   // media meta-data table
   if(rval)
   {
      SchemaObject schema;
      schema["table"] = MLDB_TABLE_MEDIA;

      DatabaseClient::addSchemaColumn(schema,
         "media_id", "INTEGER PRIMARY KEY", "id", UInt64);
      DatabaseClient::addSchemaColumn(schema,
         "type", "TEXT", "type", String);
      DatabaseClient::addSchemaColumn(schema,
         "title", "TEXT", "title", String);

      rval = dbc->define(schema);
   }

   // contributors meta-data table
   if(rval)
   {
      SchemaObject schema;
      schema["table"] = MLDB_TABLE_CONTRIBUTORS;

      DatabaseClient::addSchemaColumn(schema,
         "media_id", "BIGINT UNSIGNED", "mediaId", UInt64);
      DatabaseClient::addSchemaColumn(schema,
         "type", "TEXT", "type", String);
      DatabaseClient::addSchemaColumn(schema,
         "name", "TEXT", "name", String);

      rval = dbc->define(schema);
   }

   // create tables if they do not exist
   if((rval = dbc->begin(conn)))
   {
      rval =
         dbc->create(MLDB_TABLE_FILES, true, conn) &&
         dbc->create(MLDB_TABLE_MEDIA, true, conn) &&
         dbc->create(MLDB_TABLE_CONTRIBUTORS, true, conn);

      // create contributors insert trigger
      if(rval)
      {
         Statement* s = conn->prepare(
            "CREATE TRIGGER IF NOT EXISTS fki_media_id_contributors "
            "BEFORE INSERT ON " MLDB_TABLE_CONTRIBUTORS " FOR EACH ROW "
            "BEGIN "
             "SELECT RAISE(ABORT, '" INSERT_TRIGGER_ABORT_ERROR "') "
              "WHERE "
               "(SELECT media_id FROM " MLDB_TABLE_MEDIA
               " WHERE media_id=NEW.media_id) IS NULL;"
            "END;");
         rval = (s != NULL) && s->execute();
      }

      // create contributors update trigger
      if(rval)
      {
         Statement* s = conn->prepare(
            "CREATE TRIGGER IF NOT EXISTS fku_media_id_contributors "
            "BEFORE UPDATE ON " MLDB_TABLE_CONTRIBUTORS " FOR EACH ROW "
            "BEGIN "
             "SELECT RAISE(ABORT, '" UPDATE_TRIGGER_ABORT_ERROR "') "
              "WHERE "
               "(SELECT media_id FROM " MLDB_TABLE_MEDIA
               " WHERE media_id=NEW.media_id) IS NULL;"
            "END;");
         rval = (s != NULL) && s->execute();
      }

      // create contributors delete trigger
      if(rval)
      {
         // this trigger will cascade media deletes to the contributors table
         Statement* s = conn->prepare(
            "CREATE TRIGGER IF NOT EXISTS fkd_media_id_contributors "
            "BEFORE DELETE ON " MLDB_TABLE_MEDIA " FOR EACH ROW "
            "BEGIN "
             "DELETE FROM " MLDB_TABLE_CONTRIBUTORS
             " WHERE media_id=OLD.media_id;"
            "END;");
         rval = (s != NULL) && s->execute();
      }

      // end transaction
      rval = dbc->end(conn, rval) && rval;
   }

   if(rval)
   {
      // call all media library extensions
      for(ExtensionList::iterator i = mExtensions.begin();
          rval && i != mExtensions.end(); i++)
      {
         rval = (*i)->mediaLibraryInitialized(userId, conn, dbc);
      }
   }

   // close connection
   conn->close();

   return rval;
}

void MediaLibraryDatabase::cleanupPerUserDatabase(
   ConnectionGroupId id, UserId userId)
{
   // call all media library extension clean up code
   for(ExtensionList::iterator i = mExtensions.begin();
       i != mExtensions.end(); i++)
   {
      (*i)->mediaLibraryCleaningUp(userId);
   }
}

void MediaLibraryDatabase::addMediaLibraryExtension(
   IMediaLibraryExtension* extension)
{
   mExtensions.push_back(extension);
}

void MediaLibraryDatabase::removeMediaLibraryExtension(
   IMediaLibraryExtension* extension)
{
   ExtensionList::iterator i = find(
      mExtensions.begin(), mExtensions.end(), extension);
   if(i != mExtensions.end())
   {
      mExtensions.erase(i);
   }
}

bool MediaLibraryDatabase::updateFile(
   UserId userId, FileInfo& fi, MediaLibraryId* mlId, bool* added,
   Connection* conn)
{
   bool rval = false;

   if(added != NULL)
   {
      *added = false;
   }

   // get database client
   DatabaseClientRef dbc = getDatabaseClient(userId);
   if(!dbc.isNull())
   {
      DynamicObject row = fi.clone();

      // get file info format details in JSON format
      row["formatDetails"] = JsonWriter::writeToString(
         fi["formatDetails"], true, false).c_str();
      if(row["formatDetails"]->length() > 0)
      {
         // try to insert first
         SqlExecutableRef se;
         bool inserted = false;
         {
            se = dbc->insertOrIgnore(MLDB_TABLE_FILES, row);
            if((rval = dbc->execute(se, conn)))
            {
               inserted = (se->rowsAffected == 1);
               if(inserted)
               {
                  if(mlId != NULL)
                  {
                     // get media library ID
                     *mlId = se->lastInsertRowId;
                  }

                  if(added != NULL)
                  {
                     *added = true;
                  }
               }
            }
         }

         // update if not inserted and no error
         if(rval && !inserted)
         {
            DynamicObject where;
            where["id"] = fi["id"];
            se = dbc->update(MLDB_TABLE_FILES, row, &where, 1);
            if((rval = dbc->execute(se, conn)))
            {
               if(mlId != NULL)
               {
                  // get media library ID
                  DynamicObject members;
                  members["mediaLibraryId"];
                  se = dbc->selectOne(MLDB_TABLE_FILES, &where, &members);
                  rval = dbc->execute(se, conn);
                  if(rval)
                  {
                     *mlId = se->result->getUInt64();
                  }
               }
            }
         }
      }
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Could not update file in media library database.",
         MLDB_EXCEPTION ".UpdateFileException");
      BM_ID_SET(e->getDetails()["fileId"], BM_FILE_ID(fi["id"]));
      BM_ID_SET(e->getDetails()["userId"], userId);
      e->getDetails()["path"] = fi["path"];
      Exception::push(e);
   }

   return rval;
}

bool MediaLibraryDatabase::removeFile(
   UserId userId, FileInfo& fi, Connection* conn)
{
   bool rval = false;

   // get database connection
   Connection* c = (conn == NULL ? getConnection(userId) : conn);
   if(c != NULL)
   {
      // we have to do a database transaction here to ensure the file path
      // matches the entry in the database ... so we can delete the file
      // after removing it from the media library, if we want to
      DatabaseClientRef dbc = getDatabaseClient(userId);
      if((rval = dbc->begin(c)))
      {
         // ensure path is correct in the file info
         DynamicObject where;
         where["id"] = BM_FILE_ID(fi["id"]);
         SqlExecutableRef se = dbc->selectOne(MLDB_TABLE_FILES, &where);
         if(!se.isNull())
         {
            // store result in file info
            se->result = fi;
            if(dbc->execute(se, c))
            {
               if(se->rowsRetrieved == 1)
               {
                  rval = true;
               }
               else
               {
                  ExceptionRef e = new Exception(
                     "The file does not exist in the database.",
                     MLDB_EXCEPTION ".FileNotFound");
                  BM_ID_SET(e->getDetails()["fileId"], BM_FILE_ID(fi["id"]));
                  Exception::set(e);
                  rval = false;
               }
            }
         }

         // delete file from database
         if(rval)
         {
            se = dbc->remove(MLDB_TABLE_FILES, &where);
            rval = dbc->execute(se, c);
         }

         // end transaction
         rval = dbc->end(c, rval) && rval;
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
         "There was a database error when attempting to remove the "
         "file from the media library.",
         MLDB_EXCEPTION ".DeleteFileException");
      BM_ID_SET(e->getDetails()["userId"], userId);
      BM_ID_SET(e->getDetails()["fileId"], fi["id"]);
      Exception::push(e);
   }

   return rval;
}

/**
 * Update a FileInfo once it has been populated from the database.
 *
 * @param fi the FileInfo to update.
 */
static bool _updatePopulatedFileInfo(FileInfo& fi)
{
   bool rval = false;

   // remove media library ID
   fi->removeMember("mediaLibraryId");

   // include extension
   File file(fi["path"]->getString());
   const char* ext = file->getExtension();
   fi["extension"] = ((ext != NULL) ? (ext + 1) : "");

   // convert format details from json string
   string json = fi["formatDetails"]->getString();
   rval = JsonReader::readFromString(
      fi["formatDetails"], json.c_str(), json.length());

   return rval;
}

bool MediaLibraryDatabase::populateFile(
   UserId userId, FileInfo& fi,
   MediaLibraryId mlId, Connection* conn)
{
   bool rval = false;

   // get database client
   DatabaseClientRef dbc = getDatabaseClient(userId);
   if(!dbc.isNull())
   {
      DynamicObject where;
      if(mlId == 0)
      {
         // get file info based on file ID
         where["id"] = fi["id"];
      }
      else
      {
         // get file info based on media library ID
         where["mediaLibraryId"] = mlId;
      }

      // select file info
      SqlExecutableRef se = dbc->selectOne(MLDB_TABLE_FILES, &where);
      if(!se.isNull())
      {
         // store result in file info
         se->result = fi;
         if(dbc->execute(se, conn))
         {
            if(se->rowsRetrieved == 1)
            {
               rval = true;
               _updatePopulatedFileInfo(fi);
            }
            else
            {
               ExceptionRef e = new Exception(
                  "File not found in media library.",
                  MLDB_EXCEPTION_NOT_FOUND);
               Exception::set(e);
               rval = false;
            }
         }
      }
   }

   // set an exception if there was a failure for some reason
   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Failed to populate the media library file.",
         MLDB_EXCEPTION ".FilePopulationFailure");
      BM_ID_SET(e->getDetails()["userId"], userId);
      if(mlId == 0)
      {
         BM_ID_SET(e->getDetails()["fileId"], fi["id"]->getString());
      }
      else
      {
         e->getDetails()["mediaLibraryId"] = mlId;
      }
      Exception::push(e);
   }

   return rval;
}

/**
 * Common function to populate media data. This function should be called
 * inside a transaction to ensure data consistency. The passed connection
 * should be open and will not be closed.
 */
static bool sPopulateMedia(
   UserId userId, Media& media, MediaLibraryId mlId, Connection* conn)
{
   bool rval = false;

   // prepare statement to get media info
   Statement* s;
   if(mlId == 0)
   {
      // search based on media ID
      s = conn->prepare(
         "SELECT * FROM " MLDB_TABLE_MEDIA
         " WHERE media_id=:mediaId LIMIT 1");
      rval =
         (s != NULL) &&
         s->setUInt64(":mediaId", BM_MEDIA_ID(media["id"])) &&
         s->execute();
   }
   else
   {
      // search based on media library ID
      s = conn->prepare(
         "SELECT m.*,f.media_id "
         "FROM " MLDB_TABLE_MEDIA " m JOIN " MLDB_TABLE_FILES " f "
         "ON m.media_id=f.media_id "
         " WHERE f.media_library_id=:mlId LIMIT 1");
      rval =
         (s != NULL) &&
         s->setUInt64(":mlId", mlId) &&
         s->execute();
   }

   if(rval)
   {
      // populate Media from row
      Row* row = s->fetch();
      if(row == NULL)
      {
         ExceptionRef e = new Exception(
            "Media not found in media library.",
            MLDB_EXCEPTION_NOT_FOUND);
         Exception::set(e);
         rval = false;
      }
      else
      {
         string str;
         MediaId mediaId;

         // extract the information from the database row and place it
         // into the media object
         if(rval && (rval = row->getUInt64("media_id", mediaId)))
         {
            BM_ID_SET(media["id"], mediaId);
         }

         if(rval && (rval = row->getText("type", str)))
         {
            media["type"] = str.c_str();
         }

         if(rval && (rval = row->getText("title", str)))
         {
            media["title"] = str.c_str();
         }

         // finish out the result set
         s->fetch();
      }
   }

   if(rval)
   {
      // populate media contributors
      Statement* s = conn->prepare(
         "SELECT * FROM " MLDB_TABLE_CONTRIBUTORS
         " WHERE media_id=:mediaId");
      rval =
         (s != NULL) &&
         s->setUInt64(":mediaId", BM_MEDIA_ID(media["id"])) &&
         s->execute();
      if(rval)
      {
         string str;
         Row* row;
         while(rval && (row = s->fetch()) != NULL)
         {
            if((rval = row->getText("type", str)))
            {
               if(!media["contributors"]->hasMember(str.c_str()))
               {
                  DynamicObject contributorArray;
                  contributorArray->setType(Array);
                  media["contributors"][str.c_str()] = contributorArray;
               }
               DynamicObject& contributor =
                  media["contributors"][str.c_str()]->append();
               if((rval = row->getText("name", str)))
               {
                  contributor["name"] = str.c_str();
               }
            }
         }
      }
   }

   // set an exception if there was a failure for some reason
   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Failed to populate the media library media.",
         MLDB_EXCEPTION ".MediaPopulationFailure");
      BM_ID_SET(e->getDetails()["userId"], userId);
      if(mlId == 0)
      {
         BM_ID_SET(e->getDetails()["mediaId"], BM_MEDIA_ID(media["id"]));
      }
      else
      {
         e->getDetails()["mediaLibraryId"] = mlId;
      }
      Exception::push(e);
   }

   return rval;
}

/**
 * Create a FileInfo from a row.
 */
static bool _rowToFileInfo(Row* row, FileInfo& fi)
{
   bool rval;

   string str;
   MediaId mediaId;
   uint64_t size;

   // extract the information from the database row and place it
   // into the file info object
   if((rval = row->getText("file_id", str)))
   {
      BM_ID_SET(fi["id"], str.c_str());
   }

   if(rval && (rval = row->getUInt64("media_id", mediaId)))
   {
      BM_ID_SET(fi["mediaId"], mediaId);
   }

   if(rval && (rval = row->getText("path", str)))
   {
      fi["path"] = str.c_str();
      File file(str.c_str());
      const char* ext = file->getExtension();
      fi["extension"] = ((ext != NULL) ? (ext + 1) : "");
   }

   if(rval && (rval = row->getText("content_type", str)))
   {
      fi["contentType"] = str.c_str();
   }

   if(rval && (rval = row->getUInt64("content_size", size)))
   {
      fi["contentSize"] = size;
   }

   if(rval && (rval = row->getUInt64("size", size)))
   {
      fi["size"] = size;
   }

   if(rval && (rval = row->getText("format_details", str)))
   {
      rval = JsonReader::readFromString(
         fi["formatDetails"], str.c_str(), str.length());
   }

   return rval;
}

bool MediaLibraryDatabase::populateFileSet(
   UserId userId, DynamicObject& query, ResourceSet& fileSet, Connection* conn)
{
   bool rval = false;

   // set the starting offset into the result set
   unsigned int start = 0;
   if(query->hasMember("start"))
   {
      start = query["start"]->getUInt64();
   }

   // set the maximum number of resources to retrieve
   unsigned int num = 10;
   if(query->hasMember("num"))
   {
      num = query["num"]->getUInt64();
   }

   // order field and direction
   const char* order = "title";
   if(query->hasMember("order"))
   {
      order = query["order"]->getString();
   }
   const char* dir = "asc";
   if(query->hasMember("asc"))
   {
      dir = query["dir"]->getString();
   }

   // set whether or not an extension was specified
   bool extensionSpecified = query->hasMember("extension");

   // set whether or not an extension was specified
   bool idsSpecified = query->hasMember("ids");

   // set whether or not to include media info
   bool includeMedia =
      query->hasMember("media") && query["media"]->getBoolean();

   // the total is set further down when the call is successful
   uint64_t total = 0;

   // get database connection
   Connection* c = (conn == NULL ? getConnection(userId) : conn);
   // start transaction to keep multiple queries consistent
   if(c != NULL && (rval = c->begin()))
   {
      // build the counting SQL statement as well as the data retrieval
      // SQL statement
      string countSql =
         "SELECT COUNT(*)"
         " FROM " MLDB_TABLE_FILES " f";
      string selectSql =
         "SELECT "
         "f.file_id,f.media_id,f.path,f.content_type,f.content_size,"
         "f.size,f.format_details"
         " FROM " MLDB_TABLE_FILES " f"
         " LEFT JOIN " MLDB_TABLE_MEDIA " m"
         " ON f.media_id=m.media_id";

      if(extensionSpecified)
      {
         countSql.append(" WHERE f.path LIKE '%.").append(
            query["extension"]->getString()).append("'");
         selectSql.append(" WHERE f.path LIKE '%.").append(
            query["extension"]->getString()).append("'");
      }
      // ids override start/num
      if(idsSpecified)
      {
         selectSql.append(" AND f.file_id IN (");
         for(int i = 0, len = query["ids"]->length(); i < len; i++)
         {
            selectSql.push_back('?');
            if(i != (len - 1))
            {
               selectSql.push_back(',');
            }
         }
         selectSql.push_back(')');
      }
      else
      {
         // User input order fields are checked in the MediaLibraryService
         // validiator.
         // Here we pick a useful sub-order.
         if(strcmp(order, "path") == 0)
         {
            // path's are always unique so no sub-order needed
            selectSql.append(" ORDER BY f.path");
         }
         else if(strcmp(order, "size") == 0)
         {
            selectSql.append(" ORDER BY f.size, m.title, f.path");
         }
         else if(strcmp(order, "title") == 0)
         {
            selectSql.append(" ORDER BY m.title, f.path");
         }
         else if(strcmp(order, "type") == 0)
         {
            selectSql.append(" ORDER BY f.content_type, m.title, f.path");
         }
         selectSql.append(strcmp(dir, "desc") == 0 ? " DESC" : " ASC");
         // :start,:num
         selectSql.append(" LIMIT ?,?");
      }

      // prepare statements
      Statement* cs = c->prepare(countSql.c_str());
      Statement* s = c->prepare(selectSql.c_str());

      if((rval = (cs != NULL && s != NULL)))
      {
         // run count statement
         rval = cs->execute();
         // run files statement
         if(rval)
         {
            if(idsSpecified)
            {
               int offset = 1;
               for(int i = 0, len = query["ids"]->length();
                  rval && (i < len);
                  i++)
               {
                  rval = s->setText(i + offset, query["ids"][i]->getString());
               }
            }
            else
            {
               rval =
                  s->setUInt64(1, start) &&
                  s->setUInt64(2, num);
            }
         }
         rval = rval && s->execute();

         // transform each row into a FileInfo object
         if(rval)
         {
            string fileId;
            Row* row;
            while(rval && (row = s->fetch()) != NULL)
            {
               // add to the file info list
               DynamicObject& info = fileSet["resources"]->append();
               FileInfo& fi = info["fileInfo"];
               fi->setType(Map);

               // extract the information from the database row and place it
               // into the file info object
               rval = _rowToFileInfo(row, fi);

               if(rval && includeMedia)
               {
                  Media& m = info["media"];
                  // populate media based on media id
                  BM_ID_SET(m["id"], BM_MEDIA_ID(fi["mediaId"]));
                  rval = sPopulateMedia(userId, m, 0, c);
                  // if media not found, ignore errors and set media to null
                  if(!rval)
                  {
                     ExceptionRef e = Exception::get();
                     if(e->hasType(MLDB_EXCEPTION_NOT_FOUND))
                     {
                        info["media"].setNull();
                        Exception::clear();
                        rval = true;
                     }
                  }
               }
            }
         }

         // set the total number of rows given the query values
         if(rval)
         {
            rval = false;
            Row* crow = cs->fetch();
            if(crow != NULL)
            {
               rval = crow->getUInt64((unsigned int)0, total);
               cs->fetch();
            }

            if(!rval)
            {
               ExceptionRef e = new Exception(
                  "Failed to retrieve the row count for the given query.",
                  MLDB_EXCEPTION ".FileSetRetrievalFailure");
               BM_ID_SET(e->getDetails()["userId"], userId);
               e->getDetails()["start"] = start;
               e->getDetails()["num"] = num;
               if(extensionSpecified)
               {
                  e->getDetails()["extension"] = query["extension"];
               }
               Exception::push(e);
            }
         }
      }

      // commit transaction if everything worked
      rval = rval ? c->commit() : c->rollback() && false;
   }

   if(conn == NULL && c != NULL)
   {
      // close connection
      c->close();
   }

   // set an exception if there was a failure for some reason
   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Failed to retrieve the set of media associated with the given "
         "userId.",
         MLDB_EXCEPTION ".FileSetRetrievalFailure");
      BM_ID_SET(e->getDetails()["userId"], userId);
      e->getDetails()["start"] = start;
      e->getDetails()["num"] = num;
      if(extensionSpecified)
      {
         e->getDetails()["extension"] = query["extension"];
      }
      Exception::push(e);
   }
   else
   {
      // set the start, num and total values for the file set
      fileSet["start"] = start;
      fileSet["num"] = num;
      fileSet["total"] = total;
      // ensure the resources array exists. this may create an empty one
      fileSet["resources"]->setType(Array);
   }

   return rval;
}

bool MediaLibraryDatabase::updateMedia(
   UserId userId, Media& media, Connection* conn)
{
   bool rval = false;

   // get database connection
   Connection* c = (conn == NULL ? getConnection(userId) : conn);
   if(c != NULL)
   {
      // do update in a transaction because we must update multiple records
      // at once
      DatabaseClientRef dbc = getDatabaseClient(userId);
      if((rval = dbc->begin(c)))
      {
         // update media table
         if(rval)
         {
            SqlExecutableRef se = dbc->replace(MLDB_TABLE_MEDIA, media);
            rval = dbc->execute(se, c);
         }

         // delete existing contributors for media
         if(rval)
         {
            DynamicObject where;
            where["mediaId"] = media["id"];
            SqlExecutableRef se = dbc->remove(MLDB_TABLE_CONTRIBUTORS, &where);
            rval = dbc->execute(se, c);
         }

         // update contributors table
         if(rval)
         {
            media["contributors"]->setType(Map);
            DynamicObjectIterator i = media["contributors"].getIterator();
            while(rval && i->hasNext())
            {
               DynamicObject& array = i->next();
               const char* type = i->getName();
               DynamicObjectIterator ci = array.getIterator();
               while(rval && ci->hasNext())
               {
                  DynamicObject& contributor = ci->next();
                  DynamicObject row;
                  row["mediaId"] = media["id"];
                  row["name"] = contributor["name"];
                  row["type"] = type;
                  SqlExecutableRef se = dbc->insert(
                     MLDB_TABLE_CONTRIBUTORS, row);
                  rval = dbc->execute(se, c);
               }
            }
         }

         // end transaction
         rval = dbc->end(c, rval) && rval;
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
         "Could not update media in media library database.",
         MLDB_EXCEPTION ".UpdateMediaException");
      BM_ID_SET(e->getDetails()["userId"], userId);
      BM_ID_SET(e->getDetails()["mediaId"], BM_MEDIA_ID(media["id"]));
      Exception::push(e);
   }

   return rval;
}

bool MediaLibraryDatabase::removeMedia(
   UserId userId, MediaId mediaId, Connection* conn)
{
   bool rval = false;

   // get database client
   DatabaseClientRef dbc = getDatabaseClient(userId);
   if(!dbc.isNull())
   {
      // delete media entry
      DynamicObject where;
      where["id"] = mediaId;
      SqlExecutableRef se = dbc->remove(MLDB_TABLE_MEDIA, &where);
      rval = dbc->execute(se, conn);
      if(rval)
      {
         // set exception if nothing affected
         if(se->rowsAffected)
         {
            ExceptionRef e = new Exception(
               "The media does not exist in the database.",
               MLDB_EXCEPTION ".MediaNotFound");
            BM_ID_SET(e->getDetails()["mediaId"], mediaId);
            Exception::set(e);
            rval = false;
         }
      }
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "There was a database error when attempting to remove the "
         "media from the media library.",
         MLDB_EXCEPTION ".DeleteMediaException");
      BM_ID_SET(e->getDetails()["userId"], userId);
      BM_ID_SET(e->getDetails()["mediaId"], mediaId);
      Exception::push(e);
   }

   return rval;
}

bool MediaLibraryDatabase::populateMedia(
   UserId userId, Media& media, MediaLibraryId mlId, Connection* conn)
{
   bool rval = false;

   // get database connection
   Connection* c = (conn == NULL ? getConnection(userId) : conn);
   if(c != NULL)
   {
      // start transaction because multiple tables will be queried
      rval = c->begin();
      if(rval)
      {
         rval = sPopulateMedia(userId, media, mlId, c);

         // commit transaction if everything worked
         rval = rval ? c->commit() : c->rollback() && false;
      }

      if(conn == NULL)
      {
         // close connection
         c->close();
      }
   }

   return rval;
}

Connection* MediaLibraryDatabase::getConnection(UserId userId)
{
   return mPerUserDB->getConnection(mConnectionGroupId, userId);
}

DatabaseClientRef MediaLibraryDatabase::getDatabaseClient(UserId userId)
{
   return mPerUserDB->getDatabaseClient(mConnectionGroupId, userId);
}

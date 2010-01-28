/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_medialibrary_MediaLibraryDatabase_H
#define bitmunk_medialibrary_MediaLibraryDatabase_H

#include "bitmunk/common/TypeDefinitions.h"
#include "bitmunk/medialibrary/IMediaLibrary.h"
#include "bitmunk/peruserdb/IPerUserDBModule.h"
#include "bitmunk/peruserdb/PerUserDatabase.h"
#include "bitmunk/node/Node.h"
#include "monarch/sql/DatabaseClient.h"

#include <vector>

namespace bitmunk
{
namespace medialibrary
{

/**
 * The MediaLibraryDatabase stores the data related to the MediaLibraryModule.
 * This data includes filenames, IDs, and other meta-data needed to manage
 * the on-disk media library.
 *
 * Note: The media library database may be extended (have additional tables
 * added to it) but other node modules. The naming convention for the tables
 * in the database must follow this format:
 *
 * <moduletype>_<table name>
 *
 * Where any "." characters are replaced by underscores. For instance,
 * the bitmunk.medialibrary module creates a table to store files that
 * is named:
 *
 * bitmunk_medialibrary_files
 *
 * According to sqlite3's docs, there is no hard limit to table names.
 *
 * @author Manu Sporny
 * @author Dave Longley
 */
class MediaLibraryDatabase : public bitmunk::peruserdb::PerUserDatabase
{
protected:
   /**
    * The node using the media library database.
    */
   bitmunk::node::Node* mNode;

   /**
    * A reference to the per-user database module's interface.
    */
   bitmunk::peruserdb::IPerUserDBModule* mPerUserDB;

   /**
    * The ID for a connection to the purchase database.
    */
   bitmunk::peruserdb::ConnectionGroupId mConnectionGroupId;

   /**
    * A list of media library extensions.
    */
   typedef std::vector<IMediaLibraryExtension*> ExtensionList;
   ExtensionList mExtensions;

public:
   /**
    * Creates a new MediaLibraryDatabase.
    */
   MediaLibraryDatabase();

   /**
    * Destructs this MediaLibraryDatabase.
    */
   virtual ~MediaLibraryDatabase();

   /**
    * Initializes this database for use.
    *
    * @param node the Node for this database.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool initialize(bitmunk::node::Node* node);

   /**
    * Gets the url and and maximum number of concurrent connections a
    * user can have to this database.
    *
    * This will be called when the first connection is requested for a
    * particular connection type.
    *
    * @param id the ID of the connection group.
    * @param userId the ID of the user.
    * @param url the url string to populate.
    * @param maxCount the maximum concurrent connection count to populate.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool getConnectionParams(
      bitmunk::peruserdb::ConnectionGroupId id, bitmunk::common::UserId userId,
      std::string& url, uint32_t& maxCount);

   /**
    * Initializes a database for a user.
    *
    * This will be called when the first connection is requested for a
    * particular for a particular connection type.
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
      bitmunk::peruserdb::ConnectionGroupId id, bitmunk::common::UserId userId,
      monarch::sql::Connection* conn, monarch::sql::DatabaseClientRef& dbc);

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
      bitmunk::peruserdb::ConnectionGroupId id, bitmunk::common::UserId userId);

   /**
    * Adds an IMediaLibraryExtension instance to be called when a particular
    * user's database is initialized.
    *
    * @param extension the extension to add.
    */
   virtual void addMediaLibraryExtension(IMediaLibraryExtension* extension);

   /**
    * Removes an IMediaLibraryExtension instance.
    *
    * @param extension the IMediaLibraryExtension instance.
    */
   virtual void removeMediaLibraryExtension(IMediaLibraryExtension* extension);

   /**
    * Inserts a new or updates an existing file in the database.
    *
    * @param userId the ID of the user the file belongs to.
    * @param fi the file info to store, which must contain "id", "mediaId", and
    *           "path" fields.
    * @param mlId to be set to the media library ID of the file if not NULL.
    * @param added to be set to true if the file was inserted, false if updated.
    * @param conn the connection to use, NULL to open and close one.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool updateFile(
      bitmunk::common::UserId userId, bitmunk::common::FileInfo& fi,
      MediaLibraryId* mlId = NULL, bool* added = NULL,
      monarch::sql::Connection* conn = NULL);

   /**
    * Deletes the given file from the database, based on the file ID in
    * the file info object. If the path in the passed file info is incorrect,
    * it will be corrected after the call returns but the remove operation
    * will still be carried out
    *
    * @param userId the ID of the user the file belongs to.
    * @param fi the file info to delete from the database.
    * @param conn the connection to use, NULL to open and close one.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool removeFile(
      bitmunk::common::UserId userId, bitmunk::common::FileInfo& fi,
      monarch::sql::Connection* conn = NULL);

   /**
    * Populates a file based on its file ID or media library ID.
    *
    * @param userId the ID of the user the file belongs to.
    * @param fi the file info to populate, with either its ID set or
    *           a media library ID other than 0 provided.
    * @param mlId the media library ID of the file or 0 to use its file ID.
    * @param conn the connection to use, NULL to open and close one.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool populateFile(
      bitmunk::common::UserId userId, bitmunk::common::FileInfo& fi,
      MediaLibraryId mlId = 0,
      monarch::sql::Connection* conn = NULL);

   /**
    * Retrieves the list of files that are stored in the database for the
    * given userId.
    *
    * @param userId the ID of the user the file belongs to.
    * @param query the query parameters to use when retrieving the set of files.
    * @param fileSet the set of files that were retrieved based on the query
    *                parameters.
    * @param conn the connection to use, NULL to open and close one.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool populateFileSet(
      bitmunk::common::UserId userId, monarch::rt::DynamicObject& query,
      bitmunk::common::ResourceSet& fileSet, monarch::sql::Connection* conn = NULL);

   /**
    * Inserts a new or updates an existing media in the database.
    *
    * @param userId the ID of the user the file belongs to.
    * @param media the media to store, which must contain "id" and
    *              "title" fields.
    * @param conn the connection to use, NULL to open and close one.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool updateMedia(
      bitmunk::common::UserId userId,
      bitmunk::common::Media& media, monarch::sql::Connection* conn = NULL);

   /**
    * Deletes the given media from the database, based on its ID.
    *
    * @param userId the ID of the user the file belongs to.
    * @param mediaId the ID of the media to delete from the database.
    * @param conn the connection to use, NULL to open and close one.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool removeMedia(
      bitmunk::common::UserId userId,
      bitmunk::common::MediaId mediaId, monarch::sql::Connection* conn = NULL);

   /**
    * Populates a media based on its media ID.
    *
    * @param userId the ID of the user the file belongs to.
    * @param media the media object to populate with its ID set or
    *              a media library ID other than 0 provided.
    * @param mlId the media library ID associated with the media or 0 to use
    *             a media ID.
    * @param conn the connection to use, NULL to open and close one.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool populateMedia(
      bitmunk::common::UserId userId,
      bitmunk::common::Media& media,
      MediaLibraryId mlId = 0, monarch::sql::Connection* conn = NULL);

   /**
    * Gets a connection from the connection pool for a specific userId. It
    * must be closed (but not deleted) when the caller is finished with it.
    *
    * @param userId the id of the user that the connection should operate under.
    *
    * @return a connection or NULL if an exception occurred.
    */
   virtual monarch::sql::Connection* getConnection(bitmunk::common::UserId userId);

   /**
    * Gets the database client associated with the media library.
    *
    * @param userId the id of the user that the database client should
    *               operate under.
    *
    * @return the database client associated with the media library.
    */
   virtual monarch::sql::DatabaseClientRef getDatabaseClient(
      bitmunk::common::UserId userId);
};

} // end namespace medialibrary
} // end namespace bitmunk
#endif

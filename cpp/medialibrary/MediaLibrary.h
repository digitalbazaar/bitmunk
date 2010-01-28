/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_medialibrary_MediaLibrary_H
#define bitmunk_medialibrary_MediaLibrary_H

#include "bitmunk/bfp/Bfp.h"
#include "bitmunk/common/TypeDefinitions.h"
#include "bitmunk/medialibrary/IMediaLibrary.h"
#include "bitmunk/medialibrary/MediaLibraryDatabase.h"

namespace bitmunk
{
namespace medialibrary
{

/**
 * The MediaLibrary handles all digital media management for a given node.
 * It is capable of adding, removing, and modifying meta-data associated with
 * files on disk as well as the files themselves. Higher-level modules depend
 * on the media library for performing lower-level functions such as
 * media type detection, moving files on the file system, meta-data
 * management, and other low-level tasks.
 *
 * @author Manu Sporny
 * @author Dave Longley
 */
class MediaLibrary :
public bitmunk::medialibrary::IMediaLibrary
{
protected:
   /**
    * The associated Bitmunk Node.
    */
   bitmunk::node::Node* mNode;

   /**
    * The media library database that should be used for all storage and
    * retrieval of media library metadata.
    */
   MediaLibraryDatabase* mMediaLibraryDatabase;

   /**
    * Observer for the file updated event.
    */
   monarch::event::ObserverRef mFileUpdatedObserver;

public:
   /**
    * Creates a new MediaLibrary.
    *
    * @param node the associated Bitmunk Node.
    */
   MediaLibrary(bitmunk::node::Node* node);

   /**
    * Destructs this MediaLibrary.
    */
   virtual ~MediaLibrary();

   /**
    * Initializes this media library.
    *
    * @return true if successful, false if not.
    */
   virtual bool initialize();

   /**
    * Cleans up this media library. Should be called regardless of the
    * return value of initialize.
    */
   virtual void cleanup();

   /**
    * Adds an IMediaLibraryExtension instance to the media library. This
    * interface will be called whenever a user's media library is initialized
    * to allow the implementer of that interface to perform their own
    * initialization. This is typically used by modules that extend the
    * media library that need to create their own tables or do other work
    * once a user's media library becomes available for use.
    *
    * @param extension the IMediaLibraryExtension instance.
    */
   virtual void addMediaLibraryExtension(IMediaLibraryExtension* extension);

   /**
    * Removes an IMediaLibraryExtension instance from the media library.
    *
    * @param extension the IMediaLibraryExtension instance.
    */
   virtual void removeMediaLibraryExtension(IMediaLibraryExtension* extension);

   /**
    * Gets a connection to a user's media library database. The connection
    * must not be deleted, but must be closed after use. While the connection
    * is not closed, it may block other threads from accessing the database.
    *
    * @param userId the ID of the user to get a connection for.
    *
    * @return a connection to the media library database or NULL on error.
    */
   virtual monarch::sql::Connection* getConnection(
      bitmunk::common::UserId userId);

   /**
    * Gets the database client associated with the media library.
    *
    * @param userId the ID of the user to get a database client for.
    *
    * @return the database client associated with the media library.
    */
   virtual monarch::sql::DatabaseClientRef getDatabaseClient(
      bitmunk::common::UserId userId);

   /**
    * Starts an asynchronous operation to sets the file ID and format details
    * in a file info. If the media ID is not specified in the file info, an
    * attempt will also be made to set it. Once the operation completes, an
    * event will be fired that has the type:
    *
    * "bitmunk.medialibrary.File.scanned" OR
    * "bitmunk.medialibrary.File.exception"
    *
    * If the operation failed, the event will contain an exception field with
    * the exception that occurred.
    *
    * @param userId the ID of the user that owns the file.
    * @param fi the FileInfo for the file, with at least its path set.
    * @param update true to also update the file in the media library, false to
    *               do nothing but return the results in an event.
    * @param userData some user data to include in any event that is fired.
    */
   virtual void scanFile(
      bitmunk::common::UserId userId, bitmunk::common::FileInfo& fi,
      bool update, monarch::rt::DynamicObject* userData = NULL);

   /**
    * Adds or updates a file in the media library.
    *
    * @param userId the ID of the user that owns the file.
    * @param fi the FileInfo for the file, with at least its path set.
    * @param userData some user data to include in any event that is fired.
    *
    * @return true if the file was added, false if it could not be.
    */
   virtual bool updateFile(
      bitmunk::common::UserId userId, bitmunk::common::FileInfo& fi,
      monarch::rt::DynamicObject* userData = NULL);

   /**
    * Removes a file from the media library.
    *
    * @param userId the ID of the user that owns the file.
    * @param fi the FileInfo for the file, with at least its ID set.
    * @param erase true to delete the file from disk, false not to.
    *
    * @return true if a file was removed, false if one was not.
    */
   virtual bool removeFile(
      bitmunk::common::UserId userId, bitmunk::common::FileInfo& fi,
      bool erase);

   /**
    * Gets a file from the media library.
    *
    * @param userId the ID of the user that owns the file.
    * @param fi the FileInfo for the file, with its ID set unless a media
    *           library ID is provided.
    * @param mlId the media library ID for the file or 0 to use a file ID.
    * @param conn a database connection to use, NULL to open and close one.
    *
    * @return true if a file was populated, false on error.
    */
   virtual bool populateFile(
      bitmunk::common::UserId userId, bitmunk::common::FileInfo& fi,
      MediaLibraryId mlId = 0, monarch::sql::Connection* conn = NULL);

   /**
    * Populates a set of files from the media library that match the given
    * query parameters.
    *
    * @param userId the ID of the user that owns the file.
    * @param query the query to use when filtering the set of files.
    * @param fileSet the file set to populate.
    * @param conn a database connection to use, NULL to open and close one.
    *
    * @return true if a file was populated, false on error.
    */
   virtual bool populateFileSet(
      bitmunk::common::UserId userId, monarch::rt::DynamicObject& query,
      bitmunk::common::ResourceSet& fileSet, monarch::sql::Connection* conn = NULL);

   /**
    * Updates the local storage for a media using the data from Bitmunk.
    *
    * @param userId the ID of the user to update the media for.
    * @param media the media with its "id" field set.
    *
    * @return true if successful, false on error.
    */
   virtual bool updateMedia(
      bitmunk::common::UserId userId, bitmunk::common::Media media);

   /**
    * Populates media from the media library.
    *
    * @param userId the ID of the user to populate the media for.
    * @param media the media object to populate with its ID set or
    *              a media library ID other than 0 provided.
    * @param mlId the media library ID associated with the media or 0 to use
    *             a media ID.
    * @param conn a database connection to use, NULL to open and close one.
    *
    * @return true if successful, false on error.
    */
   virtual bool populateMedia(
      bitmunk::common::UserId userId, bitmunk::common::Media media,
      MediaLibraryId mlId = 0, monarch::sql::Connection* conn = NULL);

protected:
   /**
    * Run in an operation to scan a file.
    *
    * @param d a DynamicObject with "fileInfo" and "userId".
    */
   virtual void scanFile(monarch::rt::DynamicObject& d);

   /**
    * Called when a file is updated.
    *
    * @param e the event.
    */
   virtual void fileUpdated(monarch::event::Event& e);
};

} // end namespace medialibrary
} // end namespace bitmunk
#endif

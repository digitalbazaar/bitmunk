/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_medialibrary_IMediaLibrary_H
#define bitmunk_medialibrary_IMediaLibrary_H

#include "bitmunk/common/TypeDefinitions.h"
#include "bitmunk/medialibrary/IMediaLibraryExtension.h"
#include "monarch/kernel/MicroKernelModuleApi.h"
#include "monarch/sql/Connection.h"

namespace bitmunk
{
namespace medialibrary
{

// media library IDs are per-user and identify a file entry in the
// media library database... they can be used as keys by media library
// extensions that have their own database tables
typedef uint32_t MediaLibraryId;

/**
 * The IMediaLibrary is an interface to a medialibrary module.
 *
 * @author Dave Longley
 */
class IMediaLibrary : public monarch::kernel::MicroKernelModuleApi
{
public:
   /**
    * Creates a new IMediaLibrary.
    */
   IMediaLibrary() {};

   /**
    * Destructs this IMediaLibrary.
    */
   virtual ~IMediaLibrary() {};

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
   virtual void addMediaLibraryExtension(
      IMediaLibraryExtension* extension) = 0;

   /**
    * Removes an IMediaLibraryExtension instance from the media library.
    *
    * @param extension the IMediaLibraryExtension instance.
    */
   virtual void removeMediaLibraryExtension(
      IMediaLibraryExtension* extension) = 0;

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
      bitmunk::common::UserId userId) = 0;

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
      bool update, monarch::rt::DynamicObject* userData = NULL) = 0;

   /**
    * Adds or updates a file in the media library. If a media ID is specified
    * in the file info, then it will be used. If one is not specified, then an
    * attempt will be made to detect the media ID using the file's contents
    * based on the path in the file info.
    *
    * @param userId the ID of the user that owns the file.
    * @param fi the FileInfo for the file, with at least its path set.
    * @param userData some user data to include in any event that is fired.
    *
    * @return true if the file was added, false if it could not be.
    */
   virtual bool updateFile(
      bitmunk::common::UserId userId, bitmunk::common::FileInfo& fi,
      monarch::rt::DynamicObject* userData = NULL) = 0;

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
      bool erase) = 0;

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
      MediaLibraryId mlId = 0, monarch::sql::Connection* conn = NULL) = 0;
};

} // end namespace medialibrary
} // end namespace bitmunk
#endif

/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_medialibrary_IMediaLibraryExtension_H
#define bitmunk_medialibrary_IMediaLibraryExtension_H

#include "bitmunk/common/TypeDefinitions.h"
#include "monarch/sql/DatabaseClient.h"

namespace bitmunk
{
namespace medialibrary
{

/**
 * The IMediaLibraryExtension is an interface to a module that extends
 * the medialibrary module. 
 * 
 * @author Dave Longley
 */
class IMediaLibraryExtension
{
public:
   /**
    * Creates a new IMediaLibraryExtension.
    */
   IMediaLibraryExtension() {};
   
   /**
    * Destructs this IMediaLibraryExtension.
    */
   virtual ~IMediaLibraryExtension() {};
   
   /**
    * Called when the media library has been initialized. This call may be
    * used to perform any initialization that is required by the media
    * library extension.
    * 
    * This will be called when the first connection is requested to a user's
    * media library. It will be called for each different user.
    * 
    * @param userId the ID of the user of the media library.
    * @param conn a connection to the media library to use to perform
    *             any custom initialization, *MUST NOT* be closed before
    *             returning.
    * @param dbc a DatabaseClient to use -- but it can only use the provided
    *            connection.
    * 
    * @return true if successful, false if an exception occurred.
    */
   virtual bool mediaLibraryInitialized(
      bitmunk::common::UserId userId, monarch::sql::Connection* conn,
      monarch::sql::DatabaseClientRef& dbc) = 0;
   
   /**
    * Called when the media library is being cleaned up. This call may be
    * used to clean up any locally cached information that is required by
    * the media library extension.
    * 
    * This will be called when a user logs out and their media library becomes
    * inaccessible. It will be called for each different user.
    * 
    * @param userId the ID of the user of the media library.
    * 
    * @return true if successful, false if an exception occurred.
    */
   virtual void mediaLibraryCleaningUp(bitmunk::common::UserId userId) {};
};

} // end namespace medialibrary
} // end namespace bitmunk
#endif

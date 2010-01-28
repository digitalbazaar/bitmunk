/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_medialibrary_MediaLibraryService_H
#define bitmunk_medialibrary_MediaLibraryService_H

#include "bitmunk/medialibrary/MediaLibrary.h"
#include "bitmunk/node/NodeService.h"

namespace bitmunk
{
namespace medialibrary
{

/**
 * A MediaLibraryService provides a restful HTTP interface to the MediaLibrary.
 *
 * @author Manu Sporny
 * @author Dave Longley
 */
class MediaLibraryService : public bitmunk::node::NodeService
{
protected:
   /**
    * The media library to use.
    */
   MediaLibrary* mLibrary;

public:
   /**
    * Creates a new MediaLibraryService.
    *
    * @param node the associated Bitmunk Node.
    * @param path the home path of this service.
    * @param ml the media library that this service accesses.
    */
   MediaLibraryService(
      bitmunk::node::Node* node, const char* path, MediaLibrary* ml);

   /**
    * Destructs this MediaLibraryService.
    */
   virtual ~MediaLibraryService();

   /**
    * Initializes this MediaLibraryService.
    *
    * @return true if initialized, false if an Exception occurred.
    */
   virtual bool initialize();

   /**
    * Cleans up this MediaLibraryService.
    *
    * Must be called after initialize() regardless of its return value.
    */
   virtual void cleanup();

   /**
    * Updates file information in the media library.
    * 
    * If a file ID is given, it will be used.
    * 
    * If no fileId is given, the file will be fingerprinted in an asynchronous
    * process and a 202 Accepted will be returned. An event of type:
    * "bitmunk.medialibrary.File.scanned" will be generated once the file ID
    * or "fingerprint" process has finished. An event of type:
    * "bitmunk.medialibrary.File.updated" will be generated once the file
    * has been added to the library. If there is any exception during the
    * process, then the event "bitmunk.medialibrary.File.exception" will be
    * generated.
    * 
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool updateFile(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);

   /**
    * Removes one or more files from the media library that match the given
    * query.
    *
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool removeFiles(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);

   /**
    * Gets a file from the media library given the file ID as a path parameter.
    *
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool getFile(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);

   /**
    * Gets a list of files from the media library that match the given query or
    * match a list of ids.
    *
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool getFileSet(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);
   
   /**
    * Updates a media in the media library using the latest information
    * from Bitmunk.
    * 
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool updateMedia(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);
   
   /**
    * Populates a media from the media library database.
    * 
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool populateMedia(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);
};

} // end namespace medialibrary
} // end namespace bitmunk
#endif

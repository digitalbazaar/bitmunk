/*
 * Copyright (c) 2007-2009 Digital Bazaar, Inc.  All rights reserved.
 */
#ifndef bitmunk_sell_SampleService_H
#define bitmunk_sell_SampleService_H

#include "bitmunk/node/NodeService.h"

namespace bitmunk
{
namespace sell
{

/**
 * A SampleService provides samples for media in a catalog.
 * 
 * @author Dave Longley
 */
class SampleService : public bitmunk::node::NodeService
{
protected:
   /**
    * Controls access to samples by limiting the number of samples
    * that can be streamed concurrently.
    */
   monarch::rt::DynamicObject mSampleSemaphore;
   
   /**
    * A lock for mSampleSemaphore.
    */
   monarch::rt::ExclusiveLock mSampleSemaphoreLock;
   
   /**
    * A cache for sample ranges.
    */
   monarch::rt::DynamicObject mSampleRangeCache;
   
   /**
    * A lock for mSampleRangeCache.
    */
   monarch::rt::ExclusiveLock mSampleRangeCacheLock;
   
public:
   /**
    * Creates a new SampleService.
    * 
    * @param node the associated Bitmunk Node.
    * @param path the path this servicer handles requests for.
    */
   SampleService(bitmunk::node::Node* node, const char* path);
   
   /**
    * Destructs this SampleService.
    */
   virtual ~SampleService();
   
   /**
    * Initializes this BtpService.
    * 
    * @return true if initialized, false if an Exception occurred.
    */
   virtual bool initialize();
   
   /**
    * Cleans up this BtpService.
    * 
    * Must be called after initialize() regardless of its return value.
    */
   virtual void cleanup();
   
   /**
    * Serves sample files.
    * 
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool getSampleFile(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);
   
   /**
    * Serves sample media.
    * 
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool getSampleMedia(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);
   
   /**
    * Serves sample play lists.
    * 
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool getSamplePlayList(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);
   
protected:
   
   /**
    * Serves sample file based on mediaId and fileId.
    * 
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    * @param mediaId the file MediaId
    * @param fileId the file FileId
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool getSampleFileByIds(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out,
      bitmunk::common::MediaId mediaId, bitmunk::common::FileId fileId);
   
   /**
    * Common code to get FileInfos for this request.
    * 
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    * @param fileInfos DynamicObject Array to populate
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool getFileInfos(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out,
      monarch::rt::DynamicObject& fileInfos);
   
   /**
    * Gets the sample range for a Media.
    * 
    * @param media the Media to get the sample range for.
    * 
    * @return true if successful, false if failure or no sample range.
    */
   virtual bool getSampleRange(bitmunk::common::Media& media);
   
   /**
    * Sends an m3u playlist for a list of files.
    * 
    * @param action the BtpAction.
    * @param fileInfos the list of file infos.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool sendm3u(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& fileInfos);
   
   /**
    * Sends a sample for a single file.
    * 
    * @param action the BtpAction.
    * @param fi the FileInfo to send a sample of.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool sendFile(
      bitmunk::protocol::BtpAction* action, bitmunk::common::FileInfo& fi);
   
   /**
    * Sends a sample playlist for the files in a ware.
    * 
    * @param action the BtpAction.
    * @param ware the Ware to sample.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool sendPlayList(
      bitmunk::protocol::BtpAction* action, bitmunk::common::Ware& ware);
};

} // end namespace sell
} // end namespace bitmunk
#endif

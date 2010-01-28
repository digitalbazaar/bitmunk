/*
 * Copyright (c) 2007-2008 Digital Bazaar, Inc.  All rights reserved.
 */
#ifndef bitmunk_data_Id3v2TagWriter_H
#define bitmunk_data_Id3v2TagWriter_H

#include "monarch/io/MutationAlgorithm.h"
#include "monarch/data/id3v2/Tag.h"
#include "monarch/data/id3v2/FrameSource.h"

namespace bitmunk
{
namespace data
{

/**
 * An Id3v2TagWriter is used to write/strip ID3 tags in data.
 * 
 * @author Dave Longley
 */
class Id3v2TagWriter : public monarch::io::MutationAlgorithm
{
protected:
   /**
    * The Id3Tag to write, NULL to strip.
    */
   monarch::data::id3v2::Tag* mTag;
   
   /**
    * True to delete mTag when destructing, false not to.
    */
   bool mCleanupTag;
   
   /**
    * The source for the frame data, NULL to strip.
    */
   monarch::data::id3v2::FrameSource* mSource;
   
   /**
    * True to delete mSource when destructing, false not to.
    */
   bool mCleanupSource;
   
   /**
    * Set to true once the tag header has been written.
    */
   bool mTagHeaderWritten;
   
   /**
    * A list of frames to be written.
    */
   typedef std::list<monarch::data::id3v2::FrameHeader*> FrameList;
   FrameList mFrames;
   
   /**
    * The current frame header.
    */
   monarch::data::id3v2::FrameHeader* mFrameHeader;
   
   /**
    * The header for the id3v2 tag to strip, if one exists.
    */
   monarch::data::id3v2::TagHeader* mStripTagHeader;
   
   /**
    * Stores the number of tag bytes left to strip.
    */
   int mBytesToStrip;
   
   /**
    * A helper function that strips an existing id3v2 tag in the source data.
    * 
    * @param src the buffer of source data.
    * @param dst the buffer of destination data.
    * @param finish true if no more data will be passed, false if not.
    * 
    * @return the mutation algorithm result.
    */
   monarch::io::MutationAlgorithm::Result stripTag(
      monarch::io::ByteBuffer* src, monarch::io::ByteBuffer* dst, bool finish);
   
   /**
    * A helper function that adds a new id3v2 tag to the destination data.
    * 
    * @param src the buffer of source data.
    * @param dst the buffer of destination data.
    * @param finish true if no more data will be passed, false if not.
    * 
    * @return the mutation algorithm result.
    */
   monarch::io::MutationAlgorithm::Result addTag(
      monarch::io::ByteBuffer* src, monarch::io::ByteBuffer* dst, bool finish);
   
public:
   /**
    * Creates a new Id3v2TagWriter that will either strip an existing id3v2
    * tag or it will write a new id3v2 tag.
    * 
    * A data source must be provided for the frame data.
    * 
    * @param tag the Tag to write out or NULL to strip an existing tag.
    * @param cleanupTag true to delete the Tag when destructing, false not to.
    * @param source the data source or NULL to strip an existing tag.
    * @param cleanupSource true to delete the source when destructing, false
    *                      not to.
    */
   Id3v2TagWriter(
      monarch::data::id3v2::Tag* tag,
      bool cleanupTag = false,
      monarch::data::id3v2::FrameSource* source = NULL,
      bool cleanupSource = false);
   
   /**
    * Destructs this Id3v2TagWriter.
    */
   virtual ~Id3v2TagWriter();
   
   /**
    * Gets data out of the source ByteBuffer, mutates it in some implementation
    * specific fashion, and then puts it in the destination ByteBuffer.
    * 
    * The return value of this method should be:
    * 
    * NeedsData: If this algorithm requires more data in the source buffer to
    * execute its next step.
    * 
    * Stepped: If this algorithm had enough data to execute its next step,
    * regardless of whether or not it wrote data to the destination buffer.
    * 
    * CompleteAppend: If this algorithm completed and any remaining source data
    * should be appended to the data it wrote to the destination buffer.
    * 
    * CompleteTruncate: If this algorithm completed and any remaining source
    * data must be ignored (it *must not* be appended to the data written to
    * the destination buffer). The remaining source data will be untouched so
    * that it can be used for another purpose if so desired.
    * 
    * Error: If an exception occurred.
    * 
    * Once one a CompleteX result is returned, this method will no longer
    * be called for the same data stream.
    * 
    * Note: The source and/or destination buffer may be resized by this
    * algorithm to accommodate its data needs.
    * 
    * @param src the source ByteBuffer with bytes to mutate.
    * @param dst the destination ByteBuffer to write the mutated bytes to.
    * @param finish true if there will be no more source data and the mutation
    *               algorithm should finish, false if there is more data.
    * 
    * @return the MutationAlgorithm::Result.
    */
   virtual monarch::io::MutationAlgorithm::Result mutateData(
      monarch::io::ByteBuffer* src, monarch::io::ByteBuffer* dst, bool finish);
   
   /**
    * Gets the ID3 tag.
    * 
    * @return the ID3 tag.
    */
   virtual monarch::data::id3v2::Tag* getTag();
   
   /**
    * Gets the frame source for this reader.
    * 
    * @return the frame source for this reader.
    */
   virtual monarch::data::id3v2::FrameSource* getFrameSource();
};

} // end namespace data
} // end namespace bitmunk
#endif

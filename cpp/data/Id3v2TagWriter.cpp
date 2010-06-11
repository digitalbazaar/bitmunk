/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/data/Id3v2TagWriter.h"

using namespace monarch::io;
using namespace monarch::data::id3v2;
using namespace bitmunk::data;

Id3v2TagWriter::Id3v2TagWriter(
   Tag* tag, bool cleanupTag,
   FrameSource* source, bool cleanupSource)
{
   // initialize data
   mTag = tag;
   mCleanupTag = cleanupTag;
   mSource = source;
   mCleanupSource = cleanupSource;
   mStripTagHeader = NULL;
   mFrameHeader = NULL;
   mTagHeaderWritten = false;
   
   if(mTag != NULL)
   {
      // add all frame headers to list of headers to write
      for(FrameList::iterator i = mTag->getFrameHeaders().begin();
          i != mTag->getFrameHeaders().end(); ++i)
      {
         mFrames.push_back(*i);
      }
   }
}

Id3v2TagWriter::~Id3v2TagWriter()
{
   if(mCleanupTag && mTag != NULL)
   {
      delete mTag;
   }
   
   if(mCleanupSource && mSource != NULL)
   {
      delete mSource;
   }
   
   if(mStripTagHeader != NULL)
   {
      delete mStripTagHeader;
   }
}

MutationAlgorithm::Result Id3v2TagWriter::stripTag(
   ByteBuffer* src, ByteBuffer* dst, bool finish)
{
   MutationAlgorithm::Result rval = MutationAlgorithm::Stepped;
   
   // see if no tag to strip found yet
   if(mStripTagHeader == NULL)
   {
      if(src->length() < TagHeader::sHeaderSize)
      {
         if(!finish)
         {
            // needs more data to detect tag
            rval = MutationAlgorithm::NeedsData;
         }
         else
         {
            // no tag, work complete
            rval = MutationAlgorithm::CompleteAppend;
         }
      }
      else
      {
         // look for a tag to strip at the beginning of the data 
         TagHeader* header = new TagHeader();
         if(header->convertFromBytes(src->data()))
         {
            // store the tag header and number of bytes to strip
            mStripTagHeader = header;
            mBytesToStrip =
               mStripTagHeader->getTagSize() + TagHeader::sHeaderSize;
         }
         else
         {
            // no valid tag found, work complete
            delete header;
            rval = MutationAlgorithm::CompleteAppend;
         }
      }
   }
   // tag to strip found
   else
   {
      if(src->isEmpty())
      {
         // needs more data to strip tag
         rval = MutationAlgorithm::NeedsData;
      }
      else
      {
         // strip bytes from source
         mBytesToStrip -= src->clear(mBytesToStrip);
         if(mBytesToStrip == 0)
         {
            // tag is stripped, work complete
            delete mStripTagHeader;
            mStripTagHeader = NULL;
            rval = MutationAlgorithm::CompleteAppend;
         }
      }
   }
   
   return rval;
}

MutationAlgorithm::Result Id3v2TagWriter::addTag(
   ByteBuffer* src, ByteBuffer* dst, bool finish)
{
   MutationAlgorithm::Result rval = MutationAlgorithm::Stepped;
   
   // write tag header if not written
   if(!mTagHeaderWritten)
   {
      // make room for tag header, write it, extend dst buffer 
      dst->allocateSpace(TagHeader::sHeaderSize, true);
      mTag->getHeader()->convertToBytes(dst->data());
      dst->extend(TagHeader::sHeaderSize);
      mTagHeaderWritten = true;
   }
   // get the next frame header if necessary
   else if(mFrameHeader == NULL)
   {
      if(mFrames.empty())
      {
         // no more frames to write, work complete
         rval = MutationAlgorithm::CompleteAppend;
      }
      else
      {
         // get next frame
         mFrameHeader = mFrames.front();
         mFrames.pop_front();
         
         // start frame
         mSource->startFrame(mFrameHeader);
      }
   }
   // write current frame data
   else
   {
      // write frame bytes into destination buffer, do not resize
      int numBytes = mSource->getFrame(dst, false);
      if(numBytes == 0)
      {
         // finished with current frame
         mFrameHeader = NULL;
      }
      else if(numBytes == -1)
      {
         // error in reading frame data
         rval = MutationAlgorithm::Error;
      }
   }
   
   return rval;
}

MutationAlgorithm::Result Id3v2TagWriter::mutateData(
   ByteBuffer* src, ByteBuffer* dst, bool finish)
{
   // if mTag is NULL, then clean existing tag, else write a new tag
   return mTag == NULL ?
      stripTag(src, dst, finish) : addTag(src, dst, finish);
}

Tag* Id3v2TagWriter::getTag()
{
   return mTag;
}

FrameSource* Id3v2TagWriter::getFrameSource()
{
   return mSource;
}

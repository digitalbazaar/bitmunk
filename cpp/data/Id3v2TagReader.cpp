/*
 * Copyright (c) 2007-2009 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/data/Id3v2TagReader.h"

#include "bitmunk/data/Id3v2TagFrameIO.h"

using namespace std;
using namespace monarch::io;
using namespace monarch::data;
using namespace monarch::data::id3v2;
using namespace monarch::rt;
using namespace bitmunk::common;
using namespace bitmunk::data;

Id3v2TagReader::Id3v2TagReader(Tag* tag, FrameSink* sink) :
   mTag(tag),
   mTagRef(NULL),
   mSink(sink),
   mHeaderParsed(false),
   mExtendedHeaderParsed(false),
   mCaptureHeader(NULL),
   mBytesCaptured(0),
   mTagBytesRead(0)
{
}

Id3v2TagReader::Id3v2TagReader(Id3v2TagRef& tag) :
   mTag(&(*tag)),
   mTagRef(tag),
   mSink(tag->getFrameSink()),
   mHeaderParsed(false),
   mExtendedHeaderParsed(false),
   mCaptureHeader(NULL),
   mBytesCaptured(0),
   mTagBytesRead(0)
{
}

Id3v2TagReader::~Id3v2TagReader()
{
}

int Id3v2TagReader::detectFormat(const char* b, int length)
{
   int rval = 0;

   if(!mHeaderParsed ||
      (mTagBytesRead < mTag->getHeader()->getTagSize() &&
       mTag->getHeader()->getTagSize() > 0 &&
       mTag->getHeader()->getTagSize() < TagHeader::sMaxTagSize))
   {
      // if capturing frame data
      if(mCaptureHeader != NULL)
      {
         // get remaining data to write
         int size = mCaptureHeader->getFrameSize() - mBytesCaptured;
         rval = (length < size) ? length : size;
         mBytesCaptured += rval;

         // store data if a sink is available
         if(mSink != NULL)
         {
            ByteBuffer src((char*)b, 0, rval, rval, false);
            mSink->putFrameData(&src);
         }

         // release frame if it is fully captured
         if(mCaptureHeader->getFrameSize() == mBytesCaptured)
         {
            mCaptureHeader = NULL;
            mBytesCaptured = 0;
         }

         // increment tag bytes read
         mTagBytesRead += rval;
      }
      else
      {
         // not processing a frame
         if(length >= 10)
         {
            // a header could potentially be parsed
            if(!mHeaderParsed)
            {
               // try to parse the main header
               if(mTag->getHeader()->convertFromBytes(b))
               {
                  // header has been parsed, 10 bytes inspected
                  mHeaderParsed = true;
                  rval = 10;
               }
               else
               {
                  // FIXME: very primitive detection only checks at the
                  // beginning of the stream for now
                  setFormatRecognized(false);
                  rval = length;
               }
            }
            else if(mTag->getHeader()->getExtendedHeaderFlag() &&
                    !mExtendedHeaderParsed)
            {
               // get size of extended header
               if(length >= 4)
               {
                  // pass entire extended header
                  int size = FrameHeader::convertBytesToInt(b);
                  if(length >= size)
                  {
                     rval = size;
                  }
               }

               // increment tag bytes read
               mTagBytesRead += rval;
            }
            else
            {
               // create a frame header and parse it
               mCaptureHeader = new FrameHeader();
               mCaptureHeader->convertFromBytes(b, length);
               mBytesCaptured = 0;

               // add the frame header
               mTag->addFrameHeader(mCaptureHeader, false);

               if(mSink != NULL)
               {
                  // start frame
                  mSink->startFrame(mCaptureHeader);
               }

               // pass frame header
               rval = 10;

               // increment tag bytes read
               mTagBytesRead += rval;
            }
         }
      }
   }

   // see if header has been fully parsed
   if(mHeaderParsed && mTagBytesRead == mTag->getHeader()->getTagSize())
   {
      // format recognized
      setFormatRecognized(true);

      // no more data to inspect
      rval = length;
   }

   return rval;
}

DynamicObject Id3v2TagReader::getFormatDetails()
{
   DynamicObject formatDetails;

   formatDetails["inspectorName"] = "bitmunk.data.Id3v2TagReader";
   if(!isFormatRecognized())
   {
      formatDetails["inspectorType"] = DataFormatInspector::Unknown;
      formatDetails["contentType"] = "application/octet-stream";
   }
   else
   {
      formatDetails["inspectorType"] = DataFormatInspector::Metadata;
      formatDetails["contentType"] = "application/x-id3-tag";

      Id3v2TagFrameIO* sink = dynamic_cast<Id3v2TagFrameIO*>(getFrameSink());
      if(sink != NULL)
      {
         Media& media = sink->getMedia();
         if(!media.isNull())
         {
            formatDetails["media"] = media.clone();
         }
      }

      // FIXME: add ID3 fields
   }

   return formatDetails;
}

bool Id3v2TagReader::headerParsed()
{
   return mHeaderParsed;
}

monarch::data::id3v2::Tag* Id3v2TagReader::getTag()
{
   return mTag;
}

monarch::data::id3v2::FrameSink* Id3v2TagReader::getFrameSink()
{
   return mSink;
}

/*
 * Copyright (c) 2007-2008 Digital Bazaar, Inc.  All rights reserved.
 */
#ifndef bitmunk_data_Id3v2TagReader_H
#define bitmunk_data_Id3v2TagReader_H

#include "monarch/data/AbstractDataFormatInspector.h"
#include "monarch/data/id3v2/Tag.h"
#include "monarch/data/id3v2/FrameSink.h"
#include "bitmunk/data/Id3v2Tag.h"

namespace bitmunk
{
namespace data
{

/**
 * An Id3v2TagReader is used to read ID3 tags in data.
 * 
 * Note: If no id3v2 tag is found at the beginning of the data stream, then
 * the inspection algorithm will be aborted.
 * 
 * @author Dave Longley
 */
class Id3v2TagReader : public monarch::data::AbstractDataFormatInspector
{
protected:
   /**
    * The ID3v2 Tag to populate.
    */
   monarch::data::id3v2::Tag* mTag;
   bitmunk::data::Id3v2TagRef mTagRef;
   
   /**
    * The sink for the frame data.
    */
   monarch::data::id3v2::FrameSink* mSink;
   
   // FIXME: add cleanup flag for sink
   
   /**
    * Set to true if the Id3 tag header has been parsed.
    */
   bool mHeaderParsed;
   
   /**
    * Set to true if the Id3 extended header has been parsed.
    */
   bool mExtendedHeaderParsed;
   
   /**
    * Stores the current capture frame header.
    */
   monarch::data::id3v2::FrameHeader* mCaptureHeader;
   
   /**
    * Stores the number of bytes captured for the current capture frame.
    */
   int mBytesCaptured;
   
   /**
    * Stores the total number of tag bytes read (excluding the main header).
    */
   int mTagBytesRead;
   
public:
   /**
    * Creates a new Id3v2TagReader that will populate the passed ID3v2 tag. A
    * sink may be provided to store the actual tag frame data if desired.
    * 
    * @param tag the Id3v2Tag to populate.
    * @param sink the FrameSink to store tag frame data in.
    */
   Id3v2TagReader(
      monarch::data::id3v2::Tag* tag, monarch::data::id3v2::FrameSink* sink);
   
   /**
    * Creates a new Id3v2TagReader that will populate the passed ID3v2 tag.
    * 
    * @param tag the Id3v2Tag to populate.
    */
   Id3v2TagReader(Id3v2TagRef& tag);
   
   /**
    * Destructs this Id3v2TagReader.
    */
   virtual ~Id3v2TagReader();
   
   /**
    * Inspects the data in the passed buffer and tries to detect its
    * format. The number of bytes that were inspected is returned so that
    * they can be safely cleared from the passed buffer.
    *
    * Subsequent calls to this method should be treated as if the data
    * in the passed buffer is consecutive in nature (i.e. read from a stream).
    *
    * Once this inspector has determined that the inspected data is in
    * a known or unknown format, this inspector may opt to stop inspecting
    * data.
    *
    * @param b the buffer with data to inspect.
    * @param length the maximum number of bytes to inspect.
    *
    * @return the number of bytes that were inspected in the passed buffer.
    */
   virtual int detectFormat(const char* b, int length);
   
   /**
    * Gets details of the data inspection.
    *
    * @return details of the data inspection.
    */
   virtual monarch::rt::DynamicObject getFormatDetails();
   
   /**
    * Returns whether or not a valid tag header has been parsed.
    * 
    * @return true if a valid tag header has been parsed.
    */
   virtual bool headerParsed();
   
   /**
    * Gets the ID3 tag.
    * 
    * @return the ID3 tag.
    */
   virtual monarch::data::id3v2::Tag* getTag();
   
   /**
    * Gets the frame sink for this reader.
    * 
    * @return the frame sink for this reader.
    */
   virtual monarch::data::id3v2::FrameSink* getFrameSink();
};

} // end namespace data
} // end namespace bitmunk
#endif

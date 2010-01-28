/*
 * Copyright (c) 2007-2008 Digital Bazaar, Inc.  All rights reserved.
 */
#ifndef bitmunk_data_MpegAudioFrameParser_H
#define bitmunk_data_MpegAudioFrameParser_H

#include "monarch/data/mpeg/AudioFrameHeader.h"

namespace bitmunk
{
namespace data
{

/**
 * An MpegAudioFrameParser parses a byte array for an MPEG Audio frame.
 * 
 * @author Dave Longley
 */
class MpegAudioFrameParser
{
protected:
   /**
    * Stores the last MpegAudioFrameHeader that was found.
    */
   monarch::data::mpeg::AudioFrameHeader* mLastHeader;
   
   /**
    * Stores the last offset at which an MpegAudioFrame was found in a given
    * byte array. 
    */
   int mLastOffset;
   
   /**
    * Resets this parser. Sets the last header and offset to none.
    */
   virtual void reset();
   
   /**
    * A helper method that tries to parse a frame out of the given byte array.
    * The start of the byte array is already known to match up with
    * "frame sync". This method returns the number of additional bytes
    * required to parse out a frame, or 0 if a frame has been found or no
    * frame exists at the beginning of the array.
    * 
    * @param b the byte array to parse the frame from.
    * @param length the number of valid bytes in the byte array.
    * 
    * @return the number of bytes required to parse out the entire frame at
    *         the given offset, or 0 if the frame was parsed or no frame exists
    *         at the given offset.
    */
   virtual int parseFrame(const char* b, int length);
   
public:
   /**
    * Creates a new MpegAudioFrameParser.
    */
   MpegAudioFrameParser();
   
   /**
    * Destructs this MpegAudioFrameParser.
    */
   virtual ~MpegAudioFrameParser();
   
   /**
    * Attempts to find an MPEG AudioFrame in the given byte array.
    * 
    * If only part of a frame is found in the array, this method returns the
    * number of extra bytes required to find the entire frame.
    * 
    * If a full frame was found in the array or no frame was found at all,
    * this method returns 0.
    * 
    * @param b the byte array to search.
    * @param length the number of valid bytes in the byte array. 
    * 
    * @return if a partial frame is found the number of bytes required to
    *         find a whole frame, if a whole frame or no frame is found, 0.
    */
   virtual int findFrame(const char* b, int length);
   
   /**
    * Gets the last MpegAudioFrameHeader that was found.
    * 
    * @return the last MpegAudioFrameHeader that was found.
    */
   virtual monarch::data::mpeg::AudioFrameHeader* getLastHeader();
   
   /**
    * Gets the last offset at which an MpegAudioFrame was found.
    * 
    * @return the last offset at which an MpegAudioFrame was found.
    */
   virtual int getLastOffset();
};

} // end namespace data
} // end namespace bitmunk
#endif

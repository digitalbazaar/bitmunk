/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/data/MpegAudioFrameParser.h"

using namespace monarch::data::mpeg;
using namespace bitmunk::data;

MpegAudioFrameParser::MpegAudioFrameParser()
{
   // no header, or offset found yet
   mLastHeader = NULL;
   mLastOffset = -1;
}

MpegAudioFrameParser::~MpegAudioFrameParser()
{
   if(mLastHeader != NULL)
   {
      delete mLastHeader;
   }
}

void MpegAudioFrameParser::reset()
{
   if(mLastHeader != NULL)
   {
      delete mLastHeader;
      
      // no header, or offset found yet
      mLastHeader = NULL;
      mLastOffset = -1;
   }
}

int MpegAudioFrameParser::parseFrame(const char* b, int length)
{
   int rval = 0;
   
   // create a header
   AudioFrameHeader* header = new AudioFrameHeader();
   
   // convert the header from the byte array and proceed if it is valid
   if(header->convertFromBytes(b, length))
   {
      // valid header found
      mLastHeader = header;
      
      // see how much data is needed to obtain the frame
      int frameLength = header->getFrameLength();
      if(length < frameLength)
      {
         // no frame found,
         // return the number of bytes needed to get the entire frame
         rval = frameLength - length;
      }
   }
   else
   {
      // clean up header, it is invalid
      delete header;
   }
   
   return rval;
}

int MpegAudioFrameParser::findFrame(const char* b, int length)
{
   int rval = 0;
   
   // reset
   reset();
   
   // see if there isn't even enough room to parse a header
   if(length < 4)
   {
      // return the number of bytes needed to get the frame header
      rval = 4 - length;
   }
   else
   {
      // mpeg audio frames start with a header that has "frame sync" --
      // meaning the first byte is 255 and the second byte is greater than 223
      // so use unsigned bytes to compare
      const unsigned char* ub = (const unsigned char*)b;
      
      // go through the array one byte at a time searching for a frame header
      // search until 1 minus offset + length as we look ahead 1 byte during
      // the search
      int end = length - 1;
      for(int offset = 0; offset < end; ++offset)
      {
         // if the byte after the current offset is not at least 224, we know
         // we can skip two bytes of data in our search for a frame, and if the
         // first byte is not 255, we know we can skip 1 byte
         
         // look ahead one byte
         if(ub[offset + 1] > 0xdf)
         {
            // look at the current byte
            if(ub[offset] == 0xff)
            {
               // we have frame sync, so we may have a frame -- try to parse it
               rval = parseFrame(b + offset, end - offset + 1);
               if(mLastHeader != NULL)
               {
                  // store last offset
                  mLastOffset = offset;
               }
               break;
            }
         }
         else
         {
            // we can skip an offset because the second byte can't possibly
            // match a frame header's second byte
            ++offset;
         }
      }
   }
   
   return rval;
}

AudioFrameHeader* MpegAudioFrameParser::getLastHeader()
{
   return mLastHeader;
}

int MpegAudioFrameParser::getLastOffset()
{
   return mLastOffset;
}

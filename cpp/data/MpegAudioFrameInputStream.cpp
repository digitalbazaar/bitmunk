/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/data/MpegAudioFrameInputStream.h"

#include "bitmunk/data/Id3v2TagWriter.h"
#include "monarch/io/MutatorInputStream.h"
#include "monarch/rt/Exception.h"

using namespace bitmunk::data;
using namespace monarch::data::mpeg;
using namespace monarch::io;
using namespace monarch::rt;

#define BUFFER_SIZE 2048

MpegAudioFrameInputStream::MpegAudioFrameInputStream(
   InputStream* is, bool cleanup) :
   FilterInputStream(is, cleanup),
   mInputBuffer(BUFFER_SIZE),
   mFrameBytesLeft(0),
   mFrameCount(0)
{
   // attach id3v2 tag stripper
   MutatorInputStream* mis = new MutatorInputStream(
      is, cleanup, new Id3v2TagWriter(NULL), true);
   mInputStream = mis;
   mCleanupInputStream = true;
}

MpegAudioFrameInputStream::~MpegAudioFrameInputStream()
{
}

int MpegAudioFrameInputStream::read(char* b, int length)
{
   int rval = 0;
   
   // fill input buffer with frame bytes if necessary
   if(mFrameBytesLeft > 0 && length > mFrameBytesLeft)
   {
      rval = mInputBuffer.fill(mInputStream);
   }
   
   bool eos = false;
   while(rval != -1 && !eos && mFrameBytesLeft == 0)
   {
      // fill the input buffer
      rval = mInputBuffer.fill(mInputStream);
      if(rval != -1)
      {
         // find the next frame
         int needed = mFrameParser.findFrame(
            mInputBuffer.data(), mInputBuffer.length());
         if(mFrameParser.getLastOffset() == 0)
         {
            // frame found, record number of bytes remaining in frame
            mFrameCount++;
            AudioFrameHeader* header = mFrameParser.getLastHeader();
            mFrameBytesLeft = header->getFrameLength();
         }
         else if(needed == 0)
         {
            // no consecutive mpeg frame found, mark as end of mpeg stream
            rval = 0;
            eos = true;
         }
         else if(rval == 0 && !mInputBuffer.isFull())
         {
            // no more data added to the input buffer when it has room,
            // so end of stream found
            eos = true;
         }
      }
   }
   
   if(!eos && length > 0)
   {
      // read data from input buffer up to mFrameBytesLeft
      int readSize = (length < mFrameBytesLeft ? length : mFrameBytesLeft);
      rval = mInputBuffer.get(b, readSize);
      if(rval > 0)
      {
         mFrameBytesLeft -= rval;
      }
   }
   
   return rval;
}

int MpegAudioFrameInputStream::readFrame(
   ByteBuffer* b, int maxSize, int& frameLength)
{
   // fill input buffer with frame bytes
   int rval = read(NULL, 0);
   
   if(rval != -1 && mFrameBytesLeft > 0)
   {
      // record frame length
      frameLength = mFrameBytesLeft;
      
      // make sure frame length doesn't exceed given max size for buffer
      if(frameLength > maxSize)
      {
         ExceptionRef e = new Exception(
            "Mpeg audio frame is too large.",
            "bitmunk.data.MpegAudioFrameTooLarge");
         Exception::set(e);
         rval = -1;
      }
      else
      {
         // fill passed buffer with frame
         rval = 1;
         b->allocateSpace(mFrameBytesLeft, true);
         while(rval > 0 && mFrameBytesLeft > 0)
         {
            rval = b->put(this, mFrameBytesLeft);
         }
      }
   }
   
   return rval;
}

AudioFrameHeader* MpegAudioFrameInputStream::getLastHeader()
{
   return mFrameParser.getLastHeader();
}

int MpegAudioFrameInputStream::getFrameCount()
{
   return mFrameCount;
}

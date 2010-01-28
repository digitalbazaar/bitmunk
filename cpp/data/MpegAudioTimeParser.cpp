/*
 * Copyright (c) 2007-2009 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/data/MpegAudioTimeParser.h"

#include "monarch/rt/Exception.h"

using namespace monarch::io;
using namespace monarch::data::mpeg;
using namespace monarch::rt;
using namespace bitmunk::data;

MpegAudioTimeParser::MpegAudioTimeParser(double start, double end) :
   StreamTimeParser(start, end)
{
   mAcceptBytes = 0;
   mStripBytes = 0;
}

MpegAudioTimeParser::~MpegAudioTimeParser()
{
}

MutationAlgorithm::Result MpegAudioTimeParser::mutateData(
   ByteBuffer* src, ByteBuffer* dst, bool finish)
{
   MutationAlgorithm::Result rval = MutationAlgorithm::Stepped;
   
   // see if there isn't enough source data to accept/strip or
   // if there isn't enough to parse the next frame
   if(src->isEmpty() ||
      (mAcceptBytes == 0 && mStripBytes == 0 && src->length() < 4))
   {
      if(finish)
      {
         // algorithm complete, truncate data
         rval = MutationAlgorithm::CompleteTruncate;
      }
      else
      {
         // more data needed
         rval = MutationAlgorithm::NeedsData;
      }
   }
   else
   {
      // FIXME: move accept/strip to the base class?
      if(mAcceptBytes > 0)
      {
         // accept bytes
         mAcceptBytes -= src->get(dst, mAcceptBytes, true);
      }
      else if(mStripBytes > 0)
      {
         // strip bytes
         mStripBytes -= src->clear(mStripBytes);
      }
      else
      {
         // find an mpeg frame
         mFrameParser.findFrame(src->data(), src->length());
         if(mFrameParser.getLastHeader() != NULL)
         {
            if(mFrameParser.getLastOffset() > src->offset())
            {
               // set accept bytes, allow bytes to pass between current
               // data position and next frame
               mAcceptBytes = mFrameParser.getLastOffset() - src->offset();
            }
            else
            {
               // get the header
               AudioFrameHeader* header = mFrameParser.getLastHeader();
               
               // increase time passed by audio time in the frame
               setCurrentTime(getCurrentTime() + header->getAudioLength());
               
               // see if time is in a valid time set
               if(isTimeValid(getCurrentTime()))
               {
                  // set accept bytes
                  mAcceptBytes = header->getFrameLength();
               }
               else
               {
                  // set strip bytes
                  mStripBytes = header->getFrameLength();
               }
            }
         }
         else
         {
            // error, no header found
            ExceptionRef e = new Exception(
               "Invalid mpeg data. No header found while parsing sample.");
            Exception::set(e);
            rval = MutationAlgorithm::Error;
         }
      }
   }
   
   return rval;
}

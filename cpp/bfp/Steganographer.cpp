/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/bfp/Steganographer.h"

using namespace monarch::io;
using namespace bitmunk::bfp;

Steganographer::Steganographer() :
   mWatermark(NULL)
{
   reset();
}

Steganographer::~Steganographer()
{
}

void Steganographer::reset()
{
   // no watermark
   mWatermark.setNull();
   
   // no immutable bytes yet
   mImmutableBytes = 0;
   
   // no need to retry mutation yet
   mRetryMutation = false;
}

int Steganographer::inspectData(const char* b, int length)
{
   int rval = 0;
   
   // update immutable bytes as necessary
   if(mImmutableBytes == 0)
   {
      mImmutableBytes = mWatermark->getImmutableBytes(b, length, false);
   }
   
   if(mImmutableBytes == 0)
   {
      // do watermark inspection
      rval = mWatermark->inspect(b, length);
   }
   else if(mImmutableBytes == -1)
   {
      // not enough data, do another pass
      mImmutableBytes = 0;
   }
   
   if(mImmutableBytes > 0)
   {
      // mark immutable bytes as inspected
      rval = (int)(mImmutableBytes & 0x7fffffff);
      mImmutableBytes -= rval;
   }
   
   return rval;
}

bool Steganographer::isDataSatisfied()
{
   // use watermark value, or assume yes if no watermark
   return mWatermark.isNull() ? true : mWatermark->isDataSatisfied();
}

bool Steganographer::keepInspecting()
{
   // do not keep inspecting watermarks if the watermark is data-satisfied
   return false;
}

MutationAlgorithm::Result Steganographer::mutateData(
   ByteBuffer* src, ByteBuffer* dst, bool finish)
{
   MutationAlgorithm::Result rval = MutationAlgorithm::Stepped;
   
   if(mWatermark.isNull())
   {
      // no watermark, so algorithm is complete
      rval = MutationAlgorithm::CompleteAppend;
   }
   else
   {
      if(mImmutableBytes == 0)
      {
         // get immutable bytes if not retrying the mutation
         if(!mRetryMutation)
         {
            // get the immutable bytes in the data stream
            mImmutableBytes = mWatermark->getImmutableBytes(
               src->data(), src->length(), finish);
         }
         
         // if there are no bytes to pass over, then mutate the data
         if(mImmutableBytes == 0)
         {
            // try mutation, store if it must be retried with more data
            mRetryMutation = !mWatermark->mutate(src, dst, finish);
            if(mRetryMutation)
            {
               rval = MutationAlgorithm::NeedsData;
            }
         }
         else if(mImmutableBytes == -1)
         {
            // more data is required
            rval = MutationAlgorithm::NeedsData;
            mImmutableBytes = 0;
         }
         else if(mImmutableBytes < -1)
         {
            // error
            rval = MutationAlgorithm::Error;
         }
      }
      
      if(mImmutableBytes > 0)
      {
         if(!src->isEmpty())
         {
            // move any immutable bytes into the destination buffer
            mImmutableBytes -= src->get(
               dst, (int)(mImmutableBytes & 0x7fffffff), true);
         }
         else
         {
            // source is empty, more data required
            rval = MutationAlgorithm::NeedsData;
         }
      }
      
      if(rval == MutationAlgorithm::NeedsData && finish)
      {
         // Algorithm complete, no more input data, there should be
         // no more source data unless the input stream is from a
         // file piece that was truncated -- which will be handled
         // at the bfp level by re-using the same source buffer for
         // each mutation stream. Therefore, remaining data is
         // truncated.
         rval = MutationAlgorithm::CompleteTruncate;
      }
   }
   
   return rval;
}

void Steganographer::setWatermark(WatermarkRef& wm)
{
   mWatermark = wm;
}

WatermarkRef Steganographer::getWatermark()
{
   return mWatermark;
}

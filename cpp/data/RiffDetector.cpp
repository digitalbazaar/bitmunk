/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/data/RiffDetector.h"

using namespace bitmunk::data;
using namespace monarch::data::riff;

RiffDetector::RiffDetector()
{
}

RiffDetector::~RiffDetector()
{
}

int RiffDetector::findRiffChunkHeader(
   RiffChunkHeader& rch, const char* b, int length)
{
   int headerOffset = -1;
   
   int currentOffset = 0;
   int limit = length - RiffChunkHeader::HEADER_SIZE;
   
   // keep going until we find a chunk header
   while(headerOffset == -1 && currentOffset <= limit)
   {
      int size = length - currentOffset;
      if(rch.convertFromBytes(b + currentOffset, size))
      {
         headerOffset = currentOffset; 
      }
      else
      {
         ++currentOffset;
      }
   }
   
   return headerOffset;
}

int RiffDetector::findRiffListHeader(
   RiffListHeader& rlh, const char* b, int length)
{
   int headerOffset = -1;
   
   int currentOffset = 0;
   int limit = length - RiffListHeader::HEADER_SIZE;
   
   // keep going until we find a list header
   while(headerOffset == -1 && currentOffset <= limit)
   {
      int size = length - currentOffset;
      if(rlh.convertFromBytes(b + currentOffset, size))
      {
         headerOffset = currentOffset; 
      }
      else
      {
         ++currentOffset;
      }
   }
   
   return headerOffset;
}

int RiffDetector::findRiffFormHeader(
   RiffFormHeader& rfh, const char* b, int length)
{
   return findRiffListHeader(rfh, b, length);
}

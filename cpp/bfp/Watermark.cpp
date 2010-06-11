/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_CONSTANT_MACROS

#include "bitmunk/bfp/Watermark.h"

#include "monarch/util/Data.h"

#include <cstdlib>

using namespace monarch::io;
using namespace monarch::rt;
using namespace bitmunk::bfp;
using namespace bitmunk::common;

Watermark::Watermark() :
   mWatermarkData(24)
{
   mWrittenEmbeds = 0;
   
   // default mode is to encode a watermark
   setMode(Encode);
}

Watermark::~Watermark()
{
   // reset cleans up data
   Watermark::reset();
}

bool Watermark::isDataSatisfied()
{
   // assume watermark is never data-satisfied until the end
   return false;
}

bool Watermark::mutate(ByteBuffer* src, ByteBuffer* dst, bool finish)
{
   // default to enough data to mutate
   bool rval = true;
   
   switch(mMode)
   {
      case Encode:
         // write the watermark
         rval = write(src, dst, finish);
         break;
      case Clean:
         // clean the watermark
         rval = clean(src, dst, finish);
         break;
      case Decode:
         // inspect data
         int inspected = inspect(src->data(), src->length());
         if(inspected > 0)
         {
            // nothing to mutate, just copy the data
            src->get(dst, inspected, true);
         }
         else if(src->isFull())
         {
            // inspection failed but source is full, so copy data
            src->get(dst, src->length(), true);
         }
         else
         {
            // more data required
            rval = false;
         }
         break;
   }
   
   return rval;
}

void Watermark::reset()
{
   // clear watermark data
   mWatermarkData.clear();
   
   // clean up detected embeds
   for(EmbedDataMap::iterator im = mEmbedDataMap.begin();
       im != mEmbedDataMap.end(); ++im)
   {
      for(EmbedDataList::iterator il = im->second->begin();
          il != im->second->end(); ++il)
      {
         // free hash
         free(il->hash);
      }
      
      // delete list
      delete im->second;
   }
   
   // clear map
   mEmbedDataMap.clear();
}

void Watermark::setMode(Mode mode)
{
   mMode = mode;
}

Watermark::Mode Watermark::getMode()
{
   return mMode;
}

bool Watermark::isValid()
{
   return mWrittenEmbeds > 0 || mEmbedDataMap.size() > 0;
}

void Watermark::setPreprocessData(DynamicObject& data)
{
   mPreprocessData = data;
}

DynamicObject Watermark::getPreprocessData()
{
   return mPreprocessData;
}

// FIXME: remove this method when separating receipts from watermarks
void Watermark::setReceiptData(Contract& c)
{
   // default does nothing
}

void Watermark::setEmbedHash(uint32_t index, const char* hash)
{
   // add big-endian file piece index
   index = MO_UINT32_TO_BE(index);
   mWatermarkData.put((char*)&index, 4, true);
   
   // add contract section hash
   mWatermarkData.put(hash, 20, true);
}

bool Watermark::getEmbedHash(uint32_t index, char* hash)
{
   bool rval = false;
   
   // get the highest embed count for the index
   int count = getEmbedCount(index);
   if(count > 0)
   {
      // get existing embed data list
      EmbedDataMap::iterator i = mEmbedDataMap.find(index);
      EmbedDataList* list = i->second;
      
      // copy hash that matches the embed count
      for(EmbedDataList::iterator itr = list->begin();
          !rval && itr != list->end(); ++itr)
      {
         if(itr->count == count)
         {
            memcpy(hash, itr->hash, 20);
            rval = true;
         }
      }
   }
   
   return rval;
}

int Watermark::getEmbedCount(uint32_t index)
{
   int rval = 0;
   
   // get existing embed data list
   EmbedDataMap::iterator i = mEmbedDataMap.find(index);
   if(i != mEmbedDataMap.end())
   {
      EmbedDataList* list = i->second;
      
      // compare existing hashes
      for(EmbedDataList::iterator itr = list->begin();
          itr != list->end(); ++itr)
      {
         if(itr->count > rval)
         {
            rval = itr->count;
         }
      }
   }
   
   return rval;
}

int Watermark::getEmbedCount()
{
   int rval = 0;
   
   switch(mMode)
   {
      case Encode:
         rval = mWrittenEmbeds;
         break;
      case Clean:
         rval = 0;
         break;
      case Decode:
         rval = mEmbedDataMap.size();
         break;
   }
   
   return rval;
}

void Watermark::addDetectedEmbedData(unsigned char* data)
{
   // parse out big-endian file piece index
   uint32_t index;
   memcpy(&index, data, 4);
   index = MO_UINT32_FROM_BE(index);
   
   // parse out contract hash
   char hash[20];
   memcpy(&hash, data + 4, 20);
   
   // get existing embed data list
   EmbedDataList* list;
   EmbedDataMap::iterator i = mEmbedDataMap.find(index);
   if(i != mEmbedDataMap.end())
   {
      list = i->second;
   }
   else
   {
      // no existing list found, so add one
      list = new EmbedDataList;
      mEmbedDataMap.insert(make_pair(index, list));
   }
   
   // compare existing hashes
   bool found = false;
   for(EmbedDataList::iterator itr = list->begin();
       !found && itr != list->end(); ++itr)
   {
      if(memcmp(itr->hash, hash, 20) == 0)
      {
         // existing hash found, up embed count
         ++itr->count;
         found = true;
      }
   }
   
   if(!found)
   {
      // no same hash found, so add new EmbedData to the list
      EmbedData edata;
      edata.index = index;
      edata.hash = (char*)malloc(20);
      memcpy(edata.hash, hash, 20);
      edata.count = 0;
      list->push_back(edata);
   }
}

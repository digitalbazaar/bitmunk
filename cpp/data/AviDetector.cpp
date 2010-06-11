/*
 * Copyright (c) 2005-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/data/AviDetector.h"

#include "monarch/compress/deflate/Deflater.h"
#include "monarch/data/DynamicObjectOutputStream.h"
#include "monarch/data/json/JsonReader.h"
#include "monarch/io/MutatorOutputStream.h"
#include "bitmunk/common/Logging.h"

#include <cstdio>
#include <vector>

using namespace std;
using namespace bitmunk::common;
using namespace bitmunk::data;
using namespace monarch::compress::deflate;
using namespace monarch::data;
using namespace monarch::data::avi;
using namespace monarch::data::json;
using namespace monarch::data::riff;
using namespace monarch::io;
using namespace monarch::rt;

typedef vector<AviStreamHeaderList*> StreamHeaderLists;

// FIXME: This needs to be better state-machinified for a simpler design
AviDetector::AviDetector() :
   mFormHeader(false),
   mHeaderList(NULL),
   mStreamHeaderList(NULL),
   mOdmlList(false),
   mOdmlHeader(NULL),
   mInfoList(false),
   mMovieList(false),
   mIndexChunk(false),
   mMedia(NULL)
{
}

AviDetector::~AviDetector()
{
   resetComponents();
}

void AviDetector::resetComponents()
{
   /*
   if(mFormHeader != NULL)
   {
      delete mFormHeader;
      mFormHeader = NULL;
   }
   */
   if(mHeaderList != NULL)
   {
      delete mHeaderList;
      mHeaderList = NULL;
   }
   if(mStreamHeaderList != NULL)
   {
      delete mStreamHeaderList;
      mStreamHeaderList = NULL;
   }
   if(mOdmlHeader != NULL)
   {
      delete mOdmlHeader;
      mOdmlHeader = NULL;
   }
   /*
   if(mInfoList != NULL)
   {
      delete mInfoList;
      mInfoList = NULL;
   }
   if(mMovieList != NULL)
   {
      delete mMovieList;
      mMovieList = NULL;
   }
   if(mIndexChunk != NULL)
   {
      delete mIndexChunk;
      mIndexChunk = NULL;
   }
   */
   mOdmlHeader = false;
   mInfoList = false;
   mMovieList = false;
   mIndexChunk = false;

   mMedia.setNull();

   mFormatDetails->clear();
   mFormatDetails["inspectorName"] = "bitmunk.data.AviDetector";
   mFormatDetails["inspectorType"] = DataFormatInspector::Unknown;
   mFormatDetails["contentType"] = "application/octet-stream";
   mFormatDetailsNeedUpdate = false;
}

int AviDetector::foundRiffChunkHeader(
   RiffChunkHeader& rch, const char* b, int length)
{
   int rval = RiffChunkHeader::HEADER_SIZE;

   if(MO_FOURCC_CMP_STR(rch.getIdentifier(), "idx1"))
   {
      // set index chunk header found
      mIndexChunk = true;
      MO_CAT_DEBUG_DETAIL(BM_DATA_CAT, "AviDetector: "
         "AVI Index Chunk detected, size=%u", rch.getPaddedSize());

      // skip chunk
      setSkipBytes(rch.getPaddedSize());
   }
   else if(mMedia.isNull() && MO_FOURCC_CMP_STR(rch.getIdentifier(), "JUNK"))
   {
      MO_CAT_DEBUG_DETAIL(BM_DATA_CAT, "AviDetector: "
         "AVI JUNK Chunk detected, size=%d", rch.getPaddedSize());

      // make sure there is enough room to get the tag
      if(length >= RiffChunkHeader::HEADER_SIZE + 4)
      {
         fourcc_t tag =
            MO_FOURCC_FROM_STR(b + RiffChunkHeader::HEADER_SIZE);
         if(MO_FOURCC_CMP_STR(tag, "bmcr"))
         {
            // determine if this junk chunk contains a media
            Media m;
            rval = parseJunkChunk(m, rch, b, length);
            if(!m.isNull() && BM_MEDIA_ID_VALID(BM_MEDIA_ID(m["id"])))
            {
               mMedia = m;
               MO_CAT_DEBUG(BM_DATA_CAT, "AviDetector: "
                  "Embedded Media detected.");
            }
            else
            {
               // skip chunk
               rval = RiffChunkHeader::HEADER_SIZE;
               setSkipBytes(rch.getPaddedSize());
            }
         }
         else
         {
            // skip chunk
            setSkipBytes(rch.getPaddedSize());
         }
      }
   }
   else if(mStreamHeaderList != NULL &&
           MO_FOURCC_CMP_STR(rch.getIdentifier(), "strh"))
   {
      // wait until enough data is available
      int chunk = RiffChunkHeader::HEADER_SIZE + rch.getPaddedSize();
      if(length >= chunk)
      {
         MO_CAT_DEBUG_DETAIL(BM_DATA_CAT, "AviDetector: "
            "AVI stream header detected, size=%d", rch.getPaddedSize());

         // store stream header
         AviStreamHeader* h = new AviStreamHeader();
         h->convertFromBytes(b, length);
         mStreamHeaderList->setStreamHeader(h);

         // skip chunk
         setSkipBytes(rch.getPaddedSize());
      }
      else
      {
         // not enough data yet
         rval = 0;
      }
   }
   else if(mStreamHeaderList != NULL &&
           MO_FOURCC_CMP_STR(rch.getIdentifier(), "strf"))
   {
      // wait until enough data is available
      int chunk = RiffChunkHeader::HEADER_SIZE + rch.getPaddedSize();
      if(length >= chunk)
      {
         MO_CAT_DEBUG_DETAIL(BM_DATA_CAT, "AviDetector: "
            "AVI stream format detected, size=%d", rch.getPaddedSize());

         // store stream format
         AviStreamFormat* f = new AviStreamFormat();
         f->convertFromBytes(b, length);
         mStreamHeaderList->setStreamFormat(f);

         // add stream header list
         mHeaderList->getStreamHeaderLists().push_back(mStreamHeaderList);
         mStreamHeaderList = NULL;

         // skip chunk
         setSkipBytes(rch.getPaddedSize());
      }
      else
      {
         // not enough data yet
         rval = 0;
      }
   }
   else if(mOdmlList && mOdmlHeader == NULL &&
           MO_FOURCC_CMP_STR(rch.getIdentifier(), "dmlh"))
   {
      // wait until enough data is available
      int dmlhSize = RiffChunkHeader::HEADER_SIZE + 4;
      if(length >= dmlhSize)
      {
         MO_CAT_DEBUG_DETAIL(BM_DATA_CAT, "AviDetector: "
            "OpenDML header detected, size=%d", rch.getPaddedSize());

         // store odml header
         mOdmlHeader = new OdmlHeader();
         mOdmlHeader->convertFromBytes(b, length);

         // skip odml header chunk
         setSkipBytes(rch.getPaddedSize());
      }
      else
      {
         // not enough data yet
         rval = 0;
      }
   }
   else
   {
      // skip chunk
      setSkipBytes(rch.getPaddedSize());
   }

   return rval;
}

int AviDetector::foundRiffListHeader(
   RiffListHeader& rlh, const char* b, int length)
{
   int rval = RiffListHeader::HEADER_SIZE;

   if(mHeaderList == NULL &&
      MO_FOURCC_CMP_STR(rlh.getIdentifier(), "hdrl"))
   {
      // wait until enough data is available to read the main avi header
      int avihSize =
         RiffListHeader::HEADER_SIZE +
         RiffChunkHeader::HEADER_SIZE + AviHeader::HEADER_SIZE;
      if(length >= avihSize)
      {
         MO_CAT_DEBUG_DETAIL(BM_DATA_CAT, "AviDetector: "
            "AVI header list detected, size=%d", rlh.getListSize());

         // store header list
         mHeaderList = new AviHeaderList();
         mHeaderList->convertFromBytes(b, avihSize);

         // skip avi header only
         setSkipBytes(avihSize - rval);
      }
      else
      {
         // not enough data yet
         rval = 0;
      }
   }
   else if(MO_FOURCC_CMP_STR(rlh.getIdentifier(), "strl"))
   {
      MO_CAT_DEBUG_DETAIL(BM_DATA_CAT, "AviDetector: "
         "AVI stream header list detected, size=%d", rlh.getListSize());

      // add any existing stream header list
      if(mStreamHeaderList != NULL)
      {
         mHeaderList->getStreamHeaderLists().push_back(mStreamHeaderList);
      }
      mStreamHeaderList = new AviStreamHeaderList();
   }
   else if(MO_FOURCC_CMP_STR(rlh.getIdentifier(), "odml"))
   {
      // set odml list found
      mOdmlList = true;
      MO_CAT_DEBUG_DETAIL(BM_DATA_CAT, "AviDetector: "
         "OpenDML list detected, size=%d", rlh.getListSize());
   }
   else if(MO_FOURCC_CMP_STR(rlh.getIdentifier(), "INFO"))
   {
      // set info list found
      mInfoList = true;
      MO_CAT_DEBUG_DETAIL(BM_DATA_CAT, "AviDetector: "
         "AVI INFO list detected, size=%d", rlh.getListSize());

      // skip list
      setSkipBytes(rlh.getListSize());
   }
   else if(!mMovieList && MO_FOURCC_CMP_STR(rlh.getIdentifier(), "movi"))
   {
      // AVI format detected
      mFormatDetails["inspectorType"] = DataFormatInspector::Video;
      mFormatDetails["contentType"] = "video/x-msvideo";
      setFormatRecognized(true);
      mFormatDetailsNeedUpdate = true;

      // set movie list found
      mMovieList = true;
      MO_CAT_DEBUG_DETAIL(BM_DATA_CAT, "AviDetector: "
         "AVI Movie list detected, size=%d", rlh.getListSize());

      // add any existing stream header list
      if(mStreamHeaderList != NULL)
      {
         mHeaderList->getStreamHeaderLists().push_back(mStreamHeaderList);
         mStreamHeaderList = NULL;
      }

      // skip list
      setSkipBytes(rlh.getListSize());
   }
   else
   {
      // skip list
      setSkipBytes(rlh.getListSize());
   }

   return rval;
}

int AviDetector::foundRiffFormHeader(
   RiffFormHeader& rfh, const char* b, int length)
{
   if(!mFormHeader && MO_FOURCC_CMP_STR(rfh.getIdentifier(), "AVI "))
   {
      // store form header
      mFormHeader = true; //&rfh;

      // AVI form header found
      MO_CAT_DEBUG_DETAIL(BM_DATA_CAT, "AviDetector: "
         "AVI RIFF form header found, AVI file size=%d", rfh.getFileSize());

      // skip all data
      //setSkipBytes(rfh.getFileSize());
   }
   else
   {
      // skip data
      setSkipBytes(rfh.getFileSize());
   }

   // return header size
   return RiffFormHeader::HEADER_SIZE;
}

int AviDetector::parseJunkChunk(
   Media& m, RiffChunkHeader& rch, const char* b, int length)
{
   int rval = 0;

   // if the whole junk chunk is present
   if(rch.getPaddedSize() <= (unsigned int)length)
   {
      MO_CAT_DEBUG(BM_DATA_CAT, "parseJunkChunk found %d bytes", length);

      // whole chunk inspected
      rval = RiffChunkHeader::HEADER_SIZE + (int)rch.getPaddedSize();

      // FIXME: this 1-byte padding problem should never happen because the
      // chunk size is the size without the padding... correct?

      // if the ending byte is null, it is padding so skip it
      int start = RiffChunkHeader::HEADER_SIZE + 4;
      int size = (int)rch.getChunkSize();
      if(size > 0 && b[size - 1] == 0x00)
      {
         size--;
      }

      // inflate contract
      Contract c;
      JsonReader reader;
      DynamicObjectOutputStream doos(c, &reader, false);

      Deflater inf;
      inf.startInflating(false);
      MutatorOutputStream mos(&doos, false, &inf, false);
      mos.write(b + start, size);
      mos.close();

      // get media
      m = c["media"];

      // FIXME: handle exceptions properly
      if(Exception::isSet())
      {
         MO_CAT_ERROR(BM_DATA_CAT, "parseJunkChunk of %d bytes of JSON", size);
      }
   }
   else
   {
      // not enough bytes to parse
      // FIXME: do we need to buffer bytes so they are not lost?
   }

   return rval;
}

int AviDetector::detectFormat(const char* b, int length)
{
   int rval = 0;

   // look for some RIFF chunk data
   RiffChunkHeader rch;
   int headerOffset = findRiffChunkHeader(rch, b, length);

   // if the header offset is not at the data offset, let the data
   // before the header offset pass
   if(headerOffset > 0)
   {
      // ignore data before offset
      rval = headerOffset;
   }
   else if(rch.getIdentifier() == RiffFormHeader::CHUNK_ID)
   {
      // try to convert a RIFF form header
      if(length >= RiffFormHeader::HEADER_SIZE)
      {
         RiffFormHeader rfh;
         rfh.convertFromBytes(b, length);

         // report that a form header was found
         rval = foundRiffFormHeader(rfh, b, length);
      }
   }
   else if(rch.getIdentifier() == RiffListHeader::CHUNK_ID)
   {
      if(length >= RiffListHeader::HEADER_SIZE)
      {
         RiffListHeader rlh;
         rlh.convertFromBytes(b, length);

         // found a list
         rval = foundRiffListHeader(rlh, b, length);
      }
   }
   else
   {
      // found a chunk
      rval = foundRiffChunkHeader(rch, b, length);
   }

   if(!mFormHeader && length >= RiffFormHeader::HEADER_SIZE)
   {
      // form header not found, format not recognized
      setFormatRecognized(false);
      rval = length;
      setSkipBytes(0);
   }

   return rval;
}

DynamicObject AviDetector::getFormatDetails()
{
   if(mFormatDetailsNeedUpdate)
   {
      mFormatDetails["inspectorName"] = "bitmunk.data.AviDetector";
      if(!isFormatRecognized())
      {
         mFormatDetails["inspectorType"] = DataFormatInspector::Unknown;
         mFormatDetails["contentType"] = "application/octet-stream";
      }
      else
      {
         mFormatDetails["inspectorType"] = DataFormatInspector::Video;
         mFormatDetails["contentType"] = "video/x-msvideo";
      }

      AviHeader* header = getAviHeader();
      if(header != NULL)
      {
         mFormatDetails["resolutionWidth"] = header->getWidth();
         mFormatDetails["resolutionHeight"] = header->getHeight();
         mFormatDetails["streamCount"] = header->getStreamCount();
         mFormatDetails["hasIndex"] = header->isAviHasIndex();
         mFormatDetails["mustUseIndex"] = header->isAviMustUseIndex();
         mFormatDetails["interleaved"] = header->isAviIsInterleaved();
         mFormatDetails["realTime"] = header->isAviWasCaptureFile();
         mFormatDetails["copyrighted"] = header->isAviCopyrighted();
         mFormatDetails["timeScale"] = header->getTimeScale();
         mFormatDetails["suggestedBufferSize"] =
            header->getSuggestedBufferSize();
         mFormatDetails["dataRate"] = header->getDataRate();
         mFormatDetails["frameRate"] = header->getFrameRate();
         mFormatDetails["initialFrames"] = header->getInitialFrames();
         mFormatDetails["microsecondsPerFrame"] =
            header->getMicrosecondsPerFrame();

         // get total frames from OpenDML header if it exists
         uint32_t totalFrames = (mOdmlHeader != NULL) ?
            mOdmlHeader->getTotalFrames() : header->getTotalFrames();
         double videoTime = 0.0;
         if(totalFrames != 0)
         {
            videoTime = header->getMicrosecondsPerFrame() / 1000000.0;
            videoTime *= totalFrames;
         }
         mFormatDetails["totalFrames"] = totalFrames;
         mFormatDetails["videoLength"] = header->getVideoLength();
         char tmp[100];
         snprintf(tmp, 100, "%.2f", videoTime);
         mFormatDetails["videoTime"] = tmp;
         mFormatDetails["streams"]->setType(Array);

         // get avi stream header video/audio information for first video
         // and first audio streams
         bool firstVids = true;
         bool firstAuds = true;
         bool vids = false;
         bool auds = false;
         StreamHeaderLists& shl = mHeaderList->getStreamHeaderLists();
         for(StreamHeaderLists::iterator i = shl.begin(); i != shl.end(); ++i)
         {
            DynamicObject stream;
            stream->setType(Map);

            // check stream header and format
            AviStreamHeaderList* hl = *i;
            AviStreamHeader* ash = hl->getStreamHeader();
            if(ash != NULL)
            {
               MO_FOURCC_TO_STR(ash->getType(), tmp);
               tmp[4] = 0;
               stream["type"] = tmp;

               MO_FOURCC_TO_STR(ash->getHandler(), tmp);
               tmp[4] = 0;
               stream["handler"] = tmp;

               stream["initialFrames"] = ash->getInitialFrames();
               stream["timeScale"] = ash->getTimeScale();
               stream["rate"] = ash->getRate();
               stream["startTime"] = ash->getStartTime();
               stream["length"] = ash->getLength();
               stream["suggestedBufferSize"] = ash->getSuggestedBufferSize();
               stream["quality"] = ash->getQuality();
               stream["sampleSize"] = ash->getSampleSize();

               vids = MO_FOURCC_CMP_STR(ash->getType(), "vids");
               auds = MO_FOURCC_CMP_STR(ash->getType(), "auds");
            }

            if(vids || auds)
            {
               AviStreamFormat* asf = hl->getStreamFormat();
               if(asf != NULL)
               {
                  if(vids)
                  {
                     stream["bitmapInfoSize"] = asf->getBitmapInfoSize();
                     stream["bitmapWidth"] = asf->getBitmapWidth();
                     stream["bitmapHeight"] = asf->getBitmapHeight();
                     stream["bitCount"] = asf->getBitCount();
                     stream["compression"] = asf->getCompression();
                     stream["imageSize"] = asf->getImageSize();
                     stream["horizontalResolution"] =
                        asf->getHorizontalResolution();
                     stream["verticalResolution"] =
                        asf->getVerticalResolution();
                     stream["colorIndicies"] = asf->getColorInidices();
                     stream["colorIndiciesRequired"] =
                        asf->getColorInidicesRequired();

                     if(firstVids)
                     {
                        mFormatDetails["videoCodec"] = stream["handler"];
                        firstVids = false;
                     }
                  }
                  else if(auds)
                  {
                     stream["formatTag"] = asf->getAudioFormatTag();
                     stream["channels"] = asf->getAudioChannels();
                     stream["samplesPerSecond"] =
                        asf->getAudioSamplesPerSecond();
                     stream["avgBytesPerSecond"] =
                        asf->getAudioAvgBytesPerSecond();
                     stream["blockAlignment"] = asf->getBlockAligmentUnit();
                     stream["bitsPerSample"] = asf->getAudioBitsPerSample();
                     stream["extraSize"] = asf->getExtraAudioInfoSize();

                     if(firstAuds)
                     {
                        // FIXME: need a mapping from format tag to string
                        uint16_t formatTag = asf->getAudioFormatTag();
                        switch(formatTag)
                        {
                           // AC2
                           case 0x0030:
                              mFormatDetails["audioCodec"] = "AC2";
                              break;
                           // MPEG
                           case 0x0050:
                              mFormatDetails["audioCodec"] = "MPEG";
                              break;
                           // MP3
                           case 0x0055:
                              mFormatDetails["audioCodec"] = "MP3";
                              break;
                           // AC3
                           case 0x2000:
                              mFormatDetails["audioCodec"] = "AC3";
                              break;
                           default:
                              mFormatDetails["audioCodec"] = "unknown";
                        }
                        firstAuds = false;
                     }
                  }
               }
            }

            mFormatDetails["streams"]->append(stream);
         }

         // get media information
         if(!mMedia.isNull())
         {
            mFormatDetails["media"] = mMedia.clone();
         }
         else
         {
            mFormatDetails->removeMember("media");
         }
         mFormatDetailsNeedUpdate = false;
      }
   }

   return mFormatDetails.clone();
}

AviHeader* AviDetector::getAviHeader()
{
   AviHeader* rval = NULL;

   if(mHeaderList != NULL)
   {
      rval = &mHeaderList->getMainHeader();
   }

   return rval;
}

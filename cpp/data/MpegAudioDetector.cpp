/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/data/MpegAudioDetector.h"

#include <cmath>
#include <cstdio>

using namespace std;
using namespace monarch::data::mpeg;
using namespace monarch::rt;
using namespace bitmunk::data;

// defines the maximum number of fake mpeg frames in a row that can be
// detected before we give up
#define MAX_FALSE_POSITIVES   5

MpegAudioDetector::MpegAudioDetector() :
   mFalsePositives(0)
{
   resetFormatDetails();
}

MpegAudioDetector::~MpegAudioDetector()
{
}

int MpegAudioDetector::detectFormat(const char* b, int length)
{
   int rval = 0;

   // mpeg header required at least 4 bytes
   if(length > 3)
   {
      rval = length;

      // try to find a frame in the data
      mFrameParser.findFrame(b, length);
      AudioFrameHeader* header = mFrameParser.getLastHeader();
      if(header != NULL)
      {
         // get the header offset
         if(mFrameParser.getLastOffset() > 0)
         {
            // if the first frame has been detected and now there is a gap,
            // then assume that first frame was just a fluke
            if(getFramesDetected() == 1)
            {
               resetFormatDetails();
               mFalsePositives++;
            }
            // if two frames in a row have been detected and there is a gap
            // then assume we're actually looking a movie file or something
            else if(getFramesDetected() == 2)
            {
               // we don't know the format
               if(!isFormatRecognized())
               {
                  setFormatRecognized(false);
               }
            }

            if(mFalsePositives < MAX_FALSE_POSITIVES)
            {
               // data before header offset inspected now, skip it and the
               // next call to this function will be aligned with the next
               // mpeg frame
               rval = mFrameParser.getLastOffset();
            }
            else if(!isFormatRecognized())
            {
               // too many false positives, bail out
               setFormatRecognized(false);
            }
         }
         else
         {
            // get the frame length
            int frameLength = header->getFrameLength();

            // determine the number of bytes that have been inspected
            rval = frameLength;
            if(rval > length)
            {
               // skip the rest of the frame length since it is not
               // available in the current buffer
               setSkipBytes(rval - length);
               rval = length;
            }

            // if the header supports CRC protection, make sure we
            // can inspect the audio data
            if(header->isCrcEnabled())
            {
               // get audio data amount
               int audioDataAmount = mCrc16.getAudioDataAmount(header);

               // get audio data offset (2 bytes past the 4 byte header)
               int audioDataOffset = 6;

               // must have header (4), crc (2), and audio data amount
               if((6 + audioDataAmount) <= length)
               {
                  // record the header information
                  recordHeaderInfo(header);

                  // CRC can be calculated
                  int calcCrc = mCrc16.calculateCrc(
                     header, b + audioDataOffset);

                  // read in the existing CRC-16, it should be the first 2
                  // bytes of the frame data, both bytes should be unsigned
                  const unsigned char* ub = (const unsigned char*)b;
                  unsigned int b0 = ub[4];
                  unsigned int b1 = ub[5];

                  // CRC stored Big Endian, so most significant byte first
                  int existingCrc = (b0 << 8) | b1;

                  // compare CRC's
                  if(calcCrc != existingCrc)
                  {
                     // increment frame CRC errors
                     mFrameCrcErrors++;
                  }
               }
               else
               {
                  // more data required, none inspected yet
                  rval = 0;
                  setSkipBytes(0);
               }
            }
            else
            {
               // record the header information
               recordHeaderInfo(header);
            }
         }
      }
   }

   return rval;
}

DynamicObject MpegAudioDetector::getFormatDetails()
{
   DynamicObject formatDetails;

   formatDetails["inspectorName"] = "bitmunk.data.MpegAudioDetector";
   if(!isFormatRecognized())
   {
      formatDetails["inspectorType"] = DataFormatInspector::Unknown;
      formatDetails["contentType"] = "application/octet-stream";
   }
   else
   {
      formatDetails["inspectorType"] = DataFormatInspector::Audio;
      formatDetails["contentType"] = "audio/mpeg";

      double mpegSize = getTotalMpegAudioSize();
      double mpegPercentage = (mpegSize / getBytesInspected()) * 100;
      char mpegPercent[20];
      snprintf(mpegPercent, 20, "%.2f", mpegPercentage);

      char audioTime[20];
      snprintf(audioTime, 20, "%.2f", getAudioTime());

      formatDetails["framesDetected"] = getFramesDetected();
      formatDetails["frameCrcErrors"] = getFrameCrcErrors();
      formatDetails["totalMpegAudioSize"] = getTotalMpegAudioSize();
      formatDetails["totalAudioTime"] = audioTime;
      formatDetails["exactMpegAudioTime"] = getAudioTime();
      formatDetails["totalBytesInspected"] = getBytesInspected();
      formatDetails["averageBitrate"] = getAverageBitrate();
      formatDetails["variableBitrate"] = isVariableBitrate();
      formatDetails["mpegAudioPercentage"] = mpegPercent;
      formatDetails["mpeg1Frames"] = getFramesDetected(AudioVersion::Mpeg1);
      formatDetails["mpeg2Frames"] = getFramesDetected(AudioVersion::Mpeg2);
      formatDetails["mpeg25Frames"] = getFramesDetected(AudioVersion::Mpeg25);
      formatDetails["layer1Frames"] = getFramesDetected(AudioLayer::Layer1);
      formatDetails["layer2Frames"] = getFramesDetected(AudioLayer::Layer2);
      formatDetails["layer3Frames"] = getFramesDetected(AudioLayer::Layer3);
   }

   return formatDetails;
}

unsigned int MpegAudioDetector::getFramesDetected()
{
   return mFramesDetected;
}

unsigned int MpegAudioDetector::getFramesDetected(AudioVersion::Type version)
{
   return mVersionMap[version];
}

unsigned int MpegAudioDetector::getFramesDetected(AudioLayer::Type layer)
{
   return mLayerMap[layer];
}

unsigned int MpegAudioDetector::getFrameCrcErrors()
{
   return mFrameCrcErrors;
}

unsigned long long MpegAudioDetector::getTotalMpegAudioSize()
{
   return mTotalMpegAudioSize;
}

unsigned int MpegAudioDetector::getAverageBitrate()
{
   unsigned int rval = 0;

   if(getFramesDetected() > 0)
   {
      rval = (int)round(((double)mSummedBitrates) / getFramesDetected());
   }

   if(rval <= 0)
   {
      rval = mHighBitrate;
   }

   return rval;
}

double MpegAudioDetector::getAudioTime()
{
   return mAudioTime;
}

bool MpegAudioDetector::isVariableBitrate()
{
   return (mLowBitrate != mHighBitrate);
}

void MpegAudioDetector::resetFormatDetails()
{
   // clear frame detection
   mFramesDetected = 0;

   // clear frame CRC errors
   mFrameCrcErrors = 0;

   // clear total mpeg audio size
   mTotalMpegAudioSize = 0;

   // initialize version map
   mVersionMap[AudioVersion::Mpeg1] = 0;
   mVersionMap[AudioVersion::Mpeg2] = 0;
   mVersionMap[AudioVersion::Mpeg25] = 0;

   // initialize layer map
   mLayerMap[AudioLayer::Layer1] = 0;
   mLayerMap[AudioLayer::Layer2] = 0;
   mLayerMap[AudioLayer::Layer3] = 0;

   // clear bitrate stats
   mHighBitrate = 0;
   mLowBitrate = 0;
   mSummedBitrates = 0;

   // clear audio time
   mAudioTime = 0.0;
}

void MpegAudioDetector::recordHeaderInfo(AudioFrameHeader* header)
{
   // increment frames detected
   mFramesDetected++;

   // update version map
   AudioVersion version;
   header->getVersion(version);
   mVersionMap[version.type]++;

   // update layer map
   AudioLayer layer;
   header->getLayer(layer);
   mLayerMap[layer.type]++;

   // update bitrate stats
   unsigned int bitrate = header->getBitrate();
   if(bitrate > mHighBitrate)
   {
      mHighBitrate = bitrate;
   }

   if(mLowBitrate == 0 || bitrate < mLowBitrate)
   {
      mLowBitrate = bitrate;
   }

   mSummedBitrates += bitrate;

   // update total mpeg audio size
   mTotalMpegAudioSize += header->getFrameLength();

   // increment total audio time
   mAudioTime += header->getAudioLength();

   // if at least frames have been detected, set format to recognized
   if(!isDataSatisfied() && getFramesDetected() > 10)
   {
      setFormatRecognized(true);
   }
}

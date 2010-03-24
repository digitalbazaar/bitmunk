/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_data_MpegAudioDetector_H
#define bitmunk_data_MpegAudioDetector_H

#include "monarch/data/AbstractDataFormatInspector.h"
#include "monarch/data/mpeg/AudioFrameHeader.h"
#include "monarch/data/mpeg/AudioCrc16.h"
#include "bitmunk/data/MpegAudioFrameParser.h"

namespace bitmunk
{
namespace data
{

/**
 * An MpegAudioDetector is used to detect whether or not a stream of data
 * is MPEG Audio.
 *
 * @author Dave Longley
 */
class MpegAudioDetector : public monarch::data::AbstractDataFormatInspector
{
protected:
   /**
    * The number of frames detected by this detector.
    */
   unsigned int mFramesDetected;

   /**
    * The number of false-positive frames detected.
    */
   unsigned int mFalsePositives;

   /**
    * The total number of CRC errors detected in the MPEG Audio frames.
    */
   unsigned int mFrameCrcErrors;

   /**
    * The total size, in bytes, of all of the detected MPEG Audio data so far.
    */
   uint64_t mTotalMpegAudioSize;

   /**
    * A table that stores the total number of frames detected according
    * to their version.
    */
   typedef std::map<monarch::data::mpeg::AudioVersion::Type, unsigned int>
      VersionMap;
   VersionMap mVersionMap;

   /**
    * A table that stores the total number of frames detected according
    * to their layer.
    */
   typedef std::map<monarch::data::mpeg::AudioLayer::Type, unsigned int>
      LayerMap;
   LayerMap mLayerMap;

   /**
    * Stores the highest bitrate detected so far.
    */
   unsigned int mHighBitrate;

   /**
    * Stores the lowest bitrate detected so far (that is non-zero).
    */
   unsigned int mLowBitrate;

   /**
    * The sum total of all the bitrates detected so far.
    */
   uint64_t mSummedBitrates;

   /**
    * The amount of audio time that has passed so far.
    */
   double mAudioTime;

   /**
    * An mpeg audio frame parser.
    */
   MpegAudioFrameParser mFrameParser;

   /**
    * An mpeg audio crc-16 calculator.
    */
   monarch::data::mpeg::AudioCrc16 mCrc16;

public:
   /**
    * Creates a new MpegAudioDetector.
    */
   MpegAudioDetector();

   /**
    * Destructs this MpegAudioDetector.
    */
   virtual ~MpegAudioDetector();

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
    * Gets the type specific details of this stream.
    *
    * @return the type specific deatils of this stream.
    */
   virtual monarch::rt::DynamicObject getFormatDetails();

   /**
    * Gets the total number of mpeg audio frames detected.
    *
    * @return the total number of mpeg audio frames detected.
    */
   virtual unsigned int getFramesDetected();

   /**
    * Gets the number of frames detected for a given AudioVersion.
    *
    * @param version the AudioVersion to get the detected frames for.
    *
    * @return the number of frames detected for a given AudioVersion.
    */
   virtual unsigned int getFramesDetected(
      monarch::data::mpeg::AudioVersion::Type version);

   /**
    * Gets the number of frames detected for a given AudioLayer.
    *
    * @param layer the AudioLayer to get the detected frames for.
    *
    * @return the number of frames detected for a given AudioLayer.
    */
   virtual unsigned int getFramesDetected(
      monarch::data::mpeg::AudioLayer::Type layer);

   /**
    * Gets the number of frame CRC errors detected.
    *
    * @return the number of frame CRC errors detected.
    */
   virtual unsigned int getFrameCrcErrors();

   /**
    * Gets the total length, in bytes, of all of the mpeg audio frames
    * detected.
    *
    * @return the total length, in bytes, of all of the mpeg audio frames
    *         detected.
    */
   virtual uint64_t getTotalMpegAudioSize();

   /**
    * Gets the average bitrate detected.
    *
    * @return the average bitrate detected.
    */
   virtual unsigned int getAverageBitrate();

   /**
    * Gets the amount of audio time, in seconds, in the mpeg data.
    *
    * @return the amount of audio time, in seconds, in the mpeg data.
    */
   virtual double getAudioTime();

   /**
    * Gets the whether or not the bitrate is variable.
    *
    * @return true if the bitrate is variable, false otherwise.
    */
   virtual bool isVariableBitrate();

protected:
   /**
    * Resets all stored format details.
    */
   virtual void resetFormatDetails();

   /**
    * Gets the header information from the given header.
    *
    * @param header the MpegAudioFrameHeader to get the information from.
    */
   virtual void recordHeaderInfo(monarch::data::mpeg::AudioFrameHeader* header);
};

} // end namespace data
} // end namespace bitmunk
#endif

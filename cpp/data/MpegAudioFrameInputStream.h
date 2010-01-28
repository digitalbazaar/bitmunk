/*
 * Copyright (c) 2009 Digital Bazaar, Inc.  All rights reserved.
 */
#ifndef bitmunk_data_MpegFrameInputStream_H
#define bitmunk_data_MpegFrameInputStream_H

#include "bitmunk/data/MpegAudioFrameParser.h"
#include "monarch/io/ByteBuffer.h"
#include "monarch/io/FilterInputStream.h"

namespace bitmunk
{
namespace data
{

/**
 * An MpegAudioFrameInputStream parses the first set of consecutive mpeg
 * audio frames found out of a data stream. Any other data, i.e. ID3 tags, will
 * be skipped and not read out.
 * 
 * @author Dave Longley
 */
class MpegAudioFrameInputStream : public monarch::io::FilterInputStream
{
protected:
   /**
    * An mpeg frame parser for finding frames in the underlying stream.
    */
   MpegAudioFrameParser mFrameParser;
   
   /**
    * An input buffer to hold bytes to scan for an mpeg frame.
    */
   monarch::io::ByteBuffer mInputBuffer;
   
   /**
    * Stores the number of bytes left in the current frame.
    */
   int mFrameBytesLeft;
   
   /**
    * Counts the number of frames found.
    */
   int mFrameCount;
   
public:
   /**
    * Creates a new MpegAudioFrameInputStream that reads from the passed
    * InputStream.
    * 
    * @param is the underlying InputStream to read from.
    * @param cleanup true to clean up the passed InputStream when destructing,
    *                false not to.
    */
   MpegAudioFrameInputStream(monarch::io::InputStream* is, bool cleanup = false);
   
   /**
    * Destructs this MpegAudioFrameInputStream.
    */
   virtual ~MpegAudioFrameInputStream();
   
   /**
    * Reads some bytes from the stream. This method will block until at least
    * one byte can be read or until the end of the stream is reached. A
    * value of 0 will be returned if the end of the stream has been reached,
    * a value of -1 will be returned if an IO exception occurred, otherwise
    * the number of bytes read will be returned.
    * 
    * @param b the array of bytes to fill.
    * @param length the maximum number of bytes to read into the buffer.
    * 
    * @return the number of bytes read from the stream or 0 if the end of the
    *         stream has been reached or -1 if an IO exception occurred.
    */
   virtual int read(char* b, int length);
   
   /**
    * Reads the next mpeg audio frame into the passed byte buffer, resizing
    * it if necessary, up to a maximum size. If this method returns -1 and
    * frameLength exceeds maxSize, the byte buffer could not be resized
    * to fit the entire frame.
    * 
    * @param b the byte buffer to fill.
    * @param maxSize the maximum size frame to read out.
    * @param frameLength set to the length of the frame.
    * 
    * @return 1 if a frame was read, 0 if end of stream, -1 if there was an
    *         error.
    */
   virtual int readFrame(monarch::io::ByteBuffer* b, int maxSize, int& frameLength);
   
   /**
    * Returns the last mpeg audio frame header.
    * 
    * @return the last mpeg audio frame header.
    */
   virtual monarch::data::mpeg::AudioFrameHeader* getLastHeader();
   
   /**
    * Returns the number of frames read.
    * 
    * @return the number of frames read.
    */
   virtual int getFrameCount();
};

} // end namespace data
} // end namespace bitmunk
#endif

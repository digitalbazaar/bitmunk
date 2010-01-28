/*
 * Copyright (c) 2007-2008 Digital Bazaar, Inc.  All rights reserved.
 */
#ifndef bitmunk_data_MpegAudioTimeParser_H
#define bitmunk_data_MpegAudioTimeParser_H

#include "bitmunk/data/StreamTimeParser.h"
#include "bitmunk/data/MpegAudioFrameParser.h"

namespace bitmunk
{
namespace data
{

/**
 * An MpegAudioTimeParser is used to parse out sections of an MPEG audio
 * stream based on audio play time. 
 * 
 * @author Dave Longley
 */
class MpegAudioTimeParser : public StreamTimeParser
{
protected:
   /**
    * The MpegAudioFrameParser used by this class to find MPEG audio frames.
    */
   MpegAudioFrameParser mFrameParser;
   
   /**
    * An amount of bytes to accept (allow to pass through).
    */
   unsigned long long mAcceptBytes;
   
   /**
    * An amount of bytes to strip.
    */
   unsigned long long mStripBytes;
   
public:
   /**
    * Creates a new MpegAudioTimeParser that defaults to parsing no data out
    * of the stream it processes unless start and end are set. If end is less
    * than start, the time set will not be added.
    * 
    * @param start the start time in seconds.
    * @param end the end time in seconds.
    */
   MpegAudioTimeParser(double start = -1, double end = -1);
   
   /**
    * Destructs this MpegAudioTimeParser.
    */
   virtual ~MpegAudioTimeParser();
   
   /**
    * Gets data out of the source ByteBuffer, mutates it in some implementation
    * specific fashion, and then puts it in the destination ByteBuffer.
    * 
    * The return value of this method should be:
    * 
    * NeedsData: If this algorithm requires more data in the source buffer to
    * execute its next step.
    * 
    * Stepped: If this algorithm had enough data to execute its next step,
    * regardless of whether or not it wrote data to the destination buffer.
    * 
    * CompleteAppend: If this algorithm completed and any remaining source data
    * should be appended to the data it wrote to the destination buffer.
    * 
    * CompleteTruncate: If this algorithm completed and any remaining source
    * data must be ignored (it *must not* be appended to the data written to
    * the destination buffer). The remaining source data will be untouched so
    * that it can be used for another purpose if so desired.
    * 
    * Error: If an exception occurred.
    * 
    * Once one a CompleteX result is returned, this method will no longer
    * be called for the same data stream.
    * 
    * Note: The source and/or destination buffer may be resized by this
    * algorithm to accommodate its data needs.
    * 
    * @param src the source ByteBuffer with bytes to mutate.
    * @param dst the destination ByteBuffer to write the mutated bytes to.
    * @param finish true if there will be no more source data and the mutation
    *               algorithm should finish, false if there is more data.
    * 
    * @return the MutationAlgorithm::Result.
    */
   virtual monarch::io::MutationAlgorithm::Result mutateData(
      monarch::io::ByteBuffer* src, monarch::io::ByteBuffer* dst, bool finish);
};

} // end namespace data
} // end namespace bitmunk
#endif

/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_bfp_Steganographer_H
#define bitmunk_bfp_Steganographer_H

#include "monarch/data/DataInspector.h"
#include "monarch/io/MutationAlgorithm.h"
#include "bitmunk/bfp/Watermark.h"

namespace bitmunk
{
namespace bfp
{

/**
 * A Steganographer encodes, cleans, or decodes a Watermark in a stream
 * of data.
 * 
 * @author Dave Longley
 */
class Steganographer :
public monarch::data::DataInspector,
public monarch::io::MutationAlgorithm
{
protected:
   /**
    * The Watermark for this Steganographer. 
    */
   WatermarkRef mWatermark;
   
   /**
    * Stores the current number of immutable bytes that must be skipped
    * without watermark mutation.
    */
   long long mImmutableBytes;
   
   /**
    * Set to true if the mutation algorithm should be run again if it failed
    * to run due to insufficient bytes. 
    */
   bool mRetryMutation;
   
public:
   /**
    * Creates a new Steganographer.
    */
   Steganographer();
   
   /**
    * Destructs this Steganographer.
    */
   virtual ~Steganographer();
   
   /**
    * Resets this Steganographer.
    */
   virtual void reset();
   
   /**
    * Inspects the data in the passed buffer for some implementation
    * specific attributes. This method returns the number of bytes that
    * were successfully inspected such that the passed buffer can safely
    * clear that number of bytes.
    * 
    * An inspector should treat calls to this method as if the data in
    * the passed buffer is consecutive in nature (i.e. read from a stream).
    * 
    * This method may return a number of bytes that is actually greater
    * than the number passed in, if the DataInspector can determine it does
    * not need to inspect them directly.
    * 
    * @param b the buffer with data to inspect.
    * @param length the number of bytes in the buffer.
    * 
    * @return the number of bytes inspected by this DataInspector and that
    *         should not be passed to it again.
    */
   virtual int inspectData(const char* b, int length);
   
   /**
    * Returns whether or not this inspector is "data-satisfied." The inspector
    * is data-satisfied when it has determined that it doesn't need to inspect
    * any more data.
    * 
    * @return true if this inspector has determined that it doesn't need to
    *         inspect any more data, false if not.
    */
   virtual bool isDataSatisfied();
   
   /**
    * Gets whether or not this inspector should keep inspecting data after it
    * is data-satisfied.
    * 
    * @return true to keep inspecting, false not to.
    */
   virtual bool keepInspecting();
   
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
   
   /**
    * Sets the Watermark to use with this Steganographer.
    * 
    * @param wm the Watermark to use with this Steganographer.
    */
   virtual void setWatermark(WatermarkRef& wm);
   
   /**
    * Gets the Watermark for this Steganographer.
    * 
    * @return the Watermark for this Steganographer.
    */
   virtual WatermarkRef getWatermark();
};

} // end namespace bfp
} // end namespace bitmunk
#endif

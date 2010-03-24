/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_bfp_Watermark_H
#define bitmunk_bfp_Watermark_H

#include "monarch/data/Data.h"
#include "monarch/io/ByteBuffer.h"
#include "bitmunk/common/TypeDefinitions.h"

#include <list>
#include <map>

namespace bitmunk
{
namespace bfp
{

/**
 * A Watermark is used by a Steganographer to manipulate hidden information
 * in a stream of data. It can be used to embed, clean, or detect specific
 * data.
 *
 * @author Dave Longley
 */
class Watermark
{
public:
   /**
    * A WaterMark Mode indicates whether a Watermark should be encoded,
    * cleaned, or decoded. The encode mode overwrites an existing watermark.
    */
   enum Mode
   {
      Encode, Clean, Decode
   };

   /**
    * The preprocess data to use.
    */
   monarch::rt::DynamicObject mPreprocessData;

   /**
    * Watermark EmbedData is the data that is embedded in a data stream.
    *
    * It contains a file piece index, a contract hash, and an embed count.
    */
   struct EmbedData
   {
      uint32_t index;
      char* hash;
      int count;
   };

protected:
   /**
    * The mode for this Watermark.
    */
   Mode mMode;

   /**
    * Stores the watermark data. This includes a header and the embed data.
    */
   monarch::io::ByteBuffer mWatermarkData;

   /**
    * Stores the number of written embeds.
    */
   int mWrittenEmbeds;

   /**
    * Stores the detected embed data. It is a map of file piece index to
    * embed data list.
    */
   typedef std::list<EmbedData> EmbedDataList;
   typedef std::map<uint32_t, EmbedDataList*> EmbedDataMap;
   EmbedDataMap mEmbedDataMap;

public:
   /**
    * Creates a new Watermark.
    */
   Watermark();

   /**
    * Destructs this Watermark.
    */
   virtual ~Watermark();

   /**
    * Inspects the passed data buffer for a Watermark. If one is found it may
    * be read entirely or partially read. This method may return more inspected
    * bytes than the number passed, if it can determine that it doesn't need
    * to inspect the bytes directly.
    *
    * @param b the buffer to find a Watermark in.
    * @param length the number of bytes in the buffer.
    *
    * @return the number of bytes inspected, which can be more than the number
    *         passed.
    */
   virtual int inspect(const char* b, int length) = 0;

   /**
    * Returns whether or not this watermark is "data-satisfied." The watermark
    * is data-satisfied when it has determined that it doesn't need to inspect
    * any more data.
    *
    * @return true if this watermark has determined that it doesn't need to
    *         inspect any more data, false if not.
    */
   virtual bool isDataSatisfied();

   /**
    * Determines the number of bytes, in a stream of data, that must be skipped
    * before this Watermark's mutate() or inspect() method can be called. This
    * information can be determined by examining a chunk of the stream data
    * which is passed to this method. The number of bytes returned may be
    * greater than the number of bytes in the source buffer.
    *
    * A Steganographer uses this method to determine how many bytes in a stream
    * must be written out before calling this Watermark's mutate() or
    * inspect() method. If this method returns 0, then the Steganographer will
    * call mutate() or inspect() on this Watermark.
    *
    * @param b the source data from the stream to inspect.
    * @param length the length of the source data.
    * @param finish true if there will be no more source data from the stream.
    *
    * @return the number of bytes that must not be processed by this Watermark
    *         or -1 there isn't enough source data to determine the number of
    *         bytes, -2 if there was an error.
    */
   virtual int64_t getImmutableBytes(
      const char* b, int length, bool finish) = 0;

   /**
    * Mutates the passed data according to this Watermark's mode and
    * algorithm.
    *
    * @param src the ByteBuffer with data to mutate.
    * @param dst the ByteBuffer to write the mutated data to, if any.
    * @param finish true if there will be no more source data.
    *
    * @return true if there was enough data in the source buffer to mutate it,
    *         false if not.
    */
   virtual bool mutate(
      monarch::io::ByteBuffer* src, monarch::io::ByteBuffer* dst, bool finish);

   /**
    * Resets this watermark before it is to be used on a stream again.
    */
   virtual void reset();

   /**
    * Sets the Watermark mode.
    *
    * @param mode the Watermark mode to use.
    */
   virtual void setMode(Mode mode);

   /**
    * Gets the Watermark mode.
    *
    * @return the Watermark mode.
    */
   virtual Mode getMode();

   /**
    * Returns true if this Watermark is valid, false of not.
    *
    * @return true if this Watermark is valid, false if not.
    */
   virtual bool isValid();

   /**
    * Sets the preprocess data. This method must be called before
    * setting an embed hash.
    *
    * @param data the preprocess data to use.
    */
   virtual void setPreprocessData(monarch::rt::DynamicObject& data);

   /**
    * Gets the generated preprocess data.
    *
    * @return the generated preprocess data.
    */
   virtual monarch::rt::DynamicObject getPreprocessData();

   /**
    * Sets the receipt data for embedding a receipt.
    *
    * @param c the contract to embed.
    */
   // FIXME: remove this method when separating receipts from watermarks
   virtual void setReceiptData(bitmunk::common::Contract& c);

   /**
    * Sets the watermark data. This is the 32-bit integer file piece index
    * and the 20-byte contract section hash.
    *
    * @param index the file piece index.
    * @param hash the 20-byte contract section hash.
    */
   virtual void setEmbedHash(uint32_t index, const char* hash);

   /**
    * Gets the detected embedded hash for the given file piece index.
    *
    * @param index the file piece index to get the hash for.
    * @param hash a byte-array that is at least 20-bytes in size.
    *
    * @return true if the embedded hash was found, false if not.
    */
   virtual bool getEmbedHash(uint32_t index, char* hash);

   /**
    * Gets the number of detected embeds for the given file piece index
    * detected in the data stream.
    *
    * @param index the file piece index to get the hash for.
    *
    * @return the number of embeds for the given file piece index detected
    *         in the data stream.
    */
   virtual int getEmbedCount(uint32_t index);

   /**
    * If decoding, gets the number of unique index embeds. If encoding, gets
    * the number of written embeds.
    *
    * @return the number of embeds.
    */
   virtual int getEmbedCount();

protected:
   /**
    * Adds detected embedded data (24-bytes) to the embed map.
    *
    * @param data the 24-bytes of detected embedded data.
    */
   virtual void addDetectedEmbedData(unsigned char* data);

   /**
    * Gets data from the passed source buffer, removes its Watermark, then
    * writes the cleaned data to the destination buffer. This call may either
    * remove part of this Watermark or all of it.
    *
    * @param src the ByteBuffer with data to clean.
    * @param dst the ByteBuffer to write the cleaned data to.
    * @param finish true if there will be no more source data.
    *
    * @return true if enough data was available to perform the clean,
    *         false if more is needed.
    */
   virtual bool clean(
      monarch::io::ByteBuffer* src, monarch::io::ByteBuffer* dst, bool finish) = 0;

   /**
    * Gets data from the passed source buffer, encodes it with a Watermark,
    * then writes the encoded data to the destination buffer. This call may
    * either write part of this Watermark or all of it.
    *
    * @param src the ByteBuffer with data to encode with this Watermark.
    * @param dst the ByteBuffer to write the encoded data to.
    * @param finish true if there will be no more source data.
    *
    * @return true if enough data was available to perform the write,
    *         false if more is needed.
    */
   virtual bool write(
      monarch::io::ByteBuffer* src, monarch::io::ByteBuffer* dst, bool finish) = 0;
};

// typedef for a reference counted Watermark
typedef monarch::rt::Collectable<Watermark> WatermarkRef;

} // end namespace bfp
} // end namespace bitmunk
#endif

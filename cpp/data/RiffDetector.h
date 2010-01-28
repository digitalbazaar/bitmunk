/*
 * Copyright (c) 2005-2008 Digital Bazaar, Inc.  All rights reserved.
 */
#ifndef bitmunk_data_RiffDetector_H
#define bitmunk_data_RiffDetector_H

#include "monarch/data/AbstractDataFormatInspector.h"
#include "monarch/data/riff/RiffChunkHeader.h"
#include "monarch/data/riff/RiffFormHeader.h"
#include "monarch/data/riff/RiffListHeader.h"

namespace bitmunk
{
namespace data
{

/**
 * A RiffDetector attempts to detect the RIFF format in a stream of data.
 *  
 * @author Dave Longley
 */
class RiffDetector : public monarch::data::AbstractDataFormatInspector
{
protected:
   
public:
   /**
    * Creates a new RiffDetector.
    */
   RiffDetector();
   
   /**
    * Destructs this RiffDetector.
    */
   virtual ~RiffDetector();

   /**
    * Finds the next arbitrary RIFF Chunk header. Will fail if no
    * RIFF Chunk Header is detected within a certain number of bytes read.
    * 
    * @param rch the RIFF Chunk header object to populate.
    * @param b the byte buffer to use when searching for a RIFF Chunk header.
    * @param length the length of the search window, abort the search after
    *               this many bytes have been scanned.
    * 
    * @return the offset of the RIFF Chunk header if one is found, or -1 if no
    *         header is found.
    */
   static int findRiffChunkHeader(monarch::data::riff::RiffChunkHeader& rch,
      const char* b, int length);
  
   /**
    * Finds the next arbitrary RIFF List header in a buffer or fails when a
    * certain number of bytes have been read.
    * 
    * @param rlh the RIFF List header object to populate.
    * @param b the byte buffer to use when searching for a RIFF List header.
    * @param length the length of the search window, abort the search after
    *               this many bytes have been scanned.
    *
    * @return the offset of the RIFF List header if one is found, or -1 if no
    *         header is found.
    */
   static int findRiffListHeader(monarch::data::riff::RiffListHeader& rlh,
      const char* b, int length);
   
   /**
    * Finds the next RIFF Form header in a buffer or fails when a certain 
    * number of bytes have been read.
    * 
    * @param rfh the RIFF Form header object to populate.
    * @param b the byte buffer to use when searching for a RIFF Form header.
    * @param length the length of the search window, abort the search after
    *               this many bytes have been scanned.
    * 
    * @return the offset of the RIFF Form header if one is found, or -1 if no
    *         header is found.
    */
   static int findRiffFormHeader(monarch::data::riff::RiffFormHeader& rfh,
      const char* b, int length);
};

} // end namespace data
} // end namespace bitmunk
#endif

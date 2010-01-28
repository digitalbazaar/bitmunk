/*
 * Copyright (c) 2005-2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_data_AviDetector_H
#define bitmunk_data_AviDetector_H

#include "bitmunk/data/RiffDetector.h"
#include "monarch/data/avi/AviHeader.h"
#include "monarch/data/avi/AviHeaderList.h"
#include "monarch/data/avi/AviStreamHeaderList.h"
#include "monarch/data/avi/OdmlHeader.h"
#include "bitmunk/common/TypeDefinitions.h"

namespace bitmunk
{
namespace data
{

/**
 * Manages a stream of Audio Video Interleave data.
 *
 * AVI Format is as follows:
 *
 * AVI Form Header ('RIFF' size 'AVI ' data)
 *    Header List ('LIST' size 'hdrl' data)
 *       AVI Header ('avih' size data)
 *       Video Stream Header List ('LIST' size 'strl' data)
 *          Video Stream Header ('strh' size data)
 *          Video Stream Format ('strf' size data)
 *          Video Stream Data ('strd' size data) - IGNORED, for DRIVERS
 *       Audio Stream Header List ('LIST' size 'strl' data)
 *          Audio Stream Header ('strh' size data)
 *          Audio Stream Format ('strf' size data)
 *          Audio Stream Data ('strd' size data) - IGNORED, for DRIVERS
 *    Info List ('LIST' size 'INFO' data)
 *       Index Entry ({'ISBJ','IART','ICMT',...} size data)
 *    Movie List ('LIST' size 'movi' data)
 *       Movie Entry ({'00db','00dc','01wb'} size data)
 *    Index Chunk ('idx1' size data)
 *       Index Entry ({'00db','00dc','01wb',...})
 *
 * ---------------------------------------
 * In the AVI Header (a DWORD is 4 bytes):
 * ---------------------------------------
 * DWORD microseconds per frame
 * DWORD maximum bytes per second
 * DWORD reserved
 * DWORD flags
 * DWORD total frames
 * DWORD initial frames
 * DWORD streams - 2 for video+audio streams
 * DWORD suggested buffer size - large enough to contain largest chunk
 * DWORD width - in pixels
 * DWORD height - in pixels
 * DWORD scale - time scale in samples per second = rate / scale
 * DWORD rate - see scale
 * DWORD start - starting time for the AVI file, usually zero
 * DWORD length - total time of the file using rate & scale units
 *
 * The flags for the AVI Header 'avih':
 *
 * AVIF_HASINDEX - indicates whether or not the AVI has an Index Chunk
 *
 * AVIF_MUSTUSEINDEX - indicates whether or not the index should determine
 * the order of the data
 *
 * AVIF_ISINTERLEAVED - indicates whether or not the file is interleaved
 *
 * AVIF_WASCAPTUREFILE - indicates whether or not the file is used for
 * capturing real-time video
 *
 * AVIF_COPYRIGHTED - indicates whether or not the file contains copyrighted
 * data
 *
 * ---------------------------------------------------------------
 * In a Stream Header 'strh' (FOURCC means a four-character code):
 * ---------------------------------------------------------------
 * FOURCC type - 'vids' for a video stream, 'auds' for an audio stream
 * FOURCC handler - the installable compressor or decompressor for the data
 * DWORD flags
 * DWORD reserved
 * DWORD initial frames
 * DWORD scale
 * DWORD rate
 * DWORD start
 * DWORD length
 * DWORD suggested buffer size
 * DWORD quality
 * DWORD sample size
 *
 * The flags for the Stream Header:
 *
 * AVISF_DISABLED - whether or not the data should only be rendered when
 * explicitly enabled by the user
 *
 * AVISF_VIDEO_PALCHANGES - whether or not palette changes are embedded
 * in the file (chunks tagged like '00pc')
 *
 * --------------------------
 * In a Stream Format 'strf':
 * --------------------------
 * A BITMAPINFO structure for a Video Stream Format chunk.
 * A WAVEFORMATEX or PCMWAVEFORMAT structure for an Audio Stream Format chunk.
 *
 * Note:
 * The Stream Header List 'strl' applies to the first stream in the 'movi'
 * LIST, the second applies to the second stream, etc.
 *
 * -------------------------
 * In the Movie List 'movi':
 * -------------------------
 * 00dc indicates a video frame in the first stream
 * 01wb indicates audio data in the second stream
 *
 * The Movie List contains the actual image and sound data for the file. Each
 * chunk contains a FOURCC that consists of the stream number and a
 * two-character code that identifies the type of data in the chunk. A
 * waveform chunk uses TWOCC 'wb'. If it is found in the second stream
 * in the file it would fall under '01wb'.
 *
 * An uncompressed DIB chunk in the first stream would use '00db'. A
 * compressed DIB chunk in the first stream would use '00dc'.
 *
 * A palette change in the first stream would use '00pc'. A palette chunk
 * has the following format:
 *
 * BYTE first entry - the first entry to change
 * BYTE number of entries - the number of entries to change
 * WORD flags
 * PALETTEENTRY the new palette entries - all of the new color entries
 *
 * Movie chunks may reside directly in the 'movi' LIST or may be found
 * in 'rec ' lists. Chunks in 'rec ' lists should be read all at once from
 * disk. This is used for files specifically interleaved to play from CD-ROM.
 *
 * --------------------------
 * In the Index Chunk 'idx1':
 * --------------------------
 * DWORD chunk id (like '00db', '00dc', '01wb')
 * DWORD flags
 * DWORD chunk offset - relative to the 'movi' list
 * DWORD chunk length - the length of the chunk data EXCLUDING the 8 byte
 * RIFF header
 *
 * The flags for an Index Entry:
 *
 * AVIIF_KEYFRAME - indicates that a chunk has key frames
 * AVIIF_NOTIME - indicates that a chunk does not affect the timing of the
 * video stream, i.e. used to ignore palette chunks
 * AVIIF_LIST - set when the entry is for a RIFF LIST, and the chunk ID field
 * identifies the kind of list
 *
 * @author Dave Longley
 */
class AviDetector : public RiffDetector
{
protected:
   /**
    * The RIFF form header for the AVI data.
    */
   //monarch::data::riff::RiffFormHeader* mFormHeader;
   bool mFormHeader;

   /**
    * Storage of stream details.
    */
   monarch::rt::DynamicObject mFormatDetails;

   /**
    * Flag if details need to be updated.
    */
   bool mFormatDetailsNeedUpdate;

   /**
    * The AVI header list.
    */
   monarch::data::avi::AviHeaderList* mHeaderList;

   /**
    * The current stream header list, if any.
    */
   monarch::data::avi::AviStreamHeaderList* mStreamHeaderList;

   /**
    * Whether or not an OpenDML list was found.
    */
   bool mOdmlList;

   /**
    * The ODML header, if any.
    */
   monarch::data::avi::OdmlHeader* mOdmlHeader;

   /**
    * The info list (an AVI component).
    */
   //monarch::data::riff::RiffListHeader* mInfoList;
   bool mInfoList;

   /**
    * The movie list (an AVI component).
    */
   //monarch::data::riff::RiffListHeader* mMovieList;
   bool mMovieList;

   /**
    * The index chunk (an AVI component).
    */
   //monarch::data::riff::RiffChunkHeader* mIndexChunk;
   bool mIndexChunk;

   /**
    * A detected media info, if any.
    */
   bitmunk::common::Media mMedia;

   /**
    * Sets default values for all of the AVI components, sets up ordered
    * vector and map.
    */
   void resetComponents();

   /**
    * This method is called whenever a RIFF Chunk header is detected.
    *
    * @param rch the header that was detected.
    * @param b the data the header was detected in.
    * @param length the number of valid bytes of data.
    *
    * @return the number of bytes inspected.
    */
   int foundRiffChunkHeader(monarch::data::riff::RiffChunkHeader& rch,
      const char* b, int length);

   /**
    * This method is called whenever a RIFF List header is detected.
    *
    * @param rlh the header that was detected.
    * @param b the data the header was detected in.
    * @param length the number of valid bytes of data.
    *
    * @return the number of inspected bytes.
    */
   int foundRiffListHeader(monarch::data::riff::RiffListHeader& rlh,
      const char* b, int length);

   /**
    * This method is called whenever a RIFF Form header is detected.
    *
    * @param rfh the header that was detected.
    * @param b the data the header was detected in.
    * @param length the number of valid bytes of data.
    *
    * @return the number of inspected bytes.
    */
   int foundRiffFormHeader(monarch::data::riff::RiffFormHeader& rfh,
      const char* b, int length);

   /**
    * Convenience method. Parses out a junk chunk that contains a Media.
    *
    * @param mi the Media to parse into.
    * @param rch the chunk header.
    * @param b the data the chunk is in.
    * @param length the number of valid bytes of data.
    *
    * @return the number of bytes inspected.
    */
   static int parseJunkChunk(
      bitmunk::common::Media& mi, monarch::data::riff::RiffChunkHeader& rch,
      const char* b, int length);

public:
   /**
    * Constructs an AVI stream detector.
    */
   AviDetector();

   /**
    * Destructs this AviDetector.
    */
   virtual ~AviDetector();

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
    * Gets the main AVI header, if one was detected.
    *
    * @return the main AVI header, if detected, otherwise NULL.
    */
   virtual monarch::data::avi::AviHeader* getAviHeader();
};

} // end namespace data
} // end namespace bitmunk
#endif

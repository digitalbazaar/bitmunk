/*
 * Copyright (c) 2007-2008 Digital Bazaar, Inc.  All rights reserved.
 */
#ifndef bitmunk_data_Id3v2TagFrameIO_H
#define bitmunk_data_Id3v2TagFrameIO_H

#include "monarch/data/id3v2/FrameSource.h"
#include "monarch/data/id3v2/FrameSink.h"
#include "monarch/io/ByteBuffer.h"
#include "monarch/io/File.h"
#include "monarch/io/InputStream.h"
#include "bitmunk/common/TypeDefinitions.h"

namespace bitmunk
{
namespace data
{

/**
 * A Bitmunk Id3v2TagFrameIO is used to handle the Bitmunk-specific frame data
 * for an ID3 tag.
 *
 * FIXME: We probably want individual classes/base classes for writing out
 * common tags or a single class for writing out TXXX tags, etc.
 *
 * The following frames have been declared in the Id3v2.3.0 draft:
 *
 * 4.20    AENC    [#sec4.20 Audio encryption]
 * 4.15    APIC    [#sec4.15 Attached picture]
 * 4.11    COMM    [#sec4.11 Comments]
 * 4.25    COMR    [#sec4.25 Commercial frame]
 * 4.26    ENCR    [#sec4.26 Encryption method registration]
 * 4.13    EQUA    [#sec4.13 Equalization]
 * 4.6     ETCO    [#sec4.6 Event timing codes]
 * 4.16    GEOB    [#sec4.16 General encapsulated object]
 * 4.27    GRID    [#sec4.27 Group identification registration]
 * 4.4     IPLS    [#sec4.4 Involved people list]
 * 4.21    LINK    [#sec4.21 Linked information]
 * 4.5     MCDI    [#sec4.5 Music CD identifier]
 * 4.7     MLLT    [#sec4.7 MPEG location lookup table]
 * 4.24    OWNE    [#sec4.24 Ownership frame]
 * 4.28    PRIV    [#sec4.28 Private frame]
 * 4.17    PCNT    [#sec4.17 Play counter]
 * 4.18    POPM    [#sec4.18 Popularimeter]
 * 4.22    POSS    [#sec4.22 Position synchronisation frame]
 * 4.19    RBUF    [#sec4.19 Recommended buffer size]
 * 4.12    RVAD    [#sec4.12 Relative volume adjustment]
 * 4.14    RVRB    [#sec4.14 Reverb]
 * 4.10    SYLT    [#sec4.10 Synchronized lyric/text]
 * 4.8     SYTC    [#sec4.8 Synchronized tempo codes]
 * 4.2.1   TALB    [#TALB Album/Movie/Show title]
 * 4.2.1   TBPM    [#TBPM BPM (beats per minute)]
 * 4.2.1   TCOM    [#TCOM Composer]
 * 4.2.1   TCON    [#TCON Content type]
 * 4.2.1   TCOP    [#TCOP Copyright message]
 * 4.2.1   TDAT    [#TDAT Date]
 * 4.2.1   TDLY    [#TDLY Playlist delay]
 * 4.2.1   TENC    [#TENC Encoded by]
 * 4.2.1   TEXT    [#TEXT Lyricist/Text writer]
 * 4.2.1   TFLT    [#TFLT File type]
 * 4.2.1   TIME    [#TIME Time]
 * 4.2.1   TIT1    [#TIT1 Content group description]
 * 4.2.1   TIT2    [#TIT2 Title/songname/content description]
 * 4.2.1   TIT3    [#TIT3 Subtitle/Description refinement]
 * 4.2.1   TKEY    [#TKEY Initial key]
 * 4.2.1   TLAN    [#TLAN Language(s)]
 * 4.2.1   TLEN    [#TLEN Length]
 * 4.2.1   TMED    [#TMED Media type]
 * 4.2.1   TOAL    [#TOAL Original album/movie/show title]
 * 4.2.1   TOFN    [#TOFN Original filename]
 * 4.2.1   TOLY    [#TOLY Original lyricist(s)/text writer(s)]
 * 4.2.1   TOPE    [#TOPE Original artist(s)/performer(s)]
 * 4.2.1   TORY    [#TORY Original release year]
 * 4.2.1   TOWN    [#TOWN File owner/licensee]
 * 4.2.1   TPE1    [#TPE1 Lead performer(s)/Soloist(s)]
 * 4.2.1   TPE2    [#TPE2 Band/orchestra/accompaniment]
 * 4.2.1   TPE3    [#TPE3 Conductor/performer refinement]
 * 4.2.1   TPE4    [#TPE4 Interpreted, remixed, or otherwise modified by]
 * 4.2.1   TPOS    [#TPOS Part of a set]
 * 4.2.1   TPUB    [#TPUB Publisher]
 * 4.2.1   TRCK    [#TRCK Track number/Position in set]
 * 4.2.1   TRDA    [#TRDA Recording dates]
 * 4.2.1   TRSN    [#TRSN Internet radio station name]
 * 4.2.1   TRSO    [#TRSO Internet radio station owner]
 * 4.2.1   TSIZ    [#TSIZ Size]
 * 4.2.1   TSRC    [#TSRC ISRC (international standard recording code)]
 * 4.2.1   TSSE    [#TSEE Software/Hardware and settings used for encoding]
 * 4.2.1   TYER    [#TYER Year]
 * 4.2.2   TXXX    [#TXXX User defined text information frame]
 * 4.1     UFID    [#sec4.1 Unique file identifier]
 * 4.23    USER    [#sec4.23 Terms of use]
 * 4.9     USLT    [#sec4.9 Unsychronized lyric/text transcription]
 * 4.3.1   WCOM    [#WCOM Commercial information]
 * 4.3.1   WCOP    [#WCOP Copyright/Legal information]
 * 4.3.1   WOAF    [#WOAF Official audio file webpage]
 * 4.3.1   WOAR    [#WOAR Official artist/performer webpage]
 * 4.3.1   WOAS    [#WOAS Official audio source webpage]
 * 4.3.1   WORS    [#WORS Official internet radio station homepage]
 * 4.3.1   WPAY    [#WPAY Payment]
 * 4.3.1   WPUB    [#WPUB Publishers official webpage]
 * 4.3.2   WXXX    [#WXXX User defined URL link frame]
 *
 * @author Dave Longley
 */
class Id3v2TagFrameIO :
public monarch::data::id3v2::FrameSource,
public monarch::data::id3v2::FrameSink
{
public:
   /**
    * The modes for tag frame IO.
    */
   enum Mode
   {
      Source, Sink
   };

protected:
   /**
    * The mode (Source/Sink).
    */
   Mode mMode;

   /**
    * The Contract to read/write.
    */
   bitmunk::common::Contract mContract;

   /**
    * The Media to write.
    */
   bitmunk::common::Media mMedia;

   /**
    * The track number to write.
    */
   int mTrackNumber;

   /**
    * A buffer for the current frame.
    */
   monarch::io::ByteBuffer mFrameBuffer;

   /**
    * The current frame header.
    */
   monarch::data::id3v2::FrameHeader* mCurrentHeader;

   /**
    * Stores an input stream for reading image data.
    */
   monarch::io::InputStream* mImageInput;

   /**
    * Stores a temporary image file.
    */
   monarch::io::File mImageFile;

   /**
    * The image mime type.
    */
   char* mImageMimeType;

   /**
    * The image description.
    */
   std::string mImageDescription;

public:
   /**
    * Creates a new Id3v2TagFrameIO.
    *
    * @param mode the source/sink mode.
    */
   Id3v2TagFrameIO(Mode mode);

   /**
    * Destructs this Id3v2TagFrameIO.
    */
   virtual ~Id3v2TagFrameIO();

   /**
    * Sets the Contract to read/write.
    *
    * @param c the Contract to read/write.
    */
   virtual void setContract(bitmunk::common::Contract& c);

   /**
    * Gets the Contract to read/write.
    *
    * @return the Contract to read/write.
    */
   virtual bitmunk::common::Contract& getContract();

   /**
    * Sets the Media to write.
    *
    * @param m the Media to write.
    */
   virtual void setMedia(bitmunk::common::Media& m);

   /**
    * Gets the Media to write.
    *
    * @return the Media to write.
    */
   virtual bitmunk::common::Media& getMedia();

   /**
    * Sets the track number to write.
    *
    * @param track the track number to write.
    */
   virtual void setTrackNumber(int track);

   /**
    * Stores a temporary file for an image.
    *
    * @param file the temporary file for the image.
    * @param mimeType the mime type for the image.
    * @param description a description for the image in UTF-8.
    *
    * @return true if successful, false on error.
    */
   virtual bool setImageFile(
      monarch::io::File& file, const char* mimeType, const char* description);

   /**
    * Updates the passed frame header's set frame size according to the
    * data this source can provide for the frame.
    *
    * Note: The data to be read from this source must be set prior to
    * calling this method.
    *
    * @param header the id3v2 tag frame header to update.
    */
   virtual void updateFrameSize(monarch::data::id3v2::FrameHeader* header);

   /**
    * Prepares this frame source/sink for handling the given header.
    *
    * @param header the id3v2 tag frame header to start handling.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool startFrame(monarch::data::id3v2::FrameHeader* header);

   /**
    * Gets the frame bytes for the current frame header, including the frame
    * header in binary format. This method can be called repeated to retrieve
    * the entire frame.
    *
    * @param dst the ByteBuffer to populate with data.
    * @param resize true to permit resizing the ByteBuffer, false not to.
    *
    * @return the number of bytes read, 0 if the end of the data has been
    *         reached, or -1 if an exception occurred.
    */
   virtual int getFrame(monarch::io::ByteBuffer* dst, bool resize);

   /**
    * Writes frame data to this sink for the current frame header. The source
    * should begin at the start of the frame data, after the frame header.
    *
    * @param src the ByteBuffer with frame data bytes to read.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool putFrameData(monarch::io::ByteBuffer* src);

protected:
   /**
    * Gets the text data for the given header.
    *
    * @param header the frame header to get the text data for.
    *
    * @return the text data.
    */
   virtual std::string getTextData(monarch::data::id3v2::FrameHeader* header);

   /**
    * Buffers the text data for reading a text frame from this source.
    *
    * @param header the header to buffer the data for.
    */
   virtual void bufferTextData(monarch::data::id3v2::FrameHeader* header);

   /**
    * Writes the compressed contract data to the given byte buffer and
    * determines the uncompressed and compressed size for the data.
    *
    * @param b the ByteBuffer to write to, NULL to write nowhere.
    * @param uncompressed an integer to set to the number of uncompressed
    *                     bytes, NULL if not interested.
    * @param compressed an integer to set to the number of compressed
    *                   bytes, NULL if not interested.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool getContractData(
      monarch::io::ByteBuffer* b, uint32_t* uncompressed, uint32_t* compressed);

   /**
    * Buffers the contract data for reading a compressed contract frame
    * from this source.
    *
    * @return true if successful, false if not.
    */
   virtual bool bufferContractData();

   /**
    * Buffers the contract data for reading a compressed contract frame
    * from this source.
    *
    * @return true if successful, false if not.
    */
   virtual bool bufferImageData();
};

} // end namespace data
} // end namespace bitmunk
#endif

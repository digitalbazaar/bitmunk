/*
 * Copyright (c) 2007-2009 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_CONSTANT_MACROS
#define __STDC_FORMAT_MACROS

#include "bitmunk/data/Id3v2TagFrameIO.h"

#include "monarch/compress/deflate/Deflater.h"
#include "monarch/data/CharacterSetMutator.h"
#include "monarch/data/Data.h"
#include "monarch/data/DynamicObjectInputStream.h"
#include "monarch/data/DynamicObjectOutputStream.h"
#include "monarch/data/json/JsonReader.h"
#include "monarch/data/json/JsonWriter.h"
#include "monarch/io/ByteArrayInputStream.h"
#include "monarch/io/ByteArrayOutputStream.h"
#include "monarch/io/NullOutputStream.h"
#include "monarch/io/FileInputStream.h"
#include "monarch/io/MutatorInputStream.h"
#include "monarch/io/MutatorOutputStream.h"
#include "monarch/util/Data.h"

#include <cstdlib>

using namespace std;
using namespace monarch::compress::deflate;
using namespace monarch::data;
using namespace monarch::data::id3v2;
using namespace monarch::data::json;
using namespace monarch::io;
using namespace monarch::rt;
using namespace bitmunk::common;
using namespace bitmunk::data;

Id3v2TagFrameIO::Id3v2TagFrameIO(Id3v2TagFrameIO::Mode mode) :
   mMode(mode),
   mContract(NULL),
   mMedia(NULL),
   mTrackNumber(0),
   mCurrentHeader(NULL),
   mImageInput(NULL),
   mImageFile((FileImpl*)NULL),
   mImageMimeType(NULL)
{
}

Id3v2TagFrameIO::~Id3v2TagFrameIO()
{
   if(mImageInput != NULL)
   {
      mImageInput->close();
      delete mImageInput;
   }

   if(!mImageFile.isNull())
   {
      mImageFile->remove();
   }

   if(mImageMimeType != NULL)
   {
      free(mImageMimeType);
   }
}

inline void Id3v2TagFrameIO::setContract(Contract& c)
{
   mContract = c;
}

inline Contract& Id3v2TagFrameIO::getContract()
{
   return mContract;
}

inline void Id3v2TagFrameIO::setMedia(Media& m)
{
   mMedia = m;
}

inline Media& Id3v2TagFrameIO::getMedia()
{
   return mMedia;
}

inline void Id3v2TagFrameIO::setTrackNumber(int track)
{
   mTrackNumber = track;
}

bool Id3v2TagFrameIO::setImageFile(
   File& file, const char* mimeType, const char* description)
{
   bool rval = true;

   // set image file
   mImageFile = file;

   // set image mime type
   if(mImageMimeType != NULL)
   {
      free(mImageMimeType);
   }
   mImageMimeType = strdup(mimeType);

   // get image description in ISO-8859-1
   if(!CharacterSetMutator::convert(
      description, "UTF-8", mImageDescription, "ISO-8859-1"))
   {
      // use blank image description
      mImageDescription = "";
      rval = false;
   }

   return rval;
}

void Id3v2TagFrameIO::updateFrameSize(FrameHeader* header)
{
   int size = header->getFrameSize();

   if(header->getId()[0] == 'T')
   {
      // get text data
      string text = getTextData(header);

      if(strcmp(header->getId(), "TXXX") == 0)
      {
         // special 'TXXX' format:
         //
         // Text Encoding: 1 byte ("ISO-8859-1" = 0x00)
         // Description: X bytes + 1 byte null termination
         // Text: X bytes (always 0 in this implementation)
         size = 1 + text.length() + 1 + 0;
      }
      else
      {
         // Text Encoding: 1 byte ("ISO-8859-1" = 0x00)
         // Text: X bytes
         size = 1 + text.length();
      }
   }
   else if(strcmp(header->getId(), "XBMC") == 0)
   {
      // get contract data
      uint32_t compressed;
      getContractData(NULL, NULL, &compressed);

      // XBMC is a compressed tag, so it has 4 bytes of uncompressed
      // size following the frame header, then the zlib-compressed
      // data follows that
      size = 4 + (int)compressed;
   }
   else if(strcmp(header->getId(), "APIC") == 0)
   {
      // Text Encoding: 1 byte (ISO-8859-1 = 0x00)
      // Mime Type: X bytes + 1 byte null termination
      // Picture Type: 1 byte (0x03 cover front)
      // Description: Text description + 1 byte null termination
      // Picture Data: X bytes
      size =
         1 +                              // 0x00 ISO-8859-1
         strlen(mImageMimeType) + 1 +     // X bytes + 1 byte null termination
         1 +                              // 1 byte 0x03 (cover front)
         mImageDescription.length() + 1 + // X bytes + 1 byte null termination
         (int)mImageFile->getLength();    // X bytes for picture
   }

   // update frame size
   header->setFrameSize(size);
}

bool Id3v2TagFrameIO::startFrame(FrameHeader* header)
{
   bool rval = true;

   // set current header, clear frame buffer
   mCurrentHeader = header;
   mFrameBuffer.clear();

   switch(mMode)
   {
      case Source:
      {
         // allocate space for frame header and data
         mFrameBuffer.allocateSpace(
            header->getFrameSize() + FrameHeader::HEADER_SIZE, true);

         // write frame header to beginning of frame buffer
         header->convertToBytes(mFrameBuffer.data());
         mFrameBuffer.extend(FrameHeader::HEADER_SIZE);

         if(header->getId()[0] == 'T')
         {
            bufferTextData(header);
         }
         else if(strcmp(header->getId(), "XBMC") == 0)
         {
            rval = bufferContractData();
         }
         else if(strcmp(header->getId(), "APIC") == 0)
         {
            rval = bufferImageData();
         }
         break;
      }
      case Sink:
      {
         // allocate space for frame data
         mFrameBuffer.allocateSpace(header->getFrameSize(), true);
         break;
      }
   }

   return rval;
}

int Id3v2TagFrameIO::getFrame(ByteBuffer* dst, bool resize)
{
   int rval = 0;

   // read from frame buffer if not empty
   if(!mFrameBuffer.isEmpty())
   {
      rval = mFrameBuffer.get(dst, mFrameBuffer.length(), resize);
   }
   // read from image input stream if available
   else if(mImageInput != NULL)
   {
      int numBytes = dst->put(mImageInput, resize);
      if(numBytes > 0)
      {
         rval += numBytes;
      }
      else if(numBytes == 0)
      {
         // image data fully read, so clean up
         delete mImageInput;
         mImageInput = NULL;
      }
      else
      {
         // error while reading image data
         rval = -1;
      }
   }

   return rval;
}

bool Id3v2TagFrameIO::putFrameData(ByteBuffer* src)
{
   bool rval = true;

   // read until frame buffer is full
   if(!mFrameBuffer.isFull())
   {
      src->get(&mFrameBuffer, src->length(), false);
   }

   // process frame buffer if it is full
   if(mFrameBuffer.isFull())
   {
      // only interested in XBMC frame
      if(strcmp(mCurrentHeader->getId(), "XBMC") == 0)
      {
         // skip 4 bytes of uncompressed size
         // (part of id3v2 compressed frame standard)
         mFrameBuffer.clear(4);
         ByteArrayInputStream bais(&mFrameBuffer, false);

         // decompress zlib-compressed data
         Deflater def;
         def.startInflating(false);
         MutatorInputStream mis(&bais, false, &def, false);

         // create/clear contract as necessary
         if(mContract.isNull())
         {
            mContract = Contract();
         }
         else
         {
            mContract->clear();
         }

         // decompress JSON-formatted contract
         JsonReader reader;
         reader.start(mContract);
         rval = (reader.read(&mis) && reader.finish());
         mis.close();

         // set media
         mMedia.setNull();
         if(mContract->hasMember("media"))
         {
            mMedia = mContract["media"];
         }
      }
   }

   return rval;
}

string Id3v2TagFrameIO::getTextData(FrameHeader* header)
{
   string rval;

   // Note: TXXX tags have a text encoding byte
   // (set to 0 for ISO-8859-1) that is followed by the actual text

   // (*must* use ISO-8859-1 encoding) for text bytes, UTF-8 is not
   // compatible with windows media player (ugh)

   // get text for specific ID
   if(strcmp(header->getId(), "TPE1") == 0)
   {
      // artist data:
      // text is in UTF-8, so convert it to ISO-8859-1
      if(!CharacterSetMutator::convert(
         mMedia["contributors"]["Performer"][0]["name"]->getString(),
         "UTF-8", rval, "ISO-8859-1"))
      {
         // fall back on utf-8, sorry windows media player
         rval = mMedia["contributors"]["Performer"][0]["name"]->getString();
      }
   }
   else if(strcmp(header->getId(), "TIT2") == 0)
   {
      // title data:
      // text is in UTF-8, so convert it to ISO-8859-1
      if(!CharacterSetMutator::convert(
         mMedia["title"]->getString(), "UTF-8", rval, "ISO-8859-1"))
      {
         // fall back on utf-8, sorry windows media player
         rval = mMedia["title"]->getString();
      }
   }
   else if(strcmp(header->getId(), "TRCK") == 0)
   {
      // track data
      char temp[22];
      sprintf(temp, "%d", mTrackNumber);
      rval = temp;
   }
   else if(strcmp(header->getId(), "TLEN") == 0)
   {
      // song length is in milliseconds (media "length" is in seconds)
      uint64_t songLength = mMedia["length"]->getUInt64() * 1000;
      char temp[22];
      snprintf(temp, 22, "%" PRIu64, songLength);
      rval = temp;
   }
   else if(strcmp(header->getId(), "TXXX") == 0)
   {
      // description data
      rval = header->getDescription();
   }

   return rval;
}

void Id3v2TagFrameIO::bufferTextData(FrameHeader* header)
{
   // get text data
   string text = getTextData(header);

   if(strcmp(header->getId(), "TXXX") == 0)
   {
      // special 'TXXX' format:
      //
      // Text Encoding: 1 byte ("ISO-8859-1" = 0x00)
      // Description: X bytes + 1 byte null termination
      // Text: X bytes (always 0 in this implementation)

      // buffer frame description
      mFrameBuffer.putByte(0x00, 1, false);
      mFrameBuffer.put(text.c_str(), text.length() + 1, false);
   }
   else
   {
      // Text Encoding: 1 byte ("ISO-8859-1" = 0x00)
      // Text: X bytes

      // buffer text
      mFrameBuffer.putByte(0x00, 1, false);
      mFrameBuffer.put(text.c_str(), text.length(), false);
   }
}

bool Id3v2TagFrameIO::getContractData(
   ByteBuffer* b, uint32_t* uncompressed, uint32_t* compressed)
{
   bool rval;

   // create output stream
   OutputStream* os = (b == NULL) ?
      static_cast<OutputStream*>(new NullOutputStream()) :
      static_cast<OutputStream*>(new ByteArrayOutputStream(b, false));

   // zlib-compress data
   Deflater def;
   def.startDeflating(-1, false);
   MutatorOutputStream mos(os, true, &def, false);

   // produce JSON-formatted contract data
   JsonWriter writer;
   writer.setCompact(true);
   rval = writer.write(mContract, &mos);
   mos.close();

   // store uncompressed bytes
   if(uncompressed != NULL)
   {
      *uncompressed = def.getTotalInputBytes();
   }

   // store compressed bytes
   if(compressed != NULL)
   {
      *compressed = def.getTotalOutputBytes();
   }

   return rval;
}

bool Id3v2TagFrameIO::bufferContractData()
{
   bool rval;

   // make room for 4 bytes of uncompressed size in frame buffer
   // (part of id3v2 compressed frame standard)
   uint32_t uncompressed = 0;
   char* end = mFrameBuffer.end();
   mFrameBuffer.put((char*)&uncompressed, 4, false);

   // write contract data to frame buffer, store uncompressed size
   rval = getContractData(&mFrameBuffer, &uncompressed, NULL);

   // copy uncompressed size to data buffer
   uncompressed = MO_UINT32_TO_BE(uncompressed);
   memcpy(end, &uncompressed, 4);
   rval = true;

   return rval;
}

bool Id3v2TagFrameIO::bufferImageData()
{
   bool rval;

   if(mImageFile.isNull())
   {
      ExceptionRef e = new Exception(
         "Could not buffer id3v2 frame image data. No image file set.",
         "bitmunk.data.NoId3v2ImageFile");
      Exception::set(e);
      rval = false;
   }
   else
   {
      // Text Encoding: 1 byte (ISO-8859-1 = 0x00)
      // Mime Type: X bytes + 1 byte null termination
      // Picture Type: 1 byte (0x03 cover front)
      // Description: Text description + 1 byte null termination
      // Picture Data: X bytes

      // image mime type
      mFrameBuffer.putByte(0x00, 1, false);
      mFrameBuffer.put(mImageMimeType, strlen(mImageMimeType) + 1, false);

      // picture type
      mFrameBuffer.putByte(0x03, 1, false);

      // image description
      mFrameBuffer.put(
         mImageDescription.c_str(), mImageDescription.length() + 1, false);

      // setup picture data stream
      mImageInput = new FileInputStream(mImageFile);
      rval = true;
   }

   return rval;
}

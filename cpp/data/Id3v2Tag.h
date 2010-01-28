/*
 * Copyright (c) 2007-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_data_Id3v2Tag_H
#define bitmunk_data_Id3v2Tag_H

#include "monarch/data/id3v2/Tag.h"
#include "bitmunk/common/TypeDefinitions.h"
#include "bitmunk/data/Id3v2TagFrameIO.h"

namespace bitmunk
{
namespace data
{

/**
 * A Bitmunk Id3v2Tag is an ID3v2 tag to use when reading/writing Bitmunk files.
 * 
 * @author Dave Longley
 */
class Id3v2Tag : public monarch::data::id3v2::Tag
{
protected:
   /**
    * The frame IO for this tag.
    */
   Id3v2TagFrameIO mFrameIO;
   
   /**
    * The frame source for this tag.
    */
   Id3v2TagFrameIO* mFrameSource;
   
   /**
    * The frame sink for this tag.
    */
   Id3v2TagFrameIO* mFrameSink;
   
   /**
    * Adds the appropriate frames to be embedded.
    * 
    * @param c the Contract to use, if any.
    * @param fi the FileInfo to use, if any.
    * @param m the Media to use, if any.
    */
   virtual void addFrames(
      bitmunk::common::Contract* c,
      bitmunk::common::FileInfo* fi,
      bitmunk::common::Media* m);
   
public:
   /**
    * Creates a new Id3v2Tag.
    */
   Id3v2Tag();
   
   /**
    * Creates a new Id3v2Tag with the given Contract for embedding into
    * the tag.
    * 
    * @param c the Contract to use.
    * @param fi the FileInfo for the file.
    */
   Id3v2Tag(bitmunk::common::Contract& c, bitmunk::common::FileInfo& fi);
   
   /**
    * Creates a new Id3v2Tag that will embed only Media information.
    * 
    * @param m the Media to use.
    */
   Id3v2Tag(bitmunk::common::Media& m);
   
   /**
    * Destructs this Id3v2Tag.
    */
   virtual ~Id3v2Tag();
   
   /**
    * Gets the FrameSource for this tag.
    * 
    * @return the FrameSource for this tag.
    */
   virtual Id3v2TagFrameIO* getFrameSource();
   
   /**
    * Gets the FrameSink for this tag.
    * 
    * @return the FrameSink for this tag.
    */
   virtual Id3v2TagFrameIO* getFrameSink();
};

// typedef for reference counted Id3v2Tag
typedef monarch::rt::Collectable<bitmunk::data::Id3v2Tag> Id3v2TagRef;

} // end namespace data
} // end namespace bitmunk
#endif

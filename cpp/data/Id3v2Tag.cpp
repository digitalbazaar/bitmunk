/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/data/Id3v2Tag.h"

#include "bitmunk/data/Id3v2TagFrameIO.h"
#include "monarch/io/FileOutputStream.h"
#include "monarch/http/HttpClient.h"

using namespace monarch::data;
using namespace monarch::data::id3v2;
using namespace monarch::io;
using namespace monarch::http;
using namespace monarch::net;
using namespace monarch::rt;
using namespace bitmunk::common;
using namespace bitmunk::data;

Id3v2Tag::Id3v2Tag() :
   mFrameIO(Id3v2TagFrameIO::Sink),
   mFrameSource(NULL),
   mFrameSink(&mFrameIO)
{
}

Id3v2Tag::Id3v2Tag(Contract& c, FileInfo& fi) :
   mFrameIO(Id3v2TagFrameIO::Source),
   mFrameSource(&mFrameIO),
   mFrameSink(NULL)
{
   // add frames
   addFrames(&c, &fi, NULL);
}

Id3v2Tag::Id3v2Tag(Media& m) :
   mFrameIO(Id3v2TagFrameIO::Source),
   mFrameSource(&mFrameIO),
   mFrameSink(NULL)
{
   // add frames
   addFrames(NULL, NULL, &m);
}

void Id3v2Tag::addFrames(Contract* c, FileInfo* fi, Media* m)
{
   int track = 0;
   Media media(NULL);

   // handle contract information
   if(c != NULL)
   {
      // clone contract, clear out any sections
      Contract contract = c->clone();
      contract["sections"]->clear();

      // get media based on media ID in file info
      MediaId mediaId = BM_MEDIA_ID((*fi)["mediaId"]);
      media = contract["media"];
      if(strcmp(media["type"]->getString(), "collection") == 0)
      {
         // find appropriate media in media contents
         bool found = false;
         DynamicObjectIterator gi = media["contents"].getIterator();
         while(!found && gi->hasNext())
         {
            DynamicObject& group = gi->next();
            MediaIterator mi = group.getIterator();
            while(!found && mi->hasNext())
            {
               Media& next = mi->next();
               ++track;
               if(BM_MEDIA_ID_EQUALS(BM_MEDIA_ID(next["id"]), mediaId))
               {
                  media = next;
                  found = true;
               }
            }
         }
      }

      // set contract
      mFrameIO.setContract(contract);
   }
   else if(m != NULL)
   {
      media = *m;
   }

   // set media
   mFrameIO.setMedia(media);

   // create frame headers:

   // performer header
   FrameHeader* tpe1 = new FrameHeader();
   tpe1->setId("TPE1");
   mFrameIO.updateFrameSize(tpe1);
   addFrameHeader(tpe1, true);

   // title header
   FrameHeader* tit2 = new FrameHeader();
   tit2->setId("TIT2");
   mFrameIO.updateFrameSize(tit2);
   addFrameHeader(tit2, true);

   // track header, if applicable
   if(track > 0)
   {
      FrameHeader* trck = new FrameHeader();
      trck->setId("TRCK");
      mFrameIO.setTrackNumber(track);
      mFrameIO.updateFrameSize(trck);
      addFrameHeader(trck, true);
   }

   // song length header
   FrameHeader* tlen = new FrameHeader();
   tlen->setId("TLEN");
   mFrameIO.updateFrameSize(tlen);
   addFrameHeader(tlen, true);

   // FIXME: temporarily disabled until cover image url support added
#if 0
   // image header, if applicable
   if(media->hasMember("coverImageUrl"))
   {
      // obtain image
      Url url(media["coverImageUrl"]->getString());
      HttpClient client;
      if(client.connect(&url))
      {
         HttpResponse* response = client.get(&url);
         if(response != NULL && response->getHeader()->getStatusCode() == 200)
         {
            // create temporary file to store image
            File imageFile = File::createTempFile("tmpimage.");

            // receive content
            HttpTrailer trailer;
            FileOutputStream fos(imageFile);
            if(client.receiveContent(&fos, &trailer))
            {
               // set image file and description
               FrameHeader* apic = new FrameHeader();
               apic->setId("APIC");
               mFrameIO.setImageFile(imageFile, "image/jpeg", "Cover Art");
               mFrameIO.updateFrameSize(apic);
               addFrameHeader(apic, true);
            }
            fos.close();
         }
      }
   }
#endif

   // embedded contract/receipt header, if applicable
   if(c != NULL)
   {
      FrameHeader* xbmc = new FrameHeader();
      xbmc->setId("XBMC");

      // FIXME: windows media player refuses to play mp3's with id3
      // tags that have compressed flag turned on ... so we shut it
      // off to fool it!
      //
      // Thanks, Microsoft. You also burned my roast.
      //xbmc->setCompressed(true);

      mFrameIO.updateFrameSize(xbmc);
      addFrameHeader(xbmc, true);
   }
}

Id3v2Tag::~Id3v2Tag()
{
}

Id3v2TagFrameIO* Id3v2Tag::getFrameSource()
{
   return mFrameSource;
}

Id3v2TagFrameIO* Id3v2Tag::getFrameSink()
{
   return mFrameSink;
}

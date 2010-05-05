/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk/sell/SampleService.h"

#include "bitmunk/common/Signer.h"
#include "bitmunk/data/Id3v2Tag.h"
#include "bitmunk/data/Id3v2TagWriter.h"
#include "bitmunk/data/MpegAudioTimeParser.h"
#include "bitmunk/node/BtpActionDelegate.h"
#include "bitmunk/node/RestResourceHandler.h"
#include "bitmunk/common/CatalogInterface.h"
#include "monarch/io/ByteArrayInputStream.h"
#include "monarch/io/FileInputStream.h"
#include "monarch/io/MutatorInputStream.h"
#include "monarch/util/StringTokenizer.h"

using namespace std;
using namespace monarch::config;
using namespace monarch::data::id3v2;
using namespace monarch::http;
using namespace monarch::io;
using namespace monarch::net;
using namespace monarch::rt;
using namespace monarch::util;
using namespace bitmunk::common;
using namespace bitmunk::data;
using namespace bitmunk::protocol;
using namespace bitmunk::node;
using namespace bitmunk::sell;

typedef BtpActionDelegate<SampleService> Handler;

SampleService::SampleService(Node* node, const char* path) :
   NodeService(node, path)
{
}

SampleService::~SampleService()
{
}

bool SampleService::initialize()
{
   // FIXME: get number from config instead of just using "30"
   // configure sample semaphore
   mSampleSemaphore["max"] = 30;
   mSampleSemaphore["current"] = 0;

   // FIXME: get number from config instead of just using "100"
   // configure sample range cache
   mSampleRangeCache["capacity"] = 100;
   mSampleRangeCache["cache"]->setType(Map);

   // file
   {
      RestResourceHandlerRef file = new RestResourceHandler();
      addResource("/file", file);

      // GET .../file/<mediaId>/<fileId>
      {
         ResourceHandler h = new Handler(
            mNode, this, &SampleService::getSampleFile,
            BtpAction::AuthOptional);
         file->addHandler(h, BtpMessage::Get, 2);
      }
   }

   // media
   {
      RestResourceHandlerRef media = new RestResourceHandler();
      addResource("/media", media);

      // GET .../media/<mediaId>?contentType=audio/mpeg
      {
         ResourceHandler h = new Handler(
            mNode, this, &SampleService::getSampleMedia,
            BtpAction::AuthOptional);
         media->addHandler(h, BtpMessage::Get, 1);
      }
   }

   // playlist
   {
      RestResourceHandlerRef playlist = new RestResourceHandler();
      addResource("/playlist", playlist);

      // GET .../playlist/<mediaId>?contentType=audio/mpeg
      {
         ResourceHandler h = new Handler(
            mNode, this, &SampleService::getSamplePlayList,
            BtpAction::AuthOptional);
         playlist->addHandler(h, BtpMessage::Get, 1);
      }
   }

   return true;
}

void SampleService::cleanup()
{
   // remove resources
   removeResource("/file");
   removeResource("/media");
   removeResource("/playlist");
}

bool SampleService::getSampleFileByIds(
   BtpAction* action, DynamicObject& in, DynamicObject& out,
   MediaId mediaId, FileId fileId)
{
   bool rval;

   // do not send dyno as response
   out.setNull();

   // determine if too busy or not
   bool permit = false;
   mSampleSemaphoreLock.lock();
   {
      int current = mSampleSemaphore["current"]->getInt32();
      if(current < mSampleSemaphore["max"]->getInt32())
      {
         mSampleSemaphore["current"] = current + 1;
         permit = true;
      }
   }
   mSampleSemaphoreLock.unlock();

   // ensure permit was granted to stream sample
   if(!permit)
   {
      // too many samples streaming
      action->getResponse()->getHeader()->setStatus(503, "Service Unavailable");
      action->sendResult();
      rval = true;
   }
   else
   {
      // get catalog interface
      CatalogInterface* ci = dynamic_cast<CatalogInterface*>(
         mNode->getModuleApiByType("bitmunk.catalog"));

      // get targeted node user
      DynamicObject vars;
      action->getResourceQuery(vars);
      if(!vars->hasMember("nodeuser"))
      {
         BM_ID_SET(vars["nodeuser"], mNode->getDefaultUserId());
      }

      // get resource parameters
      DynamicObject params;
      action->getResourceParams(params);

      FileInfo fi;
      BM_ID_SET(fi["mediaId"], mediaId);
      BM_ID_SET(fi["id"], fileId);
      if((rval = ci->populateFileInfo(BM_USER_ID(vars["nodeuser"]), fi)))
      {
         // send the sample
         rval = sendFile(action, fi);
      }

      // release permit
      mSampleSemaphoreLock.lock();
      {
         int current = mSampleSemaphore["current"]->getInt32();
         mSampleSemaphore["current"] = current - 1;
      }
      mSampleSemaphoreLock.unlock();

      if(!rval)
      {
         // sample not found
         action->getResponse()->getHeader()->setStatus(404, "Not Found");
         action->sendResult();
         rval = true;
      }
   }

   return rval;
}

bool SampleService::getFileInfos(
   BtpAction* action, DynamicObject& in, DynamicObject& out,
   DynamicObject& fileInfos)
{
   bool rval;

   // do not send dyno as response
   out.setNull();

   // get catalog interface
   CatalogInterface* ci = dynamic_cast<CatalogInterface*>(
      mNode->getModuleApiByType("bitmunk.catalog"));

   // get targeted node user
   DynamicObject vars;
   action->getResourceQuery(vars);
   if(!vars->hasMember("nodeuser"))
   {
      BM_ID_SET(vars["nodeuser"], mNode->getDefaultUserId());
   }

   // get resource parameters
   DynamicObject params;
   action->getResourceParams(params);

   // populate ware bundle
   Ware ware;
   BM_ID_SET(ware["mediaId"], BM_MEDIA_ID(params[0]));
   MediaPreferenceList mpl;
   mpl[0]["preferences"][0]["contentType"] = vars["contentType"];
   mpl[0]["preferences"][0]["minBitrate"] = 1;
   // FIXME: Make sure the node user is checked against logged in user?
   if((rval = ci->populateWareBundle(
      BM_USER_ID(vars["nodeuser"]), ware, mpl)))
   {
      FileInfoIterator i = ware["fileInfos"].getIterator();
      while(i->hasNext())
      {
         FileInfo& fi = i->next();

         // FIXME: get content type for file info instead of "extension"
         const char* extension = fi["extension"]->getString();
         // FIXME: only support for mp3 audio at this time
         if(strcmp(extension, "mp3") == 0)
         {
            // get sample range for file info
            Media media;
            BM_ID_SET(media["id"], BM_MEDIA_ID(fi["mediaId"]));
            if(getSampleRange(media))
            {
               fi["media"] = media;
               fileInfos->append() = fi;
            }
         }
      }
   }

   return rval;
}

bool SampleService::getSampleFile(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   // get resource parameters
   DynamicObject params;
   action->getResourceParams(params);

   return getSampleFileByIds(
      action, in, out, BM_MEDIA_ID(params[0]), BM_FILE_ID(params[1]));
}

bool SampleService::getSampleMedia(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval;

   DynamicObject fileInfos;
   rval = getFileInfos(action, in, out, fileInfos);

   if(rval)
   {
      // send the sample file if only one
      // FIXME: Does it make sense to send sample of first fileinfo?
      // FIXME: Is first sample representative of a collection?
      if(fileInfos->length() == 1)
      {
         FileInfo& fi = fileInfos[0];
         rval = getSampleFileByIds(action, in, out,
            BM_MEDIA_ID(fi["mediaId"]), BM_FILE_ID(fi["id"]));
      }
   }

   return rval;
}

bool SampleService::getSamplePlayList(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval;

   DynamicObject fileInfos;
   rval = getFileInfos(action, in, out, fileInfos);

   if(rval)
   {
      // send the sample play list
      rval = sendPlayList(action, fileInfos);
   }

   if(!rval)
   {
      // sample not found
      action->getResponse()->getHeader()->setStatus(404, "Not Found");
      action->sendResult();
      rval = true;
   }

   return rval;
}

bool SampleService::getSampleRange(Media& media)
{
   bool rval = false;

   // FIXME: change cache to just store sample range array? not entire media?
   // keep in mind that the code currently uses other information from
   // the media like the performer and title for the sample id3v2 tags

   // check with cache
   bool found = false;
   mSampleRangeCacheLock.lock();
   {
      if(mSampleRangeCache["cache"]->hasMember(media["id"]->getString()))
      {
         media = mSampleRangeCache["cache"][media["id"]->getString()];
         rval = found = true;
      }
   }
   mSampleRangeCacheLock.unlock();

   if(!found)
   {
      // get sample range for media from bitmunk
      Url url;
      url.format("/api/3.0/media/%" PRIu64, BM_MEDIA_ID(media["id"]));

      StringTokenizer st;
      if((rval = mNode->getMessenger()->getFromBitmunk(&url, media)))
      {
         // validate sample range
         st.tokenize(media["sampleRange"]->getString(), '-');
         if((rval = (st.getTokenCount() == 2)))
         {
            // set sample range start and end
            media["sampleRange"][0] =
               (uint32_t)strtoul(st.nextToken(), NULL, 10);
            media["sampleRange"][1] =
               (uint32_t)strtoul(st.nextToken(), NULL, 10);

            // update media length to use sample length
            int sampleLength =
               media["sampleRange"][1]->getUInt32() -
               media["sampleRange"][0]->getUInt32();
            media["length"] = sampleLength;

            // update media title to include "- Bitmunk Sample"
            string title = media["title"]->getString();
            title.append(" - Bitmunk Sample");
            media["title"] = title.c_str();

            // update cache
            mSampleRangeCacheLock.lock();
            {
               if(mSampleRangeCache["cache"]->length() + 1 >=
                  mSampleRangeCache["capacity"]->getInt32())
               {
                  // clear cache, capacity reached
                  mSampleRangeCache->clear();
               }

               mSampleRangeCache["cache"][media["id"]->getString()] = media;
            }
            mSampleRangeCacheLock.unlock();
         }
         else
         {
            // no sample range available
            media["sampleRange"][0] = 0;
            media["sampleRange"][1] = 0;
         }

         // clear any sample content-length
         media->removeMember("sampleContentLength");
      }
   }

   return rval;
}

bool SampleService::sendm3u(BtpAction* action, DynamicObject& fileInfos)
{
   bool rval = true;

   // build m3u file
   string content;
   content.append("#EXTM3U\n");

   // create play list
   char temp[22];
   FileInfoIterator i = fileInfos.getIterator();
   while(i->hasNext())
   {
      FileInfo& fi = i->next();
      Media& media = fi["media"];
      int sampleLength =
         media["sampleRange"][1]->getUInt32() -
         media["sampleRange"][0]->getUInt32();
      snprintf(temp, 22, "%u", (sampleLength < 0 ? 0 : sampleLength));

      string artist;
      if(media["contributors"]->hasMember("Performer"))
      {
         artist = media["contributors"]["Performer"][0]["name"]->getString();
      }
      else if(media["contributors"]->hasMember("Artist"))
      {
         artist = media["contributors"]["Artist"][0]["name"]->getString();
      }

      content.append("#EXTINF:");
      content.append(temp);
      content.push_back(',');
      content.append(artist);
      content.append(" - ");
      content.append(media["title"]->getString());
      content.push_back('\n');

      // FIXME: get host from configuration or something
      // needs to be public IP that remote side can access
      DynamicObject vars;
      action->getResourceQuery(vars);
      if(!vars->hasMember("nodeuser"))
      {
         BM_ID_SET(vars["nodeuser"], mNode->getDefaultUserId());
      }

      HttpRequestHeader* header = action->getRequest()->getHeader();
      string host = header->getFieldValue("X-Forwarded-Host");
      if(host.length() == 0)
      {
         host = header->getFieldValue("Host");
      }
      content.append("http://");
      content.append(host);
      content.append("/api/3.0/sales/samples/file/");
      content.append(fi["mediaId"]->getString());
      content.push_back('/');
      content.append(fi["id"]->getString());
      content.push_back('\n');
   }

   // set up response header
   HttpResponseHeader* header = action->getResponse()->getHeader();
   header->setField("Content-Type", "audio/x-mpegurl");
   header->setField(
      "Content-Disposition", "attachment; filename=bitmunk-sample.m3u");

   // create content input stream
   ByteBuffer b(content.length());
   b.put(content.c_str(), content.length(), false);
   ByteArrayInputStream bais(&b);

   // send sample
   action->getResponse()->getHeader()->setStatus(200, "OK");
   action->sendResult(&bais);

   return rval;
}

bool SampleService::sendFile(BtpAction* action, FileInfo& fi)
{
   bool rval = false;

   // FIXME: only audio supported at this time

   // FIXME: get content type for file info instead of "extension"
   const char* extension = fi["extension"]->getString();
   if((rval = (strcmp(extension, "mp3") == 0)))
   {
      // ensure file exists, get sample range for media
      File file(fi["path"]->getString());
      Media media;
      BM_ID_SET(media["id"], BM_MEDIA_ID(fi["mediaId"]));
      if((rval = (file->exists() && getSampleRange(media))))
      {
         // set up response header
         HttpResponseHeader* header = action->getResponse()->getHeader();
         header->setField("Content-Type", "audio/mpeg");
         header->setField(
            "Content-Disposition", "attachment; filename=bitmunk-sample.mp3");

         // calculate content length for sample for HTTP/1.0
         if(strcmp(
            action->getRequest()->getHeader()->getVersion(), "HTTP/1.0") == 0)
         {
            // check for cached sample content length
            int64_t contentLength = 0;
            if(media->hasMember("sampleContentLength"))
            {
               contentLength = media["sampleContentLength"]->getUInt64();
            }
            else
            {
               // create time parser to produce sample
               MpegAudioTimeParser matp;
               matp.addTimeSet(
                  media["sampleRange"][0]->getUInt32(),
                  media["sampleRange"][1]->getUInt32());

               // FIXME: build a method into the catalog that will return
               // the content-length for a sample

               // read data, strip id3 tag, parse mp3 data, add new id3 tag
               // get content length
               Id3v2Tag tag(media);
               Id3v2TagWriter stripper(NULL);
               Id3v2TagWriter embedder(
                  &tag, false, tag.getFrameSource(), false);

               FileInputStream fis(file);
               MutatorInputStream strip(&fis, false, &stripper, false);
               MutatorInputStream parse(&strip, false, &matp, false);
               MutatorInputStream embed(&parse, false, &embedder, false);
               int64_t numBytes;
               while((numBytes = embed.skip(file->getLength())) > 0)
               {
                  contentLength += numBytes;
               }
               embed.close();

               if(numBytes != -1)
               {
                  // cache content length
                  media["sampleContentLength"] = contentLength;
               }
            }

            // replace chunked encoding with content length
            header->setField("Connection", "close");
            header->removeField("Transfer-Encoding");
            header->removeField("TE");
            header->setField("Content-Length", contentLength);
         }

         // create time parser to produce sample
         MpegAudioTimeParser matp;
         matp.addTimeSet(
            media["sampleRange"][0]->getUInt32(),
            media["sampleRange"][1]->getUInt32());

         // read data, strip id3 tag, embed new id3 tag, produce sample
         Id3v2Tag tag(media);
         Id3v2TagWriter stripper(NULL);
         Id3v2TagWriter embedder(
            &tag, false, tag.getFrameSource(), false);

         FileInputStream fis(file);
         MutatorInputStream strip(&fis, false, &stripper, false);
         MutatorInputStream parse(&strip, false, &matp, false);
         MutatorInputStream embed(&parse, false, &embedder, false);

         // send sample, remove any special encoding to prevent compression
         action->getRequest()->getHeader()->removeField("Accept-Encoding");
         action->getResponse()->getHeader()->setStatus(200, "OK");
         action->sendResult(&embed);

         // close stream
         embed.close();
      }
   }

   return rval;
}

bool SampleService::sendPlayList(BtpAction* action, DynamicObject& fileInfos)
{
   bool rval = false;

   if(fileInfos->length() > 0)
   {
      // FIXME: only support for m3u play lists at this time
      // FIXME: use query param to select other playlist formats
      rval = sendm3u(action, fileInfos);
   }

   return rval;
}

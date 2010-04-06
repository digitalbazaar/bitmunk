/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk/bfp/IBfpModuleImpl.h"

#include "monarch/crypto/MessageDigest.h"
#include "monarch/io/FileInputStream.h"
#include "monarch/io/FileOutputStream.h"
#include "monarch/http/HttpConnection.h"
#include "monarch/rt/Platform.h"
#include "bitmunk/bfp/BfpApiVersion.h"
#include "bitmunk/bfp/BfpModule.h"
#include "bitmunk/common/Logging.h"

using namespace std;
using namespace monarch::config;
using namespace monarch::crypto;
using namespace monarch::io;
using namespace monarch::http;
using namespace monarch::modest;
using namespace monarch::net;
using namespace monarch::rt;
using namespace monarch::util;
using namespace bitmunk::bfp;
using namespace bitmunk::common;
using namespace bitmunk::protocol;
using namespace bitmunk::node;

IBfpModuleImpl::IBfpModuleImpl(bitmunk::node::Node* node) :
   mNode(node),
   mCheckBfp(true)
{
}

IBfpModuleImpl::~IBfpModuleImpl()
{
}

static bool fileSha1(File file, string& sha1)
{
   MessageDigest md("SHA1");
   bool rval = md.digestFile(file);
   if(rval)
   {
      sha1 = md.getDigest();
   }
   return rval;
}

Bfp* IBfpModuleImpl::createBfp(BfpId id)
{
   Bfp* rval = NULL;

   // get appropriate factory
   BfpFactory* f = getBfpFactory(id);
   if(f != NULL)
   {
      // create bfp
      rval = f->createBfp(id);
   }
   else
   {
      bool success;

      Platform::PlatformInfo pi = Platform::getCurrent();
      // check cache directory for bfp first, then check SVA
      Config cfg =
         mNode->getConfigManager()->getModuleConfig("bitmunk.bfp.Bfp");
      const char* cachePath = cfg["cachePath"]->getString();
      string fullCachePath;
      success = mNode->getConfigManager()->expandBitmunkHomePath(
         cachePath, fullCachePath);
      string path;
      if(success)
      {
         // make a platform specific library name
         // format is <prefix>bmbfp-<id>-<apiversion>-<os>-<cputype>.<ext>
         // ex: libbmbfp-1-3.1-200901010101-linux-x86.so
         DynamicObject libname;
         libname->format("%sbmbfp-%u-%s-%s-%s.%s",
            Platform::getDynamicLibraryPrefix(pi["os"]->getString()),
            id, BITMUNK_BFP_API_VERSION,
            pi["os"]->getString(), pi["cpuType"]->getString(),
            Platform::getDynamicLibraryExt(pi["os"]->getString()));
         path = File::join(fullCachePath.c_str(), libname->getString());
      }
      // file will be bogus if !success but not used in that case
      File file(path.c_str());

      // lock to synchronize checking/updating bfp on-disk cache
      // FIXME: multiple clients could run this code concurrently
      mLock.lock();
      {
         // check if loaded while waiting for the lock
         bool loaded = (getBfpFactory(id) != NULL);

         if(success && !loaded && (mCheckBfp || !file->exists()))
         {
            MO_CAT_INFO(BM_BFP_CAT, "Checking BFP...");
            // get url for bfp factory module
            Url url;
            url.format(
               "%s/api/3.0/sva/bfp?id=%u&apiVersion=%s&os=%s&cpuType=%s",
               mNode->getMessenger()->getSecureBitmunkUrl()->toString().c_str(),
               id, BITMUNK_BFP_API_VERSION,
               pi["os"]->getString(), pi["cpuType"]->getString());

            // Modified copy of BtpClient::exchange.
            // This version uses "ETag" with sha1 of local file and checks the
            // response code before potentially writing the response to the
            // bfp file.
            BtpClient* c = mNode->getMessenger()->getBtpClient();
            // create connection
            HttpConnection* hc = c->createConnection(1, &url);
            if((success = (hc != NULL)))
            {
               // create request and response
               HttpRequest* request = (HttpRequest*)hc->createRequest();
               HttpResponse* response =
                  (HttpResponse*)request->createResponse();

               // by default, close connection
               request->getHeader()->setField("Connection", "close");
               // check with SHA1 ETag on startup
               if(file->exists())
               {
                  MO_CAT_INFO(BM_BFP_CAT, "Checking for BFP update: %s",
                     file->getAbsolutePath());
                  string sha1;
                  success = fileSha1(file, sha1);
                  // add file sha1 ETag
                  DynamicObject etag;
                  etag->format("\"%s\"", sha1.c_str());
                  request->getHeader()->setField("ETag", etag->getString());
               }
               else
               {
                  MO_CAT_INFO(BM_BFP_CAT, "Downloading BFP: %s",
                     file->getAbsolutePath());
               }

               // setup messages
               BtpMessage out;
               out.setType(BtpMessage::Get);
               BtpMessage in;

               // send message
               if(success && (success = c->sendMessage(
                  &url, &out, request, &in, response)))
               {
                  HttpResponseHeader* hrh = response->getHeader();
                  // 304 Not Modified if ETag matches
                  int code = hrh->getStatusCode();
                  if(code == 304)
                  {
                     MO_CAT_INFO(BM_BFP_CAT, "BFP unchanged.");
                  }
                  else if(code == 200)
                  {
                     // response is the bfp library
                     // make dirs if needed
                     success = file->mkdirs();
                     // set content sink to cache file, close when done
                     FileOutputStream fos(file);
                     in.setContentSink(&fos, true);
                     // receive and output the bfp library
                     success = success && in.receiveContent(response);
                     if(success)
                     {
                        MO_CAT_INFO(BM_BFP_CAT, "BFP downloaded");
                     }
                     else
                     {
                        MO_CAT_ERROR(BM_BFP_CAT, "BFP download error");
                     }
                  }
                  else
                  {
                     // FIXME: deal with other <400 codes
                     MO_CAT_WARNING(BM_BFP_CAT,
                        "BFP check got unknown response code: %i", code);
                  }
               }

               // disconnect
               //rval = hc->close();
               hc->close();

               // clean up
               delete hc;
               delete request;
               delete response;
            }
         }

         if(success && !loaded)
         {
            MO_CAT_INFO(BM_BFP_CAT, "Loading BFP: %s", file->getAbsolutePath());
            // try to load module into kernel
            ModuleLibrary* lib = mNode->getKernel()->getModuleLibrary();
            if(lib->loadModule(file->getAbsolutePath()) != NULL)
            {
               loaded = true;
            }
            // mark bfp as checked
            mCheckBfp = false;
         }

         if(success && loaded)
         {
            // create bfp object
            f = getBfpFactory(id);
            rval = f->createBfp(id);
         }
      }
      mLock.unlock();
   }

   return rval;
}

void IBfpModuleImpl::freeBfp(Bfp* bfp)
{
   // get appropriate factory
   BfpFactory* f = (bfp != NULL) ? getBfpFactory(bfp->getId()) : NULL;
   if(f != NULL)
   {
      // free bfp
      f->freeBfp(bfp);
   }
}

BfpFactory* IBfpModuleImpl::getBfpFactory(bitmunk::common::BfpId id)
{
   // create module ID for bfp factory module
   char version[24];
   snprintf(version, 24, "1.%" PRIu32, id);
   ModuleId mid("bitmunk.bfp.BfpFactory", version);

   // check node for the bfp factory module
   ModuleLibrary* lib = mNode->getKernel()->getModuleLibrary();
   return dynamic_cast<BfpFactory*>(lib->getModuleInterface(&mid));
}

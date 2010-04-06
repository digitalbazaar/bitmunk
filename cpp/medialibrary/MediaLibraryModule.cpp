/*
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/medialibrary/MediaLibraryModule.h"

#include "bitmunk/medialibrary/MediaLibrary.h"
#include "bitmunk/medialibrary/MediaLibraryService.h"
#include "bitmunk/node/BtpServer.h"

using namespace monarch::config;
using namespace monarch::kernel;
using namespace monarch::logging;
using namespace monarch::modest;
using namespace monarch::rt;
using namespace bitmunk::node;
using namespace bitmunk::protocol;
using namespace bitmunk::medialibrary;

// Logging category initialized during module initialization.
Category* BM_MEDIALIBRARY_CAT;

MediaLibraryModule::MediaLibraryModule() :
   BitmunkModule("bitmunk.medialibrary.MediaLibrary", "1.0"),
   mMediaLibrary(NULL)
{
}

MediaLibraryModule::~MediaLibraryModule()
{
}

void MediaLibraryModule::addDependencyInfo(DynamicObject& depInfo)
{
   // set dependencies:

   // requires the bfp module, version 1.0
   {
      DynamicObject dep;
      dep["name"] = "bitmunk.bfp.Bfp";
      dep["version"] = "1.0";
      depInfo["dependencies"]->append(dep);
   }

   // requires the peruserdb module, version 1.0
   {
      DynamicObject dep;
      dep["name"] = "bitmunk.peruserdb.PerUserDB";
      dep["version"] = "1.0";
      depInfo["dependencies"]->append(dep);
   }
}

bool MediaLibraryModule::initialize(Node* node)
{
   bool rval = false;

   BM_MEDIALIBRARY_CAT = new Category(
      "BM_MEDIALIBRARY",
      "Bitmunk Media Library",
      NULL);

   // create the media library
   mMediaLibrary = new MediaLibrary(node);
   rval = mMediaLibrary->initialize();
   if(rval)
   {
      // create and add media library service
      BtpServiceRef bs = new MediaLibraryService(
         node, "/api/3.2/medialibrary", mMediaLibrary);
      rval = node->getBtpServer()->addService(bs, Node::SslOn);
   }
   else
   {
      // clean up uninitialized media library
      mMediaLibrary->cleanup();
      delete mMediaLibrary;
      mMediaLibrary = NULL;
   }

   return rval;
}

void MediaLibraryModule::cleanup(Node* node)
{
   // remove services
   node->getBtpServer()->removeService("/api/3.2/medialibrary");

   // clean up media library
   if(mMediaLibrary != NULL)
   {
      mMediaLibrary->cleanup();
      delete mMediaLibrary;
      mMediaLibrary = NULL;
   }

   delete BM_MEDIALIBRARY_CAT;
   BM_MEDIALIBRARY_CAT = NULL;
}

MicroKernelModuleApi* MediaLibraryModule::getApi(Node* node)
{
   return mMediaLibrary;
}

Module* createModestModule()
{
   return new MediaLibraryModule();
}

void freeModestModule(Module* m)
{
   delete m;
}

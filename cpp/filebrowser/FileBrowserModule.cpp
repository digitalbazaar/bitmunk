/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/filebrowser/FileBrowserModule.h"

#include "bitmunk/filebrowser/FileBrowserService.h"
#include "bitmunk/node/BtpServer.h"

using namespace monarch::kernel;
using namespace monarch::modest;
using namespace monarch::rt;
using namespace bitmunk::node;
using namespace bitmunk::filebrowser;
using namespace bitmunk::protocol;

FileBrowserModule::FileBrowserModule() :
   NodeModule("bitmunk.filebrowser.FileBrowser", "1.0")
{
}

FileBrowserModule::~FileBrowserModule()
{
}

void FileBrowserModule::addDependencyInfo(DynamicObject& depInfo)
{
   // no dependencies
}

bool FileBrowserModule::initialize(Node* node)
{
   bool rval;
   BtpServiceRef bs;

   // create and add a filebrowser service
   bs = new FileBrowserService(node, "/api/3.2/filesystem");
   rval = node->getBtpServer()->addService(bs, Node::SslOn);

   return rval;
}

void FileBrowserModule::cleanup(Node* node)
{
   // remove services
   node->getBtpServer()->removeService("/api/3.2/filesystem");
}

MicroKernelModuleApi* FileBrowserModule::getApi(Node* node)
{
   // return NULL since only services should use this interface.
   return NULL;
}

Module* createModestModule()
{
   return new FileBrowserModule();
}

void freeModestModule(Module* m)
{
   delete m;
}

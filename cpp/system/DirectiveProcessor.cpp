/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk/system/DirectiveProcessor.h"

#include "bitmunk/common/Logging.h"
#include "monarch/rt/RunnableDelegate.h"

using namespace bitmunk::common;
using namespace bitmunk::system;
using namespace bitmunk::node;
using namespace monarch::event;
using namespace monarch::modest;
using namespace monarch::rt;
using namespace monarch::net;
using namespace std;

DirectiveProcessor::DirectiveProcessor(Node* node) :
   NodeFiber(node),
   mDirective(NULL),
   mResult(NULL)
{
}

DirectiveProcessor::~DirectiveProcessor()
{
}

void DirectiveProcessor::setDirective(DynamicObject& d)
{
   mDirective = d;
}

void DirectiveProcessor::createDownloadState()
{
   Messenger* messenger = mNode->getMessenger();
   
   // create download state by posting to self
   Url url;
   url.format(
      "%s/api/3.0/purchase/contracts/downloadstates?nodeuser=%" PRIu64,
      messenger->getSelfUrl(true).c_str(),
      BM_USER_ID(mDirective["userId"]));
   mResult = DynamicObject();
   bool success = messenger->post(
      &url, &mDirective["data"], &mResult, BM_USER_ID(mDirective["userId"]));
   
   // send event based on success
   sendEvent(!success);
   
   // send message to self
   DynamicObject msg;
   msg["downloadStateCreated"] = true;
   messageSelf(msg);
}

void DirectiveProcessor::processMessages()
{
   // FIXME: currently only peerbuy directives are supported
   if(strcmp(mDirective["type"]->getString(), "peerbuy") == 0)
   {
      // run a new create download state operation
      RunnableRef r = new RunnableDelegate<DirectiveProcessor>(
         this, &DirectiveProcessor::createDownloadState);
      Operation op = r;
      getNode()->runOperation(op);
      
      // wait for downloadStateCreated message
      const char* keys[] = {"downloadStateCreated", NULL};
      waitForMessages(keys, &op)[0];
   }
   else
   {
      // invalid directive type send exception event
      ExceptionRef e = new Exception(
         "Could not process directive. Directive type is invalid.",
         "bitmunk.system.DirectiveProcessor.InvalidDirectiveType");
      BM_ID_SET(e->getDetails()["userId"], BM_USER_ID(mDirective["userId"]));
      e->getDetails()["directiveId"] = mDirective["id"];
      Exception::set(e);
      sendEvent(true);
   }
}

void DirectiveProcessor::sendEvent(bool error)
{
   Event e;
   BM_ID_SET(e["details"]["userId"], BM_USER_ID(mDirective["userId"]));
   e["details"]["directive"] = mDirective;
   
   if(error)
   {
      // failed to process directive
      ExceptionRef ex = new Exception(
         "Could not process directive.",
         "bitmunk.system.DirectiveProcessor.DirectiveFailed");
      Exception::push(ex);
      
      // set failure event
      e["type"] = "bitmunk.system.Directive.exception";
      e["details"]["exception"] = Exception::convertToDynamicObject(ex);
   }
   else
   {
      // set success event
      e["type"] = "bitmunk.system.Directive.processed";
      
      if(!mResult.isNull())
      {
         e["details"]["result"] = mResult;
      }
   }
   
   // send event
   getNode()->getEventController()->schedule(e);
}

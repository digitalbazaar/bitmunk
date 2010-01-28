/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/system/TestService.h"

#include "bitmunk/node/BtpActionDelegate.h"
#include "bitmunk/node/RestResourceHandler.h"
#include "monarch/data/json/JsonWriter.h"

using namespace std;
using namespace monarch::data::json;
using namespace monarch::http;
using namespace monarch::io;
using namespace monarch::net;
using namespace monarch::rt;
using namespace bitmunk::common;
using namespace bitmunk::protocol;
using namespace bitmunk::node;
using namespace bitmunk::system;
namespace v = monarch::validation;

typedef BtpActionDelegate<TestService> Handler;

TestService::TestService(Node* node, const char* path) :
   NodeService(node, path)
{
}

TestService::~TestService()
{
}

bool TestService::initialize()
{
   // ping
   {
      RestResourceHandlerRef ping = new RestResourceHandler();
      addResource("/ping", ping);
      
      // GET .../ping
      {
         ResourceHandler h = new Handler(
            mNode, this, &TestService::getPing,
            BtpAction::AuthOptional);
         ping->addHandler(h, BtpMessage::Get);
      }
   }
   
   // echo
   {
      RestResourceHandlerRef echo = new RestResourceHandler();
      addResource("/echo", echo);
      
      // GET .../echo
      {
         ResourceHandler h = new Handler(
            mNode, this, &TestService::getEcho,
            BtpAction::AuthOptional);
         
         v::ValidatorRef qValidator =
            new v::Map(
               "echo", new v::Optional(new v::All(
                  new v::Type(String),
                  new v::Max(100,
                     "Echo data is too long.  100 characters maximum."),
                  NULL)),
               "callback", new v::Optional(new v::All(
                  new v::Type(String),
                  new v::Min(1,
                     "Callback too short. 1 characters minimum."),
                  new v::Max(100, 
                     "Callback too long. 100 characters maximum."),
                  NULL)),
               NULL);
   
         echo->addHandler(h, BtpMessage::Get, 0, &qValidator);
      }
      
      // POST .../echo
      {
         ResourceHandler h = new Handler(
            mNode, this, &TestService::postEcho,
            BtpAction::AuthOptional);
         
         v::ValidatorRef cValidator =
            new v::Any(
               new v::Null(),
               new v::Map(
                  "echo", new v::Optional(new v::All(
                     new v::Type(String),
                     new v::Max(100,
                        "Echo data is too long.  100 characters maximum."),
                     NULL)),
                  NULL),
               NULL);
   
         echo->addHandler(h, BtpMessage::Post, 0, NULL, &cValidator);
      }
   }
   
   return true;
}

void TestService::cleanup()
{
   // remove resources
   removeResource("/ping");
   removeResource("/echo");   
}

bool TestService::getPing(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   out.setNull();
   
   // set the header fields
   HttpResponseHeader* header = action->getResponse()->getHeader();
   header->setStatus(200, "OK");
   header->setField("Content-Length", 0);
   
   return true;
}

bool TestService::getEcho(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = true;
   
   // get query
   DynamicObject q;
   action->getResourceQuery(q);
   if(!q->hasMember("echo"))
   {
      q["echo"].setNull();
   }

   if(q->hasMember("callback"))
   {
      // avoid standard dyno output
      out.setNull();
      action->setResultSent();
      
      // create output
      DynamicObject echo;
      echo["echo"] = q["echo"];
      
      HttpResponseHeader* header = action->getResponse()->getHeader();
      header->setStatus(200, "OK");
      header->setField("Content-Type", "text/javascript");
      header->setField("Transfer-Encoding", "chunked");
      
      OutputStreamRef os;
      HttpTrailerRef trailer;
      rval = action->getOutMessage()->sendHeader(
         action->getResponse()->getConnection(),
         header, os, trailer);
      if(rval)
      {
         JsonWriter jw;
         const char* cb = q["callback"]->getString();
         rval = os->write(cb, strlen(cb)) &&
            os->write("(", 1) &&
            jw.write(echo, &(*os)) &&
            os->write(")", 1) &&
            os->finish();
      }
   }
   else
   {
      out["echo"] = q["echo"];
   }
   
   return rval;
}

bool TestService::postEcho(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = true;
   
   if(!in.isNull() && in->hasMember("echo"))
   {
      out["echo"] = in["echo"];
   }
   else
   {
      out["echo"].setNull();
   }
   
   return rval;
}

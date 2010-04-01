/*
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/node/NodeService.h"

#include "monarch/data/json/JsonWriter.h"
#include "monarch/http/CookieJar.h"
#include "monarch/util/Date.h"
#include "monarch/util/StringTokenizer.h"

using namespace std;
using namespace bitmunk::protocol;
using namespace bitmunk::node;
using namespace monarch::config;
using namespace monarch::event;
using namespace monarch::data::json;
using namespace monarch::http;
using namespace monarch::net;
using namespace monarch::rt;
using namespace monarch::util;

NodeService::NodeService(
   Node* node, const char* path, bool dynamicResources) :
   BtpService(path, dynamicResources)
{
   mNode = node;
   mExceptionTypeBlockList->setType(Array);
}

NodeService::~NodeService()
{
}

bool NodeService::addExceptionTypeToBlockList(const char* type)
{
   bool rval = true;

   // append the exception type to the list of exceptions to block
   mExceptionTypeBlockList->append() = type;

   return rval;
}

bool NodeService::sendExceptionEvent(ExceptionRef e, BtpAction* action)
{
   bool rval = true;

   // search the list of Exception names to block
   bool blockException = false;
   ExceptionRef cause = e->getCause();
   if(!cause.isNull())
   {
      DynamicObjectIterator i = mExceptionTypeBlockList.getIterator();
      while(!blockException && i->hasNext())
      {
         if(cause->isType(i->next()->getString()))
         {
            blockException = true;
         }
      }
   }

   // propagate the exception if it isn't marked to be blocked
   if(!blockException)
   {
      // create DynamicObject from exception
      DynamicObject dyno = Exception::convertToDynamicObject(e);

      // pretty-print exception in json
      string estr = JsonWriter::writeToString(dyno);

      // set exception details
      Date d;
      string date;
      DynamicObject details;
      details["date"] = d.format(date).c_str();
      details["resource"] = action->getResource();
      // FIXME: string vs dyno?
      details["exception"] = estr.c_str();

      // include exception message
      if(strlen(e->getMessage()) > 50)
      {
         char tmp[50];
         snprintf(tmp, 50, "%s", e->getMessage());
         string str = tmp;
         str.append("...");
         details["exceptionMsg"] = str.c_str();
      }
      else
      {
         details["exceptionMsg"] = e->getMessage();
      }

      // send a mail event using webservice exception template
      Event ev;
      ev["type"] = "bitmunk.node.NodeService.exception";
      ev["details"] = details;
      sendScrubbedEvent(ev, action);
   }

   return rval;
}

bool NodeService::sendScrubbedEvent(Event& e, BtpAction* action)
{
   bool rval = true;

   // get exception details
   DynamicObject& details = e["details"];

   // copy request header for modification
   HttpRequestHeader reqHeader;
   action->getRequest()->getHeader()->writeTo(&reqHeader);
   if(reqHeader.hasField("Cookie"))
   {
      // get request header, get cookies
      CookieJar jar;
      jar.readCookies(&reqHeader, CookieJar::Client);
      // Note: special knowledge of bitmunk website cookies
      Cookie userIdCookie = jar.getCookie("bitmunk-user-id");
      Cookie usernameCookie = jar.getCookie("bitmunk-username");
      if(!userIdCookie.isNull())
      {
         details["cookieUserId"] = userIdCookie["value"]->getString();
      }
      else
      {
         details["cookieUserId"] = "N/A";
      }
      if(!usernameCookie.isNull())
      {
         details["cookieUsername"] = Url::decode(
            usernameCookie["value"]->getString()).c_str();
      }
      else
      {
         details["cookieUsername"] = "N/A";
      }
   }

   // clear cookie fields, save rest of request
   reqHeader.removeField("Cookie");
   reqHeader.removeField("Cookie2");
   string request = reqHeader.toString();

   // set request
   details["request"] = request.c_str();

   HttpResponseHeader resHeader;
   action->getResponse()->getHeader()->writeTo(&resHeader);
   resHeader.removeField("Set-Cookie");
   string response = resHeader.toString();

   // set response
   details["response"] = response.c_str();

   // schedule event
   mNode->getEventController()->schedule(e);

   return rval;
}

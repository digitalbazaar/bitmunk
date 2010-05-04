/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/protocol/BtpAction.h"

#include "bitmunk/common/Logging.h"
#include "monarch/data/json/JsonWriter.h"

#include <cctype>
#include <algorithm>

using namespace std;
using namespace monarch::data::json;
using namespace monarch::http;
using namespace monarch::io;
using namespace monarch::net;
using namespace monarch::rt;
using namespace bitmunk::common;
using namespace bitmunk::protocol;

BtpAction::BtpAction(const char* resource) :
   mResourceParams(NULL),
   mQueryVars(NULL),
   mArrayQueryVars(NULL),
   mContent(NULL)
{
   // store resource
   mResource = strdup(resource);
   mBaseResourcePath = NULL;

   // no request or response yet
   mRequest = NULL;
   mResponse = NULL;

   mContentReceived = false;
   setResultSent(false);
   mSelectContentEncoding = true;
}

BtpAction::~BtpAction()
{
   free(mResource);

   if(mBaseResourcePath != NULL)
   {
      free(mBaseResourcePath);
   }
}

void BtpAction::setContentEncoding()
{
   // check user-agent (MSIE barfs on deflate because it assumes it is raw
   // DEFLATE, not zlib+DEFLATE which is what the HTTP spec calls for...
   // zlib adds a 2 byte header that it dies on)
   // Safari has the same issue.

   bool mustGzip = true;
   string userAgent;
   if(mRequest->getHeader()->getField("User-Agent", userAgent))
   {
      // convert user agent to lower-case to normalize comparisons
      transform(
         userAgent.begin(), userAgent.end(), userAgent.begin(), ::tolower);
      mustGzip =
         (strstr(userAgent.c_str(), "msie") != NULL) ||
         (strstr(userAgent.c_str(), "webkit") != NULL) ||
         (strstr(userAgent.c_str(), "konqueror") != NULL);
   }

   // use accept content-encoding
   string contentEncoding;
   if(mRequest->getHeader()->getField("Accept-Encoding", contentEncoding))
   {
      // gzip gets precendence because not everyone handles deflate properly
      if(!mustGzip && strstr(contentEncoding.c_str(), "deflate") != NULL)
      {
         mResponse->getHeader()->setField("Content-Encoding", "deflate");
      }
      else if(strstr(contentEncoding.c_str(), "gzip") != NULL)
      {
         mResponse->getHeader()->setField("Content-Encoding", "gzip");
      }
   }
}

void BtpAction::setSelectContentEncoding(bool on)
{
   mSelectContentEncoding = on;
}

bool BtpAction::checkSecurity()
{
   bool rval = true;

   // check btp security
   if(rval)
   {
      switch(getInMessage()->getSecurityStatus())
      {
         case BtpMessage::Unchecked:
         {
            // set exception
            ExceptionRef e = new Exception(
               "Secure message required.", "bitmunk.btp.Security");
            DynamicObject& details = e->getDetails();
            details["resource"] = getResource();
            details["method"] =
               BtpMessage::typeToString(getInMessage()->getType());
            Exception::set(e);
            rval = false;
            break;
         }
         case BtpMessage::Breach:
         {
            // set exception
            ExceptionRef e = new Exception(
               "Message security breach.", "bitmunk.btp.Security");
            DynamicObject& details = e->getDetails();
            details["resource"] = getResource();
            details["method"] =
               BtpMessage::typeToString(getInMessage()->getType());
            Exception::set(e);
            rval = false;
            break;
         }
         default:
            break;
      }
   }

   return rval;
}

bool BtpAction::receiveContent(OutputStream* os, bool close)
{
   // set content sink, receive content
   mInMessage.setContentSink(os, close);
   return mInMessage.receiveContent(mRequest);
}

bool BtpAction::receiveContent(DynamicObject& dyno)
{
   bool rval = true;

   // only receive content if not received already
   if(!mContentReceived)
   {
      // check to see if there is content to receive
      if(mRequest->getHeader()->hasContent())
      {
         // set content object, receive content
         mInMessage.setDynamicObject(dyno);
         rval = mInMessage.receiveContent(mRequest);
      }
      else
      {
         // no content
         dyno.setNull();
      }

      // content now received (Note: dyno is not cloned here, so changes
      // will affect the cached received value)
      mContent = dyno;
      mContentReceived = true;
   }
   else
   {
      // set to stored content
      dyno = mContent;
   }

   return rval;
}

bool BtpAction::sendResult()
{
   bool rval = true;

   if(!isResultSent())
   {
      // return headers, set no content code if code not set
      if(mResponse->getHeader()->getStatusCode() == 0)
      {
         mResponse->getHeader()->setStatus(204, "No Content");
      }
      else
      {
         // ensure content-length is set to none
         mResponse->getHeader()->setField("Content-Length", 0);
      }

      mOutMessage.setContentSource(NULL);
      rval = mOutMessage.send(mResponse);
      if(rval)
      {
         setResultSent();
      }
   }

   return rval;
}

bool BtpAction::sendResult(InputStream* is)
{
   bool rval = true;

   if(!isResultSent())
   {
      // set content source
      mOutMessage.setContentSource(is);

      // set content encoding if not set
      if(mSelectContentEncoding &&
         !mResponse->getHeader()->hasField("Content-Encoding"))
      {
         setContentEncoding();
      }

      // send message
      rval = mOutMessage.send(mResponse);
      if(rval)
      {
         setResultSent();
      }
   }

   return rval;
}

bool BtpAction::sendResult(DynamicObject& dyno)
{
   bool rval = true;

   if(!isResultSent())
   {
      if(mResponse->getHeader()->getStatusCode() == 0)
      {
         // send 200 OK
         mResponse->getHeader()->setStatus(200, "OK");
      }

      // use accept content-type
      string contentType;
      if(mRequest->getHeader()->getField("Accept", contentType))
      {
         if(strstr(contentType.c_str(), "text/xml") != NULL)
         {
            contentType = "text/xml";
         }
         else
         {
            // default to JSON
            contentType = "application/json";
         }
      }
      else
      {
         // default to JSON
         contentType = "application/json";
      }

      // set content object
      mOutMessage.setDynamicObject(dyno, contentType.c_str());

      // set content encoding if not set
      if(mSelectContentEncoding &&
         !mResponse->getHeader()->hasField("Content-Encoding"))
      {
         setContentEncoding();
      }

      // send message
      rval = mOutMessage.send(mResponse);
      if(rval)
      {
         setResultSent();
      }
   }

   return rval;
}

bool BtpAction::sendException(ExceptionRef& e, bool client)
{
   bool rval = true;

   if(!isResultSent())
   {
      // set status code if necessary
      if(mResponse->getHeader()->getStatusCode() == 0)
      {
         if(client)
         {
            // set 400 Bad Request
            mResponse->getHeader()->setStatus(400, "Bad Request");
         }
         else
         {
            // set 500 Internal Server Error
            mResponse->getHeader()->setStatus(500, "Internal Server Error");
         }
      }

      // check the accept field
      string contentType;
      if(mRequest->getHeader()->getField("Accept", contentType))
      {
         if(strstr(contentType.c_str(), "text/xml") != NULL)
         {
            contentType = "text/xml";
         }
         else
         {
            // default to JSON
            contentType = "application/json";
         }
      }
      else
      {
         // default to JSON
         contentType = "application/json";
      }

      // convert exception to dyno
      DynamicObject dyno = Exception::convertToDynamicObject(e);
      mOutMessage.setDynamicObject(dyno, contentType.c_str());

      // set content encoding if not set
      if(mSelectContentEncoding &&
         !mResponse->getHeader()->hasField("Content-Encoding"))
      {
         setContentEncoding();
      }

      // log exception
      MO_CAT_DEBUG(BM_PROTOCOL_CAT,
         "Exception while serving resource '%s':\nException %s",
         getResource(), JsonWriter::writeToString(dyno).c_str());

      // send message
      if((rval = mOutMessage.send(mResponse)))
      {
         setResultSent();
      }
   }

   return rval;
}

inline const char* BtpAction::getResource()
{
   return mResource;
}

bool BtpAction::getResourceParams(DynamicObject& params)
{
   bool rval;

   if(mResourceParams.isNull())
   {
      // parse params
      Url url(mResource);
      DynamicObject d;
      mResourceParams = d;
      rval = url.getTokenizedPath(mResourceParams, mBaseResourcePath);
   }
   else
   {
      // check stored params (previously parsed)
      rval = (mResourceParams->length() > 0);
   }

   // clone stored params
   params = mResourceParams.clone();

   return rval;
}

bool BtpAction::getResourceQuery(DynamicObject& vars, bool asArrays)
{
   bool rval;

   // choose which cache ivar to use
   DynamicObject& qvars = asArrays ? mArrayQueryVars : mQueryVars;

   if(qvars.isNull())
   {
      // parse query
      Url url(mResource);
      DynamicObject d;
      qvars = d;
      rval = url.getQueryVariables(qvars, asArrays);
   }
   else
   {
      // check stored vars (previously parsed)
      rval = (qvars->length() > 0);
   }

   // clone stored vars
   vars = qvars.clone();

   return rval;
}

BtpMessage* BtpAction::getInMessage()
{
   return &mInMessage;
}

BtpMessage* BtpAction::getOutMessage()
{
   return &mOutMessage;
}

void BtpAction::setRequest(HttpRequest* request)
{
   mRequest = request;

   // set message type
   const char* method = request->getHeader()->getMethod();
   BtpMessage::Type type = BtpMessage::stringToType(method);
   if(type != BtpMessage::Undefined)
   {
      getInMessage()->setType(type);
   }
}

HttpRequest* BtpAction::getRequest()
{
   return mRequest;
}

void BtpAction::setResponse(HttpResponse* response)
{
   mResponse = response;
}

HttpResponse* BtpAction::getResponse()
{
   return mResponse;
}

void BtpAction::setBaseResourcePath(const char* res)
{
   if(mBaseResourcePath != NULL)
   {
      free(mBaseResourcePath);
   }

   // append slash to passed resource as needed
   int length = strlen(res);
   if(res[length - 1] != '/')
   {
      mBaseResourcePath = (char*)malloc(length + 2);
      strcpy(mBaseResourcePath, res);
      mBaseResourcePath[length] = '/';
      mBaseResourcePath[length + 1] = 0;
   }
   else
   {
      mBaseResourcePath = strdup(res);
   }
}

bool BtpAction::isResultSent()
{
   return mResultSent;
}

void BtpAction::setResultSent(bool resultSent)
{
   mResultSent = resultSent;
}

bool BtpAction::getClientInternetAddress(InternetAddress* address)
{
   return mRequest->getConnection()->writeRemoteAddress(address);
}

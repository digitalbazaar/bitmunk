/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk/protocol/BtpClient.h"

#include "bitmunk/common/Logging.h"
#include "bitmunk/common/Tools.h"
#include "monarch/data/json/JsonWriter.h"
#include "monarch/net/SslSocket.h"

using namespace std;
using namespace monarch::data::json;
using namespace monarch::http;
using namespace monarch::net;
using namespace monarch::rt;
using namespace bitmunk::common;
using namespace bitmunk::protocol;

BtpClient::BtpClient() :
   mSslContext(NULL)
{
   mReadThrottler = NULL;
   mWriteThrottler = NULL;

   // create client ssl context ("TLS" is most secure and recent SSL)
   mSslContext = new SslContext("TLS", true);

   // create ssl session cache
   mSslSessionCache = new SslSessionCache(50);
}

BtpClient::~BtpClient()
{
}

HttpConnection* BtpClient::createConnection(
   UserId userId, Url* url, uint32_t timeout)
{
   HttpConnection* hc;

   if(userId == 0)
   {
      MO_CAT_DEBUG(BM_PROTOCOL_CAT,
         "BtpClient connecting to: %s", url->toString().c_str());
   }
   else
   {
      MO_CAT_DEBUG(BM_PROTOCOL_CAT,
         "BtpClient connecting to peer user %" PRIu64 ": %s",
         userId, url->toString().c_str());
   }

   // determine if SSL should be used
   if(strcmp(url->getScheme().c_str(), "https") == 0)
   {
      // create SSL connection
      if(userId != 0)
      {
         // add bitmunk user virtual host as a valid common name
         string vHost = Tools::getBitmunkUserCommonName(userId);
         DynamicObject commonNames;
         commonNames->append() = vHost.c_str();
         hc = HttpClient::createSslConnection(
            url, *mSslContext, *mSslSessionCache, timeout, &commonNames,
            true, vHost.c_str());
      }
      else
      {
         hc = HttpClient::createSslConnection(
            url, *mSslContext, *mSslSessionCache, timeout);
      }
   }
   else
   {
      // create non-SSL connection
      hc = HttpClient::createConnection(url, NULL, NULL, timeout);
   }

   if(hc != NULL)
   {
      // set timeouts
      uint32_t to = timeout * 1000;
      hc->setReadTimeout(to);
      hc->setWriteTimeout(to);

      // set bandwidth throttlers
      hc->setBandwidthThrottler(getBandwidthThrottler(true), true);
      hc->setBandwidthThrottler(getBandwidthThrottler(false), false);
   }
   else
   {
      ExceptionRef e = new Exception(
         "Could not establish BTP connection.",
         "bitmunk.protocol.ConnectError");
      e->getDetails()["url"] = url->toString().c_str();
      Exception::push(e);

      if(userId == 0)
      {
         MO_CAT_DEBUG(BM_PROTOCOL_CAT,
            "BtpClient failed to connect to: %s, %s",
            url->toString().c_str(),
            JsonWriter::writeToString(Exception::getAsDynamicObject()).c_str());
      }
      else
      {
         MO_CAT_DEBUG(BM_PROTOCOL_CAT,
            "BtpClient failed to connect to peer user %" PRIu64 ": %s, %s",
            userId, url->toString().c_str(),
            JsonWriter::writeToString(Exception::getAsDynamicObject()).c_str());
      }
   }

   return hc;
}

bool BtpClient::sendMessage(
   Url* url,
   BtpMessage* out, HttpRequest* request,
   BtpMessage* in, HttpResponse* response)
{
   bool rval;

   // send message
   if((rval = out->send(url, request)))
   {
      if(in != NULL)
      {
         // receive response header
         if((rval = response->receiveHeader()))
         {
            // check response security
            rval = checkResponseSecurity(url, request, response, in);
         }

         // receive response content
         if(rval)
         {
            // check response code
            bool responseError = !checkResponseCode(response);

            if(in->getSecurityStatus() != BtpMessage::Breach)
            {
               if(responseError)
               {
                  rval = in->receiveContent(response);
               }
            }
            else
            {
               // set exception
               ExceptionRef e = new Exception(
                  "Message security breach.",
                  "bitmunk.protocol.Security");
               e->getDetails()["resource"] = url->toString().c_str();
               responseError ? Exception::push(e) : Exception::set(e);
               rval = false;
            }

            if(responseError)
            {
               rval = false;
            }
         }
      }
   }

   return rval;
}

bool BtpClient::exchange(
   UserId userId, Url* url, BtpMessage* out, BtpMessage* in, uint32_t timeout)
{
   bool rval = false;

   // create connection
   HttpConnection* hc = createConnection(userId, url, timeout);
   if(hc != NULL)
   {
      // create request and response
      HttpRequest* request = hc->createRequest();
      HttpResponse* response = request->createResponse();

      // send message
      if((rval = sendMessage(url, out, request, in, response)))
      {
         if(in != NULL)
         {
            // receive response content
            rval = in->receiveContent(response);
         }
      }

      // disconnect
      hc->close();

      // clean up
      delete hc;
      delete request;
      delete response;
   }

   return rval;
}

SslContextRef& BtpClient::getSslContext()
{
   return mSslContext;
}

SslSessionCacheRef& BtpClient::getSslSessionCache()
{
   return mSslSessionCache;
}

void BtpClient::setBandwidthThrottler(BandwidthThrottler* bt, bool read)
{
   (read) ? mReadThrottler = bt : mWriteThrottler = bt;
}

BandwidthThrottler* BtpClient::getBandwidthThrottler(bool read)
{
   return (read) ? mReadThrottler : mWriteThrottler;
}

bool BtpClient::checkResponseCode(HttpResponse* response)
{
   bool rval = false;

   HttpRequest* request = response->getRequest();
   int code = response->getHeader()->getStatusCode();

   // server error
   if(code >= 500)
   {
      ExceptionRef e = new Exception(
         "Server error.",
         "bitmunk.protocol.ServerError", code);
      Exception::set(e);
   }
   // client error
   else if(code >= 400)
   {
      switch(code)
      {
         case 403:
         {
            ExceptionRef e = new Exception(
               "Resource forbidden.",
               "bitmunk.protocol.ResourceForbidden", code);
            e->getDetails()["resource"] = request->getHeader()->getPath();
            Exception::set(e);
            break;
         }
         case 404:
         {
            ExceptionRef e = new Exception(
               "Resource not found.",
               "bitmunk.protocol.ResourceNotFound", code);
            e->getDetails()["resource"] = request->getHeader()->getPath();
            Exception::set(e);
            break;
         }
         case 405:
         {
            ExceptionRef e = new Exception(
               "Method not allowed.",
               "bitmunk.protocol.MethodNotAllowed", code);
            e->getDetails()["invalidMethod"] =
               request->getHeader()->getMethod();
            Exception::set(e);
            break;
         }
         default:
         {
            ExceptionRef e = new Exception(
               "Bad request.",
               "bitmunk.protocol.BadRequest", code);
            Exception::set(e);
            break;
         }
      }
   }
   // no error
   else
   {
      rval = true;
   }

   return rval;
}

bool BtpClient::checkResponseSecurity(
   Url* url, HttpRequest* request, HttpResponse* response, BtpMessage* in)
{
   bool rval = true;

   // print out header
   MO_CAT_DEBUG(BM_PROTOCOL_CAT,
      "BtpClient received header:\n%s",
      response->getHeader()->toString().c_str());

   // check header security
   in->checkHeaderSecurity(response->getHeader());
   if(in->getSecurityStatus() == BtpMessage::Breach)
   {
      MO_CAT_DEBUG(BM_PROTOCOL_CAT,
         "BtpClient detected Btp breach. "
         "Bad/missing btp headers or signature. Perhaps a non-btp response.");

      // set exception
      ExceptionRef e = new Exception(
         "Btp security breach. Bad/missing btp headers or signature.",
         "bitmunk.protocol.Security");
      e->getDetails()["resource"] = url->toString().c_str();
      e->getDetails()["responseStatusCode"] =
         response->getHeader()->getStatusCode();
      e->getDetails()["responseStatusMessage"] =
         response->getHeader()->getStatusMessage();
      Exception::set(e);
      rval = false;
   }
   else
   {
      // confirm that hosts did not change
      string reqHost;
      string resHost;
      request->getHeader()->getField("Host", reqHost);
      response->getHeader()->getField("Host", resHost);
      if(strcmp(reqHost.c_str(), resHost.c_str()) != 0)
      {
         // security breach in header, hosts are different!
         in->setSecurityStatus(BtpMessage::Breach);

         MO_CAT_DEBUG(BM_PROTOCOL_CAT,
            "BtpClient detected Btp breach. "
            "Host headers differ: request=%s response=%s",
            reqHost.c_str(), resHost.c_str());

         // set exception
         ExceptionRef e = new Exception(
            "Message security breach. Request and response hosts differ.",
            "bitmunk.protocol.Security");
         e->getDetails()["resource"] = url->toString().c_str();
         e->getDetails()["requestHost"] = reqHost.c_str();
         e->getDetails()["responseHost"] = resHost.c_str();
         e->getDetails()["responseStatusCode"] =
            response->getHeader()->getStatusCode();
         e->getDetails()["responseStatusMessage"] =
            response->getHeader()->getStatusMessage();
         Exception::set(e);
         rval = false;
      }
   }

   return rval;
}

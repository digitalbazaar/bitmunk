/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk/protocol/BtpMessage.h"

#include "bitmunk/common/Logging.h"
#include "bitmunk/protocol/BtpTrailer.h"
#include "monarch/crypto/DigitalSignatureInputStream.h"
#include "monarch/crypto/DigitalSignatureOutputStream.h"
#include "monarch/data/DynamicObjectInputStream.h"
#include "monarch/data/DynamicObjectOutputStream.h"
#include "monarch/data/json/JsonReader.h"
#include "monarch/data/json/JsonWriter.h"
#include "monarch/data/xml/XmlReader.h"
#include "monarch/data/xml/XmlWriter.h"
#include "monarch/compress/gzip/Gzipper.h"
#include "monarch/io/ByteArrayOutputStream.h"
#include "monarch/io/MutatorInputStream.h"
#include "monarch/io/MutatorOutputStream.h"
#include "monarch/util/Convert.h"
#include "monarch/util/Timer.h"

using namespace std;
using namespace monarch::compress::deflate;
using namespace monarch::compress::gzip;
using namespace monarch::crypto;
using namespace monarch::data;
using namespace monarch::data::json;
using namespace monarch::data::xml;
using namespace monarch::http;
using namespace monarch::io;
using namespace monarch::net;
using namespace monarch::rt;
using namespace monarch::util;
using namespace bitmunk::common;
using namespace bitmunk::protocol;

BtpMessage::BtpMessage() :
   mType(Undefined),
   mSecurityStatus(Unchecked),
   mUserId(0),
   mAgentUserId(0),
   mAgentProfileId(0),
   mPublicKeySource(NULL),
   mContentType(NULL),
   mContentSource(NULL),
   mContentSink(NULL),
   mCloseSink(false),
   mDynamicObject(NULL),
   mCustomHeaders(NULL),
   mSaveHeaders(false),
   mRequestHeader(NULL),
   mResponseHeader(NULL)
{
}

BtpMessage::~BtpMessage()
{
   if(mContentType != NULL)
   {
      free(mContentType);
   }

   if(mRequestHeader != NULL)
   {
      delete mRequestHeader;
   }

   if(mResponseHeader != NULL)
   {
      delete mResponseHeader;
   }
}

bool BtpMessage::isHeaderSecurityPresent(HttpHeader* header)
{
   string userId;
   return header->getField("Btp-User-Id", userId);
}

void BtpMessage::checkHeaderSecurity(HttpHeader* header)
{
   // get btp information
   string version;
   string userId;
   string agentUserId;
   string agentProfileId;
   header->getField("Btp-Version", version);
   header->getField("Btp-User-Id", userId);
   header->getField("Btp-Agent-User-Id", agentUserId);
   header->getField("Btp-Agent-Profile-Id", agentProfileId);

   // get btp header signature
   string hs;
   header->getField("Btp-Header-Signature", hs);

   if(userId.length() > 0 &&
      agentUserId.length() > 0 && agentProfileId.length() > 0)
   {
      // convert ID strings
      mUserId = strtoull(userId.c_str(), NULL, 10);
      mAgentUserId = strtoull(agentUserId.c_str(), NULL, 10);
      mAgentProfileId = strtoull(agentProfileId.c_str(), NULL, 10);
   }
   else
   {
      // reset IDs
      mUserId = 0;
      mAgentUserId = 0;
      mAgentProfileId = 0;
      hs.erase();
   }

   // only do full security check if public key source is not NULL
   if(getPublicKeySource() != NULL)
   {
      if(mUserId == 0 ||
         mAgentUserId == 0 || mAgentProfileId == 0 || hs.length() == 0)
      {
         MO_CAT_DEBUG(BM_PROTOCOL_CAT,
            "Btp breach. Secure message missing user ID, "
            "agent user ID, profile ID, or header signature.");

         // security breach
         setSecurityStatus(BtpMessage::Breach);
      }
      else if(strcmp(version.c_str(), "1.0") != 0)
      {
         MO_CAT_DEBUG(BM_PROTOCOL_CAT,
            "Btp breach. Secure message using unsupported BTP version '%s'.",
            version.c_str());

         // security breach
         setSecurityStatus(BtpMessage::Breach);
      }
      else
      {
         // get agent's public key
         bool isDelegate;
         PublicKeyRef publicKey = getPublicKeySource()->getPublicKey(
            mAgentUserId, mAgentProfileId, &isDelegate);
         if(!publicKey.isNull())
         {
            // do delegate check (if agent != user then delegate must be true)
            if(mAgentUserId != mUserId && !isDelegate)
            {
               MO_CAT_DEBUG(BM_PROTOCOL_CAT,
                  "Btp breach. Secure message uses an agent user ID "
                  "acting as a delegate that is not registered as a delegate.");

               // breach! agent is not a delegate but is acting as one
               setSecurityStatus(BtpMessage::Breach);
            }
            else
            {
               // create digital signature for verification
               DigitalSignature ds(publicKey);

               // create header-sig data by concatenating start line and host
               string data;
               string host;
               header->getStartLine(data);
               // Note: use proxy'd host field if available
               if(!header->getField("X-Forwarded-Host", host))
               {
                  header->getField("Host", host);
               }
               data.append(host);
               ds.update(data.c_str(), data.length());

               // verify signature
               if(verifySignature(&ds, hs))
               {
                  setSecurityStatus(BtpMessage::Secure);
               }
               else
               {
                  MO_CAT_DEBUG(BM_PROTOCOL_CAT,
                     "Btp breach. Header signature does not match.");
                  setSecurityStatus(BtpMessage::Breach);
               }
            }
         }
         else
         {
            MO_CAT_DEBUG(BM_PROTOCOL_CAT,
               "Btp breach. Agent does not have a public key or it could not "
               "be retrieved.");

            // security failure
            setSecurityStatus(BtpMessage::Breach);
         }
      }
   }
   else
   {
      // security not checked
      setSecurityStatus(BtpMessage::Unchecked);
   }
}

void BtpMessage::checkContentSecurity(
   HttpHeader* header, HttpTrailer* trailer, DigitalSignature* ds)
{
   // check security if digital signature is present
   if(ds != NULL)
   {
      // get content length
      int64_t contentLength = 0;
      if(trailer != NULL)
      {
         contentLength = trailer->getContentLength();
      }
      else
      {
         header->getField("Content-Length", contentLength);
      }

      // only check security if content length is > 0
      if(contentLength > 0)
      {
         // get content signature from trailer or header
         string cs;
         if(trailer == NULL || !trailer->getField("Btp-Content-Signature", cs))
         {
            header->getField("Btp-Content-Signature", cs);
         }

         // verify signature
         if(cs.length() > 0 && verifySignature(ds, cs))
         {
            // security good
            setSecurityStatus(BtpMessage::Secure);
         }
         else
         {
            MO_CAT_DEBUG(BM_PROTOCOL_CAT,
               "Btp breach. Message content signature did not match.");

            // security breach
            setSecurityStatus(BtpMessage::Breach);
         }
      }
   }
}

void BtpMessage::setupRequestHeader(Url* url, HttpRequestHeader* header)
{
   // sync types between header and message
   if(mType != Undefined)
   {
      // set request method
      const char* type = typeToString(mType);
      header->setMethod(type);
   }
   else
   {
      // set type
      const char* method = header->getMethod();
      mType = stringToType(method);
   }

   // set base request header
   header->setPath(url->getPathAndQuery().c_str());
   header->setVersion("HTTP/1.1");
   header->setField("Host", url->getAuthority());
   header->setField("User-Agent", "BtpClient/1.0");
   header->setField("Accept-Encoding", "deflate, gzip");

   // add accept for json if not found
   if(!header->hasField("Accept"))
   {
      header->setField("Accept", "application/json");
   }
}

bool BtpMessage::sendHeader(
   HttpConnection* hc, HttpHeader* header,
   OutputStreamRef& os, HttpTrailerRef& trailer)
{
   bool rval;

   // set references to null
   os.setNull();
   trailer.setNull();

   DigitalSignature* ds = NULL;

   if(!mAgentProfile.isNull())
   {
      // get signature
      ds = mAgentProfile->createSignature();
   }

   // add btp headers
   addHeaders(header, ds);

   // send header
   if((rval = hc->sendHeader(header)))
   {
      // print out header
      MO_CAT_DEBUG(BM_PROTOCOL_CAT,
         "BtpMessage sent header to %s:%i:\n%s",
         hc->getRemoteAddress()->getAddress(),
         hc->getRemoteAddress()->getPort(),
         header->toString().c_str());

      // get body output stream, if applicable
      mTrailer = trailer = new BtpTrailer(ds);
      OutputStream* out = hc->getBodyOutputStream(header, &(*mTrailer));

      // use deflating/gzip if available
      string contentEncoding;
      if(header->getField("Content-Encoding", contentEncoding))
      {
         if(strstr(contentEncoding.c_str(), "deflate") != NULL)
         {
            // create deflater to deflate content
            Deflater* def = new Deflater();
            def->startDeflating(-1, false);
            out = new MutatorOutputStream(out, true, def, true);
         }
         else if(strstr(contentEncoding.c_str(), "gzip") != NULL)
         {
            // create gzipper to deflate content
            Gzipper* gzipper = new Gzipper();
            gzipper->startCompressing();
            out = new MutatorOutputStream(out, true, gzipper, true);
         }
      }

      if(ds != NULL)
      {
         // wrap body with digital signature output stream, make sure it
         // will clean up the signature and the underlying stream
         out = new DigitalSignatureOutputStream(ds, true, out, true);
      }

      // set reference to point at output stream
      os = out;
   }
   else if(ds != NULL)
   {
      // clean up digital signature if sending header failed
      delete ds;
   }

   return rval;
}

bool BtpMessage::send(Url* url, HttpRequest* request)
{
   bool rval;

   // update request header
   setupRequestHeader(url, request->getHeader());

   if(!mDynamicObject.isNull())
   {
      rval = sendHeaderAndObject(
         request->getConnection(), request->getHeader(), mDynamicObject);
   }
   else
   {
      rval = sendHeaderAndStream(
         request->getConnection(), request->getHeader(), mContentSource);
   }

   return rval;
}

bool BtpMessage::send(HttpResponse* response)
{
   bool rval;

   if(!mDynamicObject.isNull())
   {
      rval = sendHeaderAndObject(
         response->getConnection(), response->getHeader(), mDynamicObject);
   }
   else
   {
      rval = sendHeaderAndStream(
         response->getConnection(), response->getHeader(), mContentSource);
   }

   return rval;
}

bool BtpMessage::receiveContent(HttpRequest* request)
{
   bool rval = true;

   // update message type
   setType(stringToType(request->getHeader()->getMethod()));

   // check to see if there is content to receive
   long long length = 0;
   request->getHeader()->getField("Content-Length", length);
   if(length != 0 || request->getHeader()->hasField("Transfer-Encoding"))
   {
      if(!mDynamicObject.isNull())
      {
         rval = receiveContentObject(
            request->getConnection(), request->getHeader(), mDynamicObject);
      }
      else if(mContentSink != NULL)
      {
         rval = receiveContentStream(
            request->getConnection(), request->getHeader(), mContentSink);

         // close content sink as appropriate
         if(mCloseSink && mContentSink != NULL)
         {
            mContentSink->close();
         }
      }
   }

   return rval;
}

bool BtpMessage::receiveContent(HttpResponse* response)
{
   bool rval = true;

   // check to see if there is content to receive
   long long length = 0;
   response->getHeader()->getField("Content-Length", length);
   if(length != 0 || response->getHeader()->hasField("Transfer-Encoding"))
   {
      // check status code for other than success
      if(response->getHeader()->getStatusCode() >= 400)
      {
         // receive content as a dynamic object
         // (which will be translated to a exception)
         if(mDynamicObject.isNull())
         {
            // create new object
            mDynamicObject = DynamicObject();
         }
         else
         {
            // clear old object
            mDynamicObject->clear();
         }

         if((rval = receiveContentObject(
            response->getConnection(), response->getHeader(), mDynamicObject)))
         {
            // create Exception from DynamicObject, return false
            ExceptionRef e = Exception::convertToException(mDynamicObject);
            Exception::set(e);
            rval = false;
         }
      }
      else if(!mDynamicObject.isNull())
      {
         rval = receiveContentObject(
            response->getConnection(), response->getHeader(), mDynamicObject);
      }
      else if(getContentSink() != NULL)
      {
         rval = receiveContentStream(
            response->getConnection(), response->getHeader(), mContentSink);

         // close content sink as appropriate
         if(mCloseSink && mContentSink != NULL)
         {
            mContentSink->close();
         }
      }
   }

   return rval;
}

void BtpMessage::getContentReceiveStream(
   HttpRequest* request,
   InputStreamRef& is, HttpTrailerRef& trailer, DigitalSignatureRef& ds)
{
   getContentReceiveStream(
      request->getConnection(), request->getHeader(), is, trailer, ds);
}

void BtpMessage::getContentReceiveStream(
   HttpResponse* response,
   InputStreamRef& is, HttpTrailerRef& trailer, DigitalSignatureRef& ds)
{
   getContentReceiveStream(
      response->getConnection(), response->getHeader(), is, trailer, ds);
}

void BtpMessage::setType(BtpMessage::Type type)
{
   mType = type;
}

BtpMessage::Type BtpMessage::getType()
{
   return mType;
}

void BtpMessage::setSecurityStatus(BtpMessage::SecurityStatus status)
{
   mSecurityStatus = status;
}

BtpMessage::SecurityStatus BtpMessage::getSecurityStatus() const
{
   return mSecurityStatus;
}

void BtpMessage::setUserId(UserId id)
{
   mUserId = id;
}

UserId BtpMessage::getUserId() const
{
   return mUserId;
}

UserId BtpMessage::getAgentUserId() const
{
   return mAgentUserId;
}

ProfileId BtpMessage::getAgentProfileId() const
{
   return mAgentProfileId;
}

void BtpMessage::setAgentProfile(ProfileRef& p)
{
   mAgentProfile = p;
}

ProfileRef& BtpMessage::getAgentProfile()
{
   return mAgentProfile;
}

void BtpMessage::setPublicKeySource(PublicKeySource* source)
{
   mPublicKeySource = source;
}

PublicKeySource* BtpMessage::getPublicKeySource()
{
   return mPublicKeySource;
}

void BtpMessage::setContentType(const char* contentType)
{
   if(mContentType != NULL)
   {
      free(mContentType);
      mContentType = NULL;
   }

   if(contentType != NULL)
   {
      mContentType = strdup(contentType);
   }
}

const char* BtpMessage::getContentType()
{
   return mContentType;
}

DynamicObject& BtpMessage::getCustomHeaders()
{
   if(mCustomHeaders.isNull())
   {
      mCustomHeaders = DynamicObject();
      mCustomHeaders->setType(Map);
   }

   return mCustomHeaders;
}

void BtpMessage::setContentSource(InputStream* is)
{
   mContentSource = is;
   mDynamicObject.setNull();
}

InputStream* BtpMessage::getContentSource()
{
   return mContentSource;
}

void BtpMessage::setContentSink(OutputStream* os, bool close)
{
   mContentSink = os;
   mCloseSink = close;
   mDynamicObject.setNull();
}

OutputStream* BtpMessage::getContentSink()
{
   return mContentSink;
}

void BtpMessage::setDynamicObject(DynamicObject& dyno, const char* contentType)
{
   mDynamicObject = dyno;
   mContentSource = NULL;
   mContentSink = NULL;

   if(contentType == NULL)
   {
      // default to JSON
      setContentType("application/json");
   }
   else
   {
      setContentType(contentType);
   }
}

DynamicObject& BtpMessage::getDynamicObject()
{
   return mDynamicObject;
}

HttpTrailerRef& BtpMessage::getTrailer()
{
   return mTrailer;
}

void BtpMessage::setSaveHeaders(bool save)
{
   mSaveHeaders = save;
}

HttpRequestHeader* BtpMessage::getRequestHeader()
{
   if(mRequestHeader == NULL)
   {
      mRequestHeader = new HttpRequestHeader();
   }

   return mRequestHeader;
}

HttpResponseHeader* BtpMessage::getResponseHeader()
{
   if(mResponseHeader == NULL)
   {
      mResponseHeader = new HttpResponseHeader();
   }

   return mResponseHeader;
}

BtpMessage::Type BtpMessage::stringToType(const char* type)
{
   Type rval;

   if(strcmp(type, "GET") == 0)
   {
      rval = Get;
   }
   else if(strcmp(type, "PUT") == 0)
   {
      rval = Put;
   }
   else if(strcmp(type, "POST") == 0)
   {
      rval = Post;
   }
   else if(strcmp(type, "DELETE") == 0)
   {
      rval = Delete;
   }
   else if(strcmp(type, "HEAD") == 0)
   {
      rval = Head;
   }
   else if(strcmp(type, "OPTIONS") == 0)
   {
      rval = Options;
   }
   else if(strcmp(type, "TRACE") == 0)
   {
      rval = Trace;
   }
   else if(strcmp(type, "CONNECT") == 0)
   {
      rval = Connect;
   }
   else
   {
      rval = Undefined;
   }

   return rval;
}

const char* BtpMessage::typeToString(BtpMessage::Type type)
{
   const char* rval = NULL;

   switch(type)
   {
      case BtpMessage::Get:
         rval = "GET";
         break;
      case BtpMessage::Put:
         rval = "PUT";
         break;
      case BtpMessage::Post:
         rval = "POST";
         break;
      case BtpMessage::Delete:
         rval = "DELETE";
         break;
      case BtpMessage::Head:
         rval = "HEAD";
         break;
      case BtpMessage::Options:
         rval = "OPTIONS";
         break;
      case BtpMessage::Trace:
         rval = "TRACE";
         break;
      case BtpMessage::Connect:
         rval = "CONNECT";
         break;
      case BtpMessage::Undefined:
         rval = NULL;
         break;
   }

   return rval;
}

bool BtpMessage::checkContentType(
   HttpHeader* header, DynoContentType& type, bool send)
{
   bool rval = true;

   // set content type if appropriate
   if(send)
   {
      if(getContentType() != NULL)
      {
         header->setField("Content-Type", getContentType());
      }
   }

   // get header content-type
   type = Invalid;
   string contentType;
   if(header->getField("Content-Type", contentType))
   {
      // Check prefix, ignore options such as charset
      if(strncmp(contentType.c_str(), "application/json", 16) == 0)
      {
         type = Json;
      }
      else if(strncmp(contentType.c_str(), "text/xml", 8) == 0)
      {
         type = Xml;
      }
      else if(strncmp(
         contentType.c_str(), "application/x-www-form-urlencoded", 33) == 0)
      {
         type = Form;
      }
   }

   if(type == Invalid)
   {
      // unsupported content-type
      ExceptionRef e = new Exception(
         "Unsupported Content-Type for BtpMessage using DynamicObject.",
         "bitmunk.protocol.InvalidContentType");
      e->getDetails()["contentType"] = contentType.c_str();
      Exception::set(e);
      rval = false;
   }

   return rval;
}

void BtpMessage::addHeaders(HttpHeader* header, DigitalSignature* ds)
{
   // add any custom headers
   if(!mCustomHeaders.isNull() && mCustomHeaders->getType() == Map)
   {
      DynamicObjectIterator i = mCustomHeaders.getIterator();
      while(i->hasNext())
      {
         DynamicObject& field = i->next();
         const char* name = i->getName();

         // add all values if array
         if(field->getType() == Array)
         {
            DynamicObjectIterator ii = field.getIterator();
            while(ii->hasNext())
            {
               DynamicObject& next = i->next();
               if(next->getType() != Array && next->getType() != Map)
               {
                  header->addField(name, next->getString());
               }
            }
         }
         // any type but map is legal
         else if(field->getType() != Map)
         {
            header->addField(name, field->getString());
         }
      }
   }

   // handle response transfer-encoding
   if(header->getType() == HttpHeader::Response)
   {
      bool httpVersion10 = (strcmp(header->getVersion(), "HTTP/1.0") == 0);
      if(!httpVersion10 && (!mDynamicObject.isNull() || mContentSource != NULL))
      {
         // use chunked encoding if no length is set
         if(!header->hasField("Content-Length"))
         {
            header->setField("Transfer-Encoding", "chunked");
         }
      }
   }
   // handle request transfer-encoding
   else if(!mDynamicObject.isNull() || mContentSource != NULL)
   {
      // add connection header if missing
      string connection;
      header->getField("Connection", connection);
      if(connection.empty())
      {
         // by default, close connection
         connection = "close";
         header->setField("Connection", connection.c_str());
      }

      if(strstr(connection.c_str(), ", TE") == NULL)
      {
         connection.append(", TE");
         header->setField("Connection", connection.c_str());
      }

      // append ", TE" to connection field
      header->setField("TE", "trailers, chunked");

      // use chunked encoding if no length is set
      if(!header->hasField("Content-Length"))
      {
         header->setField("Transfer-Encoding", "chunked");
      }
   }

   // if message is secure, include security headers
   if(!mAgentProfile.isNull())
   {
      mAgentUserId = mAgentProfile->getUserId();
      mAgentProfileId = mAgentProfile->getId();
      header->setField("Btp-Version", "1.0");
      header->setField("Btp-User-Id", mUserId);
      header->setField("Btp-Agent-User-Id", mAgentUserId);
      header->setField("Btp-Agent-Profile-Id", mAgentProfileId);

      // create header-sig data by concatenating start line and host
      string data;
      string host;
      header->getStartLine(data);
      header->getField("Host", host);
      data.append(host);

      // get signature and reset
      ds->update(data.c_str(), data.length());
      char sig[ds->getValueLength()];
      unsigned int length;
      ds->getValue(sig, length);
      ds->reset();

      // add btp-header signature header
      header->setField(
         "Btp-Header-Signature", Convert::bytesToHex(sig, length).c_str());

      // FIXME: Not fully implemented! If we are sending Content-Length,
      // then we must also send Btp-Content-Signature ... which means
      // we must calculate the content signature on the output and include
      // it in the header, which means either spooling the output into
      // memory or a temporary file or double-processing it. The api
      // for BtpMessage may need to change entirely to make this is a
      // more straightforward process.

      // see if content will be sent over chunked encoding
      string chunked;
      if(header->hasField("Content-Type") &&
         header->getField("Transfer-Encoding", chunked) &&
         strcmp(chunked.c_str(), "chunked") == 0)
      {
         // notify that Btp-Content-Signature field will be included in trailer
         header->setField("Trailer", "Btp-Content-Signature");
      }
   }

   if(mSaveHeaders)
   {
      HttpHeader* save;
      if(header->getType() == HttpHeader::Response)
      {
         // save response header
         save = getRequestHeader();
      }
      else
      {
         // save request header
         save = getResponseHeader();
      }

      save->clearFields();
      header->writeTo(save);
   }
}

bool BtpMessage::verifySignature(DigitalSignature* ds, std::string& signature)
{
   bool rval;

   // decode from hex to binary
   char decoded[signature.length()];
   unsigned int length;
   rval = Convert::hexToBytes(
      signature.c_str(), signature.length(), decoded, length);

   if(rval)
   {
      // verify signature
      rval = ds->verify(decoded, length);
   }

   return rval;
}

bool BtpMessage::sendHeaderAndStream(
   HttpConnection* hc, HttpHeader* header, InputStream* is)
{
   bool rval;

   // create digital signature if a profile is set
   DigitalSignature* ds = NULL;
   DigitalSignatureInputStream dsis(NULL, false, is, false);
   if(!mAgentProfile.isNull())
   {
      // get signature and input stream
      ds = mAgentProfile->createSignature();
      dsis.setSignature(ds, true);

      // update input stream if not NULL
      if(is != NULL)
      {
         is = &dsis;
      }
   }

   // add btp headers
   addHeaders(header, ds);

   // send header
   if((rval = hc->sendHeader(header)))
   {
      // print out header
      MO_CAT_DEBUG(BM_PROTOCOL_CAT,
         "BtpMessage sent header to %s:%i:\n%s",
         hc->getRemoteAddress()->getAddress(),
         hc->getRemoteAddress()->getPort(),
         header->toString().c_str());

      // send body a content stream if input stream present
      if(is != NULL)
      {
         // use deflating/gzip if available
         MutatorInputStream mis(is, false, NULL, false);
         string contentEncoding;
         if(header->getField("Content-Encoding", contentEncoding))
         {
            if(strstr(contentEncoding.c_str(), "deflate") != NULL)
            {
               // create deflater to deflate content
               Deflater* def = new Deflater();
               def->startDeflating(-1, false);
               mis.setAlgorithm(def, true);
               is = &mis;
            }
            else if(strstr(contentEncoding.c_str(), "gzip") != NULL)
            {
               // create gzipper to deflate content
               Gzipper* gzipper = new Gzipper();
               gzipper->startCompressing();
               mis.setAlgorithm(gzipper, true);
               is = &mis;
            }
         }

         // send body
         mTrailer = new BtpTrailer(ds);
         Timer timer;
         timer.start();
         hc->setContentBytesWritten(0);
         if((rval = hc->sendBody(header, is, &(*mTrailer))) &&
            hc->getContentBytesWritten() > 0)
         {
            // log send time
            MO_CAT_DEBUG(BM_PROTOCOL_CAT,
               "BtpMessage sent stream content, %" PRIu64 " bytes "
               "in %" PRIu64 " ms.",
               hc->getContentBytesWritten(),
               timer.getElapsedMilliseconds());

            // print out trailer
            MO_CAT_DEBUG(BM_PROTOCOL_CAT,
               "BtpMessage sent trailer to %s:%i:\n%s",
               hc->getRemoteAddress()->getAddress(),
               hc->getRemoteAddress()->getPort(),
               mTrailer->toString().c_str());
         }
      }
   }

   return rval;
}

bool BtpMessage::sendHeaderAndObject(
   HttpConnection* hc, HttpHeader* header, DynamicObject& dyno)
{
   bool rval;

   // check content-type
   DynoContentType type;
   if((rval = checkContentType(header, type, true)))
   {
      // send header
      OutputStreamRef os;
      if((rval = sendHeader(hc, header, os, mTrailer)))
      {
         if(!os.isNull())
         {
            // create dynamic object writer according to data format
            if(type != Form)
            {
               DynamicObjectWriter* writer;
               if(type == Json)
               {
                  writer = new JsonWriter();
               }
               else
               {
                  writer = new XmlWriter();
               }
               writer->setCompact(true);

               // write message
               Timer timer;
               timer.start();
               hc->setContentBytesWritten(0);
               if((rval = writer->write(dyno, &(*os)) && os->finish()))
               {
                  // log send time
                  MO_CAT_DEBUG(BM_PROTOCOL_CAT,
                     "BtpMessage sent object content to %s:%i, "
                     "%" PRIu64 " bytes in %" PRIu64 " ms.",
                     hc->getRemoteAddress()->getAddress(),
                     hc->getRemoteAddress()->getPort(),
                     hc->getContentBytesWritten(),
                     timer.getElapsedMilliseconds());
               }

               // close output stream
               os->close();

               // cleanup writer
               delete writer;
            }
            else
            {
               // write out x-www-form-urlencoded data
               string form = Url::formEncode(dyno).c_str();
               Timer timer;
               timer.start();
               if((rval = os->write(form.c_str(), form.length()) &&
                  os->finish()))
               {
                  // log send time
                  MO_CAT_DEBUG(BM_PROTOCOL_CAT,
                     "BtpMessage sent object content to %s:%i, "
                     "%" PRIu64 " bytes in %" PRIu64 " ms.",
                     hc->getRemoteAddress()->getAddress(),
                     hc->getRemoteAddress()->getPort(),
                     hc->getContentBytesWritten(),
                     timer.getElapsedMilliseconds());
               }

               // close output stream
               os->close();
            }
         }
      }
   }

   return rval;
}

bool BtpMessage::receiveContentStream(
   HttpConnection* hc, HttpHeader* header, OutputStream* os)
{
   bool rval;

   // add security check for content if public key source is not NULL
   PublicKeyRef publicKey(NULL);
   DigitalSignature* ds = NULL;
   DigitalSignatureOutputStream dsos(NULL, false, os, false);
   if(getPublicKeySource() != NULL)
   {
      // create digital signature to check content
      publicKey = getPublicKeySource()->getPublicKey(
         mAgentUserId, mAgentProfileId, NULL);
      if(!publicKey.isNull())
      {
         ds = new DigitalSignature(publicKey);
         dsos.setSignature(ds, true);
         os = &dsos;
      }
      else
      {
         MO_CAT_DEBUG(BM_PROTOCOL_CAT,
            "Btp breach. Agent does not have a public key or it could not "
            "be retrieved.");

         // security failure
         setSecurityStatus(BtpMessage::Breach);
      }
   }

   // handle inflating if necessary
   MutatorOutputStream mos(os, false, NULL, false);
   string contentEncoding;
   if(header->getField("Content-Encoding", contentEncoding))
   {
      if(strstr(contentEncoding.c_str(), "deflate") != NULL ||
         strstr(contentEncoding.c_str(), "gzip") != NULL)
      {
         // create deflater to inflate content (works for zlib OR gzip)
         Deflater* def = new Deflater();
         def->startInflating(false);
         mos.setAlgorithm(def, true);
         os = &mos;
      }
   }

   // receive content
   mTrailer = new BtpTrailer(ds);
   Timer timer;
   timer.start();
   rval = hc->receiveBody(header, os, &(*mTrailer));

   // log receive time
   if(mTrailer->getContentLength() > 0)
   {
      MO_CAT_DEBUG(BM_PROTOCOL_CAT,
         "BtpMessage received content from %s:%i, %" PRIu64 " bytes "
         "in %" PRIu64 " ms.",
         hc->getRemoteAddress()->getAddress(),
         hc->getRemoteAddress()->getPort(),
         mTrailer->getContentLength(),
         timer.getElapsedMilliseconds());

      // print out trailer
      if(mTrailer->getFieldCount() > 0)
      {
         MO_CAT_DEBUG(BM_PROTOCOL_CAT,
            "BtpMessage received trailer from %s:%i:\n%s",
            hc->getRemoteAddress()->getAddress(),
            hc->getRemoteAddress()->getPort(),
            mTrailer->toString().c_str());
      }
      else
      {
         MO_CAT_DEBUG(BM_PROTOCOL_CAT,
            "BtpMessage received no trailer from %s:%i.",
            hc->getRemoteAddress()->getAddress(),
            hc->getRemoteAddress()->getPort());
      }
   }
   else
   {
      MO_CAT_DEBUG(BM_PROTOCOL_CAT,
         "BtpMessage received no content from %s:%i.",
         hc->getRemoteAddress()->getAddress(),
         hc->getRemoteAddress()->getPort());
   }

   // check content security
   if(rval)
   {
      checkContentSecurity(header, &(*mTrailer), ds);
   }

   return rval;
}

bool BtpMessage::receiveContentObject(
   HttpConnection* hc, HttpHeader* header, DynamicObject& dyno)
{
   bool rval;

   // check content-type
   DynoContentType type;
   if((rval = checkContentType(header, type, false)))
   {
      if(type != Form)
      {
         // create dynamic object reader based on data format
         DynamicObjectReader* reader;
         if(type == Json)
         {
            reader = new JsonReader();
         }
         else
         {
            reader = new XmlReader();
         }

         // use dynamic object output stream as content sink
         DynamicObjectOutputStream doos(dyno, reader, true);
         if((rval = receiveContentStream(hc, header, &doos)))
         {
            doos.close();
         }
      }
      else
      {
         // read content as a string and parse as x-www-form-urlencoded data
         ByteBuffer b(512);
         ByteArrayOutputStream baos(&b, true);
         if((rval = receiveContentStream(hc, header, &baos)))
         {
            b.putByte('\0', 1, true);
            Url::formDecode(dyno, b.data());
         }
      }
   }

   return rval;
}

void BtpMessage::getContentReceiveStream(
   HttpConnection* hc, HttpHeader* header,
   InputStreamRef& is, HttpTrailerRef& trailer, DigitalSignatureRef& ds)
{
   // see if message is secure
   ds.setNull();
   if(getPublicKeySource() != NULL)
   {
      // create digital signature to check content
      PublicKeyRef publicKey = getPublicKeySource()->getPublicKey(
         mAgentUserId, mAgentProfileId, NULL);
      if(!publicKey.isNull())
      {
         // create signature
         ds = new DigitalSignature(publicKey);
      }
      else
      {
         MO_CAT_DEBUG(BM_PROTOCOL_CAT,
            "Btp breach. Agent does not have a public key or it could not "
            "be retrieved.");

         // security failure
         setSecurityStatus(BtpMessage::Breach);
      }
   }

   // create trailer
   mTrailer = trailer = new BtpTrailer(ds.isNull() ? NULL : &(*ds));

   // get body input stream
   InputStream* in = hc->getBodyInputStream(header, &(*mTrailer));

   // handle inflating if necessary
   string contentEncoding;
   if(header->getField("Content-Encoding", contentEncoding))
   {
      if(strstr(contentEncoding.c_str(), "deflate") != NULL ||
         strstr(contentEncoding.c_str(), "gzip") != NULL)
      {
         // create deflater to inflate content (works for zlib OR gzip)
         Deflater* def = new Deflater();
         def->startInflating(false);
         in = new MutatorInputStream(in, true, def, true);
      }
   }

   if(!ds.isNull())
   {
      // create digital signature input stream wrapper (DigitalSignature is
      // automatically cleaned up)
      is = new DigitalSignatureInputStream(&(*ds), false, in, true);
   }
   else
   {
      // no wrapper needed
      is = in;
   }
}

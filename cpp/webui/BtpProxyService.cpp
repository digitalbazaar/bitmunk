/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk/webui/BtpProxyService.h"

#include "bitmunk/common/Logging.h"
#include "bitmunk/common/Tools.h"
#include "bitmunk/node/BtpActionDelegate.h"
#include "bitmunk/node/RestResourceHandler.h"
#include "monarch/validation/Validation.h"

using namespace std;
using namespace monarch::config;
using namespace monarch::crypto;
using namespace monarch::http;
using namespace monarch::io;
using namespace monarch::net;
using namespace monarch::rt;
using namespace bitmunk::common;
using namespace bitmunk::protocol;
using namespace bitmunk::node;
using namespace bitmunk::webui;
namespace v = monarch::validation;

typedef BtpActionDelegate<BtpProxyService> Handler;

BtpProxyService::BtpProxyService(SessionManager* sm, const char* path) :
   BtpService(path),
   mSessionManager(sm),
   mConnectionPool(new HttpConnectionPool())

{
}

BtpProxyService::~BtpProxyService()
{
}

bool BtpProxyService::initialize()
{
   // root
   {
      RestResourceHandlerRef proxyResource = new RestResourceHandler();
      addResource("/", proxyResource);

      // METHOD .../proxy
      ResourceHandler h = new Handler(
         mSessionManager->getNode(), this, &BtpProxyService::proxy,
         BtpAction::AuthOptional);

      v::ValidatorRef queryValidator = new v::Map(
         "url", new v::Optional(new v::All(
            new v::Type(String),
            new v::Min(1, "Btp service URL too short. 1 character minimum."),
            NULL)),
         "timeout", new v::Optional(new v::Int(v::Int::NonNegative)),
         NULL);

      proxyResource->addHandler(h, BtpMessage::Post, 0, &queryValidator);
      proxyResource->addHandler(h, BtpMessage::Put, 0, &queryValidator);
      proxyResource->addHandler(h, BtpMessage::Get, 0, &queryValidator);
      proxyResource->addHandler(h, BtpMessage::Delete, 0, &queryValidator);
      proxyResource->addHandler(h, BtpMessage::Head, 0, &queryValidator);
      proxyResource->addHandler(h, BtpMessage::Options, 0, &queryValidator);
   }

   return true;
}

void BtpProxyService::cleanup()
{
   // remove resources
   removeResource("/");

   // clean up connection pool
   mConnectionPool.setNull();
}

static bool _getProxyTimeout(
   DynamicObject& params, DynamicObject& vars, HttpHeader* header)
{
   // get optional proxy timeout
   string value;
   if(vars->hasMember("timeout"))
   {
      params["timeout"] = vars["timeout"]->getUInt32();
   }
   else if(header->getField("Btp-Proxy-Timeout", value))
   {
      params["timeout"] = (uint32_t)strtoul(value.c_str(), NULL, 10);
   }
   else
   {
      // default to 30 seconds
      params["timeout"] = 30;
   }

   return true;
}

static bool _getProxyUrl(
   BtpAction* action,
   DynamicObject& params, Node* node, DynamicObject& vars, HttpHeader* header)
{
   bool rval = true;

   // get url to btp service
   string value;
   if(vars->hasMember("url") && vars["url"]->length() > 0)
   {
      params["url"] = vars["url"]->getString();
   }
   else if(header->getField("Btp-Proxy-Url", value) && value.length() > 0)
   {
      params["url"] = value.c_str();
   }
   else
   {
      ExceptionRef e = new Exception(
         "Missing 'Btp-Proxy-Url' header.",
         "bitmunk.webui.MissingProxyHeader");
      e->getDetails()["missingHeader"] = "Btp-Proxy-Url";
      Exception::set(e);
      rval = false;
   }

   if(rval)
   {
      // convert simple path into full local url if needed
      if(params["url"]->getString()[0] == '/')
      {
         // build full url
         Config cfg = node->getConfigManager()->getNodeConfig();
         InternetAddress addr;
         action->getRequest()->getConnection()->writeLocalAddress(&addr);
         value.append("https://");
         value.append(addr.getAddress());
         value.push_back(':');
         value.append(cfg["port"]->getString());
         value.append(params["url"]->getString());
         params["url"] = value.c_str();
      }
   }

   return rval;
}

static bool _getProxyParams(
   BtpAction* action,
   DynamicObject& params, Node* node, DynamicObject& vars, HttpHeader* header)
{
   bool rval = true;

   rval =
      _getProxyTimeout(params, vars, header) &&
      _getProxyUrl(action, params, node, vars, header);

   // ensure url is valid
   if(rval)
   {
      Exception::clear();
      Url tmp(params["url"]->getString());
      if(Exception::isSet())
      {
         ExceptionRef e = new Exception(
            "Invalid BTP service URL.",
            "bitmunk.webui.InvalidBtpServiceUrl");
         e->getDetails()["url"] = params["url"];
         Exception::push(e);
         rval = false;
      }
   }

   return rval;
}

/**
 * Gets a btp connection to the service to proxy the client's request to.
 *
 * @param btpc a BtpClient to use.
 * @param pool a connection pool to use.
 * @param url the url to connect to.
 * @param params the proxy params to use.
 *
 * @return the connection or NULL if none could be established.
 */
static HttpConnectionRef _connect(
   BtpClient* btpc, HttpConnectionPoolRef& pool,
   Url* url, DynamicObject& params)
{
   HttpConnectionRef pConn(NULL);

   UserId userId = params["userId"]->getUInt64();

   // keep trying to get a connection with up to 1 retry
   int retries = 0;
   while(pConn.isNull() && retries <= 1)
   {
      // attempt to get an idle connection from the pool first
      if(userId == 0)
      {
         pConn = pool->getConnection(url);
      }
      else
      {
         pConn = pool->getConnection(
            url, Tools::getBitmunkUserCommonName(userId).c_str());
      }
      if(pConn.isNull())
      {
         // no connection available from the pool, so establish one
         pConn = btpc->createConnection(userId, url);
      }
      else
      {
         MO_CAT_DEBUG(BM_PROTOCOL_CAT,
            "BtpProxyService reusing existing keep-alive connection to: %s",
            url->toString().c_str());
      }

      if(pConn.isNull())
      {
         retries++;
         MO_CAT_DEBUG(BM_PROTOCOL_CAT,
            "BtpProxyService retrying proxy connection to: %s",
            url->toString().c_str());
      }
   }

   if(!pConn.isNull())
   {
      // set read/write timeouts (this is done here because if we are
      // are reusing a connection, its timeout may have been set differently
      // when it was last used ... so this will always update it properly)
      // get timeout in ms
      uint32_t timeout = params["timeout"]->getUInt32() * 1000;
      pConn->setReadTimeout(timeout);
      pConn->setWriteTimeout(timeout);
   }

   return pConn;
}

/**
 * Sets up the request to proxy based on the client's request.
 *
 * @param client the client's request header.
 * @param proxy the request header to proxy to the btp service.
 */
static void _setupRequestHeader(
   HttpRequestHeader* client, HttpRequestHeader* proxy)
{
   // setup request header to send to btp service
   proxy->setField("Transfer-Encoding", "chunked");
   proxy->setField("TE", "trailers, chunked");

   string value;
   if(client->getField("Connection", value))
   {
      value.append(", TE");
   }
   else
   {
      value = "TE";
   }
   proxy->setField("Connection", value.c_str());

   // include content-type from client's request header
   if(client->getField("Content-Type", value))
   {
      proxy->setField("Content-Type", value.c_str());
   }

   // include accept field from client's request header
   if(client->getField("Accept", value))
   {
      proxy->setField("Accept", value.c_str());
   }
}

/**
 * Proxies the client's request to the btp service or to NULL if the
 * passed btp message is NULL.
 *
 * @param action the incoming btp action from the client.
 * @param url the url to the btp service.
 * @param pOut the btp message for sending to the btp service.
 * @param pRequest the request to send to the btp service.
 *
 * @return true if successful, false if an exception occurred.
 */
static bool _proxyClientRequest(
   BtpAction* action, Url* url, BtpMessage* pOut, HttpRequest* pRequest)
{
   bool rval = true;

   // get client's request and header
   HttpRequest* cRequest = action->getRequest();
   HttpRequestHeader* cHeader = cRequest->getHeader();

   // see if there is content from the client to be proxied
   if(!cHeader->hasContent())
   {
      // send out message, no content to proxy
      if(pOut != NULL)
      {
         rval = pOut->send(url, pRequest);
      }
   }
   else
   {
      // there is request content to proxy, so get receive content stream
      InputStreamRef is;
      HttpTrailerRef inTrailer;
      DigitalSignatureRef inSignature;
      action->getInMessage()->getContentReceiveStream(
         cRequest, is, inTrailer, inSignature);

      // setup request header to send to btp service
      HttpRequestHeader* pHeader = NULL;
      if(pRequest != NULL)
      {
         pHeader = pRequest->getHeader();
         _setupRequestHeader(cHeader, pHeader);
         pOut->setupRequestHeader(url, pHeader);
      }

      // if there is an out message, then send out header
      OutputStreamRef os;
      HttpTrailerRef outTrailer;
      if(pOut != NULL)
      {
         rval = pOut->sendHeader(
            pRequest->getConnection(), pHeader, os, outTrailer);
      }

      // proxy content (or discard it if out message is NULL)
      int numBytes;
      ByteBuffer b(2048);
      while((numBytes = b.put(&(*is))) > 0)
      {
         if(rval && !os.isNull())
         {
            rval = os->write(b.data(), numBytes);
         }
         b.clear();
      }
      is->close();

      if(rval && !os.isNull())
      {
         // Note: incoming trailers are not proxied, to do so,
         // that code would be added here
         rval = (numBytes == 0 && os->finish());
      }

      // some kind of IO error, close output stream if its available
      if(!rval && !os.isNull())
      {
         os->close();
      }
   }

   return rval;
}

/**
 * Receives the response header from the btp service.
 *
 * @param url the url to the btp service.
 * @param cResponse the client's http response to be modified if necessary.
 * @param pIn the btp input message from the btp service.
 * @param pRequest the http request that the response is for.
 * @param pResponse the http response to receive.
 *
 * @return true if successful, false if an exception occurred.
 */
static bool _receiveProxyResponseHeader(
   Url* url, HttpResponse* cResponse,
   BtpMessage* pIn, HttpRequest* pRequest, HttpResponse* pResponse)
{
   bool rval = false;

   if(pResponse->receiveHeader())
   {
      // check response security
      if(BtpClient::checkResponseSecurity(url, pRequest, pResponse, pIn))
      {
         if(pIn->getSecurityStatus() == BtpMessage::Breach)
         {
            // set exception
            ExceptionRef e = new Exception(
               "Message security breach!",
               "bitmunk.protocol.Security");
            e->getDetails()["resource"] = url->toString().c_str();
            Exception::set(e);

            // server upstream did not meet security requirements
            cResponse->getHeader()->setStatus(502, "Bad Gateway");
         }
         else
         {
            // successful transfer and no security breach
            rval = true;
         }
      }
   }

   return rval;
}

/**
 * Sends the response received from the btp service to the client.
 *
 * @param action the action used to communicate with the client.
 * @param pIn the btp input message from the btp service.
 * @param pResponse the proxy http response from the btp service.
 *
 * @return true if successful, false if an exception occurred.
 */
static bool _proxyResponse(
   BtpAction* action, BtpMessage* pIn, HttpResponse* pResponse)
{
   bool rval = false;

   // setup response header to send to client:
   HttpResponseHeader* ch = action->getResponse()->getHeader();
   HttpResponseHeader* ph = pResponse->getHeader();
   ch->setStatus(ph->getStatusCode(), ph->getStatusMessage());

   // include content-type from btp service response header
   string value;
   if(ph->getField("Content-Type", value))
   {
      ch->setField("Content-Type", value.c_str());

      // set action content-encoding
      action->setContentEncoding();

      // include content-length/transfer-encoding from btp service
      // response header
      if(ph->getField("Content-Length", value))
      {
         ch->setField("Content-Length", value.c_str());
      }
      else if(ph->getField("Transfer-Encoding", value))
      {
         ch->setField("Transfer-Encoding", "chunked");
      }
   }

   // proxy response header from btp service to client
   HttpConnection* hc = action->getResponse()->getConnection();
   OutputStreamRef os;
   HttpTrailerRef outTrailer;
   if(action->getOutMessage()->sendHeader(hc, ch, os, outTrailer))
   {
      if(!ph->hasContent())
      {
         // no content to send, so just finish output stream
         rval = os->finish();
      }
      else
      {
         // proxy content from btp service to client:

         // get receive content stream
         InputStreamRef is;
         HttpTrailerRef inTrailer;
         DigitalSignatureRef inSignature;
         pIn->getContentReceiveStream(pResponse, is, inTrailer, inSignature);

         // proxy content
         rval = true;
         int numBytes;
         ByteBuffer b(2048);
         while((numBytes = b.put(&(*is))) > 0)
         {
            rval = rval && os->write(b.data(), numBytes);
            b.clear();
         }
         is->close();

         if(rval)
         {
            // check security on content if trailers are permitted
            HttpRequest* cRequest = action->getRequest();
            if(cRequest->getHeader()->getField("TE", value) &&
               strcmp(value.c_str(), "trailers") == 0)
            {
               if(!inSignature.isNull())
               {
                  pIn->checkContentSecurity(ph, &(*inTrailer), &(*inSignature));
                  if(pIn->getSecurityStatus() == BtpMessage::Breach)
                  {
                     // include trailer indicating a security breach
                     outTrailer->setField(
                        "Btp-Proxy-Content-Security-Breach", "true");
                  }
               }
            }

            // Note: trailers from the btp service are not proxied
            // back to the client
            rval = (numBytes == 0 && os->finish());
         }

         if(!rval)
         {
            // IO error, close connection
            hc->close();
         }
      }

      // action result sent
      action->setResultSent(true);
   }

   // response sent or IO error, close output stream
   if(!os.isNull())
   {
      os->close();
   }

   return rval;
}

void BtpProxyService::proxy(BtpAction* action)
{
   bool pass = false;

   // store proxy parameters and user's profile
   DynamicObject params;
   ProfileRef profile;
   Node* node = mSessionManager->getNode();

   // ensure session is valid
   UserId userId;
   if((pass = mSessionManager->isSessionValid(action, &userId)))
   {
      // allow keep-alive for connection to user
      setKeepAlive(action);

      // get user profile
      if((pass = node->getLoginData(userId, NULL, &profile)))
      {
         // populate proxy params
         DynamicObject vars;
         action->getResourceQuery(vars);
         HttpHeader* header = action->getRequest()->getHeader();
         pass = _getProxyParams(action, params, node, vars, header);
      }
   }

   // if pass, proxy params populated
   if(pass)
   {
      // get the url to connect to
      Url url(params["url"]->getString());

      // do proxy:
      DynamicObject vars;
      url.getQueryVariables(vars);
      UserId userId = vars["nodeuser"]->getUInt64();
      params["userId"] = userId;
      if(userId == 0)
      {
         MO_CAT_INFO(BM_PROTOCOL_CAT,
            "BtpProxyService proxying to: %s", url.toString().c_str());
      }
      else
      {
         MO_CAT_INFO(BM_PROTOCOL_CAT,
            "BtpProxyService proxying to peer user '%" PRIu64 "': %s",
            userId, url.toString().c_str());
      }

      // get a connection
      BtpClient* btpc = node->getMessenger()->getBtpClient();
      HttpConnectionRef pConn = _connect(btpc, mConnectionPool, &url, params);
      if(pConn.isNull())
      {
         // connection failed, proxy client request to NULL
         _proxyClientRequest(action, &url, NULL, NULL);
         pass = false;
      }
      else
      {
         // create proxy in message
         BtpMessage pIn;
         pIn.setPublicKeySource(node->getPublicKeyCache());

         // create proxy out message
         BtpMessage pOut;
         pOut.setType(action->getInMessage()->getType());
         pOut.setContentType(action->getInMessage()->getContentType());
         pOut.setUserId(profile->getUserId());
         pOut.setAgentProfile(profile);

         // create proxy request and response
         HttpRequest* pRequest = pConn->createRequest();
         HttpResponse* pResponse = pRequest->createResponse();

         // 1. proxy the client's request to the btp service
         // 2. receive btp service response header
         // 3. proxy btp service response to the client
         pass =
            _proxyClientRequest(action, &url, &pOut, pRequest) &&
            _receiveProxyResponseHeader(
               &url, action->getResponse(), &pIn, pRequest, pResponse) &&
            _proxyResponse(action, &pIn, pResponse);
         if(pass)
         {
            // close connection if error or close header provided
            string connection =
               pResponse->getHeader()->getFieldValue("Connection");
            if(pResponse->getHeader()->getStatusCode() >= 400 ||
               strcasecmp(connection.c_str(), "close") == 0)
            {
               // close connection to btp service
               pConn->close();
            }
            else if(!pConn->isClosed())
            {
               // give idle connection back to http connection pool for reuse
               if(userId == 0)
               {
                  mConnectionPool->addConnection(&url, pConn);
               }
               else
               {
                  mConnectionPool->addConnection(
                     &url, pConn,
                     Tools::getBitmunkUserCommonName(userId).c_str());
               }
            }
         }
         else
         {
            // error, close connection
            pConn->close();
         }

         // clean up
         delete pRequest;
         delete pResponse;
      }
   }

   if(!pass && !action->isResultSent())
   {
      // send exception (client's fault if code < 500)
      ExceptionRef e = Exception::get();
      action->sendException(e, e->getCode() < 500);
   }
}

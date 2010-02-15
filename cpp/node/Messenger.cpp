/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/node/Messenger.h"

#include "bitmunk/node/BtpServer.h"
#include "bitmunk/node/Node.h"

using namespace std;
using namespace monarch::config;
using namespace monarch::net;
using namespace monarch::rt;
using namespace bitmunk::common;
using namespace bitmunk::protocol;
using namespace bitmunk::node;

Messenger::Messenger(Node* node, Config& cfg) :
   mNode(node),
   mBitmunkUrl(NULL),
   mSecureBitmunkUrl(NULL)
{
   mBitmunkUrl = new Url(cfg["bitmunkUrl"]->getString());
   mSecureBitmunkUrl = new Url(cfg["secureBitmunkUrl"]->getString());
}

Messenger::~Messenger()
{
}

string Messenger::getSelfUrl(bool ssl)
{
   // get node configuration
   Config cfg = mNode->getConfigManager()->getNodeConfig();

   // build self url
   const char* host = NULL;
   InternetAddressRef addr = mNode->getBtpServer()->getHostAddress();
   if(!addr.isNull())
   {
      host = addr->getAddress();
   }
   else
   {
      cfg["host"]->getString();
   }

   // use local loopback if 0.0.0.0 is bound
   if(strcmp(host, "0.0.0.0") == 0)
   {
      host = "127.0.0.1";
   }

   const char* port = cfg["port"]->getString();
   int length = strlen(host) + 15;
   char url[length];
   snprintf(url, length, "http%s://%s:%s", ssl ? "s" : "", host, port);

   return url;
}

Url* Messenger::getBitmunkUrl()
{
   return &(*mBitmunkUrl);
}

Url* Messenger::getSecureBitmunkUrl()
{
   return &(*mSecureBitmunkUrl);
}

bool Messenger::exchange(
   UserId peerId,
   Url* url, BtpMessage* out, BtpMessage* in, UserId userId, UserId agentId,
   uint32_t timeout)
{
   bool rval = true;

   if(userId != 0)
   {
      // get user profile
      ProfileRef p;
      if(!mNode->getLoginData(agentId == 0 ? userId : agentId, NULL, &p))
      {
         ExceptionRef e = new Exception(
            "Could not do BTP exchange. Not logged in.",
            "bitmunk.node.Messenger.NotLoggedIn");
         BM_ID_SET(e->getDetails()["userId"], userId);
         BM_ID_SET(e->getDetails()["agentId"], agentId);
         Exception::set(e);
         rval = false;
      }
      else if(!p.isNull())
      {
         // set user ID and profile for outgoing secure message
         out->setUserId(userId);
         out->setAgentProfile(p);

         // set public key source for incoming secure message
         in->setPublicKeySource(mNode->getPublicKeyCache());
      }
   }

   // do btp exchange
   if(rval)
   {
      // if nodeuser is set, override peer ID with it
      DynamicObject vars;
      url->getQueryVariables(vars);
      if(vars->hasMember("nodeuser"))
      {
         peerId = BM_USER_ID(vars["nodeuser"]);
      }

      rval = mClient.exchange(peerId, url, out, in, timeout);
   }

   return rval;
}

bool Messenger::exchange(
   UserId peerId,
   BtpMessage::Type type, Url* url,
   DynamicObject* out, DynamicObject* in, UserId userId, UserId agentId,
   DynamicObject* headers, uint32_t timeout)
{
   bool rval = true;

   ProfileRef p;
   if(BM_ID_VALID(userId))
   {
      // get user profile
      if(!mNode->getLoginData(
         BM_ID_INVALID(agentId) ? userId : agentId, NULL, &p))
      {
         ExceptionRef e = new Exception(
            "Could not do BTP exchange. Not logged in.",
            "bitmunk.node.Messenger.NotLoggedIn");
         BM_ID_SET(e->getDetails()["userId"], userId);
         BM_ID_SET(e->getDetails()["agentId"], agentId);
         Exception::set(e);
         rval = false;
      }
   }

   if(rval)
   {
      // create messages
      BtpMessage msgOut;
      msgOut.setType(type);
      BtpMessage msgIn;

      // set custom headers
      if(headers != NULL)
      {
         msgOut.getCustomHeaders() = *headers;
      }

      // set output
      if(out != NULL)
      {
         msgOut.setDynamicObject(*out);
      }

      // set input
      if(in != NULL)
      {
         msgIn.setDynamicObject(*in);
      }

      // setup security
      if(!p.isNull())
      {
         // set user ID and profile for outgoing secure message
         msgOut.setUserId(BM_ID_VALID(userId) ? userId : p->getUserId());
         msgOut.setAgentProfile(p);

         // set public key source for incoming secure message
         msgIn.setPublicKeySource(mNode->getPublicKeyCache());
      }

      // if nodeuser is set, override peer ID with it
      DynamicObject vars;
      url->getQueryVariables(vars);
      if(vars->hasMember("nodeuser"))
      {
         peerId = BM_USER_ID(vars["nodeuser"]);
      }

      // do btp exchange
      rval = mClient.exchange(peerId, url, &msgOut, &msgIn, timeout);
   }

   return rval;
}

bool Messenger::put(
   Url* url, DynamicObject& out, DynamicObject* in,
   UserId userId, UserId agentId, DynamicObject* headers,
   uint32_t timeout)
{
   return exchange(
      0, BtpMessage::Put, url, &out, in, userId, agentId, headers, timeout);
}

bool Messenger::putToBitmunk(
   Url* url, DynamicObject& out, DynamicObject* in,
   UserId userId, UserId agentId, DynamicObject* headers,
   uint32_t timeout)
{
   std::string str = getBitmunkUrl()->toString();
   str.append(url->toString());
   Url u(str.c_str());
   return exchange(
      1, BtpMessage::Put, &u, &out, in, userId, agentId, headers, timeout);
}

bool Messenger::putSecureToBitmunk(
   Url* url, DynamicObject& out, DynamicObject* in,
   UserId userId, UserId agentId, DynamicObject* headers, uint32_t timeout)
{
   std::string str = getSecureBitmunkUrl()->toString();
   str.append(url->toString());
   Url u(str.c_str());
   return exchange(
      1, BtpMessage::Put, &u, &out, in, userId, agentId, headers, timeout);
}

bool Messenger::post(
   Url* url, DynamicObject* out, DynamicObject* in,
   UserId userId, UserId agentId, DynamicObject* headers, uint32_t timeout)
{
   return exchange(
      0, BtpMessage::Post, url, out, in, userId, agentId, headers, timeout);
}

bool Messenger::postToBitmunk(
   Url* url, DynamicObject* out, DynamicObject* in,
   UserId userId, UserId agentId, DynamicObject* headers, uint32_t timeout)
{
   std::string str = getBitmunkUrl()->toString();
   str.append(url->toString());
   Url u(str.c_str());
   return exchange(
      1, BtpMessage::Post, &u, out, in, userId, agentId, headers, timeout);
}

bool Messenger::postSecureToBitmunk(
   Url* url, DynamicObject* out, DynamicObject* in,
   UserId userId, UserId agentId, DynamicObject* headers, uint32_t timeout)
{
   std::string str = getSecureBitmunkUrl()->toString();
   str.append(url->toString());
   Url u(str.c_str());
   return exchange(
      1, BtpMessage::Post, &u, out, in, userId, agentId, headers, timeout);
}

bool Messenger::get(
   Url* url, DynamicObject& in, UserId userId, UserId agentId,
   DynamicObject* headers, uint32_t timeout)
{
   return exchange(
      0, BtpMessage::Get, url, NULL, &in, userId, agentId, headers, timeout);
}

bool Messenger::getFromBitmunk(
   Url* url, DynamicObject& in, UserId userId, UserId agentId,
   DynamicObject* headers, uint32_t timeout)
{
   std::string str = getBitmunkUrl()->toString();
   str.append(url->toString());
   Url u(str.c_str());
   return exchange(
      0, BtpMessage::Get, &u, NULL, &in, userId, agentId, headers, timeout);
}

bool Messenger::getSecureFromBitmunk(
   Url* url, DynamicObject& in, UserId userId, UserId agentId,
   DynamicObject* headers, uint32_t timeout)
{
   std::string str = getSecureBitmunkUrl()->toString();
   str.append(url->toString());
   Url u(str.c_str());
   return exchange(
      1, BtpMessage::Get, &u, NULL, &in, userId, agentId, headers, timeout);
}

bool Messenger::deleteResource(
   Url* url, DynamicObject* in, UserId userId, UserId agentId,
   DynamicObject* headers, uint32_t timeout)
{
   return exchange(
      0, BtpMessage::Delete, url, NULL, in, userId, agentId, headers, timeout);
}

bool Messenger::deleteFromBitmunk(
   Url* url, DynamicObject* in, UserId userId, UserId agentId,
   DynamicObject* headers, uint32_t timeout)
{
   std::string str = getBitmunkUrl()->toString();
   str.append(url->toString());
   Url u(str.c_str());
   return exchange(
      0, BtpMessage::Delete, &u, NULL, in, userId, agentId, headers, timeout);
}

bool Messenger::deleteSecureFromBitmunk(
   Url* url, DynamicObject* in, UserId userId, UserId agentId,
   DynamicObject* headers, uint32_t timeout)
{
   std::string str = getSecureBitmunkUrl()->toString();
   str.append(url->toString());
   Url u(str.c_str());
   return exchange(
      1, BtpMessage::Delete, &u, NULL, in, userId, agentId, headers, timeout);
}

inline BtpClient* Messenger::getBtpClient()
{
   return &mClient;
}

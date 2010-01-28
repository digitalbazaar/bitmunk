/*
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/webui/FlashPolicyServer.h"

#include "bitmunk/webui/WebUiModule.h"
#include "monarch/io/ByteBuffer.h"
#include "monarch/io/ByteArrayInputStream.h"
#include "monarch/util/StringTools.h"

using namespace std;
using namespace monarch::config;
using namespace monarch::io;
using namespace monarch::net;
using namespace monarch::rt;
using namespace monarch::util;
using namespace bitmunk::common;
using namespace bitmunk::node;
using namespace bitmunk::webui;

FlashPolicyServer::FlashPolicyServer() :
   mNode(NULL),
   mHostAddress(NULL),
   mServiceId(Server::sInvalidServiceId)
{
}

FlashPolicyServer::~FlashPolicyServer()
{
}

bool FlashPolicyServer::initialize(Node* node)
{
   bool rval = false;
   
   mNode = node;
   
   // get the flash socket policy config
   Config cfg = mNode->getConfigManager()->getModuleConfig(
      "bitmunk.webui.WebUi");
   if(!cfg.isNull())
   {
      Config& fps = cfg["flash"]["policyServer"];
      const char* host = fps["host"]->getString();
      uint32_t port = fps["port"]->getUInt32();
      
      MO_CAT_DEBUG(BM_WEBUI_CAT,
         "Initializing flash socket policy server on %s:%u", host, port);
      mHostAddress = new InternetAddress(host, port);
      
      // add this connection service to the node's server
      Exception::clear();
      mServiceId = mNode->getServer()->addConnectionService(
         &(*mHostAddress), this, NULL, "FlashPolicyServer");
      if(mServiceId == Server::sInvalidServiceId)
      {
         ExceptionRef e = new Exception(
            "Could not attach the flash policy server to node.",
            "bitmunk.webui.FlashPolicyServer.AttachFailure");
         Exception::push(e);
      }
      else
      {
         MO_CAT_DEBUG(BM_WEBUI_CAT,
            "Initialized flash socket policy server on %s:%u",
            mHostAddress->getAddress(),
            mHostAddress->getPort());
         rval = true;
      }
   }
   
   return rval;
}

void FlashPolicyServer::cleanup()
{
   if(mServiceId != Server::sInvalidServiceId)
   {
      // remove connection service from node's server
      mNode->getServer()->removePortService(mServiceId);
      mHostAddress.setNull();
      mServiceId = Server::sInvalidServiceId;
   }
}

void FlashPolicyServer::serviceConnection(Connection* c)
{
   // read the <policy_file_request/> xml string
   const char* request = "<policy-file-request/>";
   int len = strlen(request) + 1;
   char buf[len];
   memset(buf, 0, len);
   if(c->getInputStream()->read(buf, len))
   {
      if(buf[len - 1] == 0 && strcmp(buf, request) == 0) 
      {
         // respond to the policy file request string with socket policy xml
         const char* response =
            "<?xml version=\"1.0\"?>"
            "<!DOCTYPE cross-domain-policy "
            "SYSTEM \"http://www.adobe.com/xml/dtds/cross-domain-policy.dtd\">"
            "<cross-domain-policy>"
            "<allow-access-from domain=\"*\" to-ports=\"*\"/>"
            "</cross-domain-policy>";
         len = strlen(response) + 1;
         c->getOutputStream()->write(response, len);
         
         MO_CAT_DEBUG(BM_WEBUI_CAT,
            "Sent flash socket policy to %s:%i",
            c->getRemoteAddress()->getAddress(),
            c->getRemoteAddress()->getPort());
      }
   }
   
   // close connection
   c->close();
}

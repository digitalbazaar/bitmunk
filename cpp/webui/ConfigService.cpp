/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/webui/ConfigService.h"

#include "bitmunk/common/Signer.h"
#include "bitmunk/node/BtpActionDelegate.h"
#include "bitmunk/node/RestResourceHandler.h"
#include "bitmunk/common/Logging.h"
#include "monarch/data/json/JsonReader.h"
#include "monarch/data/json/JsonWriter.h"
#include "monarch/io/FileInputStream.h"

#include <algorithm>

using namespace std;
using namespace monarch::config;
using namespace monarch::data::json;
using namespace monarch::event;
using namespace monarch::http;
using namespace monarch::io;
using namespace monarch::net;
using namespace monarch::rt;
using namespace bitmunk::common;
using namespace bitmunk::protocol;
using namespace bitmunk::node;
using namespace bitmunk::webui;
namespace v = monarch::validation;

typedef BtpActionDelegate<ConfigService> Handler;

#define END_CERT_LEN   25

ConfigService::ConfigService(
   SessionManager* sm, DynamicObject& pi, const char* path) :
   BtpService(path),
   mSessionManager(sm),
   mPluginInfo(pi)
{
}

ConfigService::~ConfigService()
{
}

bool ConfigService::initialize()
{
   // plugins
   {
      RestResourceHandlerRef plugins = new RestResourceHandler();
      addResource("/plugins", plugins);

      // GET .../plugins
      {
         ResourceHandler h = new Handler(
            mSessionManager->getNode(), this, &ConfigService::getPlugins,
            BtpAction::AuthOptional);
         plugins->addHandler(h, BtpMessage::Get);
      }
   }

   // flash configuration
   {
      RestResourceHandlerRef flash = new RestResourceHandler();
      addResource("/flash", flash);

      // GET .../flashconfig
      {
         ResourceHandler h = new Handler(
            mSessionManager->getNode(), this, &ConfigService::getFlashConfig,
            BtpAction::AuthOptional);
         flash->addHandler(h, BtpMessage::Get);
      }
   }

   return true;
}

void ConfigService::cleanup()
{
   // remove resources
   removeResource("/plugins");
   removeResource("/flash");
}

bool ConfigService::getPlugins(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = true;

   // ensure session is valid
   if(mSessionManager->isSessionValid(action))
   {
      out->setType(Array);
      DynamicObjectIterator pi = mPluginInfo.getIterator();
      while(pi->hasNext())
      {
         DynamicObject& info = pi->next();
         DynamicObject& details = out->append();
         if(info->hasMember("details"))
         {
            details = info["details"].clone();
         }
         details["id"] = pi->getName();
      }
   }

   return rval;
}

bool ConfigService::getFlashConfig(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = false;

   // Note: No session required for getting flash config. In fact, this
   // flash config *must* be public for flash to be initialized prior to
   // a user logging in.

   // get node
   Node* node = mSessionManager->getNode();

   // get the node config
   Config nodeConfig = node->getConfigManager()->getNodeConfig();

   // get the flash configuration
   Config cfg = node->getConfigManager()->getModuleConfig(
      "bitmunk.webui.WebUi");
   rval = !cfg.isNull();
   if(rval)
   {
      // FIXME: this is only currently "safeish" due to localhost limitations
      // for the frontend, we need a secure long-term solution

      // prepare to send trusted certificates
      out->setType(Map);
      DynamicObject certs = out["ssl"]["certificates"];
      certs->setType(Array);

      // create files here so we can size the read buffer properly
      string caFilePath;
      string sslCertPath;
      rval =
         node->getConfigManager()->expandBitmunkHomePath(
            nodeConfig["sslCAFile"]->getString(), caFilePath) &&
         node->getConfigManager()->expandBitmunkHomePath(
            nodeConfig["sslCertificate"]->getString(), sslCertPath);
      if(rval)
      {
         File caFile(caFilePath.c_str());
         File sslCertFile(sslCertPath.c_str());

         // size the buffer to max of all files we will read
         ByteBuffer buf(
            std::max(caFile->getLength(), sslCertFile->getLength()));

         // read certificates from node's SSL CA file
         if(!caFile.readBytes(&buf))
         {
            rval = false;
         }
         else
         {
            // add null-terminator to buffer
            buf.putByte(0, 1, true);

            // split up certificates
            char* end = NULL;
            do
            {
               end = (char*)strstr(buf.data(), "-----END CERTIFICATE-----");
               if(end != NULL)
               {
                  end += END_CERT_LEN;
                  end[0] = 0;
                  certs->append() = buf.data();
                  buf.clear(end - buf.data() + 1);
                  end = buf.data();
               }
            }
            while(end != NULL && !buf.isEmpty());
         }

         // read in default certificate
         if(rval)
         {
            buf.clear();
            if(!sslCertFile.readBytes(&buf))
            {
               rval = false;
            }
            else
            {
               // add null-terminator to buffer, add certificate
               buf.putByte(0, 1, true);
               certs->append() = buf.data();
            }
         }
      }

      if(rval)
      {
         // include policy server port
         Config& fps = cfg["flash"]["policyServer"];
         out["policyServer"]["port"] = fps["port"]->getUInt32();
      }
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Could not load flash config.",
         "bitmunk.webui.WebUi.InvalidFlashConfig");
      Exception::push(e);
   }

   return rval;
}

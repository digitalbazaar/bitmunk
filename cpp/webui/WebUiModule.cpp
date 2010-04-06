/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/webui/WebUiModule.h"

#include "bitmunk/node/BtpServer.h"
#include "bitmunk/webui/BtpProxyService.h"
#include "bitmunk/webui/ConfigService.h"
#include "bitmunk/webui/ForceSslService.h"
#include "bitmunk/webui/FrontendService.h"
#include "bitmunk/webui/RedirectService.h"
#include "bitmunk/webui/SessionService.h"
#include "bitmunk/webui/StatusService.h"
#include "monarch/logging/Logging.h"
#include "bitmunk/common/LoggingCategories.h"
#include "monarch/validation/Validation.h"

using namespace monarch::config;
using namespace monarch::io;
using namespace monarch::kernel;
using namespace monarch::logging;
using namespace monarch::modest;
using namespace monarch::rt;
using namespace bitmunk::protocol;
using namespace bitmunk::node;
using namespace bitmunk::webui;
namespace v = monarch::validation;

// Logging category initialized during module initialization.
Category* BM_WEBUI_CAT;

WebUiModule::WebUiModule() :
   BitmunkModule("bitmunk.webui.WebUi", "1.0"),
   mSessionManager(NULL),
   mMainPluginId(NULL)
{
   mPluginInfo->setType(Map);
}

WebUiModule::~WebUiModule()
{
}

void WebUiModule::addDependencyInfo(DynamicObject& depInfo)
{
   // no dependencies
}

bool WebUiModule::initialize(Node* node)
{
   bool rval = false;

   BM_WEBUI_CAT = new Category(
      "BM_WEBUI",
      "Bitmunk WebUI",
      NULL);
   BM_WEBUI_CAT->setAnsiEscapeCodes(
      MO_ANSI_CSI MO_ANSI_BOLD MO_ANSI_SEP MO_ANSI_FG_HI_CYAN MO_ANSI_SGR);

   // ensure web plugins load
   if((rval = loadPlugins(node)))
   {
      // create session manager
      mSessionManager = new SessionManager(node);
      rval = mSessionManager->initialize();
      if(!rval)
      {
         delete mSessionManager;
         mSessionManager = NULL;
      }
      // initialize flash policy server
      else if((rval = mFlashPolicyServer.initialize(node)))
      {
         // Note: ALL services for webui except redirect service are SSL-only

         // add session service
         BtpServiceRef bs;

         if(rval)
         {
            bs = new SessionService(mSessionManager, "/api/3.0/webui");
            rval = node->getBtpServer()->addService(bs, Node::SslOn);
         }

         if(rval)
         {
            // add proxy service
            bs = new BtpProxyService(mSessionManager, "/api/3.0/webui/proxy");
            rval = node->getBtpServer()->addService(bs, Node::SslOn);
         }

         if(rval)
         {
            // add status service
            bs = new StatusService(mSessionManager, "/api/3.0/webui/status");
            rval = node->getBtpServer()->addService(bs, Node::SslOn);
         }

         if(rval)
         {
            // add config service
            bs = new ConfigService(
               mSessionManager, mPluginInfo, "/api/3.0/webui/config");
            rval = node->getBtpServer()->addService(bs, Node::SslAny);
         }

         if(rval)
         {
            // add frontend service
            bs = new FrontendService(
               mSessionManager, mPluginInfo, mMainPluginId, "/bitmunk");
            // special flash app provides SSL support, so the front end can
            // be hit on non-SSL and all data transfer through the proxy will
            // be done using SSL by our flash proxy app
            rval = node->getBtpServer()->addService(bs, Node::SslAny);
         }

         if(rval)
         {
            // add / => /bitmunk redirect
            bs = new RedirectService(node, "/", "/", "bitmunk");
            rval = node->getBtpServer()->addService(bs, Node::SslAny);
         }
      }
   }

   return rval;
}

void WebUiModule::cleanup(Node* node)
{
   node->getBtpServer()->removeService("/api/3.0/webui");
   node->getBtpServer()->removeService("/api/3.0/webui/proxy");
   node->getBtpServer()->removeService("/api/3.0/webui/status");
   node->getBtpServer()->removeService("/api/3.0/webui/config");
   node->getBtpServer()->removeService("/bitmunk");
   node->getBtpServer()->removeService("/");

   // clean up flash policy server
   mFlashPolicyServer.cleanup();

   // delete session manager
   if(mSessionManager != NULL)
   {
      delete mSessionManager;
      mSessionManager = NULL;
   }

   // clean up main plugin ID
   if(mMainPluginId != NULL)
   {
      free(mMainPluginId);
      mMainPluginId = NULL;
   }

   delete BM_WEBUI_CAT;
   BM_WEBUI_CAT = NULL;
}

MicroKernelModuleApi* WebUiModule::getApi(Node* node)
{
   // no API
   return NULL;
}

bool WebUiModule::loadPlugins(Node* node)
{
   bool rval = false;

   // create validators for plugin configurations
   v::ValidatorRef configValidator = new v::Map(
      // name of the config with the main plugin details
      "main", new v::Type(String),
      "plugins", new v::Type(Map),
      NULL);
   v::ValidatorRef webPluginValidator =
      new v::Map(
         "interfaces", new v::Map(
            "web", new v::Map(
               // root dir of this plugin
               "root", new v::Type(String),
               // defaults to [root]/content
               "content", new v::Optional(new v::Type(String)),
               // optional details for public use
               "details", new v::Optional(new v::Map(
                  // defaults to [content]/"init.js"
                  "init", new v::Optional(new v::Type(String)),
                  NULL)),
               // names of virtual files that are the result of other
               // concatenated files (for optimizing web traffic)
               "virtual", new v::Optional(new v::All(
                  new v::Type(Map),
                  new v::Each(new v::All(
                     new v::Type(Array),
                     new v::Min(1, "Minimum of 1 file per virtual file."),
                     NULL)),
                  NULL)),
               NULL),
            NULL),
         NULL);

   // get webui module configuration
   Config cfg = node->getConfigManager()->getModuleConfig(
      "bitmunk.webui.WebUi");

   // validate configurations
   if((rval = configValidator->isValid(cfg)))
   {
      // store main plugin ID
      mMainPluginId = strdup(cfg["main"]->getString());

      // get config map of plugins
      Config& map = cfg["plugins"];

      // build up plugin info by checking if any objects in the map for
      // have the web plugin protocol
      ConfigIterator i = map.getIterator();
      while(i->hasNext())
      {
         Config& next = i->next();
         const char* name = i->getName();
         if(webPluginValidator->isValid(next))
         {
            // copy configuration into plugins map
            mPluginInfo[name] = next["interfaces"]["web"].clone();
            DynamicObject& info = mPluginInfo[name];
            if(info->hasMember("content"))
            {
               if(!File::isPathAbsolute(info["content"]->getString()))
               {
                  // content is relative to root, prepend root
                  info["content"] = File::join(
                     info["root"]->getString(),
                     info["content"]->getString()).c_str();
               }
            }
            else
            {
               // add default content dir
               info["content"] = File::join(
                  info["root"]->getString(),
                  "content").c_str();
            }

            MO_CAT_INFO(BM_WEBUI_CAT, "Web plugin found:%s, content:%s",
               name, info["content"]->getString());
         }
         else
         {
            // log that plugin isn't found, but clear exception and
            // continue to load next plugin
            MO_CAT_DEBUG(BM_WEBUI_CAT, "Web plugin not found for %s", name);
            Exception::clear();
         }
      }
   }
   else
   {
      MO_CAT_ERROR(BM_WEBUI_CAT, "Invalid web plugin(s) config.");
   }

   return rval;
}

Module* createModestModule()
{
   return new WebUiModule();
}

void freeModestModule(Module* m)
{
   delete m;
}

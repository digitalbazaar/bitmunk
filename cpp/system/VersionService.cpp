/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/system/VersionService.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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

typedef BtpActionDelegate<VersionService> Handler;

VersionService::VersionService(Node* node, const char* path) :
   NodeService(node, path)
{
}

VersionService::~VersionService()
{
}

bool VersionService::initialize()
{
   // version
   {
      RestResourceHandlerRef version = new RestResourceHandler();
      addResource("/", version);
      
      // GET .../version
      {
         // FIXME: can this be public?
         Handler* handler = new Handler(
            mNode, this, &VersionService::getVersion,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         v::ValidatorRef qValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         version->addHandler(h, BtpMessage::Get, 0, &qValidator);
      }
   }
   
   return true;
}

void VersionService::cleanup()
{
   // remove resources
   removeResource("/");
}

bool VersionService::getVersion(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = true;
   
   out["name"] = "Bitmunk Personal Edition";
   out["version"] = PACKAGE_VERSION;
   out["homepage"] = "http://bitmunk.com/";
   out["authors"][0]["name"] = "Digital Bazaar, Inc.";
   out["authors"][0]["email"] = "support@digitalbazaar.com";
   out["authors"][0]["homepage"] = "http://digitalbazaar.com/";
   
   return rval;
}

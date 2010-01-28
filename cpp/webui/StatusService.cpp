/*
 * Copyright (c) 2007-2009 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/webui/StatusService.h"

#include "bitmunk/node/BtpActionDelegate.h"

using namespace bitmunk::common;
using namespace bitmunk::protocol;
using namespace bitmunk::node;
using namespace bitmunk::webui;
using namespace monarch::rt;

typedef BtpActionDelegate<StatusService> Handler;

StatusService::StatusService(SessionManager* sm, const char* path) :
   BtpService(path),
   mSessionManager(sm)
{
}

StatusService::~StatusService()
{
}

bool StatusService::initialize()
{
   // root
   {
      ResourceHandler status = new Handler(
         mSessionManager->getNode(),
         this, &StatusService::status,
         BtpAction::AuthOptional);
      addResource("/", status);
   }

   return true;
}

void StatusService::cleanup()
{
   // remove resources
   removeResource("/");
}

bool StatusService::status(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = true;
   UserId userId;

   // set that the node is online
   out["online"] = true;

   // set the logged in status of the sessionId making the call
   out["loggedIn"] = mSessionManager->isSessionValid(action, &userId);

   return rval;
}

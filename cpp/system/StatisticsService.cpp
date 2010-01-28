/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/system/StatisticsService.h"

#include "bitmunk/common/Signer.h"
#include "bitmunk/node/BtpActionDelegate.h"
#include "bitmunk/node/RestResourceHandler.h"

using namespace monarch::net;
using namespace monarch::rt;
using namespace bitmunk::common;
using namespace bitmunk::protocol;
using namespace bitmunk::node;
using namespace bitmunk::system;
namespace v = monarch::validation;

typedef BtpActionDelegate<StatisticsService> Handler;

StatisticsService::StatisticsService(Node* node, const char* path) :
   NodeService(node, path)
{
   mUptimeTimer.start();
}

StatisticsService::~StatisticsService()
{
}

bool StatisticsService::initialize()
{
   bool rval = true;

   // DynamicObject stats
   if(rval)
   {
      RestResourceHandlerRef dyno = new RestResourceHandler();
      addResource("/dyno", dyno);

      // GET .../dyno
      {
         Handler* handler = new Handler(
            mNode, this, &StatisticsService::getDynoStats,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         v::ValidatorRef qValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         dyno->addHandler(h, BtpMessage::Get, 0, &qValidator);
      }
   }

   // stats
   if(rval)
   {
      RestResourceHandlerRef stats = new RestResourceHandler();
      addResource("/stats", stats);

      // GET .../stats
      {
         Handler* handler = new Handler(
            mNode, this, &StatisticsService::getStats,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         v::ValidatorRef qValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         stats->addHandler(h, BtpMessage::Get, 0, &qValidator);
      }
   }

   // uptime
   if(rval)
   {
      RestResourceHandlerRef uptime = new RestResourceHandler();
      addResource("/uptime", uptime);

      // GET .../uptime
      {
         Handler* handler = new Handler(
            mNode, this, &StatisticsService::getUptime,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         v::ValidatorRef qValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         uptime->addHandler(h, BtpMessage::Get, 0, &qValidator);
      }
   }

   return rval;
}

void StatisticsService::cleanup()
{
   // remove resources
   removeResource("/dyno");
   removeResource("/stats");
   removeResource("/uptime");
}

bool StatisticsService::getDynoStats(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval;

   if((rval = mNode->checkLogin(action)))
   {
      out = DynamicObjectImpl::getStats();
   }

   return rval;
}

bool StatisticsService::getStats(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval;

   // any logged in user can get the uptime
   if((rval = mNode->checkLogin(action)))
   {
      out["stats"] = "FIXME";
   }

   return rval;
}

bool StatisticsService::getUptime(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval;

   // any logged in user can get the uptime
   if((rval = mNode->checkLogin(action)))
   {
      out["uptime"] = mUptimeTimer.getElapsedMilliseconds() / 1000;
   }

   return rval;
}

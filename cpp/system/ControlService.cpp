/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/system/ControlService.h"

#include "bitmunk/node/BtpActionDelegate.h"
#include "bitmunk/node/RestResourceHandler.h"

using namespace monarch::event;
using namespace monarch::net;
using namespace monarch::rt;
using namespace bitmunk::common;
using namespace bitmunk::protocol;
using namespace bitmunk::node;
namespace v = monarch::validation;

typedef BtpActionDelegate<bitmunk::system::ControlService> Handler;

// Note: This class name must be fully qualified because windows reserves
// "ControlService"
bitmunk::system::ControlService::ControlService(
   Node* node, const char* path) :
   NodeService(node, path)
{
}

bitmunk::system::ControlService::~ControlService()
{
}

bool bitmunk::system::ControlService::initialize()
{
   // state
   {
      RestResourceHandlerRef state = new RestResourceHandler();
      addResource("/state", state);

      /**
       * Gets the current and/or pending node state.
       *
       * @tags payswarm-api public-api
       * @qparam nodeuser the user on the node that should perform the
       *                  requested action on the caller's behalf.
       *
       * @return an HTTP 200 code if successful, an exception if not.
       */
      {
         Handler* handler = new Handler(
            mNode, this, &bitmunk::system::ControlService::getState,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         v::ValidatorRef qValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         state->addHandler(h, BtpMessage::Get, 0, &qValidator);
      }

      /**
       * Sets the node state.
       *
       * @tags payswarm-api public-api
       * @qparam nodeuser the user on the node that should perform the
       *                  requested action on the caller's behalf.
       *
       * @return an HTTP 200 code if successful, an exception if not.
       */
      {
         Handler* handler = new Handler(
            mNode, this, &bitmunk::system::ControlService::postState,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         v::ValidatorRef qValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         v::ValidatorRef validator = new v::Map(
            "state", new v::Regex("^shutdown|restart$"),
            NULL);

         state->addHandler(h, BtpMessage::Post, 0, &qValidator, &validator);
      }
   }

   return true;
}

void bitmunk::system::ControlService::cleanup()
{
   // remove resources
   removeResource("/state");
}

bool bitmunk::system::ControlService::getState(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = true;

   // FIXME: is this even needed?
   out["state"] = "active";

   return rval;
}

bool bitmunk::system::ControlService::postState(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = true;

   if(!in.isNull() &&
      in->hasMember("state"))
   {
      if(strcmp(in["state"]->getString(), "shutdown") == 0)
      {
         // success, send empty result now so that shutdown event occurs
         // afterwards
         out.setNull();
         action->sendResult();

         Event e;
         e["type"] = "bitmunk.node.Node.shutdown";
         mNode->getEventController()->schedule(e);
      }
      if(strcmp(in["state"]->getString(), "restart") == 0)
      {
         // success, send empty result now so that restart event occurs
         // afterwards
         out.setNull();
         action->sendResult();

         Event e;
         e["type"] = "bitmunk.node.Node.restart";
         mNode->getEventController()->schedule(e);
      }
   }
   else
   {
      ExceptionRef e = new Exception(
         "Invalid system state.",
         "bitmunk.node.Node.InvalidState");
      Exception::set(e);
      rval = false;
   }

   return rval;
}

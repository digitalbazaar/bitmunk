/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/eventreactor/EventReactorService.h"

#include "bitmunk/common/Logging.h"
#include "bitmunk/eventreactor/DefaultObserverContainer.h"
#include "bitmunk/node/BtpActionDelegate.h"
#include "bitmunk/node/RestResourceHandler.h"
#include "monarch/validation/Validation.h"

#include <algorithm>

using namespace std;
using namespace monarch::event;
using namespace monarch::net;
using namespace monarch::rt;
using namespace bitmunk::common;
using namespace bitmunk::protocol;
using namespace bitmunk::node;
using namespace bitmunk::eventreactor;
namespace v = monarch::validation;

typedef BtpActionDelegate<EventReactorService> Handler;

EventReactorService::EventReactorService(Node* node, const char* path) :
   NodeService(node, path)
{
}

EventReactorService::~EventReactorService()
{
}

bool EventReactorService::initialize()
{
   // users
   {
      RestResourceHandlerRef users = new RestResourceHandler();
      RestResourceHandlerRef deleteUser = new RestResourceHandler();
      addResource("/users", users);
      addResource("/delete/user", deleteUser);

      // FIXME: add support for creation of multiple reactors via
      // posting to only /users/<userId> with reactor names in the post data
   //   // POST .../users/<userId>
   //   {
   //      ResourceHandler h = new Handler(
   //         mNode, this, &EventReactorService::createReactor,
   //         BtpAction::AuthRequired);
   //
   //      v::ValidatorRef validator = new v::Map(
   //         "names", new v::Optional(new v::Each(
   //            new v::All(
   //               new v::Type(String),
   //               new v::Min(1, "Minimum name length of 1"),
   //               NULL),
   //            NULL)),
   //         NULL);
   //
   //      v::ValidatorRef queryValidator = new v::Map(
   //         "nodeuser", new v::Int(v::Int::Positive),
   //         NULL);
   //
   //      users->addHandler(h, BtpMessage::Post, 1, &queryValidator, &validator);
   //   }

      // POST .../users/<userId>/<eventreactor-name>
      {
         Handler* handler = new Handler(
            mNode, this, &EventReactorService::createReactor,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         v::ValidatorRef queryValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         users->addHandler(h, BtpMessage::Post, 2, &queryValidator);
      }

      // DELETE .../users/<userId>
      {
         Handler* handler = new Handler(
            mNode, this, &EventReactorService::deleteReactors,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         v::ValidatorRef queryValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         users->addHandler(h, BtpMessage::Delete, 1, &queryValidator);
      }

      // FIXME: add support for deleting individual reactors
   //   // DELETE .../users/<userId>/<eventreactor-name>
   //   {
   //      ResourceHandler h = new Handler(
   //         mNode, this, &EventReactorService::deleteUser,
   //         BtpAction::AuthRequired);
   //
   //      v::ValidatorRef queryValidator = new v::Map(
   //         "nodeuser", new v::Int(v::Int::Positive),
   //         NULL);
   //
   //      users->addHandler(h, BtpMessage::Delete, 2, &queryValidator);
   //   }

      // POST .../delete/user/<userId>
      // Note: Here for browser compatibility,
      // if browser cannot do DELETE method
      {
         Handler* handler = new Handler(
            mNode, this, &EventReactorService::deleteReactors,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         // FIXME: add support for deleting individual reactors via POST
   //      v::ValidatorRef validator = new v::Map(
   //         "names", new v::Optional(new v::Each(
   //            new v::All(
   //               new v::Type(String),
   //               new v::Min(1, "Minimum name length of 1"),
   //               NULL),
   //            NULL)),
   //         NULL);

         v::ValidatorRef queryValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         deleteUser->addHandler(
            h, BtpMessage::Post, 1, &queryValidator);//, &validator);
      }
   }

   // status
   {
      RestResourceHandlerRef status = new RestResourceHandler();
      addResource("/status", status);

      // GET .../status
      {
         Handler* handler = new Handler(
            mNode, this, &EventReactorService::getStatus,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         v::ValidatorRef queryValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         status->addHandler(h, BtpMessage::Get, 0, &queryValidator);
      }
   }

   return true;
}

void EventReactorService::cleanup()
{
   // remove resources
   removeResource("/users");
   removeResource("/delete/user");
   removeResource("/status");
}

bool EventReactorService::createReactor(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = true;

   // ignore POST data, simply create reactor for user if it doesn't
   // already exist

   // get the node user
   DynamicObject vars;
   action->getResourceQuery(vars);
   UserId nodeuser = vars["nodeuser"]->getUInt64();

   // get the path params
   DynamicObject params;
   action->getResourceParams(params);
   UserId userId = params[0]->getUInt64();

   // ensure that the nodeuser matches the user path parameter
   if(nodeuser != userId)
   {
      ExceptionRef e = new Exception(
         "User not authorized to create event reactors for given user.",
         "bitmunk.eventreactor.EventReactorService.Unauthorized");
      e->getDetails()["resource"] = action->getResource();
      e->getDetails()["userId"] = userId;
      e->getDetails()["nodeuser"] = nodeuser;
      rval = false;
   }
   else
   {
      // get the name
      const char* name = params[1]->getString();

      // find the event reactor for the name
      ReactorMap::iterator i = mEventReactors.find(name);
      if(i == mEventReactors.end())
      {
         // name not supported
         ExceptionRef e = new Exception(
            "Event reactor name not supported.",
            "bitmunk.eventreactor.EventReactorService.UnsupportedName");
         e->getDetails()["name"] = name;
         rval = false;
      }
      else
      {
         // event reactor for name found, register user
         EventReactor* er = i->second;
         er->getObserverContainer()->registerUser(userId);

         // send back user ID and name with event reactor
         out["userId"] = userId;
         out["name"] = name;
      }
   }

   return rval;
}

bool EventReactorService::deleteReactors(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = true;

   // ignore POST data, simply delete named reactors for user

   // get the node user
   DynamicObject vars;
   action->getResourceQuery(vars);
   UserId nodeuser = vars["nodeuser"]->getUInt64();

   // get the path params
   DynamicObject params;
   action->getResourceParams(params);
   UserId userId = params[0]->getUInt64();

   // ensure that the nodeuser matches the user path parameter
   if(nodeuser != userId)
   {
      ExceptionRef e = new Exception(
         "User not authorized to delete event reactors for given user.",
         "bitmunk.eventreactor.EventReactorService.Unauthorized");
      e->getDetails()["resource"] = action->getResource();
      e->getDetails()["userId"] = userId;
      e->getDetails()["nodeuser"] = nodeuser;
      rval = false;
   }
   else
   {
      // remove user from every event reactor
      out["userId"] = userId;
      out["names"]->setType(Array);
      for(ReactorMap::iterator i = mEventReactors.begin();
          i != mEventReactors.end(); i++)
      {
         if(i->second->getObserverContainer()->unregisterUser(userId))
         {
            // send back names for deleted event reactors
            DynamicObject name;
            name = i->first;
            out["names"]->append(name);
         }
      }
   }

   return rval;
}

bool EventReactorService::getStatus(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = true;

   // return all supported names
   out["supported"]->setType(Array);
   for(ReactorMap::iterator i = mEventReactors.begin();
       i != mEventReactors.end(); i++)
   {
      // send back names for available event reactors
      DynamicObject name;
      name = i->first;
      out["supported"]->append(name);
   }

   return rval;
}

void EventReactorService::setEventReactor(const char* name, EventReactor* er)
{
   // create an associated observer container
   ObserverContainer* oc = new DefaultObserverContainer(mNode, er);
   er->setObserverContainer(oc);
   mEventReactors.insert(make_pair(strdup(name), er));
}

void EventReactorService::removeEventReactor(const char* name)
{
   ReactorMap::iterator i = mEventReactors.find(name);
   if(i != mEventReactors.end())
   {
      char* mn = (char*)i->first;
      delete i->second->getObserverContainer();
      delete i->second;
      mEventReactors.erase(i);
      free(mn);
   }
}

/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk/system/DirectiveService.h"

#include "bitmunk/common/Logging.h"
#include "bitmunk/node/BtpActionDelegate.h"
#include "bitmunk/node/RestResourceHandler.h"
#include "bitmunk/system/DirectiveProcessor.h"
#include "monarch/validation/Validation.h"
#include "monarch/util/Random.h"

using namespace std;
using namespace monarch::event;
using namespace monarch::net;
using namespace monarch::rt;
using namespace monarch::util;
using namespace bitmunk::common;
using namespace bitmunk::protocol;
using namespace bitmunk::node;
using namespace bitmunk::system;
namespace v = monarch::validation;

typedef BtpActionDelegate<DirectiveService> Handler;

// limit the number of directives to a maximum per user
#define MAX_DIRECTIVES 100

DirectiveService::DirectiveService(Node* node, const char* path) :
   NodeService(node, path)
{
}

DirectiveService::~DirectiveService()
{
}

bool DirectiveService::initialize()
{
   // root
   {
      RestResourceHandlerRef root = new RestResourceHandler();
      addResource("/", root);

      /**
       * Inserts the given directive into the queue which can be processed
       * at a later time by calling the "process" web service call.
       *
       * @tags payswarm-api public-api
       *
       * @qparam nodeuser the user on the node that should perform the
       *                  requested action on the caller's behalf.
       *
       * @return an HTTP 20x code if successful, an exception if not.
       */
      {
         // POST .../
         Handler* handler = new Handler(
            mNode, this, &DirectiveService::createDirective,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         v::ValidatorRef qValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         v::ValidatorRef validator = new v::Map(
            "version", new v::Equals("3.0"),
            "type", new v::Equals("peerbuy"),
            "data", new v::Map(
               "ware", new v::Map(
                  "id", new v::Equals(
                     "bitmunk:bundle",
                     "Only peerbuy 'bundle' wares are permitted."),
                  "mediaId", new v::Optional(new v::Int(v::Int::Positive)),
                  "fileInfos", new v::Optional(new v::Each(
                     new v::Map(
                        "id", new v::Type(String),
                        "mediaId", new v::Int(v::Int::Positive),
                        NULL))),
                  NULL),
               "sellerLimit", new v::Optional(new v::Int(v::Int::Positive)),
               "sellers", new v::Optional(new v::Each(
                  new v::Map(
                     "userId", new v::Int(v::Int::Positive),
                     "serverId", new v::Int(v::Int::Positive),
                     // FIXME: Ensure that the value passed in is a URL
                     "url", new v::Type(String),
                     NULL))),
               NULL),
            NULL);

         root->addHandler(h, BtpMessage::Post, 0, &qValidator, &validator);
      }

      /**
       * Gets the list of currently queued directives.
       *
       * @tags payswarm-api public-api
       *
       * @qparam nodeuser the user on the node that should perform the
       *                  requested action on the caller's behalf.
       *
       * @return an HTTP 20x code if successful, an exception if not.
       */
      {
         // GET .../
         Handler* handler = new Handler(
            mNode, this, &DirectiveService::getDirectives,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         v::ValidatorRef qValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         root->addHandler(h, BtpMessage::Get, 0, &qValidator);
      }

      /**
       * Deletes a pending directive that is in the directives queue.
       *
       * @tags payswarm-api public-api
       * @pparam directiveId the ID associated with the directive to delete.
       * @qparam nodeuser the user on the node that should perform the
       *                  requested action on the caller's behalf.
       *
       * @return an HTTP 20x code if successful, an exception if not.
       */
      {
         // DELETE .../<directiveId>
         Handler* handler = new Handler(
            mNode, this, &DirectiveService::deleteDirective,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         v::ValidatorRef qValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         root->addHandler(h, BtpMessage::Delete, 1, &qValidator);
      }
   }

   // delete directive
   {
      RestResourceHandlerRef dd = new RestResourceHandler();
      addResource("/delete", dd);

      // Note: This handler duplicates the above DELETE handler to allow
      // clients that can't use the DELETE method to delete directives.

      /**
       * Deletes a pending directive that is in the directives queue.
       *
       * @tags payswarm-api public-api
       * @pparam directiveId the ID associated with the directive to delete.
       * @qparam nodeuser the user on the node that should perform the
       *                  requested action on the caller's behalf.
       *
       * @return an HTTP 20x code if successful, an exception if not.
       */
      // POST .../delete/<directiveId>
      {
         Handler* handler = new Handler(
            mNode, this, &DirectiveService::deleteDirective,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         v::ValidatorRef qValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         dd->addHandler(h, BtpMessage::Post, 1, &qValidator);
      }
   }

   // process directive
   {
      RestResourceHandlerRef pd = new RestResourceHandler();
      addResource("/process", pd);

      /**
       * Processes a directive with the given ID in the directives queue.
       *
       * @tags payswarm-api public-api
       * @pparam directiveId the ID associated with the directive to process.
       * @qparam nodeuser the user on the node that should perform the
       *                  requested action on the caller's behalf.
       *
       * @return an HTTP 20x code if successful, an exception if not.
       */
      {
         // POST .../process/<directiveId>
         Handler* handler = new Handler(
            mNode, this, &DirectiveService::processDirective,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         v::ValidatorRef qValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         pd->addHandler(h, BtpMessage::Post, 1, &qValidator);
      }
   }

   // initialize directives cache
   mDirectives->setType(Map);

   return true;
}

void DirectiveService::cleanup()
{
   // remove resources
   removeResource("/");
   removeResource("/delete");
   removeResource("/process");
}

bool DirectiveService::createDirective(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = false;

   UserId userId;
   if(mNode->checkLogin(action, &userId))
   {
      // turn user ID into a key
      char key[22];
      snprintf(key, 22, "%" PRIu64, userId);

      // lock to insert directive
      mCacheLock.lock();
      {
         // initialize user's directive cache as needed
         if(!mDirectives->hasMember(key))
         {
            mDirectives[key]->setType(Map);
         }

         DynamicObject& cache = mDirectives[key];
         if(cache->length() < MAX_DIRECTIVES)
         {
            // insert directive into cache
            insertDirective(cache, in);

            // return directive ID
            out["directiveId"] = in["id"]->getString();
            rval = true;

            // fire created event
            Event e;
            e["type"] = "bitmunk.system.Directive.created";
            e["details"]["userId"] = userId;
            e["details"]["directiveId"] = in["id"];
            e["details"]["directive"] = in;
            mNode->getEventController()->schedule(e);
         }
         else
         {
            // too many directives
            ExceptionRef e = new Exception(
               "Could not add directive. Maximum number of "
               "directives reached.",
               "bitmunk.system.DirectiveService.TooManyDirectives");
            e->getDetails()["userId"] = userId;
            e->getDetails()["max"] = MAX_DIRECTIVES;
            Exception::set(e);
         }
      }
      mCacheLock.unlock();
   }

   return rval;
}

bool DirectiveService::deleteDirective(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = false;

   UserId userId;
   if(mNode->checkLogin(action, &userId))
   {
      // get directive ID from params
      DynamicObject params;
      action->getResourceParams(params);

      // return true even if directive doesn't exist
      rval = true;
      removeDirective(userId, params[0]->getString());
   }

   return rval;
}

bool DirectiveService::getDirectives(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = false;

   UserId userId;
   if(mNode->checkLogin(action, &userId))
   {
      rval = true;

      // turn user ID into a key
      char key[22];
      snprintf(key, 22, "%" PRIu64, userId);

      // return user's directive cache
      mCacheLock.lock();
      {
         if(mDirectives->hasMember(key))
         {
            out = mDirectives[key];
         }
         else
         {
            out->setType(Map);
         }
      }
      mCacheLock.unlock();
   }

   return rval;
}

bool DirectiveService::processDirective(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = false;

   UserId userId;
   if(mNode->checkLogin(action, &userId))
   {
      // get directive by its ID
      DynamicObject params;
      action->getResourceParams(params);
      const char* id = params[0]->getString();
      DynamicObject directive(NULL);

      // remove directive from cache
      removeDirective(userId, id, &directive);

      // ensure directive is not invalid
      if(directive.isNull())
      {
         // directive not found
         ExceptionRef e = new Exception(
            "Could not process directive. Directive does not exist.",
            "bitmunk.system.DirectiveService.InvalidDirectiveId", 404);
         e->getDetails()["userId"] = userId;
         e->getDetails()["directiveId"] = id;
         Exception::set(e);
      }
      // process directive
      else
      {
         // include user ID in directive
         directive["userId"] = userId;

         // include directive in output
         out["userId"] = userId;
         out["directive"] = directive;

         // run directive processor fiber
         DirectiveProcessor* dp = new DirectiveProcessor(mNode);
         dp->setDirective(directive);
         mNode->getFiberScheduler()->addFiber(dp);
         rval = true;
      }
   }

   return rval;
}

void DirectiveService::insertDirective(
   DynamicObject& cache, DynamicObject& directive)
{
   // Note: Assume cache lock is engaged.

   // get random number for ID
   char tmp[22];
   do
   {
      // get a random number between 1 and 1000000000
      uint64_t n = Random::next(1, 1000000000);
      snprintf(tmp, 22, "%" PRIu64, n);
   }
   while(cache->hasMember(tmp));

   // set directive ID and insert into cache
   directive["id"] = tmp;
   cache[tmp] = directive;
}

void DirectiveService::removeDirective(
   UserId userId, const char* directiveId, DynamicObject* directive)
{
   // turn user ID into a key
   char key[22];
   snprintf(key, 22, "%" PRIu64, userId);

   // lock to modify cache
   mCacheLock.lock();
   {
      // make sure user's cache exists
      if(mDirectives->hasMember(key))
      {
         DynamicObject& cache = mDirectives[key];
         if(cache->hasMember(directiveId))
         {
            // save the directive if appropriate
            if(directive != NULL)
            {
               *directive = cache[directiveId];
            }

            // remove the directive
            cache->removeMember(directiveId);
         }
      }
   }
   mCacheLock.unlock();
}

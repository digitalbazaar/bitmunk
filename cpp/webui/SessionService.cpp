/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/webui/SessionService.h"

#include "bitmunk/common/Logging.h"
#include "bitmunk/node/BtpActionDelegate.h"
#include "bitmunk/node/RestResourceHandler.h"
#include "monarch/validation/Validation.h"

using namespace std;
using namespace monarch::config;
using namespace monarch::net;
using namespace monarch::rt;
using namespace bitmunk::common;
using namespace bitmunk::protocol;
using namespace bitmunk::node;
using namespace bitmunk::webui;
namespace v = monarch::validation;

typedef BtpActionDelegate<SessionService> Handler;

#define REGEX_IP_ADDRESS \
   "^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}" \
   "([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$"

SessionService::SessionService(SessionManager* sm, const char* path) :
   BtpService(path),
   mSessionManager(sm)
{
}

SessionService::~SessionService()
{
}

bool SessionService::initialize()
{
   // login
   {
      RestResourceHandlerRef login = new RestResourceHandler();
      addResource("/login", login);

      // POST .../login
      {
         ResourceHandler h = new Handler(
            mSessionManager->getNode(),
            this, &SessionService::createSession,
            BtpAction::AuthOptional);

         v::ValidatorRef validator = new v::Map(
            "username", new v::All(
               new v::Type(String),
               new v::Regex(
                  "^[^[:space:]]{1}.*$",
                  "Username must not start with whitespace."),
               new v::Regex(
                  "^.*[^[:space:]]{1}$",
                  "Username must not end with whitespace."),
               new v::Min(4, "Username too short. 4 characters minimum."),
               new v::Max(100, "Username too long. 100 characters maximum."),
               NULL),
            "password", new v::All(v::ValidatorContext::MaskInvalidValues,
               new v::Type(String),
               new v::Min(8, "Password too short. 8 characters minimum."),
               new v::Max(30, "Password too long. 30 characters maximum."),
               NULL),
            NULL);

         login->addHandler(h, BtpMessage::Post, 0, NULL, &validator);
      }
   }

   // logout
   {
      RestResourceHandlerRef logout = new RestResourceHandler();
      addResource("/logout", logout);

      // POST .../logout
      {
         ResourceHandler h = new Handler(
            mSessionManager->getNode(),
            this, &SessionService::deleteSession,
            BtpAction::AuthOptional);

         v::ValidatorRef qValidator = new v::Map(
            "full", new v::Optional(new v::Regex("^(true|false)$")),
            NULL);

         logout->addHandler(h, BtpMessage::Post, 0, &qValidator);
      }
   }

   // session userdata
   {
      RestResourceHandlerRef userdata = new RestResourceHandler();
      addResource("/session/userdata", userdata);

      // POST .../session/userdata
      {
         ResourceHandler h = new Handler(
            mSessionManager->getNode(),
            this, &SessionService::setSessionUserdata,
            BtpAction::AuthOptional);

         v::ValidatorRef validator = new v::Type(Map);

         userdata->addHandler(h, BtpMessage::Post, 0, NULL, &validator);
      }

      // GET .../session/userdata
      {
         ResourceHandler h = new Handler(
            mSessionManager->getNode(),
            this, &SessionService::getSessionUserdata,
            BtpAction::AuthOptional);
         userdata->addHandler(h, BtpMessage::Get, 0);
      }
   }

   // session access
   {
      RestResourceHandlerRef access = new RestResourceHandler();
      addResource("/session/access", access);

      // GET .../session/access/users
      {
         ResourceHandler h = new Handler(
            mSessionManager->getNode(), this,
            &SessionService::getFullAccessControlList,
            BtpAction::AuthOptional);
         access->addHandler(h, BtpMessage::Get, 0);
      }

      // GET .../session/access/users/<userId>
      {
         ResourceHandler h = new Handler(
            mSessionManager->getNode(), this,
            &SessionService::getAccessControlList,
            BtpAction::AuthOptional);
         access->addHandler(h, BtpMessage::Get, 1);
      }
   }

   // session grant
   {
      RestResourceHandlerRef grant = new RestResourceHandler();
      addResource("/session/access/grant", grant);

      // POST .../session/access/grant
      {
         ResourceHandler h = new Handler(
            mSessionManager->getNode(), this, &SessionService::grantAccess,
            BtpAction::AuthOptional);

         v::ValidatorRef validator = new v::All(
            new v::Type(Array),
            new v::Each(new v::Any(
               new v::Map(
                  "userId", new v::Int(v::Int::Positive),
                  "ip", new v::Any(
                     new v::Regex(
                        REGEX_IP_ADDRESS,
                        "IP address must be of the form: xxx.xxx.xxx.xxx"),
                     new v::Equals("*"),
                     NULL),
                  NULL),
               new v::Map(
                  "username", new v::All(
                     new v::Type(String),
                     new v::Regex(
                        "^[^[:space:]]{1}.*$",
                        "Username must not start with whitespace."),
                     new v::Regex(
                        "^.*[^[:space:]]{1}$",
                        "Username must not end with whitespace."),
                     new v::Min(
                        4, "Username too short. 4 characters minimum."),
                     new v::Max(
                        100, "Username too long. 100 characters maximum."),
                     NULL),
                  "ip", new v::Any(
                     new v::Regex(
                        REGEX_IP_ADDRESS,
                        "IP address must be of the form: xxx.xxx.xxx.xxx"),
                     new v::Equals("*"),
                     NULL),
                  NULL),
               NULL)));

         grant->addHandler(h, BtpMessage::Post, 0, NULL, &validator);
      }
   }

   // session revoke
   {
      RestResourceHandlerRef revoke = new RestResourceHandler();
      addResource("/session/access/revoke", revoke);

      // POST .../session/access/revoke
      {
         ResourceHandler h = new Handler(
            mSessionManager->getNode(), this, &SessionService::revokeAccess,
            BtpAction::AuthOptional);

         v::ValidatorRef validator = new v::All(
            new v::Type(Array),
            new v::Each(new v::Any(
               new v::Map(
                  "userId", new v::Int(v::Int::Positive),
                  "ip", new v::Any(
                     new v::Regex(
                        REGEX_IP_ADDRESS,
                        "IP address must be of the form: xxx.xxx.xxx.xxx"),
                     new v::Equals("*"),
                     NULL),
                  NULL),
               new v::Map(
                  "username", new v::All(
                     new v::Type(String),
                     new v::Regex(
                        "^[^[:space:]]{1}.*$",
                        "Username must not start with whitespace."),
                     new v::Regex(
                        "^.*[^[:space:]]{1}$",
                        "Username must not end with whitespace."),
                     new v::Min(
                        4, "Username too short. 4 characters minimum."),
                     new v::Max(
                        100, "Username too long. 100 characters maximum."),
                     NULL),
                  "ip", new v::Any(
                     new v::Regex(
                        REGEX_IP_ADDRESS,
                        "IP address must be of the form: xxx.xxx.xxx.xxx"),
                     new v::Equals("*"),
                     NULL),
                  NULL),
               NULL)));

         revoke->addHandler(h, BtpMessage::Post, 0, NULL, &validator);
      }
   }

   return true;
}

void SessionService::cleanup()
{
   // remove resources
   removeResource("/login");
   removeResource("/logout");
   removeResource("/session/userdata");
   removeResource("/session/access");
   removeResource("/session/access/grant");
   removeResource("/session/access/revoke");
}

bool SessionService::createSession(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = false;

   // get client IP
   InternetAddress address;
   if((rval = action->getClientInternetAddress(&address)))
   {
      // create session
      UserId userId;
      string session;
      if((rval = mSessionManager->createSession(
         action->getResponse()->getHeader(),
         in["username"]->getString(), in["password"]->getString(),
         &address, session, &userId)))
      {
         // no output, only cookies returned
         out.setNull();
      }
   }

   return rval;
}

bool SessionService::deleteSession(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   UserId userId = 0;
   bool logout = false;

   DynamicObject vars;
   action->getResourceQuery(vars);
   if(vars->hasMember("full") && vars["full"]->getBoolean())
   {
      // see if session is valid, get user ID, if it is invalid, it
      // will be deleted automatically
      if(mSessionManager->isSessionValid(action, &userId))
      {
         // do logout after deleting session
         logout = true;
      }
   }
   else
   {
      // delete session (which always removes the client's cookie but will only
      // remove the session from the manager if it is valid, thereby protecting
      // users from unauthorized users deleting other users' sessions)
      mSessionManager->deleteSession(action);
   }

   // immediately send result
   action->sendResult();

   if(logout)
   {
      // do logout
      mSessionManager->getNode()->logout(userId);
   }

   return true;
}

bool SessionService::setSessionUserdata(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = false;

   // setting user data will fail if the session is invalid
   if(mSessionManager->setSessionUserdata(action, in))
   {
      out = in;
      rval = true;
   }

   return rval;
}

bool SessionService::getSessionUserdata(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = false;

   // user data will be null if the session is invalid
   DynamicObject userdata = mSessionManager->getSessionUserdata(action);
   if(!userdata.isNull())
   {
      out = userdata;
      rval = true;
   }

   return rval;
}

bool SessionService::grantAccess(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = true;

   // ensure session is valid
   if(mSessionManager->isSessionValid(action))
   {
      DynamicObjectIterator i = in.getIterator();
      while(rval && i->hasNext())
      {
         DynamicObject& entry = i->next();

         // get bitmunk user ID as needed
         UserId userId;
         if(entry->hasMember("userId"))
         {
            userId = entry["userId"]->getUInt64();
         }
         else
         {
            Url url;
            url.format("/api/3.0/users?username=%s",
               entry["username"]->getString());
            DynamicObject user;
            if(mSessionManager->getNode()->getMessenger()->getFromBitmunk(
               &url, user))
            {
               userId = user["id"]->getUInt64();
            }
            else
            {
               // invalid user
               userId = 0;
               rval = false;
            }
         }

         if(rval)
         {
            const char* ip = (entry->hasMember("ip") ?
               entry["ip"]->getString() : NULL);

            // grant access to user
            rval = mSessionManager->grantAccess(userId, ip);
         }
      }
   }

   return rval;
}

bool SessionService::revokeAccess(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = true;

   // ensure session is valid
   if(mSessionManager->isSessionValid(action))
   {
      DynamicObjectIterator i = in.getIterator();
      while(rval && i->hasNext())
      {
         DynamicObject& entry = i->next();

         // get bitmunk user ID as needed
         UserId userId;
         if(entry->hasMember("userId"))
         {
            userId = entry["userId"]->getUInt64();
         }
         else
         {
            Url url;
            url.format("/api/3.0/users?username=%s",
               entry["username"]->getString());
            DynamicObject user;
            if(mSessionManager->getNode()->getMessenger()->getFromBitmunk(
               &url, user))
            {
               userId = user["id"]->getUInt64();
            }
            else
            {
               // invalid user
               userId = 0;
               rval = false;
            }
         }

         if(rval)
         {
            const char* ip = (entry->hasMember("ip") ?
               entry["ip"]->getString() : NULL);

            // revoke access from user
            rval = mSessionManager->revokeAccess(userId, ip);
         }
      }
   }

   return rval;
}

bool SessionService::getFullAccessControlList(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = false;

   // ensure session is valid
   if(mSessionManager->isSessionValid(action))
   {
      rval = mSessionManager->getAccessControlList(out);
   }

   return rval;
}

bool SessionService::getAccessControlList(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = false;

   if(mSessionManager->isSessionValid(action))
   {
      DynamicObject params;
      action->getResourceParams(params);
      UserId userId = params[0]->getUInt64();
      rval = mSessionManager->getAccessControlList(out, &userId);
   }

   return rval;
}

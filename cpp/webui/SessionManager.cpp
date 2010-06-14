/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk/webui/SessionManager.h"

#include "monarch/crypto/MessageDigest.h"
#include "monarch/http/CookieJar.h"
#include "monarch/util/Random.h"

using namespace std;
using namespace monarch::crypto;
using namespace monarch::http;
using namespace monarch::net;
using namespace monarch::rt;
using namespace monarch::util;
using namespace bitmunk::common;
using namespace bitmunk::protocol;
using namespace bitmunk::node;
using namespace bitmunk::webui;

// 1800 seconds is a 30 minute timeout
#define SESSION_TIMEOUT_MIN   30 * 60
#define SESSION_TIMEOUT_MS    (uint64_t)(SESSION_TIMEOUT_MIN * 60 * 1000)
#define COOKIES_SECURE        true
#define COOKIES_HTTPONLY      true

SessionManager::SessionManager(Node* node) :
   mNode(node)
{
}

SessionManager::~SessionManager()
{
   // clean up all sessions
   for(SessionMap::iterator i = mSessions.begin(); i != mSessions.end(); ++i)
   {
      free((char*)i->first);
   }
}

bool SessionManager::initialize()
{
   // initialize session database
   return mSessionDatabase.initialize(mNode);
}

bool SessionManager::createSession(
   HttpHeader* header,
   const char* username, const char* password,
   InternetAddress* ip, string& session, UserId* userId)
{
   bool rval = false;

   // check access control list for user
   if(checkAccessControl(username, ip))
   {
      // store the user's ID when logging in
      UserId tmpUserId;
      if(userId == NULL)
      {
         userId = &tmpUserId;
      }

      // try to log in user
      if(mNode->login(username, password, userId))
      {
         // get random string
         char str[22];
         snprintf(str, 22, "%" PRIu64, Random::next(1, 1000000000));

         // get current time
         uint64_t time = System::getCurrentMilliseconds();

         // create session ID: digest username+password+ip+random
         MessageDigest md;
         md.start("SHA1");
         md.update(username);
         md.update(password);
         md.update(ip->getAddress());
         md.update(str);
         session = md.getDigest();

         // create session data
         SessionData sd;
         sd.id = session.c_str();
         sd.userId = *userId;
         sd.username = username;
         sd.ip = ip->getAddress();
         sd.time = time;
         sd.valid = true;

         // lock to modify sessions
         mSessionLock.lock();
         {
            // clean up existing duplicate session, clean up invalid sessions
            bool removed = false;
            SessionMap::iterator tmpi;
            SessionMap::iterator i = mSessions.begin();
            while(i != mSessions.end())
            {
               // check expiration on session
               if((time - i->second.time) > SESSION_TIMEOUT_MS)
               {
                  // session is no longer valid
                  i->second.valid = false;
               }

               // clean up invalid sessions
               if(!i->second.valid)
               {
                  // save iterator and increment
                  tmpi = i++;

                  // clean up session
                  const char* tmp = tmpi->first;
                  mSessions.erase(tmpi);
                  free((char*)tmp);
               }
               else if(!removed)
               {
                  // clean up duplicate session
                  if(sd.userId == i->second.userId &&
                     strcmp(sd.ip.c_str(), i->second.ip.c_str()) == 0)
                  {
                     // save iterator and increment
                     tmpi = i++;

                     // clean up session
                     const char* tmp = tmpi->first;
                     mSessions.erase(tmpi);
                     free((char*)tmp);
                     removed = true;
                  }
                  else
                  {
                     ++i;
                  }
               }
               else
               {
                  ++i;
               }
            }

            // add the new session
            mSessions.insert(make_pair(strdup(session.c_str()), sd));
         }
         mSessionLock.unlock();

         // set cookies
         setCookies(
            header, session.c_str(),
            (userId == NULL ? tmpUserId : *userId), username);

         rval = true;
      }
   }

   return rval;
}

void SessionManager::deleteSession(BtpAction* action)
{
   // read session cookie from request header
   CookieJar jar;
   HttpHeader* header = action->getRequest()->getHeader();
   jar.readCookies(header, CookieJar::Client);
   Cookie cookie = jar.getCookie("bitmunk-session");
   if(!cookie.isNull())
   {
      // get session value
      const char* session = cookie["value"]->getString();

      // lock to modify sessions
      mSessionLock.lock();
      {
         // ensure the session is valid before removing it from the session
         // manager (which is different from simply deleting the cookies on
         // the client ... which is always permitted)
         SessionMap::iterator i = mSessions.find(session);
         if(i != mSessions.end())
         {
            InternetAddress ip;
            if(action->getClientInternetAddress(&ip) &&
               strcmp(ip.getAddress(), i->second.ip.c_str()) == 0)
            {
               // session valid, IP matches, so remove it
               const char* tmp = i->first;
               mSessions.erase(i);
               free((char*)tmp);
            }
         }
      }
      mSessionLock.unlock();
   }

   // delete cookies in response header
   jar.deleteCookie("bitmunk-session", COOKIES_SECURE);
   jar.deleteCookie("bitmunk-user-id", COOKIES_SECURE);
   jar.deleteCookie("bitmunk-username", COOKIES_SECURE);
   header = action->getResponse()->getHeader();
   jar.writeCookies(header, CookieJar::Server, false);
}

bool SessionManager::isSessionValid(
   BtpAction* action, bitmunk::common::UserId* userId)
{
   bool rval = false;

   // get a lock so that session data can be updated
   mSessionLock.lock();
   {
      // get session data
      SessionData* sd = getSessionData(action);
      if(sd != NULL)
      {
         // ensure user is logged in
         UserId uid = sd->userId;
         rval = mNode->getLoginData(uid, NULL, NULL);
         if(rval)
         {
            // update passed in user ID
            if(userId != NULL)
            {
               *userId = uid;
            }

            // update cookies
            setCookies(
               action->getResponse()->getHeader(), sd->id.c_str(), uid,
               sd->username.c_str());
         }
      }
   }
   mSessionLock.unlock();

   if(!rval)
   {
      // delete invalid session
      deleteSession(action);

      ExceptionRef e = new Exception(
         "Invalid session.",
         "bitmunk.webui.SessionManager.InvalidSession");
      Exception::push(e);
   }

   return rval;
}

void SessionManager::refreshCookies(BtpAction* action)
{
   isSessionValid(action);
}

bool SessionManager::setSessionUserdata(
   BtpAction* action, DynamicObject& userdata)
{
   bool rval = false;

   // lock to update from session
   mSessionLock.lock();
   {
      SessionData* sd = getSessionData(action);
      if(sd != NULL)
      {
         // update userdata
         sd->userdata = userdata;
         rval = true;
      }
   }
   mSessionLock.unlock();

   return rval;
}

DynamicObject SessionManager::getSessionUserdata(BtpAction* action)
{
   DynamicObject rval(NULL);

   // lock to read from sessions
   mSessionLock.lock();
   {
      SessionData* sd = getSessionData(action);
      if(sd != NULL)
      {
         // get userdata
         rval = sd->userdata;
      }
   }
   mSessionLock.lock();

   return rval;
}

bool SessionManager::grantAccess(UserId userId, const char* ip)
{
   if(ip == NULL)
   {
      ip = "*";
   }

   return mSessionDatabase.insertAccessControlEntry(userId, ip);
}

bool SessionManager::revokeAccess(UserId userId, const char* ip)
{
   if(ip == NULL)
   {
      ip = "*";
   }

   return mSessionDatabase.deleteAccessControlEntry(userId, ip);
}

bool SessionManager::getAccessControlList(
   DynamicObject& entries, UserId* userId)
{
   return (userId != NULL) ?
      mSessionDatabase.getAccessControlList(*userId, entries) :
      mSessionDatabase.getAccessControlList(entries);
}

Node* SessionManager::getNode()
{
   return mNode;
}

bool SessionManager::checkAccessControl(
   const char* username, InternetAddress* ip)
{
   bool rval;

   // check the address against 127.x.x.x for localhost status, any user
   // has access control from localhost
   rval = (strncmp(ip->getAddress(), "127.", 4) == 0);
   if(!rval)
   {
      // not localhost, will have to check session database:

      // get bitmunk user ID
      Url url;
      url.format("/api/3.0/users?username=%s", username);
      DynamicObject user;
      if(mNode->getMessenger()->getFromBitmunk(&url, user))
      {
         rval = mSessionDatabase.checkAccessControl(
            BM_USER_ID(user["id"]), ip->getAddress());
      }
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Access denied for user.",
         "bitmunk.webui.SessionManager.AccessDenied");
      e->getDetails()["username"] = username;
      e->getDetails()["ip"] = ip->getAddress();
      Exception::push(e);
   }

   return rval;
}

void SessionManager::setCookies(
   HttpHeader* header, const char* session, UserId userId, const char* username)
{
   // convert user ID into a string
   char tmp[22];
   snprintf(tmp, 22, "%" PRIu64, userId);

   CookieJar jar;
   jar.setCookie(
      "bitmunk-session", session,
      SESSION_TIMEOUT_MIN, COOKIES_SECURE, COOKIES_HTTPONLY);
   jar.setCookie(
      "bitmunk-user-id", tmp,
      SESSION_TIMEOUT_MIN, COOKIES_SECURE, false);
   jar.setCookie(
      "bitmunk-username", Url::encode(username).c_str(),
      SESSION_TIMEOUT_MIN, COOKIES_SECURE, false);
   jar.writeCookies(header, CookieJar::Server, true);
}

void SessionManager::clearCookies(HttpHeader* header)
{
   CookieJar jar;
   jar.deleteCookie("bitmunk-session", COOKIES_SECURE);
   jar.deleteCookie("bitmunk-user-id", COOKIES_SECURE);
   jar.deleteCookie("bitmunk-username", COOKIES_SECURE);
   jar.writeCookies(header, CookieJar::Server, true);
}

bool SessionManager::getSessionFromAction(
   BtpAction* action, string& session, InternetAddress* ip)
{
   bool rval = true;

   // get client cookies
   CookieJar jar;
   jar.readCookies(action->getRequest()->getHeader(), CookieJar::Client);

   // check for bitmunk-session cookie
   Cookie cookie = jar.getCookie("bitmunk-session");
   if(cookie.isNull())
   {
      ExceptionRef e = new Exception(
         "No 'bitmunk-session' cookie.",
         "bitmunk.webui.SessionManager.MissingCookie");
      e->getDetails()["missingCookie"] = "bitmunk-session";
      Exception::set(e);
      rval = false;
   }
   else
   {
      // get session ID
      session = cookie["value"]->getString();
   }

   if(rval)
   {
      // get IP
      rval = action->getClientInternetAddress(ip);
   }

   return rval;
}

SessionManager::SessionData* SessionManager::getSessionData(BtpAction* action)
{
   SessionData* rval = NULL;

   string session;
   InternetAddress address;
   if(getSessionFromAction(action, session, &address))
   {
      rval = getSessionData(session.c_str(), &address);
   }

   return rval;
}

SessionManager::SessionData* SessionManager::getSessionData(
   const char* session, InternetAddress* ip)
{
   SessionData* rval = NULL;

   SessionMap::iterator i = mSessions.find(session);
   if(i != mSessions.end() && i->second.valid)
   {
      if(strcmp(ip->getAddress(), i->second.ip.c_str()) == 0)
      {
         // session valid, IP matches
         rval = &i->second;

         // check expiration on session
         uint64_t time = System::getCurrentMilliseconds();
         if((time - i->second.time) > SESSION_TIMEOUT_MS)
         {
            // session is no longer valid
            i->second.valid = false;

            // session has expired
            ExceptionRef e = new Exception(
               "Session is invalid. It has expired.",
               "bitmunk.webui.SessionManager.SessionExpired");
            Exception::set(e);
         }
         else
         {
            // update session time
            i->second.time = time;

            // session valid, IP matches
            rval = &i->second;
         }
      }
      else
      {
         // session is not valid for the given IP
         ExceptionRef e = new Exception(
            "Session IP invalid. IP address does not match session ID.",
            "bitmunk.webui.SessionManager.InvalidSessionIP");
         Exception::set(e);
      }
   }
   else
   {
      // session ID doesn't exist in sessions map
      ExceptionRef e = new Exception(
         "Session ID invalid. No such session.",
         "bitmunk.webui.SessionManager.InvalidSessionId");
      Exception::set(e);
   }

   return rval;
}

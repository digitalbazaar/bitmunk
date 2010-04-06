/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk/node/LoginManager.h"

#include "bitmunk/common/Logging.h"
#include "bitmunk/node/Node.h"
#include "bitmunk/node/BtpServer.h"
#include "monarch/config/ConfigManager.h"
#include "monarch/crypto/AsymmetricKeyFactory.h"
#include "monarch/event/EventWaiter.h"
#include "monarch/io/FileInputStream.h"
#include "monarch/io/FileOutputStream.h"
#include "monarch/rt/RunnableDelegate.h"

#include <algorithm>

using namespace std;
using namespace bitmunk::common;
using namespace bitmunk::protocol;
using namespace bitmunk::node;
using namespace monarch::config;
using namespace monarch::crypto;
using namespace monarch::event;
using namespace monarch::io;
using namespace monarch::modest;
using namespace monarch::net;
using namespace monarch::rt;

LoginManager::LoginManager(Node* node) :
   mNode(node),
   mBlockLogins(false)
{
}

LoginManager::~LoginManager()
{
}

bool LoginManager::loginUser(
   const char* username, const char* password, UserId* userId)
{
   bool rval = true;

   // set outgoing/incoming messages
   DynamicObject out;
   out["username"] = username;
   out["password"] = password;
   DynamicObject in;

   // post user's password to login
   UserId id = 0;
   Url url("/api/3.0/users/login");
   rval = mNode->getMessenger()->postSecureToBitmunk(&url, &out, &in);
   if(rval)
   {
      if(!in->hasMember("userId"))
      {
         ExceptionRef e = new Exception(
            "Logged in but no user id.", "bitmunk.node.Login");
         Exception::set(e);
         rval = false;
      }
      else
      {
         // attempt to create user's login data
         ProfileRef profile(NULL);
         Config cfg = mNode->getConfigManager()->getNodeConfig();
         const char* cfgProfilePath = cfg["profilePath"]->getString();
         string profilePath;
         rval = mNode->getConfigManager()->expandBitmunkHomePath(
            cfgProfilePath, profilePath);
         if(rval)
         {
            size_t pNameLen = strlen(username) + 9;
            char pName[pNameLen];
            snprintf(pName, pNameLen, "%s.profile", username);
            string temp = File::join(profilePath.c_str(), pName);
            File file(temp.c_str());

            // make sure dirs exist in case profile is created
            if((rval = file->mkdirs()))
            {
               id = BM_USER_ID(in["userId"]);

               // try to load profile from disk
               profile = new Profile();
               profile->setUserId(id);
               rval = loadProfile(profile, password, file);
            }
         }

         if(rval)
         {
            // acquire login lock
            mLoginLock.lock();

            // if user is logging out, wait
            UserIdList::iterator i;
            do
            {
               i = find(mLogoutList.begin(), mLogoutList.end(), id);
            }
            while(i != mLogoutList.end() && mLoginLock.wait());

            // wait for logins to not be blocked
            while(mBlockLogins && mLoginLock.wait());

            if(i != mLogoutList.end() || mBlockLogins)
            {
               // thread interrupted while waiting and user still logging out
               ExceptionRef e = new Exception(
                  "User or all users are currently being logged out.",
                  "bitmunk.node.LoginManager.LogoutInProgress");
               Exception::set(e);
               rval = false;
            }
            else
            {
               // acquire exclusive login data lock
               mLoginDataLock.lockExclusive();

               // find existing user data, if any
               LoginData* oldData = getLoginDataUnlocked(id, NULL);
               if(oldData != NULL)
               {
                  // compare profiles
                  if(oldData->profile->getId() != profile->getId())
                  {
                     // profiles are same, already logged in, NULL new profile
                     profile.setNull();
                  }
                  else
                  {
                     // new profile, so logout old user
                     logoutUserUnblocked(id);
                  }
               }

               if(!profile.isNull())
               {
                  // loading new profile succeeded, so create login data
                  LoginData* data = new LoginData;
                  data->user["id"] = id;
                  data->user["username"] = username;
                  data->profile = profile;

                  // add new data to maps
                  mUsernameMap.insert(make_pair(username, data));
                  mUserIdMap.insert(make_pair(id, data));

                  // append to user ID queue
                  mUserIdQueue.push_back(id);
               }

               // release exclusive login data lock
               mLoginDataLock.unlockExclusive();

               // load user's config
               rval = mNode->getConfigManager()->loadUserConfig(id);
            }

            // release login lock
            mLoginLock.unlock();
         }
      }
   }

   if(!rval)
   {
      // set exception
      ExceptionRef e = new Exception(
         "Could not log in user.",
         "bitmunk.node.Login");
      e->getDetails()["username"] = username;
      Exception::push(e);
   }
   else
   {
      // return user ID, if requested
      if(userId != NULL)
      {
         *userId = id;
      }

      // no exception, post event
      Event e;
      e["type"] = "bitmunk.common.User.loggedIn";
      e["details"]["username"] = username;
      BM_ID_SET(e["details"]["userId"], id);
      mNode->getEventController()->schedule(e);
   }

   return rval;
}

void LoginManager::logoutUser(UserId id)
{
   // acquire login lock
   mLoginLock.lock();

   // wait for logins to not be blocked
   while(mBlockLogins)
   {
      mLoginLock.wait();
   }

   // do unblocked logout
   logoutUserUnblocked(id);

   // release login lock
   mLoginLock.unlock();
}

void LoginManager::logoutAllUsers()
{
   // engage block login flag
   mLoginLock.lock();
   mBlockLogins = true;
   mLoginLock.unlock();

   MO_CAT_DEBUG(BM_NODE_CAT, "Logging out all users...");

   // logout each user
   UserIdList tmp = mUserIdQueue;
   while(!tmp.empty())
   {
      UserId userId = tmp.front();
      logoutUserUnblocked(userId);
      tmp.pop_front();
   }

   MO_CAT_DEBUG(BM_NODE_CAT, "All users logged out.");

   // acquire locks
   mLoginLock.lock();
   mLoginDataLock.lockExclusive();

   // clear user ID queue and logout list
   mUserIdQueue.clear();
   mLogoutList.clear();

   // clear block login flag
   mBlockLogins = false;
   mLoginLock.notifyAll();

   // release locks
   mLoginDataLock.unlockExclusive();
   mLoginLock.unlock();
}

bool LoginManager::addUserOperation(UserId& id, Operation& op, ProfileRef* p)
{
   bool rval = false;

   // obtain the exclusive login data lock
   mLoginDataLock.lockExclusive();

   // get login data for the user
   LoginData* data = getLoginDataUnlocked(id, NULL);
   if(data != NULL)
   {
      // user is logged in, so return true
      rval = true;

      // add user operation
      data->opList.add(op);

      // prune user operations
      data->opList.prune();

      // get profile if appropriate
      if(p != NULL)
      {
         *p = data->profile;
      }
   }

   // release the exclusive login data lock
   mLoginDataLock.unlockExclusive();

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Could not add user operation. Specified user is not logged in.",
         "bitmunk.node.UserNotLoggedIn");
      e->getDetails()["userId"] = id;
      Exception::set(e);
   }

   return rval;
}

bool LoginManager::getLoginData(UserId& id, User* user, ProfileRef* profile)
{
   bool rval = false;

   // obtain the shared login data lock
   mLoginDataLock.lockShared();

   // get login data for the user
   LoginData* data = getLoginDataUnlocked(id, NULL);
   if(data != NULL)
   {
      // user is logged in, so return true
      rval = true;

      // get user if appropriate
      if(user != NULL)
      {
         *user = data->user;
      }

      // get profile if appropriate
      if(profile != NULL)
      {
         *profile = data->profile;
      }
   }
   else
   {
      ExceptionRef e = new Exception(
         "Could not get user's login data. User not logged in.",
         "bitmunk.node.UserNotLoggedIn");
      Exception::set(e);
   }

   // release the shared login data lock
   mLoginDataLock.unlockShared();

   return rval;
}

bool LoginManager::getLoginData(
   const char* username, User* user, ProfileRef* profile)
{
   bool rval = false;

   // obtain the shared login data lock
   mLoginDataLock.lockShared();

   // get login data for the user
   LoginData* data = getLoginDataUnlocked(username, NULL);
   if(data != NULL)
   {
      // user is logged in, so return true
      rval = true;

      // get user if appropriate
      if(user != NULL)
      {
         *user = data->user;
      }

      // get profile if appropriate
      if(profile != NULL)
      {
         *profile = data->profile;
      }
   }

   // release the shared login data lock
   mLoginDataLock.unlockShared();

   return rval;
}

LoginManager::LoginData* LoginManager::getLoginDataUnlocked(
   UserId& id, UserIdMap::iterator* itr)
{
   LoginData* rval = NULL;

   if(id == 0 && !mUserIdQueue.empty())
   {
      // user ID is 0, so get first user to sign in as the default
      id = mUserIdQueue.front();
   }

   if(id != 0)
   {
      // find login data
      UserIdMap::iterator i = mUserIdMap.find(id);
      if(i != mUserIdMap.end())
      {
         rval = i->second;
      }

      // pass iterator back if appropriate
      if(itr != NULL)
      {
         *itr = i;
      }
   }

   return rval;
}

LoginManager::LoginData* LoginManager::getLoginDataUnlocked(
   const char* username, UsernameMap::iterator* itr)
{
   LoginData* rval = NULL;

   // find login data
   UsernameMap::iterator i = mUsernameMap.find(username);
   if(i != mUsernameMap.end())
   {
      rval = i->second;
   }

   // pass iterator back if appropriate
   if(itr != NULL)
   {
      *itr = i;
   }

   return rval;
}

bool LoginManager::generateProfile(
   ProfileRef& p, const char* password, OutputStream* os)
{
   bool rval = false;

   PublicKeyRef key = p->generate();
   if(!key.isNull())
   {
      // send public key to UVA, get profile ID in return
      AsymmetricKeyFactory afk;
      string pem = afk.writePublicKeyToPem(key);

      // output, input objects
      DynamicObject out;
      out["password"] = password;
      out["publicKey"] = pem.c_str();
      DynamicObject in;

      // post public key securely
      Url url;
      url.format("/api/3.0/users/keys/public/%" PRIu64, p->getUserId());
      if((rval = mNode->getMessenger()->postSecureToBitmunk(&url, &out, &in)))
      {
         // set profile id
         p->setId(BM_PROFILE_ID(in["profileId"]));

         // save profile to a stream
         rval = p->save(password, os);
      }
   }

   return rval;
}

bool LoginManager::loadProfile(ProfileRef& p, const char* password, File& file)
{
   bool rval = false;

   if(file->exists())
   {
      // load profile
      FileInputStream fis(file);
      rval = p->load(password, &fis);
      fis.close();

      if(!rval)
      {
         // delete old profile, a new one will be generated
         file->remove();
      }
      else
      {
         // test profile by signing/verifying
         Exception::clear();
         PublicKeyRef key = mNode->getPublicKeyCache()->getPublicKey(
            p->getUserId(), p->getId(), NULL);
         if(key.isNull())
         {
            if(!Exception::isSet())
            {
               // if there is no exception, then assume there is
               // no public key for the user and that a new profile must be
               // generated, so delete the old profile
               file->remove();
            }
            else
            {
               // FIXME: clear any exception for now until all login stuff
               // is complete and functional
               Exception::clear();
               file->remove();
            }
         }
         else
         {
            // sign and verify data to check keys
            DigitalSignature* sign = p->createSignature();
            DigitalSignature* verify = new DigitalSignature(key);
            const char* data = "Data to sign, nothing special.";
            unsigned int length = strlen(data);
            sign->update(data, length);
            verify->update(data, length);

            char sig[sign->getValueLength()];
            sign->getValue(sig, length);
            if(!verify->verify(sig, length))
            {
               // profile is bad, so generate a new one
               file->remove();
            }

            // clean up
            delete sign;
            delete verify;
         }
      }
   }

   // determine if a new profile must be generated
   if(!file->exists())
   {
      // generate a new profile
      FileOutputStream fos(file);
      rval = generateProfile(p, password, &fos);
      fos.close();
   }

   // get a certificate if one is not set
   if(rval && p->getCertificate().isNull())
   {
      // create url for retrieving certificate
      Url url;
      url.format("/api/3.0/users/certificates/create");

      // get certificate
      DynamicObject out;
      out["userId"] = p->getUserId();
      out["password"] = password;
      out["profileId"] = p->getId();

      DynamicObject in;
      rval = mNode->getMessenger()->postSecureToBitmunk(&url, &out, &in);
      if(rval)
      {
         // FIXME: check uid & pid?
         // use factory to read cert from PEM string
         AsymmetricKeyFactory afk;
         X509CertificateRef cert;
         cert = afk.loadCertificateFromPem(
            in["certificate"]->getString(),
            in["certificate"]->length());
         if(!cert.isNull())
         {
            p->setCertificate(cert);

            // save profile with the new certificate
            FileOutputStream fos(file);
            rval = p->save(password, &fos);
            fos.close();
         }
         else
         {
            ExceptionRef e = new Exception(
               "Could not load certificate.",
               "bitmunk.node.CertificateError");
            Exception::set(e);
            rval = false;
         }
      }
   }

   if(rval)
   {
      // add virtual host entry to btp server
      rval = mNode->getBtpServer()->addVirtualHost(p);
   }

   return rval;
}

void LoginManager::logoutUserUnblocked(UserId id)
{
   Event e;
   e["type"] = "bitmunk.common.User.loggedOut";
   e["details"]["userId"] = id;

   // acquire login lock
   mLoginLock.lock();

   // acquire exclusive login data lock
   mLoginDataLock.lockExclusive();

   // get login data
   UserIdMap::iterator i;
   LoginData* data = getLoginDataUnlocked(id, &i);
   if(data != NULL)
   {
      MO_CAT_DEBUG(BM_NODE_CAT, "Logging out user: %" PRIu64, id);

      // use username in event
      e["details"]["username"] = data->user["username"]->getString();

      // remove virtual host entry
      SslContextRef ctx(NULL);
      mNode->getBtpServer()->removeVirtualHost(id, &ctx);

      // erase data from username map
      UsernameMap::iterator umi;
      if(getLoginDataUnlocked(data->user["username"]->getString(), &umi))
      {
         mUsernameMap.erase(umi);
      }

      // erase data from ID map
      mUserIdMap.erase(i);

      // remove entry from user queue
      UserIdList::iterator qi = find(
         mUserIdQueue.begin(), mUserIdQueue.end(), id);
      if(qi != mUserIdQueue.end())
      {
         mUserIdQueue.erase(qi);
      }

      // release exclusive login data lock
      mLoginDataLock.unlockExclusive();

      // add user ID to logout list
      mLogoutList.push_back(id);

      // prepare to wait for all user fibers to exit
      EventWaiter ew(mNode->getEventController());
      EventFilter ef;
      ef["details"]["userId"] = id;
      ew.start("bitmunk.node.Node.userFibersExited", &ef);

      // interrupt all user fibers
      mNode->interruptUserFibers(id);

      // save and remove user's config
      mNode->getConfigManager()->saveUserConfig(id);
      mNode->getConfigManager()->removeUserConfig(id);

      // release login lock
      mLoginLock.unlock();

      // wait for all user fibers
      ew.waitForEvent();
      ew.stop();

      // terminate all user's operations, clean up data
      data->opList.terminate();
      data->profile.setNull();
      delete data;

      // re-obtain login lock
      mLoginLock.lock();

      // remove user ID from logout list and notify
      UserIdList::iterator i = find(mLogoutList.begin(), mLogoutList.end(), id);
      mLogoutList.erase(i);
      mLoginLock.notifyAll();

      MO_CAT_DEBUG(BM_NODE_CAT, "Logged out user: %" PRIu64, id);
   }
   else
   {
      // release exclusive login data lock
      mLoginDataLock.unlockExclusive();
   }

   // post logout event
   mNode->getEventController()->schedule(e);

   // release login lock
   mLoginLock.unlock();
}

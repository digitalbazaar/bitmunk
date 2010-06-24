/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include "bitmunk/node/Node.h"

#include "bitmunk/nodemodule/NodeModule.h"
#include "bitmunk/node/BtpServer.h"
#include "monarch/data/json/JsonWriter.h"
#include "monarch/validation/Validation.h"

#include <algorithm>

using namespace std;
using namespace monarch::config;
using namespace monarch::crypto;
using namespace monarch::event;
using namespace monarch::data::json;
using namespace monarch::fiber;
using namespace monarch::io;
using namespace monarch::kernel;
using namespace monarch::logging;
using namespace monarch::modest;
using namespace monarch::net;
using namespace monarch::rt;
using namespace monarch::util;
using namespace bitmunk::common;
using namespace bitmunk::protocol;
using namespace bitmunk::node;
namespace v = monarch::validation;

// Logging category initialized during NodeModule initialization.
Category* BM_NODE_CAT;

Node::Node(MicroKernel* k) :
   mKernel(k),
   mEventHandler(this),
   mBtpServer(NULL),
   mLoginManager(this),
   mMessenger(NULL),
   mRunning(false)
{
   // NodeConfigManager needs to be able to get the ConfigManager from the
   // kernel
   mNodeConfigManager.setMicroKernel(k);

   // create BtpServer
   mBtpServer = new BtpServer(this);

   // set public key source for cache
   mPublicKeyCache.setPublicKeySource(&mPublicKeySource);
}

Node::~Node()
{
   delete mBtpServer;
}

/**
 * Validates the configuration for the node.
 *
 * @param cfg the configuration to validate.
 *
 * @return true if successful, false if an exception occurred.
 */
static bool _validateConfig(Config& cfg)
{
   bool rval = false;

   // create validator for node configuration
   v::ValidatorRef v = new v::Map(
      "host", new v::Type(String),
      "port", new v::Int(v::Int::NonNegative),
      "bitmunkUrl", new v::Type(String),
      "secureBitmunkUrl", new v::Type(String),
      "modulePath", new v::All(
         new v::Type(Array),
         new v::Each(new v::Type(String)),
         NULL),
      "sslGenerate", new v::Optional(new v::Type(Boolean)),
      "sslCertificate", new v::Type(String),
      "sslPrivateKey", new v::Type(String),
      "sslCAFile", new v::Type(String),
      NULL);

   rval = v->isValid(cfg);
   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Invalid Node configuration.",
         "bitmunk.node.InvalidNodeConfig");
      Exception::push(e);
   }

   return rval;
}

bool Node::initialize()
{
   bool rval = false;

   Config cfg = getConfigManager()->getNodeConfig();
   rval = _validateConfig(cfg);
   if(rval)
   {
      // dump some configuration information to the logs
      string home;
      getConfigManager()->getBitmunkHomePath(home);
      MO_CAT_INFO(BM_NODE_CAT,
         "Home: %s", home.c_str());
      MO_CAT_INFO(BM_NODE_CAT,
         "Bitmunk HTTP URL: %s", cfg["bitmunkUrl"]->getString());
      MO_CAT_INFO(BM_NODE_CAT,
         "Bitmunk HTTPS URL: %s", cfg["secureBitmunkUrl"]->getString());

      // setup messenger
      mMessenger = new Messenger(this, cfg);
      mPublicKeySource.setMessenger(mMessenger);
      BtpClient* btpc = mMessenger->getBtpClient();

      // setup verify certificate authority (CA) file
      File caFile(cfg["sslCAFile"]->getString());

      // init btp server, set up client SSL context, init event handling
      rval =
         mBtpServer->initialize(cfg) &&
         btpc->getSslContext()->setVerifyCAs(&caFile, NULL) &&
         mEventHandler.initialize();
      if(!rval)
      {
         // failed to initialize, clean up node
         cleanup();
      }
   }

   return rval;
}

void Node::cleanup()
{
   mEventHandler.cleanup();
   mBtpServer->cleanup();
   mMonitor.removeAll();
   mMessenger.setNull();
   mPublicKeyCache.clear();
}

bool Node::start()
{
   bool rval = mBtpServer->start();
   if(rval)
   {
      mRunning = true;

      // handle auto-login
      Config cfg = getConfigManager()->getNodeConfig();
      if(cfg["login"]["auto"]->getBoolean())
      {
         MO_CAT_INFO(BM_NODE_CAT, "Auto-login enabled.");

         const char* username = cfg["login"]["username"]->getString();
         const char* password = cfg["login"]["password"]->getString();
         if(login(username, password))
         {
            MO_CAT_INFO(BM_NODE_CAT,
               "Auto-login complete: username=%s", username);
         }
         else
         {
            MO_CAT_ERROR(BM_NODE_CAT,
               "Auto-login failed: username=%s, stopping..., %s",
               username, JsonWriter::writeToString(
                  Exception::getAsDynamicObject()).c_str());
            rval = false;
            stop();
         }
      }
      else
      {
         MO_CAT_INFO(BM_NODE_CAT, "Auto-login disabled.");
      }

      if(mRunning)
      {
         // send event that node has started
         Event e;
         e["type"] = "bitmunk.node.Node.started";
         getEventController()->schedule(e);
      }
   }

   return rval;
}

void Node::stop()
{
   if(mRunning)
   {
      // FIXME: might need to lock around this to prevent logins during stop()?
      logoutAllUsers();
      mBtpServer->stop();
      mRunning = false;

      // send event that node has stopped
      Event e;
      e["type"] = "bitmunk.node.Node.stopped";
      getEventController()->schedule(e);
   }
}

bool Node::login(const char* username, const char* password, UserId* userId)
{
   // FIXME: might need to lock here to prevent login during stop()
   return mLoginManager.loginUser(username, password, userId);
}

void Node::logout(UserId id)
{
   return mLoginManager.logoutUser(id);
}

void Node::logoutAllUsers()
{
   return mLoginManager.logoutAllUsers();
}

inline void Node::runOperation(Operation& op)
{
   // run operation on kernel
   mKernel->runOperation(op);
}

bool Node::runUserOperation(UserId& id, Operation& op)
{
   bool rval = false;

   // try to add the operation as a user operation
   if((rval = addUserOperation(id, op, NULL)))
   {
      // run operation on kernel
      mKernel->runOperation(op);
   }

   return rval;
}

Operation Node::currentOperation()
{
   return mKernel->currentOperation();
}

bool Node::addUserOperation(UserId& id, Operation& op, ProfileRef* p)
{
   return mLoginManager.addUserOperation(id, op, p);
}

void Node::addUserFiber(FiberId id, UserId userId)
{
   // if user is not logged in, interrupt fiber immediately
   if(!getLoginData(userId, NULL, NULL))
   {
      interruptFiber(id);
   }

   mUserFiberLock.lock();
   {
      // get user's fiber ID list
      UserFiberIdList* list;
      UserFiberMap::iterator i = mUserFibers.find(userId);
      if(i != mUserFibers.end())
      {
         // use existing list
         list = i->second;
      }
      else
      {
         // create new list
         list = new UserFiberIdList;
         mUserFibers.insert(make_pair(userId, list));
      }
      list->push_back(id);
   }
   mUserFiberLock.unlock();
}

void Node::removeUserFiber(FiberId id, UserId userId)
{
   mUserFiberLock.lock();
   {
      // erase fiber ID entry
      UserFiberMap::iterator i = mUserFibers.find(userId);
      if(i != mUserFibers.end())
      {
         UserFiberIdList* list = i->second;
         UserFiberIdList::iterator li = find(list->begin(), list->end(), id);
         if(li != list->end())
         {
            list->erase(li);
         }

         // clean up list if needed
         if(list->empty())
         {
            mUserFibers.erase(i);
            delete list;

            // send event that all user fibers have been cleaned up, send
            // event synchronously to ensure immediate delivery
            Event e;
            e["type"] = "bitmunk.node.Node.userFibersExited";
            e["details"]["userId"] = userId;
            getEventController()->schedule(e, false);
         }
      }
   }
   mUserFiberLock.unlock();
}

void Node::interruptFiber(FiberId id, FiberId sourceId)
{
   // send interrupt message
   DynamicObject msg;
   msg["fiberId"] = sourceId;
   msg["interrupt"] = true;
   getFiberMessageCenter()->sendMessage(id, msg);
}

void Node::interruptUserFibers(UserId id)
{
   mUserFiberLock.lock();
   {
      // find user's fibers
      UserFiberMap::iterator i = mUserFibers.find(id);
      if(i != mUserFibers.end())
      {
         // interrupt all of user's fibers
         UserFiberIdList* list = i->second;
         for(UserFiberIdList::iterator li = list->begin();
             li != list->end(); ++li)
         {
            FiberId fiberId = *li;
            interruptFiber(fiberId);
         }
      }
      else
      {
         // send event that all user fibers have been cleaned up, send
         // event synchronously to ensure immediate delivery
         Event e;
         e["type"] = "bitmunk.node.Node.userFibersExited";
         e["details"]["userId"] = id;
         getEventController()->schedule(e, false);
      }
   }
   mUserFiberLock.unlock();
}

bool Node::getLoginData(UserId& id, User* user, ProfileRef* profile)
{
   return mLoginManager.getLoginData(id, user, profile);
}

bool Node::getLoginData(const char* username, User* user, ProfileRef* profile)
{
   return mLoginManager.getLoginData(username, user, profile);
}

bool Node::checkLogin(
   BtpAction* action, UserId* userId, User* user, ProfileRef* profile)
{
   bool rval = true;

   UserId uid = action->getInMessage()->getUserId();
   if(!getLoginData(uid, user, profile))
   {
      ExceptionRef e = new Exception(
         "User not authorized.",
         "bitmunk.node.Node.UserNotAuthorized.");
      Exception::set(e);
      rval = false;
   }

   if(userId != NULL)
   {
      *userId = uid;
   }

   return rval;
}

UserId Node::getDefaultUserId()
{
   UserId id = 0;
   mLoginManager.getLoginData(id, NULL, NULL);
   return id;
}

bool Node::getDefaultProfile(ProfileRef& profile)
{
   bool rval;

   UserId id = 0;
   if((rval = getLoginData(id, NULL, &profile)))
   {
      if(profile.isNull())
      {
         ExceptionRef e = new Exception(
            "Could not get default user's profile. User has no profile.",
            "bitmunk.node.NoProfile");
         Exception::set(e);
         rval = false;
      }
   }

   return rval;
}

MicroKernelModuleApi* Node::getModuleApi(const char* name)
{
   return mKernel->getModuleApi(name);
}

MicroKernelModuleApi* Node::getModuleApiByType(const char* type)
{
   return mKernel->getModuleApiByType(type);
}

void Node::getModuleApisByType(
   const char* type, list<MicroKernelModuleApi*>& apiList)
{
   return mKernel->getModuleApisByType(type, apiList);
}

MicroKernel* Node::getKernel()
{
   return mKernel;
}

NodeConfigManager* Node::getConfigManager()
{
   return &mNodeConfigManager;
}

FiberScheduler* Node::getFiberScheduler()
{
   return mKernel->getFiberScheduler();
}

FiberMessageCenter* Node::getFiberMessageCenter()
{
   return mKernel->getFiberMessageCenter();
}

EventController* Node::getEventController()
{
   return mKernel->getEventController();
}

EventDaemon* Node::getEventDaemon()
{
   return mKernel->getEventDaemon();
}

Server* Node::getServer()
{
   return mKernel->getServer();
}

BtpServer* Node::getBtpServer()
{
   return mBtpServer;
}

Messenger* Node::getMessenger()
{
   return &(*mMessenger);
}

PublicKeyCache* Node::getPublicKeyCache()
{
   return &mPublicKeyCache;
}

NodeMonitor* Node::getMonitor()
{
   return &mMonitor;
}

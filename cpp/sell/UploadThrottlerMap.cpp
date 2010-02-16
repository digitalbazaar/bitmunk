/*
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk/sell/UploadThrottlerMap.h"

#include "bitmunk/sell/SellModule.h"
#include "monarch/net/BandwidthThrottlerChain.h"

#include <algorithm>

using namespace std;
using namespace bitmunk::common;
using namespace bitmunk::node;
using namespace bitmunk::sell;
using namespace monarch::config;
using namespace monarch::event;
using namespace monarch::net;
using namespace monarch::rt;

#define EVENT_USER_LOGGED_OUT   "bitmunk.common.User.loggedOut"
#define EVENT_CONFIG_CHANGED    "monarch.config.Config.changed"

UploadThrottlerMap::UploadThrottlerMap() :
   mNode(NULL),
   mGlobalThrottler(0),
   mUserLoggedOutObserver(NULL),
   mConfigChangedObserver(NULL)
{
}

UploadThrottlerMap::~UploadThrottlerMap()
{
   if(mNode != NULL)
   {
      if(!mConfigChangedObserver.isNull())
      {
         mNode->getEventController()->unregisterObserver(
            &(*mConfigChangedObserver), EVENT_CONFIG_CHANGED);
         mConfigChangedObserver.setNull();
         MO_CAT_DEBUG(BM_SELL_CAT,
            "UploadThrottlerMap unregistered for " EVENT_CONFIG_CHANGED);
      }

      if(!mUserLoggedOutObserver.isNull())
      {
         mNode->getEventController()->unregisterObserver(
            &(*mUserLoggedOutObserver), EVENT_USER_LOGGED_OUT);
         mUserLoggedOutObserver.setNull();
         MO_CAT_DEBUG(BM_SELL_CAT,
            "UploadThrottlerMap unregistered for " EVENT_USER_LOGGED_OUT);
      }
   }
}

bool UploadThrottlerMap::initialize(Node* node)
{
   bool rval = false;

   mNode = node;

   Config cfg = node->getConfigManager()->getModuleConfig("bitmunk.sell.Sell");
   if(!cfg.isNull())
   {
      rval = true;

      // register observer to cleanup user data on logout
      mUserLoggedOutObserver = new ObserverDelegate<UploadThrottlerMap>(
         this, &UploadThrottlerMap::userLoggedOut);
      mNode->getEventController()->registerObserver(
         &(*mUserLoggedOutObserver), EVENT_USER_LOGGED_OUT);
      MO_CAT_DEBUG(BM_SELL_CAT,
         "UploadThrottlerMap registered for " EVENT_USER_LOGGED_OUT);

      // set global upload rate limit
      mGlobalThrottler.setRateLimit(cfg["maxUploadRate"]->getInt32());

      // register observer to watch for changes to upload rate
      EventFilter ef;
      ef["details"]["config"]["bitmunk.sell.Sell"]->setType(Map);
      mConfigChangedObserver = new ObserverDelegate<UploadThrottlerMap>(
         this, &UploadThrottlerMap::configChanged);
      mNode->getEventController()->registerObserver(
         &(*mConfigChangedObserver), EVENT_CONFIG_CHANGED, &ef);
      MO_CAT_DEBUG(BM_SELL_CAT,
         "DownloadThrottlerMap registered for " EVENT_CONFIG_CHANGED);
   }

   return rval;
}

void UploadThrottlerMap::userLoggedOut(Event& event)
{
   UserId userId = BM_USER_ID(event["details"]["userId"]);
   mThrottlerMapLock.lockExclusive();
   {
      ThrottlerMap::iterator i = mThrottlerMap.find(userId);
      if(i != mThrottlerMap.end())
      {
         MO_CAT_DEBUG(BM_SELL_CAT,
            "UploadThrottlerMap cleanup for user: %" PRIu64, userId);
         mThrottlerMap.erase(i);
      }
   }
   mThrottlerMapLock.unlockExclusive();
}

void UploadThrottlerMap::configChanged(Event& event)
{
   Config& cfg = event["details"]["config"]["bitmunk.sell.Sell"];
   if(cfg->hasMember("maxUploadRate"))
   {
      // get new rate
      uint32_t rate = cfg["maxUploadRate"]->getInt32();

      // get user ID
      UserId userId = event["details"]->hasMember("userId") ?
         BM_USER_ID(event["details"]["userId"]) : 0;
      if(!BM_USER_ID_VALID(userId))
      {
         // apply globally
         mGlobalThrottler.setRateLimit(rate);
         MO_CAT_DEBUG(BM_SELL_CAT,
            "UploadThrottlerMap changed global rate to %u bps", rate);
      }
      else
      {
         // apply to a particular user's throttler
         mThrottlerMapLock.lockExclusive();
         {
            ThrottlerMap::iterator i = mThrottlerMap.find(userId);
            if(i != mThrottlerMap.end())
            {
               i->second->setRateLimit(rate);
               MO_CAT_DEBUG(BM_SELL_CAT,
                  "UploadThrottlerMap changed rate for user: %" PRIu64 " "
                  "to %u bps",
                  userId, rate);
            }
         }
         mThrottlerMapLock.unlockExclusive();
      }
   }
}

BandwidthThrottler* UploadThrottlerMap::getUserThrottler(UserId userId)
{
   BandwidthThrottler* rval = NULL;

   // shared check for user throttler
   mThrottlerMapLock.lockShared();
   {
      ThrottlerMap::iterator i = mThrottlerMap.find(userId);
      if(i != mThrottlerMap.end())
      {
         rval = &(*(i->second));
      }
   }
   mThrottlerMapLock.unlockShared();

   if(rval == NULL)
   {
      // check exclusive this time, creating throttler if necessary
      mThrottlerMapLock.lockExclusive();
      {
         ThrottlerMap::iterator i = mThrottlerMap.find(userId);
         if(i != mThrottlerMap.end())
         {
            rval = &(*(i->second));
         }

         if(rval == NULL)
         {
            // update global upload rate
            Config cfg = mNode->getConfigManager()->getModuleConfig(
               "bitmunk.sell.Sell");
            int globalRate = 0;
            if(!cfg.isNull())
            {
               // get global upload rate limit
               globalRate = cfg["maxUploadRate"]->getInt32();
               mGlobalThrottler.setRateLimit(globalRate);
            }

            // get user upload rate
            cfg = mNode->getConfigManager()->getModuleUserConfig(
               "bitmunk.sell.Sell", userId);
            int userRate = globalRate;
            if(!cfg.isNull())
            {
               // get user upload rate limit
               userRate = cfg["maxUploadRate"]->getInt32();
            }

            // create new default throttler for user
            BandwidthThrottlerRef bt = new DefaultBandwidthThrottler(userRate);

            // create throttler chain, insert it, return it
            BandwidthThrottlerChain* chain = new BandwidthThrottlerChain();
            chain->add(&mGlobalThrottler);
            chain->add(bt);
            BandwidthThrottlerRef chainRef = chain;
            mThrottlerMap.insert(make_pair(userId, chainRef));
            rval = chain;
         }
      }
      mThrottlerMapLock.unlockExclusive();
   }

   return rval;
}

/*
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk/purchase/DownloadThrottlerMap.h"

#include "bitmunk/common/Logging.h"
#include "bitmunk/purchase/PurchaseModule.h"
#include "monarch/net/BandwidthThrottlerChain.h"

#include <algorithm>

using namespace std;
using namespace bitmunk::common;
using namespace bitmunk::purchase;
using namespace bitmunk::node;
using namespace monarch::config;
using namespace monarch::event;
using namespace monarch::net;
using namespace monarch::rt;

#define EVENT_USER_LOGGED_OUT   "bitmunk.common.User.loggedOut"
#define EVENT_CONFIG_CHANGED    "monarch.config.Config.changed"

DownloadThrottlerMap::DownloadThrottlerMap() :
   mNode(NULL),
   mGlobalThrottler(0),
   mUserLoggedOutObserver(NULL),
   mConfigChangedObserver(NULL)
{
}

DownloadThrottlerMap::~DownloadThrottlerMap()
{
   if(mNode != NULL)
   {
      if(!mConfigChangedObserver.isNull())
      {
         mNode->getEventController()->unregisterObserver(
            &(*mConfigChangedObserver), EVENT_CONFIG_CHANGED);
         mConfigChangedObserver.setNull();
         MO_CAT_DEBUG(BM_PURCHASE_CAT,
            "DownloadThrottlerMap unregistered for " EVENT_CONFIG_CHANGED);
      }

      if(!mUserLoggedOutObserver.isNull())
      {
         mNode->getEventController()->unregisterObserver(
            &(*mUserLoggedOutObserver), EVENT_USER_LOGGED_OUT);
         mUserLoggedOutObserver.setNull();
         MO_CAT_DEBUG(BM_PURCHASE_CAT,
            "DownloadThrottlerMap unregistered for " EVENT_USER_LOGGED_OUT);
      }
   }
}

bool DownloadThrottlerMap::initialize(Node* node)
{
   bool rval = false;

   mNode = node;

   Config cfg = node->getConfigManager()->getModuleConfig(
      "bitmunk.purchase.Purchase");
   if(!cfg.isNull())
   {
      rval = true;

      // register observer to cleanup user data on logout
      mUserLoggedOutObserver = new ObserverDelegate<DownloadThrottlerMap>(
         this, &DownloadThrottlerMap::userLoggedOut);
      mNode->getEventController()->registerObserver(
         &(*mUserLoggedOutObserver), EVENT_USER_LOGGED_OUT);
      MO_CAT_DEBUG(BM_PURCHASE_CAT,
         "DownloadThrottlerMap registered for " EVENT_USER_LOGGED_OUT);

      // set global download rate limit
      mGlobalThrottler.setRateLimit(cfg["maxDownloadRate"]->getInt32());

      // register observer to watch for changes to download rate
      EventFilter ef;
      ef["details"]["config"]["bitmunk.purchase.Purchase"]->setType(Map);
      mConfigChangedObserver = new ObserverDelegate<DownloadThrottlerMap>(
         this, &DownloadThrottlerMap::configChanged);
      mNode->getEventController()->registerObserver(
         &(*mConfigChangedObserver), EVENT_CONFIG_CHANGED, &ef);
      MO_CAT_DEBUG(BM_PURCHASE_CAT,
         "DownloadThrottlerMap registered for " EVENT_CONFIG_CHANGED);
   }

   return rval;
}

void DownloadThrottlerMap::userLoggedOut(Event& event)
{
   UserId userId = event["details"]["userId"]->getUInt64();
   mThrottlerMapLock.lockExclusive();
   {
      ThrottlerMap::iterator i = mThrottlerMap.find(userId);
      if(i != mThrottlerMap.end())
      {
         MO_CAT_DEBUG(BM_PURCHASE_CAT,
            "DownloadThrottlerMap cleanup for user: %" PRIu64, userId);
         mThrottlerMap.erase(i);
      }
   }
   mThrottlerMapLock.unlockExclusive();
}

void DownloadThrottlerMap::configChanged(Event& event)
{
   Config& cfg = event["details"]["config"]["bitmunk.purchase.Purchase"];
   if(cfg->hasMember("maxDownloadRate"))
   {
      // get new rate
      uint32_t rate = cfg["maxDownloadRate"]->getInt32();

      // get user ID
      UserId userId = event["details"]->hasMember("userId") ?
         event["details"]["userId"]->getUInt64() : 0;
      if(userId == 0)
      {
         // apply globally
         mGlobalThrottler.setRateLimit(rate);
         MO_CAT_DEBUG(BM_PURCHASE_CAT,
            "DownloadThrottlerMap changed global rate to %u bps", rate);
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
               MO_CAT_DEBUG(BM_PURCHASE_CAT,
                  "DownloadThrottlerMap changed rate for user: %" PRIu64 " "
                  "to %u bps",
                  userId, rate);
            }
         }
         mThrottlerMapLock.unlockExclusive();
      }
   }
}
// FIXME: we could add download state ID as well, to increase granularity
// of bandwidth throttlers
BandwidthThrottler* DownloadThrottlerMap::getUserThrottler(UserId userId)
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
            // update global download rate
            Config cfg = mNode->getConfigManager()->getModuleConfig(
               "bitmunk.purchase.Purchase");
            int globalRate = 0;
            if(!cfg.isNull())
            {
               // get global download rate limit
               globalRate = cfg["maxDownloadRate"]->getInt32();
               mGlobalThrottler.setRateLimit(globalRate);
            }

            // get user download rate
            cfg = mNode->getConfigManager()->getModuleUserConfig(
               "bitmunk.purchase.Purchase", userId);
            int userRate = globalRate;
            if(!cfg.isNull())
            {
               // get user download rate limit
               userRate = cfg["maxDownloadRate"]->getInt32();
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

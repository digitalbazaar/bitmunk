/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_purchase_DownloadThrottler_H
#define bitmunk_purchase_DownloadThrottler_H

#include "bitmunk/node/Node.h"
#include "monarch/config/ConfigManager.h"
#include "monarch/event/ObserverDelegate.h"
#include "monarch/net/DefaultBandwidthThrottler.h"
#include "monarch/rt/SharedLock.h"

namespace bitmunk
{
namespace purchase
{

/**
 * A DownloadThrottlerMap is a class that is used to store users' bandwidth
 * throttlers.
 *
 * @author Dave Longley
 */
class DownloadThrottlerMap
{
protected:
   /**
    * The associated bitmunk Node.
    */
   bitmunk::node::Node* mNode;

   /**
    * The shared global bandwidth throttler.
    */
   monarch::net::DefaultBandwidthThrottler mGlobalThrottler;

   /**
    * Per-user bandwidth throttlers.
    */
   typedef std::map<bitmunk::common::UserId, monarch::net::BandwidthThrottlerRef>
      ThrottlerMap;
   ThrottlerMap mThrottlerMap;

   /**
    * Observer for user logged out event.
    */
   monarch::event::ObserverRef mUserLoggedOutObserver;

   /**
    * Observer for the config changed event.
    */
   monarch::event::ObserverRef mConfigChangedObserver;

   /**
    * User bandwidth throttler lock.
    */
   monarch::rt::SharedLock mThrottlerMapLock;

public:
   /**
    * Creates a new DownloadThrottlerMap.
    */
   DownloadThrottlerMap();

   /**
    * Destructs this DownloadThrottlerMap.
    */
   virtual ~DownloadThrottlerMap();

   /**
    * Initializes this DownloadThrottlerMap.
    *
    * @param node the node to use with this map.
    *
    * @return true if successful, false if not.
    */
   virtual bool initialize(bitmunk::node::Node* node);

   /**
    * Event handler to check *all* user logged out events and cleanup
    * throttlers if needed.
    *
    * @param event a logged out event.
    */
   virtual void userLoggedOut(monarch::event::Event& event);

   /**
    * Event handler to check for config changes.
    *
    * @param event a config changed event.
    */
   virtual void configChanged(monarch::event::Event& event);

   /**
    * Gets the download throttler for the given user ID.
    *
    * @param userId the ID of the user to get the throttler for.
    *
    * @return the user's bandwidth throttler.
    */
   virtual monarch::net::BandwidthThrottler* getUserThrottler(
      bitmunk::common::UserId userId);
};

} // end namespace purchase
} // end namespace bitmunk
#endif

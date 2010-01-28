/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_purchase_IPurchaseModule_H
#define bitmunk_purchase_IPurchaseModule_H

#include "bitmunk/purchase/DownloadThrottlerMap.h"
#include "bitmunk/purchase/PurchaseDatabase.h"
#include "monarch/kernel/MicroKernelModuleApi.h"
#include "monarch/util/RateAverager.h"

namespace bitmunk
{
namespace purchase
{

/**
 * An IPurchaseModule provides the interface for a PurchaseModule.
 * 
 * @author Dave Longley
 */
class IPurchaseModule : public monarch::kernel::MicroKernelModuleApi
{
public:
   /**
    * Creates a new IPurchaseModule.
    */
   IPurchaseModule() {};
   
   /**
    * Destructs this IPurchaseModule.
    */
   virtual ~IPurchaseModule() {};
   
   /**
    * Gets the purchase database.
    * 
    * @return the purchase database.
    */
   virtual PurchaseDatabase* getPurchaseDatabase() = 0;
   
   /**
    * Gets the download rate averager.
    * 
    * @return the download rate averager.
    */
   virtual monarch::util::RateAverager* getTotalDownloadRate() = 0;
   
   /**
    * Gets the download throttler for the given user.
    * 
    * @param userId the ID of the user to get the throttler for.
    * 
    * @return the user's download throttler.
    */
   virtual monarch::net::BandwidthThrottler* getDownloadThrottler(
      bitmunk::common::UserId userId) = 0;
   
   /**
    * Gets a specific DownloadState according to its IDs. Its contract,
    * seller data, and file progress will all be populated.
    * 
    * @param ds the DownloadState to populate with its "id" and "userId" set.
    * 
    * @return true if successful, false if an exception occurred.
    */
   virtual bool populateDownloadState(DownloadState& ds) = 0;
};

} // end namespace purchase
} // end namespace bitmunk
#endif

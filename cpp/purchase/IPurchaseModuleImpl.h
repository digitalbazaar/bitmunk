/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_purchase_IPurchaseModuleImpl_H
#define bitmunk_purchase_IPurchaseModuleImpl_H

#include "bitmunk/purchase/IPurchaseModule.h"

namespace bitmunk
{
namespace purchase
{

/**
 * An IPurchaseModule provides the interface for a PurchaseModule.
 * 
 * @author Dave Longley
 */
class IPurchaseModuleImpl : public IPurchaseModule
{
protected:
   /**
    * The purchase database.
    */
   PurchaseDatabase* mDatabase;
   
   /**
    * A RateAverager for all downloading data.
    */
   monarch::util::RateAverager mTotalDownloadRate;
   
   /**
    * A map of download throttlers for users.
    */
   DownloadThrottlerMap* mDownloadThrottlerMap;
   
public:
   /**
    * Creates a new IPurchaseModuleImpl.
    * 
    * @param pd the purchase database to use.
    * @param dm the download throttler map to use.
    */
   IPurchaseModuleImpl(PurchaseDatabase* pd, DownloadThrottlerMap* dm);
   
   /**
    * Destructs this IPurchaseModuleImpl.
    */
   virtual ~IPurchaseModuleImpl();
   
   /**
    * Gets the purchase database.
    * 
    * @return the purchase database.
    */
   virtual PurchaseDatabase* getPurchaseDatabase();
   
   /**
    * Gets the download rate averager.
    * 
    * @return the download rate averager.
    */
   virtual monarch::util::RateAverager* getTotalDownloadRate();
   
   /**
    * Gets the download throttler for the given user.
    * 
    * @param userId the ID of the user to get the throttler for.
    * 
    * @return the user's download throttler.
    */
   virtual monarch::net::BandwidthThrottler* getDownloadThrottler(
      bitmunk::common::UserId userId);
   
   /**
    * Gets a specific DownloadState according to its IDs. Its contract,
    * seller data, and file progress will all be populated.
    * 
    * @param ds the DownloadState to populate with its "id" and "userId" set.
    * 
    * @return true if successful, false if an exception occurred.
    */
   virtual bool populateDownloadState(DownloadState& ds);
};

} // end namespace purchase
} // end namespace bitmunk
#endif

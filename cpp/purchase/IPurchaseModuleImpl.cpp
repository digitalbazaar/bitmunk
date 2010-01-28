/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/purchase/IPurchaseModuleImpl.h"

using namespace bitmunk::common;
using namespace bitmunk::purchase;
using namespace monarch::net;
using namespace monarch::util;

IPurchaseModuleImpl::IPurchaseModuleImpl(
   PurchaseDatabase* pd, DownloadThrottlerMap* dt) :
   mDatabase(pd),
   mDownloadThrottlerMap(dt)
{
}

IPurchaseModuleImpl::~IPurchaseModuleImpl()
{
}

PurchaseDatabase* IPurchaseModuleImpl::getPurchaseDatabase()
{
   return mDatabase;
}

RateAverager* IPurchaseModuleImpl::getTotalDownloadRate()
{
   return &mTotalDownloadRate;
}

BandwidthThrottler* IPurchaseModuleImpl::getDownloadThrottler(UserId userId)
{
   return mDownloadThrottlerMap->getUserThrottler(userId);
}

bool IPurchaseModuleImpl::populateDownloadState(DownloadState& ds)
{
   return mDatabase->populateDownloadState(ds);
}

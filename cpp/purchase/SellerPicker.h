/*
 * Copyright (c) 2008 Digital Bazaar, Inc.  All rights reserved.
 */
#ifndef bitmunk_purchase_SellerPicker_H
#define bitmunk_purchase_SellerPicker_H

#include "bitmunk/common/TypeDefinitions.h"
#include "bitmunk/purchase/TypeDefinitions.h"

namespace bitmunk
{
namespace purchase
{

/**
 * The SellerPicker class provides shared utility algorithms for selecting
 * preferred sellers.
 * 
 * @author Dave Longley
 */
class SellerPicker
{
public:
   /**
    * Picks a preferred seller for a buyer that matches their preferences. If
    * a seller can be picked, then the passed SellerData reference will be set
    * to the picked SellerData from the given SellerDataList.
    * 
    * @param prefs the buyer's preferences.
    * @param list the list of SellerData to pick from.
    * @param pick the empty pick to be populated.
    * 
    * @return true if a seller could be picked, false if not.
    */
   static bool pickSeller(
      PurchasePreferences& prefs,
      bitmunk::common::SellerDataList& list,
      bitmunk::common::SellerData& pick);
};

} // end namespace purchase
} // end namespace bitmunk
#endif

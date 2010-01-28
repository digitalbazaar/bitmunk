/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/purchase/SellerPicker.h"

#include "monarch/crypto/BigDecimal.h"
#include "monarch/util/Random.h"
#include "bitmunk/purchase/PurchaseModule.h"

using namespace std;
using namespace monarch::crypto;
using namespace monarch::rt;
using namespace monarch::util;
using namespace bitmunk::common;
using namespace bitmunk::purchase;

bool SellerPicker::pickSeller(
   PurchasePreferences& prefs, SellerDataList& list, SellerData& pick)
{
   bool rval = false;

   // Note: This code assumes that every seller in the passed list of
   // seller data would come in at or under budget for their associated
   // files if chosen.

   // get the user's purchase preference (fast/cheap)
   bool fast = prefs["fast"]->getBoolean();

   MO_CAT_DEBUG(BM_PURCHASE_CAT,
      "SellerPicker attempting to pick the %s seller",
      fast ? "fastest" : "cheapest");

   // create a list of the possible sellers
   SellerDataList possibleSellers;
   possibleSellers->setType(Array);

   // keep a list of sellers with a rate of 0.0 (noob sellers)
   // these sellers will be included in the list of possible sellers if
   // the "fast" preference has been specified ... because we don't know
   // if they are fast or not and won't ever know unless they get some action
   SellerDataList noobSellers;
   noobSellers->setType(Array);

   // go through the source sellers and find the fastest/cheapest sellers
   bool first = true;
   BigDecimal price;
   price.setPrecision(7, Down);
   double rate;
   BigDecimal cheapest(0);
   cheapest.setPrecision(7, Down);
   double fastest = 0.0;
   SellerDataIterator i = list.getIterator();
   while(i->hasNext())
   {
      SellerData& sd = i->next();

      // get seller's price, ensure it is under allowable price
      price = sd["price"]->getString();
      rate = sd->hasMember("downloadRate") ?
         sd["downloadRate"]->getDouble() : 0.0;

      if(first)
      {
         // initialize cheapest price and fastest rate
         cheapest = price;
         fastest = rate;
         first = false;
      }

      // pick fastest seller (rate of 0.0 means not yet determined)
      if(fast && (rate == 0.0 || rate >= fastest))
      {
         if(rate == 0.0)
         {
            // save noob seller to include in fastest list at the end
            noobSellers->append(sd);
         }
         else
         {
            // if a new fastest rate has been found, clear the list
            if(rate > fastest)
            {
               possibleSellers->clear();
               fastest = rate;
            }

            // append the seller
            possibleSellers->append(sd);
         }
      }
      // pick cheapest seller
      else if(price <= cheapest)
      {
         // if a new cheapest price has been found, clear the list
         if(price < cheapest)
         {
            possibleSellers->clear();
            cheapest = price;
         }

         // append the seller
         possibleSellers->append(sd);
      }
   }

   // include noob sellers (that haven't had their rate calculated yet)
   // in possible sellers
   if(fast)
   {
      // true = append onto the end of the possible sellers list
      possibleSellers.merge(noobSellers, true);
   }

   // randomly select a seller from the possible sellers
   int count = possibleSellers->length();
   if(count > 0)
   {
      int index = (count == 1 ? 0 : Random::next(0, count - 1));
      pick = possibleSellers[index];
      rval = true;
   }

   return rval;
}

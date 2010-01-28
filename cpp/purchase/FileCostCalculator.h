/*
 * Copyright (c) 2009 Digital Bazaar, Inc.  All rights reserved.
 */
#ifndef bitmunk_purchase_FileCostCalculator_H
#define bitmunk_purchase_FileCostCalculator_H

#include "bitmunk/purchase/TypeDefinitions.h"
#include "monarch/crypto/BigDecimal.h"
#include "monarch/fiber/Fiber.h"

namespace bitmunk
{
namespace purchase
{

class DownloadManager;

/**
 * The FileCostCalculator will calculate the median cost for each file in
 * a DownloadState and the amount of money available to purchase each file.
 *
 * This class assumes that it is run inside of a Fiber and will yield
 * it as is necessary.
 *
 * @author Dave Longley
 */
class FileCostCalculator
{
protected:
   /**
    * The Fiber to operate on.
    */
   monarch::fiber::Fiber* mFiber;

   /**
    * The DownloadState to operate on.
    */
   DownloadState mDownloadState;

   /**
    * The total cost for all micropayment charges. Each time a file piece
    * is paid for, there may be a charge. This variable holds the total
    * cost for all of the file piece charges.
    */
   monarch::crypto::BigDecimal mMicroPaymentCost;

   /**
    * The total amount of money already spent on assigned/downloaded pieces.
    */
   monarch::crypto::BigDecimal mTotalSpent;

   /**
    * The total minimum price for all pieces for all files.
    */
   monarch::crypto::BigDecimal mTotalMinPrice;

   /**
    * The total median price for all pieces for all files.
    */
   monarch::crypto::BigDecimal mTotalMedPrice;

   /**
    * The total median price for all unassigned pieces for all files.
    */
   monarch::crypto::BigDecimal mTotalRemainingMedPrice;

   /**
    * The total maximum price for all pieces for all files.
    */
   monarch::crypto::BigDecimal mTotalMaxPrice;

   /**
    * Stores the amount of money left to spend on the purchase.
    */
   monarch::crypto::BigDecimal mBank;

   /**
    * A number for doing temporary calculations.
    */
   monarch::crypto::BigDecimal mTempMath;

public:
   /**
    * Creates a new FileCostCalculator.
    */
   FileCostCalculator();

   /**
    * Destructs this FileCostCalculator.
    */
   virtual ~FileCostCalculator();

   /**
    * Runs all file cost calculations for the given Fiber.
    *
    * @param fiber the fiber to run the calculations for.
    * @param ds the download state to run the calculations on.
    */
   virtual void calculate(monarch::fiber::Fiber* fiber, DownloadState& ds);

protected:
   /**
    * Calculates the total cost for using the micropayment service.
    */
   virtual void calculateMicroPaymentCost();

   /**
    * Calculates the total remaining median price and total
    * amount already spent.
    */
   virtual void calculateTotalRemainingMedianPrice();

   /**
    * Calculates the amount of funds left in the "bank" to spend on
    * unassigned file pieces.
    */
   virtual void calculateBank();

   /**
    * Calculates all new file budgets. A file budget is the amount of money
    * allocated to each file for obtaining their remaining file pieces.
    */
   virtual void calculateFileBudgets();
};
} // end namespace purchase
} // end namespace bitmunk
#endif

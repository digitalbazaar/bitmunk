/*
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk/purchase/FileCostCalculator.h"

#include "bitmunk/common/TypeDefinitions.h"
#include "bitmunk/purchase/DownloadManager.h"
#include "bitmunk/purchase/PurchaseModule.h"

using namespace bitmunk::common;
using namespace bitmunk::purchase;
using namespace monarch::crypto;
using namespace monarch::fiber;
using namespace monarch::rt;

#define MONEY_ACCURACY      7

FileCostCalculator::FileCostCalculator() :
   mFiber(NULL),
   mDownloadState(NULL)
{
   RoundingMode rm = Down;
   mMicroPaymentCost.setPrecision(MONEY_ACCURACY, rm);
   mTotalSpent.setPrecision(MONEY_ACCURACY, rm);
   mTotalMinPrice.setPrecision(MONEY_ACCURACY, rm);
   mTotalMedPrice.setPrecision(MONEY_ACCURACY, rm);
   mTotalRemainingMedPrice.setPrecision(MONEY_ACCURACY, rm);
   mTotalMaxPrice.setPrecision(MONEY_ACCURACY, rm);
   mBank.setPrecision(MONEY_ACCURACY, rm);
   mTempMath.setPrecision(MONEY_ACCURACY, rm);
}

FileCostCalculator::~FileCostCalculator()
{
}

void FileCostCalculator::calculate(Fiber* fiber, DownloadState& ds)
{
   mFiber = fiber;
   mDownloadState = ds;

   // re-zero appropriate numbers
   mMicroPaymentCost = 0;
   mTotalSpent = 0;
   mTotalMinPrice = 0;
   mTotalMedPrice = 0;
   mTotalRemainingMedPrice = 0;
   mTotalMaxPrice = 0;

   // do all calculations
   calculateMicroPaymentCost();
   calculateTotalRemainingMedianPrice();
   calculateBank();
   calculateFileBudgets();
}

void FileCostCalculator::calculateMicroPaymentCost()
{
   // there may be a charge to process each file piece payment, so
   // determine that cost, note that this will *always* be a flatfee, so
   // no payee resolution is necessary
   Media& media = mDownloadState["contract"]["media"];
   if(media->hasMember("piecePayees"))
   {
      // get the total piece payee amount (all piece payees are flat-fees)
      PayeeIterator pi = media["piecePayees"].getIterator();
      while(pi->hasNext())
      {
         Payee& p = pi->next();
         mMicroPaymentCost += p["amount"]->getString();
      }

      // get the total piece count
      uint64_t pieceCount = 0;
      FileProgressIterator fpi = mDownloadState["progress"].getIterator();
      while(fpi->hasNext())
      {
         FileProgress& progress = fpi->next();
         uint32_t pc = progress["sellerPool"]["pieceCount"]->getUInt32();
         pieceCount += pc;
         mTempMath = mMicroPaymentCost;
         mTempMath *= pc;
         progress["microPaymentCost"] = mTempMath.toString(true).c_str();
      }

      // multiply the total payee amount by the total number of pieces
      mMicroPaymentCost *= pieceCount;
      mDownloadState["totalPieceCount"] = pieceCount;
      mDownloadState["totalMicroPaymentCost"] =
         mMicroPaymentCost.toString(true).c_str();

      MO_CAT_DEBUG(BM_PURCHASE_CAT,
         "total piece count: %" PRIu64 ", "
         "uid: %" PRIu64 ", dsid: %" PRIu64,
         pieceCount,
         BM_USER_ID(mDownloadState["userId"]),
         mDownloadState["id"]->getUInt64());
   }

   MO_CAT_DEBUG(BM_PURCHASE_CAT,
      "total micropayment cost: %s, "
      "uid: %" PRIu64 ", dsid: %" PRIu64,
      mMicroPaymentCost.toString(true).c_str(),
      BM_USER_ID(mDownloadState["userId"]),
      mDownloadState["id"]->getUInt64());
}

void FileCostCalculator::calculateTotalRemainingMedianPrice()
{
   // iterate over all file progresses
   FileProgressIterator fpi = mDownloadState["progress"].getIterator();
   while(fpi->hasNext())
   {
      FileProgress& progress = fpi->next();

      // build funded pieces map
      DynamicObject fundedPieces;
      fundedPieces->setType(Map);
      fundedPieces.merge(progress["downloaded"], true);
      fundedPieces.merge(progress["assigned"], true);

      // get the full content size of the current file
      uint64_t fileSize = progress["fileInfo"]["contentSize"]->getUInt64();
      uint64_t bytesRemaining = fileSize;

      // update total prices
      SellerPoolStats& sps = progress["sellerPool"]["stats"];
      mTotalMinPrice += sps["minPrice"]->getString();
      mTotalMedPrice += sps["medPrice"]->getString();
      mTotalMaxPrice += sps["maxPrice"]->getString();

      // iterate over lists of funded pieces
      DynamicObjectIterator fpli = fundedPieces.getIterator();
      while(fpli->hasNext())
      {
         // get next list of funded pieces
         FilePieceList& pieces = fpli->next();

         // total all of the piece sizes
         uint64_t totalSize = 0;
         FilePieceIterator i = pieces.getIterator();
         while(i->hasNext())
         {
            FilePiece& piece = i->next();
            uint64_t size = piece["size"]->getUInt32();

            MO_CAT_DEBUG(BM_PURCHASE_CAT,
               "piece %i is size %" PRIu64 ", "
               "uid: %" PRIu64 ", dsid: %" PRIu64,
               piece["index"]->getInt32(), size,
               BM_USER_ID(mDownloadState["userId"]),
               mDownloadState["id"]->getUInt64());

            // a piece size of 0 means use the full file size
            if(size == 0)
            {
               size = fileSize;
            }
            totalSize += size;
         }

         MO_CAT_DEBUG(BM_PURCHASE_CAT,
            "total bytes in already funded pieces: %" PRIu64 ", "
            "uid: %" PRIu64 ", dsid: %" PRIu64,
            totalSize,
            BM_USER_ID(mDownloadState["userId"]),
            mDownloadState["id"]->getUInt64());

         // decrement total bytes remaining
         bytesRemaining -= totalSize;

         // Note: The contract section price is the seller's price as if
         // the buyer purchased the entire file using that particular
         // contract section's terms. The actual price the buyer will pay
         // will be based on the percentage of the file actually purchased
         // under that contract section's terms, as the buyer may only
         // purchase a portion of the file's pieces using said terms. The
         // buyer can purchase the remaining portion using other contract
         // sections.

         // determine the ratio of the pieces purchased to the total file size
         mTempMath = totalSize;
         if(fileSize > 0)
         {
            mTempMath /= fileSize;
         }

         // get the hash identifying the contract section for the pieces
         const char* csHash = fpli->getName();

         // multiply the portion of the file purchased under the current
         // contract section by the price of the contract section
         const char* price =
            progress["sellerData"][csHash]["price"]->getString();
         mTempMath *= price;

         // truncate cost beyond the money precision
         mTempMath.round();

         // increase the total amount considered spent
         mTotalSpent += mTempMath;

         // yield to allow other fibers to run
         mFiber->yield();
      }

      // no more funded pieces:

      // if there are pieces that haven't yet been assigned and the file
      // is not zero-byted, then we must add to the total median price
      if(progress["unassigned"]->length() > 0 && fileSize > 0)
      {
         // get the median price for the file
         const char* price = sps["medPrice"]->getString();

         // now that the total number of bytes remaining in the file has
         // been calculated, use the ratio of that amount to the total
         // file size to determine the remaining cost of the file
         mTempMath = bytesRemaining;
         mTempMath /= fileSize;
         mTempMath *= price;

         // truncate cost beyond the money precision
         mTempMath.round();

         // increase the total median price for unassigned pieces
         mTotalRemainingMedPrice += mTempMath;
      }

      // yield to allow other fibers to run
      mFiber->yield();
   }
}

void FileCostCalculator::calculateBank()
{
   // Note: Micropayment amount, total amount spent, and total median
   // price for remaining data are now set. Subtract those amounts and
   // the amount for the license from the buyer's maximum price allowance
   // to determine how much is available in the "bank" to purchase the
   // remaining file pieces.

   MO_CAT_DEBUG(BM_PURCHASE_CAT,
      "total amount already spent on data: %s, "
      "uid: %" PRIu64 ", dsid: %" PRIu64,
      mTotalSpent.toString(true).c_str(),
      BM_USER_ID(mDownloadState["userId"]),
      mDownloadState["id"]->getUInt64());

   MO_CAT_DEBUG(BM_PURCHASE_CAT,
      "total median price for unassigned pieces: %s, "
      "uid: %" PRIu64 ", dsid: %" PRIu64,
      mTotalRemainingMedPrice.toString(true).c_str(),
      BM_USER_ID(mDownloadState["userId"]),
      mDownloadState["id"]->getUInt64());

   // initialize amount of money left to spend (bank) based on the user's
   // maximum price preference
   mBank = mDownloadState["preferences"]["price"]["max"]->getString();

   // decrease the amount of money in the bank by the cost of the license
   Media& media = mDownloadState["contract"]["media"];
   mTempMath = media["licenseAmount"]->getString();
   mBank -= mTempMath;

   // decrease the amount of money in the bank by the micropayment cost
   mBank -= mMicroPaymentCost;

   // decrease the amount of money in the bank by the amount already spent
   mBank -= mTotalSpent;

   // add license amount and micro payment amounts to total prices
   mTotalMinPrice += mMicroPaymentCost;
   mTotalMedPrice += mMicroPaymentCost;
   mTotalMaxPrice += mMicroPaymentCost;
   mTotalMinPrice += mTempMath;
   mTotalMedPrice += mTempMath;
   mTotalMaxPrice += mTempMath;
   mDownloadState["totalMinPrice"] = mTotalMinPrice.toString(true).c_str();
   mDownloadState["totalMedPrice"] = mTotalMedPrice.toString(true).c_str();
   mDownloadState["totalMaxPrice"] = mTotalMaxPrice.toString(true).c_str();

   MO_CAT_DEBUG(BM_PURCHASE_CAT,
      "license amount: %s, "
      "uid: %" PRIu64 ", dsid: %" PRIu64,
      media["licenseAmount"]->getString(),
      BM_USER_ID(mDownloadState["userId"]),
      mDownloadState["id"]->getUInt64());

   MO_CAT_DEBUG(BM_PURCHASE_CAT,
      "buyer's maximum allowed price: %s, "
      "uid: %" PRIu64 ", dsid: %" PRIu64,
      mDownloadState["preferences"]["price"]["max"]->getString(),
      BM_USER_ID(mDownloadState["userId"]),
      mDownloadState["id"]->getUInt64());

   MO_CAT_DEBUG(BM_PURCHASE_CAT,
      "amount left to spend on all remaining files: %s, "
      "uid: %" PRIu64 ", dsid: %" PRIu64,
      mBank.toString(true).c_str(),
      BM_USER_ID(mDownloadState["userId"]),
      mDownloadState["id"]->getUInt64());
}

void FileCostCalculator::calculateFileBudgets()
{
   // iterate over all file progresses
   FileProgressIterator fpi = mDownloadState["progress"].getIterator();
   while(fpi->hasNext())
   {
      // calculate budget for file based on its seller pool's median price
      FileProgress& progress = fpi->next();

      // since the current file may only be 1 of several files involved
      // in the purchase, we must calculate how much of the total median
      // price each file will cost and multiply that ratio by the amount
      // in the bank to give it a proportional budget
      mTempMath = progress["sellerPool"]["stats"]["medPrice"]->getString();
      if(!mTotalRemainingMedPrice.isZero() &&
         !mTotalRemainingMedPrice.isNegative())
      {
         mTempMath /= mTotalRemainingMedPrice;
         mTempMath *= mBank;
      }
      else
      {
         // all remaining pieces are free, so total bank is available for
         // every file
         mTempMath = mBank;
      }

      // truncate cost beyond the money precision
      mTempMath.round();
      progress["budget"] = mTempMath.toString(true).c_str();

      // yield to allow other fibers to run
      mFiber->yield();
   }
}

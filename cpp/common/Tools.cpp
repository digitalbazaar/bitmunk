/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk/common/Tools.h"

#include "monarch/crypto/MessageDigest.h"
#include "monarch/config/ConfigManager.h"
#include "monarch/util/Convert.h"
#include "monarch/util/PathFormatter.h"
#include "bitmunk/common/Signer.h"

using namespace std;
using namespace monarch::config;
using namespace monarch::crypto;
using namespace monarch::event;
using namespace monarch::rt;
using namespace monarch::util;
using namespace bitmunk::common;

std::string Tools::getBitmunkUserCommonName(UserId userId)
{
   char tmp[30];
   snprintf(tmp, 30, "bitmunk-%" PRIu64, userId);
   return tmp;
}

string Tools::formatMoney(const char* amount, int round)
{
   // FIXME: clean up algorithm for future use with other currencies
   // i.e. a table for getting decimal places for each currency
   int decPlaces = 2;

   // round as appropriate
   BigDecimal bd = amount;
   RoundingMode rm = (round == 1) ? Up : (round == 0) ? HalfUp : Down;
   bd.setPrecision(decPlaces, rm);
   bd.round();

   // convert rounded amount to string
   return bd.toString(true);
}

void Tools::populateSymmetricKey(SymmetricKey* key, KeyInfo& ki)
{
   // set key algorithm
   key->setAlgorithm(ki["algorithm"]->getString());

   // decode data into bytes
   unsigned int length = ki["data"]->length();
   char data[length];
   Convert::hexToBytes(
      ki["data"]->getString(), length, data, length);

   // copy bytes into key data array
   unsigned int keyLength = ki["length"]->getUInt32();
   char* keyData = (char*)malloc(keyLength);
   memcpy(keyData, data, keyLength);

   // copy bytes into IV array, if applicable
   unsigned int ivLength = 0;
   char* iv = NULL;
   if(keyLength < length)
   {
      ivLength = length - keyLength;
      iv = (char*)malloc(ivLength);
      memcpy(iv, data + keyLength, ivLength);
   }

   // assign data to key
   key->assignData(keyData, keyLength, iv, ivLength, false);
}

void Tools::populateKeyInfo(KeyInfo& ki, SymmetricKey* key)
{
   // set key algorithm and length
   ki["algorithm"] = key->getAlgorithm();
   ki["length"] = key->length();

   // hex-encode data and IV
   char temp[key->length() + key->ivLength()];
   memcpy(temp, key->data(), key->length());
   if(key->ivLength() > 0)
   {
      memcpy(temp + key->length(), key->iv(), key->ivLength());
   }
   string encoded = Convert::bytesToHex(temp, key->length() + key->ivLength());
   ki["data"] = encoded.c_str();
}

string Tools::getContractSectionHash(ContractSection& cs)
{
   // Note: This hash must ONLY contain information that a seller
   // has access to. For instance, it CANNOT include the contract's
   // transaction ID or a buyer's account ID.
   //
   // The hash includes unique information identifying the particular terms
   // agreed to between a seller and a buyer.
   //
   // FIXME: we might want to add a "hash version" to the contract section
   // in the future, and if it is missing, we assume version 1

   // The signature on a ContractSection has the same requirements listed
   // in the above note, so use its content method.

   // use same algorithm as for signing contract section
   string str;
   Signer::appendContractSectionContent(cs, str);
   MessageDigest hash("SHA1");
   hash.update(str.c_str());
   return hash.getDigest();
}

string Tools::createSellerServerKey(UserId sellerId, ServerId serverId)
{
   string rval;

   if(BM_ID_VALID(sellerId) && BM_ID_VALID(serverId))
   {
      char tmp[45];
      snprintf(tmp, 45, "%" PRIu64 ":%u", sellerId, serverId);
      rval = tmp;
   }

   return rval;
}

string Tools::createSellerServerKey(Seller& seller)
{
   return createSellerServerKey(
      BM_USER_ID(seller["userId"]), BM_SERVER_ID(seller["serverId"]));
}

// a helper function for resolving a percentage payee of any type
static void resolvePercentagePayee(
   Payee& p,
   BigDecimal& min,
   BigDecimal& tmp,
   BigDecimal& multiplier,
   BigDecimal& total, BigDecimal* taxableTotal = NULL)
{
   // if payee is already resolved, simply use amount
   if(p["amountResolved"]->getBoolean())
   {
      // payee already resolved, get amount
      tmp = p["amount"]->getString();
   }
   else
   {
      // calculate amount and update total
      tmp = multiplier * p["percentage"]->getString();

      // ensure resolved amount meets minimum
      if(p->hasMember("min"))
      {
         min = p["min"]->getString();
         if(min > tmp)
         {
            // use minimum amount
            tmp = min;
         }
      }

      // tmp holds the resolved amount now, round down to precision
      tmp.round();
      p["amount"] = tmp.toString(true).c_str();

      // amount now resolved
      p["amountResolved"] = true;
   }

   // update total
   total += tmp;

   // if payee is taxable, update taxable total if available
   if(taxableTotal != NULL && !p["taxExempt"]->getBoolean())
   {
      *taxableTotal += tmp;
   }
}

void Tools::resolvePayeeAmounts(
   PayeeList& payees, BigDecimal* totalOut, BigDecimal* licenseIn)
{
   // Note: "taxExempt" was chosen over "taxable" so that payees will
   // default to taxExempt = false -- we prefer to accidentally tax
   // over not collecting tax as this is easier to correct, further more
   // the vast majority of payees are considered "taxable"

   // initialize variables
   BigDecimal total(0);
   BigDecimal taxableTotal(0);
   BigDecimal license = (licenseIn != NULL ? *licenseIn : 0);

   // initialize precision to 7 digits, rounding down
   total.setPrecision(7, Down);
   taxableTotal.setPrecision(7, Down);
   license.setPrecision(7, Down);

   // create a list for storing payees that use an amount type "ptotal", which
   // means "percent of total", so their amounts can be updated after the
   // first pass to determine the total without them.
   PayeeList potPayees;
   potPayees->setType(Array);

   // create a list for storing tax payees that will be updated in order after
   // the percent of total payees are calculated
   PayeeList taxPayees;
   taxPayees->setType(Array);

   // create numbers to store minimum amounts and temporary calculations
   BigDecimal min(0);
   BigDecimal tmp(0);
   min.setPrecision(7, Down);
   tmp.setPrecision(7, Down);

   // iterate over payees resolving flat fee amounts
   PayeeIterator i = payees.getIterator();
   while(i->hasNext())
   {
      Payee& p = i->next();
      p["amountResolved"]->setType(Boolean);
      p["taxExempt"]->setType(Boolean);

      // determine payee amount type
      PayeeAmountType amountType = p["amountType"]->getString();
      if(strcmp(amountType, PAYEE_AMOUNT_TYPE_FLATFEE) == 0)
      {
         // payee is already flat-fee, so simply add the amount
         total += p["amount"]->getString();

         // if payee is taxable, update taxable total
         if(!p["taxExempt"]->getBoolean())
         {
            taxableTotal += p["amount"]->getString();
         }

         // amount now resolved
         p["amountResolved"] = true;
      }
      else
      {
         // determine percentage base to resolve to flat fee
         if(strcmp(amountType, PAYEE_AMOUNT_TYPE_PLICENSE) == 0)
         {
            // use license as percentage base
            tmp = license;
         }
         else if(strcmp(amountType, PAYEE_AMOUNT_TYPE_PCUMULATIVE) == 0)
         {
            // use total as percentage base
            tmp = total;
         }
         else if(strcmp(amountType, PAYEE_AMOUNT_TYPE_PTOTAL) == 0)
         {
            // store payee to update later according to pre-total
            potPayees->append(p);
         }
         else if(strcmp(amountType, PAYEE_AMOUNT_TYPE_TAX) == 0)
         {
            // store payee to update later according to pre-taxable-total
            taxPayees->append(p);
         }

         // resolve payee amounts that don't depend on totals
         if((strcmp(amountType, PAYEE_AMOUNT_TYPE_PTOTAL) != 0) &&
            (strcmp(amountType, PAYEE_AMOUNT_TYPE_TAX) != 0))
         {
            resolvePercentagePayee(p, min, tmp, tmp, total, &taxableTotal);
         }
      }
   }

   // update all percentage payees that are based on total
   if(potPayees->length() > 0)
   {
      // use the current total as the total for all calculations (do not
      // use the real total which will accumulate more value as each payee
      // in potPayees is resolved)
      BigDecimal tmpTotal;
      tmpTotal.setPrecision(7, Down);
      tmpTotal = total;
      i = potPayees.getIterator();
      while(i->hasNext())
      {
         Payee& p = i->next();
         resolvePercentagePayee(p, min, tmp, tmpTotal, total, &taxableTotal);
      }
   }

   if(taxPayees->length() > 0)
   {
      // calculate taxes and add to total
      i = taxPayees.getIterator();
      while(i->hasNext())
      {
         Payee& p = i->next();

         // resolve percentage payee, but do not update taxable total --
         // as taxes should not be cumulative (if there is, for instance,
         // a state digital goods tax, whatever its value is should not
         // be included when calculating a following state sales tax)
         resolvePercentagePayee(p, min, tmp, taxableTotal, total, NULL);
      }
   }

   // ensure total truncates at 7-digits
   total.round();

   // set output
   if(totalOut != NULL)
   {
      totalOut->setPrecision(7, Down);
      *totalOut = total;
   }
}

DynamicObject Tools::getTotalPrice(DynamicObject options)
{
   DynamicObject rval(NULL);

   // get media license amount by calculation or passing
   BigDecimal licenseCost(0);
   if(options->hasMember("licenseAmount"))
   {
      // license amount passed in
      licenseCost = options["licenseAmount"]->getString();
   }
   else
   {
      // resolve license payees to get license cost
      options["licensePayees"]->setType(Array);
      PayeeList& payees = options["licensePayees"];
      Tools::resolvePayeeAmounts(payees, &licenseCost);
   }

   // get data amount by calculation or it passing
   BigDecimal dataCost(0);
   if(options->hasMember("dataAmount"))
   {
      // data amount passed in
      dataCost = options["dataAmount"]->getString();
   }
   else if(options->hasMember("dataPayees"))
   {
      // resolve data payees to get data cost
      options["dataPayees"]->setType(Array);
      PayeeList& payees = options["dataPayees"];
      Tools::resolvePayeeAmounts(payees, &dataCost, &licenseCost);
   }

   // calculate cost for pieces
   BigDecimal piecesCost;
   if(options->hasMember("filePieceCount") &&
      options->hasMember("piecePayees"))
   {
      // FIXME: assumes piece payees are flat fees (which is ok until
      // we determine that they can be other than flat fees)
      uint64_t piecesCount = options["filePieceCount"]->getUInt64();
      if(piecesCount > 0)
      {
         BigDecimal tmp;
         tmp.setPrecision(7, Down);
         options["piecePayees"]->setType(Array);
         PayeeList& payees = options["piecePayees"];
         PayeeIterator i = payees.getIterator();
         while(i->hasNext())
         {
            Payee& p = i->next();
            tmp = p["amount"]->getString();
            tmp *= piecesCount;
            tmp.round();
            p["amount"] = tmp.toString(true).c_str();
            p["amountResolved"] = true;
            piecesCost += tmp;
         }
      }
   }

   // calculate total (license + data + pieces)
   BigDecimal total = licenseCost + dataCost + piecesCost;
   total.setPrecision(7, Down);
   total.round();

   // calculate data total (data + pieces)
   BigDecimal dataTotal = dataCost + piecesCost;
   dataTotal.setPrecision(7, Down);
   dataTotal.round();

   // set rval to clone of options, add totals
   rval = options.clone();
   rval["licenseCost"] = licenseCost.toString(true).c_str();
   rval["dataCost"] = dataCost.toString(true).c_str();
   rval["piecesCost"] = piecesCost.toString(true).c_str();
   rval["dataTotal"] = dataTotal.toString(true).c_str();
   rval["total"] = total.toString(true).c_str();

   return rval;
}

string Tools::createMediaFilename(
   Media& media, MediaId mediaId, const char* extension)
{
   // The full name with directories and filename
   // Note: make sure not to use PathFormatter::formatFilename() on this so as
   // to not escape path separators.
   string fullname;

   unsigned int position = 0;
   unsigned int maxPosition = 0;
   Media fileMedia(NULL);

   // if the collectionId and the mediaId are not identical, that means
   // we're dealing with a collection and should generate positional
   // information in the file name and an additional directory
   if(!BM_MEDIA_ID_EQUALS(BM_MEDIA_ID(media["id"]), mediaId))
   {
      // add a directory named after the collection title if it exists
      if(media->hasMember("title"))
      {
         // format the pathname so that it can be written on Windows, Mac OS X
         // and Linux
         string title = media["title"]->getString();
         fullname.append(PathFormatter::formatFilename(title));
         fullname.push_back('/');
      }

      DynamicObjectIterator gi = media["contents"].getIterator();

      // search all of the groups, of which there should be only one
      while(gi->hasNext())
      {
         DynamicObject& group = gi->next();
         MediaIterator mi = group.getIterator();

         // iterate over every item in the group
         while(mi->hasNext())
         {
            maxPosition++;
            Media& m = mi->next();

            // Check to see if the media IDs match
            if(BM_MEDIA_ID_EQUALS(BM_MEDIA_ID(m["id"]), mediaId))
            {
               fileMedia = m;
               position = maxPosition;
            }
         }
      }
   }
   else
   {
      fileMedia = media;
   }

   // The filename past any directories
   string filename;

   // calculate the proper format for the position
   if(position > 0)
   {
      char positionString[22];
      char maxPositionText[22];
      char maxPositionFormat[26];
      unsigned int maxPositionLength;

      snprintf(maxPositionText, 22, "%u", maxPosition);
      maxPositionLength = strlen(maxPositionText);
      snprintf(maxPositionFormat, 26, "%%0%uu", maxPositionLength);
      snprintf(positionString, 22, maxPositionFormat, position);
      filename.append(positionString);
      filename.append("-");
   }

   // add either the performer, copyright owner, or publisher, in that order
   // as the "artist" part of the filename.
   if(fileMedia["contributors"]->hasMember("Performer"))
   {
      filename.append(
         fileMedia["contributors"]["Performer"][0]["name"]->getString());
      filename.push_back('-');
   }
   else if(fileMedia["contributors"]->hasMember("Copyright Owner"))
   {
      filename.append(
         fileMedia["contributors"]["Copyright Owner"][0]["name"]->getString());
      filename.push_back('-');
   }
   else if(fileMedia["contributors"]->hasMember("Publisher"))
   {
      filename.append(
         fileMedia["contributors"]["Publisher"][0]["name"]->getString());
      filename.push_back('-');
   }

   // if the title is a valid string use it, otherwise use the media ID
   if(strlen(fileMedia["title"]->getString()) > 0)
   {
      filename.append(fileMedia["title"]->getString());
   }
   else
   {
      filename.append(fileMedia["id"]->getString());
   }

   filename.push_back('.');
   filename.append(extension);

   // escape any invalid filename characters
   fullname.append(PathFormatter::formatFilename(filename));

   return fullname;
}

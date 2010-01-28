/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_common_Tools_H
#define bitmunk_common_Tools_H

#include "bitmunk/common/TypeDefinitions.h"
#include "monarch/config/ConfigManager.h"
#include "monarch/crypto/BigDecimal.h"
#include "monarch/crypto/SymmetricKey.h"
#include "monarch/event/Event.h"

namespace bitmunk
{
namespace common
{

/**
 * The Tools class provides methods for manipulating Bitmunk common objects.
 *
 * @author Dave Longley
 * @author Manu Sporny
 */
class Tools
{
public:
   /**
    * Gets a bitmunk user common name.
    *
    * @param userId the user's ID.
    *
    * @return a bitmunk user common name.
    */
   static std::string getBitmunkUserCommonName(UserId userId);

   /**
    * Formats the passed monetary amount according to a Money's currency
    * and writes it to the passed string. If any rounding is necessary,
    * then the passed rounding mode will be used.
    *
    * @param amount the monetary amount to format.
    * @param round 1 to round up, 0 to round half up, -1 to round down.
    *
    * @return the formatted string.
    */
   static std::string formatMoney(const char* amount, int round);

   /**
    * Populates a SymmetricKey object from the passed KeyInfo.
    *
    * @param key the SymmetricKey to populate.
    * @param ki the KeyInfo to populate it from.
    */
   static void populateSymmetricKey(
      monarch::crypto::SymmetricKey* key, KeyInfo& ki);

   /**
    * Populates a KeyInfo from a SymmetricKey object.
    *
    * @param ki the KeyInfo to populate.
    * @param key the SymmetricKey to populate it from.
    */
   static void populateKeyInfo(
      KeyInfo& ki, monarch::crypto::SymmetricKey* key);

   /**
    * Creates a hash that uniquely identifies the terms of the passed
    * ContractSection.
    *
    * @param cs the ContractSection to get the hash for.
    *
    * @return the generated hash.
    */
   static std::string getContractSectionHash(ContractSection& cs);

   /**
    * Creates a string key that identifies a specific seller's specific server.
    * This method will concatenate a seller's user ID and a server ID (with a
    * colon as a separator).
    *
    * @param sellerId the user ID of the seller.
    * @param serverId the seller's specific server ID.
    *
    * @return the key or blank if the seller IDs are invalid.
    */
   static std::string createSellerServerKey(UserId sellerId, ServerId serverId);

   /**
    * Creates a string key that identifies a specific seller's specific server.
    * This method will concatenate a seller's user ID and a server ID (with a
    * colon as a separator).
    *
    * @param seller the Seller to create the key for.
    *
    * @return the key or blank if the seller IDs are invalid.
    */
   static std::string createSellerServerKey(Seller& seller);

   /**
    * Resolves amounts to flat-fees for payees in the passed PayeeList
    * that are not already flat-fees (they are percentages). All amounts will
    * use 7-digit-precision and be rounded Down.
    *
    * @param payees the list of payees with amounts to resolve.
    * @param totalOut if non-NULL, will be set to the sum of all of the resolved
    *                 payee amounts, not including any license amount passed.
    * @param licenseIn the pre-calculated license amount to apply to data payees
    *                  that are based off of a percentage of license, NULL for 0
    *                  or for calculating license payees.
    */
   static void resolvePayeeAmounts(
      PayeeList& payees,
      monarch::crypto::BigDecimal* totalOut = NULL,
      monarch::crypto::BigDecimal* licenseIn = NULL);

   /**
    * Gets a total price, given a license amount (or license payees), a
    * data amount (or data payees), and an optional piece count and
    * piece payees.
    *
    * @param options:
    *           licenseAmount the total amount of the license.
    *           licensePayees the license payees (alternative to licenseAmount).
    *           dataAmount the total amount of the data.
    *           dataPayees the data payees (alternative to dataPayees).
    *           filePieceCount the number of file pieces.
    *           piecePayees the flat-fee piece payees to apply for each piece.
    *
    * @return a map with everything from options in addition to:
    *         licenseCost: the total cost of the license.
    *         dataCost: the total cost of the data.
    *         piecesCost: the total cost of the pieces.
    *         dataTotal: the total cost of the data and the pieces.
    *         total: the total cost of the license, data, and pieces.
    */
   static monarch::rt::DynamicObject getTotalPrice(
      monarch::rt::DynamicObject options);

   /**
    * Formats a filename based on a media.
    *
    * @param media if the filename is for a media that is part of a collection,
    *              this parameter must be the collection, if it is a single,
    *              this parameter must be the single media.
    * @param mediaId the ID of the specific media the filename is for.
    * @param extension the extension for the file.
    *
    * @return the media-formatted filename.
    */
   static std::string createMediaFilename(
      Media& media, MediaId mediaId, const char* extension);
};

} // end namespace common
} // end namespace bitmunk
#endif

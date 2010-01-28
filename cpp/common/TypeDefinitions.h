/*
 * Copyright (c) 2007-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_common_TypeDefinitions_H
#define bitmunk_common_TypeDefinitions_H

#include "monarch/rt/DynamicObject.h"
#include "monarch/rt/DynamicObjectIterator.h"

namespace bitmunk
{
namespace common
{

// FIXME: change all "url" to "iri"
//        IRIs are Internationalized Resource Identifiers, of which URLs are a
//        subset. -- manu

// ID type definitions
typedef uint64_t AccountId;
typedef uint32_t BfpId;
typedef uint64_t ContributorId;
typedef const char* FileId;
typedef uint64_t MediaId;
typedef uint32_t PayeeSchemeId;
typedef uint32_t PermissionGroupId;
typedef uint32_t ProfileId;
typedef uint32_t RoleId;
typedef uint32_t ServerId;
typedef const char* ServerToken;
typedef uint32_t TicketId;
typedef uint64_t TransactionId;
typedef uint64_t UserId;
typedef const char* WareId;

/**
 * An Address contains a street, locality (city), region (state), and postal
 * code (zip code).
 *
 * Names like locality, region, and postal code were chosen to better support
 * internationalization. Every data member is stored as a string for the
 * same reason.
 *
 * An Address may be optionally signed, indicating that an authority has
 * indicated that the given address is a valid real-world address. The
 * signature includes all of the data in the address, but lowercases the
 * data before processing it.
 *
 * Address
 * {
 *    "street" : string,
 *    "locality" : string,
 *    "region" : string,
 *    "postalCode" : string,
 *    "countryCode" : string,
 *    "signature" : string,
 *    "signer" : Map
 *    {
 *       "userId" : UserId (uint64),
 *       "profileId" : ProfileId (uint32)
 *    }
 * }
 *
 * @member street a street, i.e. 1700 Kraft Drive, Ste. 2408.
 * @member locality a locality or city, i.e. Blacksburg.
 * @member region region or state, i.e. VA.
 * @member postalCode a postal code or zip code, i.e. 24060.
 * @member countryCode a 2-digit country code, i.e. US
 * @member signature the signer's hex-encoded signature.
 * @member signer.userId the signer's user ID.
 * @member signer.profileId the signer's profile ID.
 */
typedef monarch::rt::DynamicObject Address;
typedef monarch::rt::DynamicObjectIterator AddressIterator;

/**
 * An Account contains information about a Bitmunk user's particular
 * financial account.
 *
 * Account
 * {
 *    "id" : AccountId (uint64),
 *    "ownerId" : UserId (uint64),
 *    "name" : string,
 *    "balance" : string
 * }
 *
 * @member id a unique AccountId for the account.
 * @member ownerId the UserId of the Bitmunk user that owns the account.
 * @member name the name of the account.
 * @member balance the amount of funds in the account (always in USD).
 */
typedef monarch::rt::DynamicObject Account;
typedef monarch::rt::DynamicObjectIterator AccountIterator;

/**
 * An Identity contains information about a particular Bitmunk entity. This
 * information includes first and last names, a physical address, and other
 * useful Bitmunk data.
 *
 * This type is likely quite similar to a "VCard".
 *
 * Identity
 * {
 *    "userId" : UserId,
 *    "firstName" : string,
 *    "lastName" : string,
 *    "address" : Address
 * }
 *
 * @member userId the ID of the user.
 * @member firstName the first name of the entity.
 * @member lastName the last name of the entity.
 * @member address the Address of the entity.
 */
typedef monarch::rt::DynamicObject Identity;
typedef monarch::rt::DynamicObjectIterator IdentityIterator;

/**
 * A Media is used to provide information about a particular piece of
 * real-world media.
 *
 * Media
 * {
 *    "id" : MediaId (uint64),
 *    "type" : MediaType,
 *    "ownerId" : UserId (uint64),
 *    "title" : string,
 *    "releaseDate" : string (YYYY-MM-DD),
 *    "publicDomain" : boolean,
 *    "ccLicenses" : string,
 *    "distribution" : string,
 *
 *    "contributors" : Map
 *    {
 *       "<contributor-type>" : [] of Contributors
 *    },
 *
 *    "payees" : [] (Payees),
 *    "payeeRules" : [] (PayeeRules),
 *    "licenseAmount" : string,
 *    "piecePayees" : [] (Payees),
 *
 *    "buyerId" : UserId (uint64),
 *    "expiration" : string (YYYY-MM-DD),
 *    "signature" : string,
 *    "signer" : Map
 *    {
 *       "userId" : UserId (uint64),
 *       "profileId" : ProfileId (uint32)
 *    }
 * }
 *
 * @member id a unique ID for the media.
 * @member type a media type.
 * @member ownerId the id of the user who owns (manages) the media.
 * @member title the title of the media.
 * @member releaseDate the date the media was released.
 * @member publicDomain a flag to indicate that the media is public domain.
 * @member ccLicenses a comma separated list of assigned CC licenses.
 * @member distribution a comma separated list of distribution types.
 *
 * @member contributors a map of contributor type to array of contributors
 *                      of that type.
 *
 * @member payees the license payees for the media.
 *                Payees must be ordered for signatures.
 * @member payeeRules the list of PayeeRules for the media.
 * @member licenseAmount the total of the resolved payee amounts for the
 *                       license.
 * @member piecePayees the payees for file piece payment processing.
 *                     Payees must be ordered for signatures.
 *                     **Note: THESE MUST BE FLAT FEE PAYEES.
 *
 * @member buyerId the user the media is signed for.
 * @member expiration the timestamp for when the media expires for
 *                    the user.
 * @member signature the signer's hex-encoded signature.
 * @member signer.userId the signer's user ID.
 * @member signer.profileId the signer's profile ID.
 */
typedef monarch::rt::DynamicObject Media;
typedef monarch::rt::DynamicObjectIterator MediaIterator;

typedef const char* MediaType;
#define MEDIA_TYPE_AUDIO      "audio"
#define MEDIA_TYPE_BOOK       "book"
#define MEDIA_TYPE_COLLECTION "collection"
#define MEDIA_TYPE_DATA       "data"
#define MEDIA_TYPE_IMAGE      "image"
#define MEDIA_TYPE_VIDEO      "video"

/**
 * A MediaPreference object contains information about preferences when
 * retrieving details about a particular media.
 *
 * MediaPreference
 * {
 *    "mediaIds" : [] of MediaIds (uint64),
 *    "preferences" : [] of
 *    {
 *       "contentType" : string,
 *       "minBitrate" : uint32
 *    }
 * }
 *
 * @member mediaIds an array of mediaIds that are associated with the
 *                  preferences.
 * @member preferences an array of preferences containing:
 *    @member contentType the preferred contentType such as "audio/mpeg" or
 *                        "audio/ogg" in the case of audio. "video/avi" for
 *                        DiVX or "application/x-pdf" for PDF.
 *    @member minBitrate the minimum desired bitrate.
 */
typedef monarch::rt::DynamicObject MediaPreference;
typedef monarch::rt::DynamicObject MediaPreferenceList;
typedef monarch::rt::DynamicObjectIterator MediaPreferenceIterator;

/**
 * A FileInfo object contains information about a particular file. See the
 * Ware description for information about file info IDs.
 *
 * FileInfo
 * {
 *    "id" : string,
 *    "path" : string,
 *    "contentType" : string,
 *    "contentSize" : uint64,
 *    "extension" : string,
 *    "size" : uint64,
 *    "mediaId" : MediaId (uint64),
 *    "pieces" : [] (FilePiece),
 *    "bfpId" : uint32
 *    "formatDetails" : Map,
 * }
 *
 * @member id a unique ID for the file the FileInfo represents (see the
 *            Ware description for information about file IDs).
 * @member path the file system path where the file resides.
 * @member contentType the fully qualified Internet Media Type such as
 *                     "audio/mpeg", "video/x-msvideo", "text/plain",
 *                     "image/jpeg".
 * @member extension the extension of the file (i.e. "mp3", "txt").
 * @member contentSize the size of the relevant content in the file (this is
 *                     the content that is transmitted during a sale as opposed
 *                     to that which is stripped or rewritten, for instance,
 *                     audio/mpeg data is included in the content size, an ID3
 *                     tag is not).
 * @member size the size of the file in bytes.
 * @member mediaId the ID of the media the FileInfo is for.
 * @member pieces an ordered-list of associated FilePieces.
 * @member bfpId optional, the ID of the bfp used to generate the file's ID.
 * @member formatDetails file format details about the file info such as
 *                       average bitrate, number of MPEG frames,
 *                       and whether or not the file is a variable bitrate
 *                       file.
 */
typedef monarch::rt::DynamicObject FileInfo;
typedef monarch::rt::DynamicObject FileInfoList;
typedef monarch::rt::DynamicObjectIterator FileInfoIterator;

/**
 * A FilesystemEntry object contains information about an inode on the
 * filesystem.
 *
 * FilesystemEntry
 * {
 *    "path" : string,
 *    "size" : uint64,
 *    "readable" : bool,
 *    "type" : string (file|directory),
 * }
 *
 * @member path the absolute pathname associated with the filesystem entry.
 * @member size the size of the filesystem entry on disk.
 * @member readable true if the entry is accessible, false if not.
 * @member type the type of the entry. For example, a 'file' or 'directory'.
 */
typedef monarch::rt::DynamicObject FilesystemEntry;
typedef monarch::rt::DynamicObject FilesystemEntryList;
typedef monarch::rt::DynamicObjectIterator FilesystemEntryIterator;

/**
 * A KeyInfo contains information about a symmetric cryptography key.
 *
 * KeyInfo
 * {
 *    "algorithm" : string,
 *    "data" : string,
 *    "length" : uint32
 * }
 *
 * @member algorithm the algorithm for the key, i.e. "AES256".
 * @member data the hex-encoded concatenated data+IV for the key.
 * @member length the length of the key data (without the IV) in bytes.
 */
typedef monarch::rt::DynamicObject KeyInfo;
typedef monarch::rt::DynamicObjectIterator KeyInfoIterator;

/**
 * A FilePiece contains information about a piece of a file.
 *
 * FilePiece data can be stored in a file that is located in the path
 * associated with this object. The name for that file will be a concatenation
 * of the File ID the piece is a part of, and the index for the piece, with
 * the identifier separated by a hypen. For example:
 *
 * FileId-1.piece
 * FileId-2.piece
 * FileId-3.piece
 *
 * FilePiece
 * {
 *    "index" : uint32,
 *    "path" : string,
 *    "size" : uint32,
 *    "encrypted" : boolean,
 *    "openKey" : KeyInfo,
 *    "pieceKey" : KeyInfo,
 *    "ciphered" : boolean,
 *    "sellerSignature" : string,
 *    "sellerProfileId" : ProfileId (uint32),
 *    "bfpSignature" : string,
 *    "bfpId" : BfpId (uint32)
 * }
 *
 * @member index the index for the FilePiece relative to all other pieces.
 * @member path the file system path where the file for the piece resides.
 * @member size the size, in bytes, for the piece data (this may be the
 *              requested size -- which may be larger than the actual size).
 *              Also, a size of 0 indicates the piece takes up the full file
 *              size.
 * @member encrypted a boolean indicating if the piece is encrypted or not.
 * @member openKey a KeyInfo (only present when the piece key is sealed).
 * @member pieceKey a KeyInfo (for encrypting/decrypting a piece), its key
 *                  data may be sealed by an openKey that itself is encrypted
 *                  such that it can only be used by the SVA to open a pieceKey.
 * @member ciphered true if the piece was encrypted for transfer, false if not.
 * @member sellerSignature the seller's hex-signature for the FilePiece.
 * @member sellerProfileId the seller's profile ID at the time of signing.
 * @member bfpSignature the bfp's hex-signature for the FilePiece.
 * @member bfpId the ID of the bfp used on the piece.
 */
typedef monarch::rt::DynamicObject FilePiece;
typedef monarch::rt::DynamicObjectIterator FilePieceIterator;
typedef monarch::rt::DynamicObject FilePieceList;

/**
 * A Payee encapsulates an AccountId and an amount. The amount may be a
 * flat-fee or a percentage of some other amount.
 *
 * If the payee's amount type is a percentage, then there are several ways
 * that the amount can be calculated:
 *
 * 1. As a license percentage, that is, a percentage of the total of all
 * royalties associated with a purchase -- excluding nontaxable amounts.
 *
 * 2. As a cumulative percentage on the total calculated amount of payees
 * that occur before it in a list of payees. This total does not include
 * Bitmunk fees.
 *
 * 3. As a percentage of the total of the list of other Payees. To calculate
 * this, the sum of all non-PercentOfTotal and non-Tax payee amounts will be
 * totaled and used as the base for all PercentOfTotal payees.
 *
 * 4. As a tax on the current total of other Payees. To calculate this, all
 * other payees not marked as nontaxable are totaled then each Tax payee is
 * applied in order to the new total.  Payees that are nontaxable include
 * Bitmunk fees, as these are paid directly to Bitmunk for using the Bitmunk
 * service and are not taxed.
 *
 * Payee
 * {
 *    "id" : AccountId (uint64),
 *    "amountType" : string,
 *    "amountResolved" : boolean,
 *    "amount" : string,
 *    "percentage" : string,
 *    "min" : string,
 *    "mediaId" : MediaId (uint64)
 *    "description" : string,
 *    "nontaxable" : boolean,
 * }
 *
 * @member id the AccountId.
 * @member amountType the payee amount type, which can be any one of
 *                    'flatfee', 'plicense', 'pcumulative', 'ptotal', or 'tax'.
 * @member amountResolved true if the amount has already been resolved to a
 *                        flat fee and should not be done so again.
 * @member amount the resolved flat fee amount, if available, as a string.
 * @member percentage the percentage, as a string. ("0.10" = 10%)
 * @member min a minimum amount -- useful for percentage-based payees.
 * @member mediaId the ID of the media this payee is a royalty for.
 * @member description a description for the payment.
 * @member nontaxable true if this Payee is exempt from Tax payee totals, false
 *         if it is not.
 */
typedef monarch::rt::DynamicObject Payee;
typedef monarch::rt::DynamicObjectIterator PayeeIterator;

typedef const char* PayeeAmountType;
#define PAYEE_AMOUNT_TYPE_FLATFEE     "flatFee"
#define PAYEE_AMOUNT_TYPE_PLICENSE    "percentOfLicense"
#define PAYEE_AMOUNT_TYPE_PCUMULATIVE "percentOfCumulative"
#define PAYEE_AMOUNT_TYPE_PTOTAL      "percentOfTotal"
#define PAYEE_AMOUNT_TYPE_TAX         "tax"

const bool PayeeResolved = true;
const bool PayeeUnresolved = false;
const bool PayeeRegular = false;
const bool PayeeNontaxable = true;
typedef monarch::rt::DynamicObject PayeeList;

/**
 * A PayeeScheme contains the ID of the user that manages the scheme, the ID
 * of the scheme, a description of the scheme and an optional list of payees
 * that are associated with the scheme. PayeeSchemes are used in order to cut
 * down on the amount of data that is required to serialize a list of payees.
 *
 * PayeeScheme
 * {
 *    "userId" : uint64,
 *    "serverId" : uint32,
 *    "id" : uint32,
 *    "description" : string,
 *    "payees" : [] (Payees)
 * }
 *
 * @member userId The ID of the user that created or manages the scheme.
 * @member serverId the ID of the user's server the scheme belongs to, not
 *                  present or 0 for a license payee scheme.
 * @member id The ID of the scheme.
 * @member description A human-readable description of the payee scheme.
 * @member payees An optional list of payees that are associated with the
 *                scheme.
 */
typedef monarch::rt::DynamicObject PayeeScheme;
typedef monarch::rt::DynamicObjectIterator PayeeSchemeIterator;

/**
 * A PayeeRule is a restriction regarding a Payee or list of Payees. Such a
 * rule is used to ensure that licenses for media whose final total amounts
 * can be determined dynamically will be compliant with some condition(s).
 *
 * For instance, a license that has 2 Payees that are percentages of the total
 * license amount may require that additional Payees be added to a license such
 * that the total license amount will be greater than or equal to $1 before the
 * license can be issued to a buyer. Similarly, a PayeeRule can specify that
 * only Payees with specific account IDs or with specific owners can be added
 * to a royalty license.
 *
 * These rules allow content owners to allow dynamic licenses to be issued
 * for their content and add some element of control over its final price and
 * who can be paid in the license.
 *
 * PayeeRule
 * {
 *    "mediaId" : MediaId (uint64),
 *    "type" : string,
 *    "value" : string
 * }
 *
 * @member mediaId the ID of the media the rule applies to.
 * @member type the type of rule (i.e. "minTotal", "maxTotal", "ownerId").
 * @member value the value for the rule (i.e. "0.99", "2.99", "12345").
 */
typedef monarch::rt::DynamicObject PayeeRule;
typedef monarch::rt::DynamicObject PayeeRuleList;
typedef monarch::rt::DynamicObjectIterator PayeeRuleIterator;

/**
 * A Ware is a single entity that can be sold on Bitmunk. It contains a
 * collection of 1-n FileInfos. A Ware also has an ID that provides information
 * about its contents.
 *
 * How a Ware's ID is generated depends on either the seller's catalog type
 * or if the Ware was custom built by a buyer.
 *
 * There are 3 categories for Ware IDs:
 *
 * 1. A Ware with a single file, typically intended for PeerBuy purchase. The
 * ID format:
 *
 * bitmunk:file:<mediaId>[-<fileId>[-<bfpId>]]
 *
 * File IDs on bitmunk are SHA-1 hashes produced by a BFP. A file that has a
 * file ID of "550ef4876a8cd668fbda18a070d515717220b38a" that is associated
 * with media ID 2 and was produced by the default BFP (which has an ID of 1)
 * would have the following ID:
 *
 * bitmunk:file:2-550ef4876a8cd668fbda18a070d515717220b38a
 *
 * 2. A Ware that was constructed by a buyer on the fly, typically on a website
 * and similar to a shopping cart, and therefore has no associated catalog
 * entry. It essentially represents a bundle of other Wares. The ID format:
 *
 * bitmunk:bundle
 *
 * The most common example of the use of this type of Ware is when a buyer
 * selects a collection (i.e. an album) to purchase via WebBuy or PeerBuy on a
 * UI. Then the UI presents the buyer will several file format, bitrate, or
 * other options to choose from in order to select the specific files that will
 * make up the collection. The specific files and their IDs are stored in the
 * Ware.
 *
 * In the case of PeerBuy the PeerBuy-software must examine the contents of the
 * Ware to construct individual Wares out of the bundle.
 *
 * As an example, if a buyer created a Ware with an ID of "bitmunk:bundle" and
 * it contained 2 files with IDs 550ef4876a8cd668fbda18a070d515717220b38a, for
 * media ID 2, and 6607ac876a8cd668fbda18a070d515717220b38a, for media ID 3,
 * then a piece of software executing the PeerBuy would create two Wares from
 * the bundle that each contain a file, having Ware IDs:
 * "bitmunk:file:2-550ef4876a8cd668fbda18a070d515717220b38a" and
 * "bitmunk:file:3-6607ac876a8cd668fbda18a070d515717220b38a" respectively.
 *
 * In the case of WebBuy the specially authorized WebBuy-software will receive
 * the ID of the contract that was purchased via an HTTP-GET from the buyer.
 * Using that ID and the credentials of the seller, it will retrieve the bundle
 * Ware from an SVA and thus obtain the file IDs of the files that it needs to
 * archive and send to the buyer. The buyer may optionally select the archive
 * format for delivery.
 *
 * 3. A Ware on a proprietary network that is using bitmunk technology. The
 * ID format:
 *
 * bitmunk:<mySpecificNetworkIdentifier>:<myIdFormat>
 *
 * It is possible that even the "bitmunk" prefix may not appear in this Ware
 * ID format, but this has not been decided upon at the time of this writing.
 *
 * Wares with this ID format are never stored in the bitmunk mastercatalog, and
 * are only meaningful to proprietary sales and purchase software that are
 * using bitmunk technology. If these Ware IDs are sent to standard bitmunk
 * software, they will be rejected as invalid.
 *
 * Ware
 * {
 *   "id" : string,
 *   "mediaId" : MediaId (uint64),
 *   "description", string,
 *   "payees" : [] (Payees),
 *   "fileInfos" : [] (FileInfos)
 * }
 *
 * @member id an ID for the Ware, see docs for ID formats.
 * @member mediaId the MediaId that refers to the collection metadata, if any.
 * @member description signed description string of what's in the ware.
 * @member payees an array of Payees.
 * @member fileInfos an array of FileInfos.
 */
typedef monarch::rt::DynamicObject Ware;
typedef monarch::rt::DynamicObjectIterator WareIterator;

/**
 * A User contains information about a Bitmunk user, including user details,
 * identity, accounts, permissions and seller keys.
 *
 * User
 * {
 *    "id" : UserId (uint64),
 *    "username" : string,
 *    "email" : string,
 *    "password" : string,
 *    "parentId" : UserId (uint64),
 *    "emailCode" : string,
 *    "permissionSet", PermissionSetId (uint32)
 * }
 *
 * FIXME: add description here
 */
typedef monarch::rt::DynamicObject User;
typedef monarch::rt::DynamicObjectIterator UserIterator;

/**
 * A Buyer contains information about a Bitmunk user that is making
 * a purchase.
 *
 * Buyer
 * {
 *    "userId" : UserId (uint64),
 *    "username" : string,
 *    "profileId" : ProfileId (uint32),
 *    "delegateId" : UserId (uint64),
 *    "accountId" : AccountId (uint64)
 * }
 *
 * @member userId the UserId of the Bitmunk user.
 * @member username the username of the Bitmunk user.
 * @member profileId the buyer's (or delegate's, if present) profile ID.
 * @member delegateId the ID of the delegate who signed on behalf of the user.
 * @member accountId an optional account ID used when making purchases.
 */
typedef monarch::rt::DynamicObject Buyer;
typedef monarch::rt::DynamicObjectIterator BuyerIterator;

/**
 * A Seller contains information about a Bitmunk user that is selling content.
 *
 * Each User must keep track of their assigned ServerId and can provide updates
 * using that ServerId in combination with a ServerToken.
 *
 * Seller
 * {
 *    "userId" : UserId (uint64),
 *    "serverId" : ServerId (uint32),
 *    "profileId" : ProfileId (uint32),
 *    "url" : string
 * }
 *
 * @member userId the user ID of the Bitmunk user.
 * @member serverId the user's unique server ID.
 * @member profileId the seller's profile ID.
 * @member url the BTP URL to the seller's unique sales server.
 */
typedef monarch::rt::DynamicObject Seller;
typedef monarch::rt::DynamicObjectIterator SellerIterator;

/**
 * SellerStats contain information about how the seller has performed over
 * the lifetime of the server's listing.
 *
 * SellerStats
 * {
 *    "avgThroughput" : uint32,
 *    "transfers" : uint32
 * }
 *
 * @member avgThroughput the average transfer speed of file pieces over the
 *                       entire lifetime of the seller.
 * @member transfers the total number of successful transfers over the entire
 *                   lifetime of the seller.
 *
 */
typedef monarch::rt::DynamicObject SellerStats;
typedef monarch::rt::DynamicObjectIterator SellerStatsIterator;

/**
 * A SellerData is a collection of information about a seller and what is or
 * could be downloaded from them, etc.
 *
 * SellerData
 * {
 *    "seller" : Seller,
 *    "price" : string,
 *    "section" : ContractSection,
 *    "downloadRate" : double
 * }
 *
 * @member seller the Seller information.
 * @member price a total price for a ware from the SellerPool.
 * @member section an optional ContractSection with no FilePieces.
 * @member downloadRate an optional download rate (bytes/second) for the seller.
 */
typedef monarch::rt::DynamicObject SellerData;
typedef monarch::rt::DynamicObject SellerDataList;
typedef monarch::rt::DynamicObjectIterator SellerDataIterator;

/**
 * SellerPoolStats stores information about seller pool's statistics.
 *
 * SellerPoolStats
 * {
 *    "listingCount" : uint32,
 *    "updateCount" : uint32,
 *    "updateTime" : string,
 *    "minPrice" : string,
 *    "medPrice" : string,
 *    "maxPrice" : string,
 *    "userMedPrice" : string,
 *    "avgRating" : uint32,
 *    "maxRating" : uint32,
 *    "minThroughput" : uint32,
 *    "medThroughput" : uint32
 * }
 *
 * @member listingCount the number of listings currently in the pool.
 * @member updateCount the number of updates that have been applied to
 *                     the pool since the stats were recalculated.
 * @member updateTime the last time the pool was updated (YYYY-MM-DD HH:MM:SS).
 * @member minPrice the lowest price sellers are offering in the pool.
 * @member medPrice the median price sellers are offering in the pool.
 * @member minPrice the highest price sellers are offering in the pool.
 * @member userMedPrice the median price for all users that are not the bitmunk
 *                      user.
 * @member avgRating the average user rating for sellers in the pool.
 * @member maxRating the maximum user rating for sellers in the pool.
 * @member minThroughput the lowest data throughput for sellers in the pool.
 * @member medThroughput the median data throughput for sellers in the pool.
 */
typedef monarch::rt::DynamicObject SellerPoolStats;
typedef monarch::rt::DynamicObjectIterator SellerPoolStatsIterator;

/**
 * A SellerPool is a collection of Sellers that are all selling the same file.
 *
 * SellerPool
 * {
 *    "id" : PoolId,
 *    "fileInfo" : FileInfo,
 *    "sellerDataSet" : ResourceSet (of SellerData),
 *    "pieceSize" : uint32,
 *    "pieceCount" : uint32,
 *    "bfpId" : uint32,
 *    "referenceFile" : boolean,
 *    "stats" : SellerPoolStats
 * }
 *
 * @member id the PoolId only used within the master catalog.
 * @member fileInfo the info for the file the seller pool is for.
 * @member sellerDataSet a ResourceSet of SellerData for sellers in the pool.
 * @member pieceSize the minimum piece size.
 * @member pieceCount the number of pieces in the file.
 * @member bfpId the ID of the bfp that must be used with the pool.
 * @member referenceFile flag that indicates that the seller pool is the
 *                       original file (aka: reference file) provided by the
 *                       media creator.
 * @member stats the seller pool statistics.
 *
 * FIXME: need seller ratings: speed+network reliability
 */
typedef monarch::rt::DynamicObject SellerPool;
typedef monarch::rt::DynamicObject SellerPoolList;
typedef monarch::rt::DynamicObjectIterator SellerPoolIterator;

/**
 * A seller listing is the data indicating that a seller is selling a file
 * in a specific seller pool.
 *
 * SellerListing["poolId"] is required for database calls. The pool ID can be
 * found using the catalog database's getPoolId() call.
 *
 * SellerListing
 * {
 *    "seller" : Seller,
 *    "poolId" : PoolId (uint64),
 *    "fileInfo" : FileInfo,
 *    "price" : string,
 *    "payeeSchemeId" : uint32
 * }
 *
 * @member seller the Seller who is selling this listing.
 * @member poolId the PoolId only used within the master catalog.
 * @member fileInfo the FileInfo containing the MediaId and FileId and any
 *                  details about the file.
 * @member price the computed total data cost of purchasing the entire file
 *               from this seller.
 * @member payeeSchemeId the id of the payee scheme to use with this listing.
 */
typedef monarch::rt::DynamicObject SellerListing;
typedef monarch::rt::DynamicObjectIterator SellerListingIterator;

/**
 * A SellerListingUpdate holds the information a Seller sends to the master
 * catalog to update and remove their listings and/or prevent their listings
 * from being marked as inactive.
 *
 * The ServerToken must be included and match the token listed for that server
 * for any heartbeat or updates to be processed.
 *
 * A SellerListingUpdate will be returned as the response to a API listing
 * update call. The response will contain the seller ServerId and the
 * SellerListing updates and removals that failed. The value of updateId
 * will be returned in the response as the value of newUpdateId after a
 * successful listing update.
 *
 * SellerListingUpdate
 * {
 *    "seller" : Seller,
 *    "serverToken" : ServerToken,
 *    "updateId" : string,
 *    "newUpdateId" : string,
 *    "listings" : Map,
 *       "updates" : [] of
 *          "fileInfo" : FileInfo,
 *          "payeeSchemeId" : PayeeSchemeId (uint32),
 *       "removals" : [] of
 *          "fileInfo" : FileInfo,
 *          "payeeSchemeId" : PayeeSchemeId (uint32),
 *    "payeeSchemes" : Map,
 *       "updates" : [] (PayeeScheme),
 *       "removals" : [] (PayeeScheme)
 * }
 *
 * @member seller the Seller that made the catalog update. The Seller's
 *                ServerId will be 0 if one needs to be assigned by the master
 *                catalog.
 * @member serverToken the ServerToken assigned to the Seller's server.
 * @member updateId the ID chosen by the seller that identifies the current
 *                  state of their listings uploaded to the master catalog.
 * @member newUpdateId the new ID chosen by the seller to track the changes
 *                     that will be applied during this update. Will not be
 *                     included in the response.
 * @member listings a map with listing updates and removals.
 *    @member updates the set of listings the seller wants to add or update.
 *    @member removals the set of listings the seller wants to remove.
 * @member payeeSchemes a map with payee scheme updates and removals.
 *    @member updates the set of schemes the seller wants to add or update.
 *    @member removals the set of schemes the seller wants to remove.
 */
typedef monarch::rt::DynamicObject SellerListingUpdate;
typedef monarch::rt::DynamicObjectIterator SellerListingUpdateIterator;

/**
 * A ContractSection is a binding digital contract between a Buyer and one or
 * more Sellers.
 *
 * Note: PeerBuy keys are generated according to a particular seller's
 * parameters. They are only used by buyers to permit them to download file
 * pieces. Therefore, sellers may generate them however they see fit.
 *
 * ContractSection
 * {
 *    "contractId" : TransactionId (uint64)
 *    "hash" : string,
 *    "buyer" : Buyer,
 *    "seller" : Seller,
 *    "sellerSignature" : string,
 *    "buyerSignature" : string,
 *    "ware"   : Ware,
 *    "webbuy" : boolean,
 *    "webbuyKey" : string,
 *    "peerbuyKey" : string,
 *    "negotiationTerms" : ?
 * }
 *
 * @member contractId the ID of the parent contract.
 * @member hash a special hash to uniquely identify this contract section, and
 *              is only required to be present for PeerBuy.
 * @member buyer the buyer associated with the section.
 * @member seller the Seller associated with the section.
 * @member sellerSignature the seller's hex-signature for the
 *                         ContractSection.
 * @member buyerSignature the buyer's hex-signature for the ContractSection.
 * @member ware the ware being purchased.
 * @member webbuy true for a WebBuy purchase, false for a PeerBuy purchase.
 * @member webbuyKey a hex-encoded key for a WebBuy purchase (provided by SVA).
 * @member peerbuyKey a hex-encoded key for downloading PeerBuy pieces
 *                    (provided by seller and can therefore be customized by
 *                     that seller).
 * @member negotiationTerms special negotiation terms understood by
 *                          implementation-specific negotiation modules.
 */
typedef monarch::rt::DynamicObject ContractSection;
typedef monarch::rt::DynamicObjectIterator ContractSectionIterator;

/**
 * A Contract is a binding digital contract between a Buyer and one or
 * more Sellers.
 *
 * A Contract is made up of ContractSections, with each section relating
 * to one Seller. Sellers may only gain access to their particular section,
 * whereas the Buyer has access to all sections of the Contract.
 *
 * ContractSections *must* only be added to the same Contract if they share
 * the same Ware.
 *
 * Contract
 * {
 *    "version" : string,
 *    "id" : TransactionId (uint64),
 *    "media" : Media,
 *    "buyer" : Buyer,
 *    "sections" : {"sellerId"->ContractSection[]},
 *    "complete" : boolean,
 *    "signature" : string
 * }
 *
 * @member version the version ("3.0") for the Contract.
 * @member id the unique ID for the Contract.
 * @member media the Media being purchased in the Contract.
 * @member buyer the Buyer in the Contract.
 * @member sections a Seller's userId mapped to ContractSections.
 * @member complete true once the Contract is complete, false otherwise.
 * @member signature the buyer's hex-signature for the Contract.
 */
typedef monarch::rt::DynamicObject Contract;
typedef monarch::rt::DynamicObjectIterator ContractIterator;

/**
 * A Permission Group contains categories of permissions for a Bitmunk group.
 *
 * PermissionGroup
 * {
 *    "id" : PermissionGroupId (uint32),
 *    "account" : Map,
 *    "catalog" : Map,
 *    "financial" : Map,
 *    "media" : Map,
 *    "review" : Map,
 *    "special" : Map,
 *    "support" : Map,
 *    "user" : Map
 * }
 *
 * @member id the PermissionGroupId of the group.
 * @member account a map of account permissions.
 * @member catalog a map of catalog permissions.
 * @member financial a map of financial permissions.
 * @member media a map of media permissions.
 * @member review a map of review permissions.
 * @member special a map of special permissions.
 * @member support a map of support permissions.
 * @member user a map of user permissions.
 */
typedef monarch::rt::DynamicObject PermissionGroup;
typedef monarch::rt::DynamicObjectIterator PermissionGroupIterator;
typedef monarch::rt::DynamicObjectIterator PermissionCategoryIterator;
typedef monarch::rt::DynamicObjectIterator PermissionIterator;

/**
 * The PurchaseRecord stores information about a media license that has been
 * purchased by a Bitmunk user and how many times the data download was
 * completed.
 *
 * FIXME: Expand structure to store dates and WebBuy/PeerBuy for each data
 * download record.
 *
 * PurchaseRecord
 * {
 *    "userId" : UserId (uint64),
 *    "mediaId" : MediaId (uint64),
 *    "transactionId" : TransactionId (uint64),
 *    "webbuyDownloads" : (uint32),
 *    "date" : string,
 *    "media" : Media
 * }
 *
 * @member userId the ID of the user who purchased the media license.
 * @member mediaId the ID of the media the user purchased.
 * @member transactionId the ID of the transaction that purchased the media
 *                       license.
 * @member webbuyDownloads the number of times the data download was completed.
 * @member date the date the license was purchased.
 * @member media the optional media info for the license that was purchased.
 */
typedef monarch::rt::DynamicObject PurchaseRecord;
typedef monarch::rt::DynamicObjectIterator PurchaseRecordIterator;

/**
 * The ResourceSet interface is used to return a collection of resources from a
 * resource call. The actual number of resources returned in the list may
 * (and often will) be less than the total number of resources in the
 * collection. The total number is indicated by the "total" member.
 *
 * ResourceSet
 * {
 *    "resources" : [] (Resources)
 *    "total" : (uint64),
 *    "start" : (uint64),
 *    "num" : (uint64)
 * }
 *
 * @member resources a list (subset) of resources from a the collection.
 * @member total the total number of resources in the collection (ALL of the
 *               resources, not just those in the subset list).
 * @member start the index of the first resource in the collection.
 * @member num the number of resources to retrieve as a subset of the
 *             collection.
 */
typedef monarch::rt::DynamicObject ResourceSet;
typedef monarch::rt::DynamicObjectIterator ResourceSetIterator;

/**
 * A support ticket contains information about a customer support request.
 * A ticket id is user dependent.
 *
 * SupportTicket
 * {
 *    "userId" : UserId (uint64),
 *    "id" : TicketId (uint32),
 *    "status" : string,
 *    "date" : string,
 *    "subject" : string,
 *    "description" : string
 * }
 */
typedef monarch::rt::DynamicObject SupportTicket;
typedef monarch::rt::DynamicObjectIterator SupportTicketIterator;

/**
 * A support ticket contains information about a customer support request.
 * A ticket id is user dependent.
 *
 * SupportTicketComment
 * {
 *    "userId" : UserId (uint64),
 *    "ticketId" : TicketId (uint32),
 *    "position" : uint32,
 *    "date" : string,
 *    "commenterId" : UserId (uint64),
 *    "comment" : string,
 *    "commenterName" : string
 * }
 */
typedef monarch::rt::DynamicObject SupportTicketComment;
typedef monarch::rt::DynamicObjectIterator SupportTicketCommentIterator;

/**
 * A Transfer represents a single movement of funds from a source to a
 * destination. A Transfer is a single part of a Transaction -- which is
 * an aggregate of Transfers under a single Transaction ID.
 *
 * Transfer
 * {
 *    "id" : uint32,
 *    "type" : TransferType (enum/int32),
 *    "date" : string,
 *    "sourceId" : AccountId (uint64),
 *    "destinationId" : AccountId (uint64),
 *    "amount" : string,
 *    "description" : string
 * }
 *
 * @member id the ID unique to the transfer inside of a Transaction.
 * @member type the type of the transfer.
 * @member date the date for the transfer.
 * @member sourceId the ID of the source account, 0 for an external source.
 * @member destinationId the ID of the destination account, 0 for an
 *                       external destination.
 * @member amount the amount of money involved.
 * @member description a description for the money being moved.
 */
typedef monarch::rt::DynamicObject Transfer;
typedef monarch::rt::DynamicObject TransferList;
typedef monarch::rt::DynamicObjectIterator TransferIterator;
enum TransferType
{
   TransferTypeLicense = 0,
   TransferTypeData = 1,
   TransferTypeDeposit = 2,
   TransferTypeTransfer = 3,
   TransferTypeWithdrawal = 4
};

/**
 * A Transaction represents a set of similar Transfers. The Transaction group
 * type is set based on the types of Transfers it encompasses.
 *
 * Transaction
 * {
 *    "id" : TransactionId (uint64),
 *    "transfers" : [] (array of Transfers),
 *    "groupType" : string
 * }
 *
 * @member id the common ID linking all of the associated transfers.
 * @member transfers the list of transfers.
 * @member groupType the type of transaction group the transfers belong to.
 */
typedef monarch::rt::DynamicObject Transaction;
typedef monarch::rt::DynamicObjectIterator TransactionIterator;

typedef const char* TransactionGroupType;
#define TRANSACTION_GROUP_TYPE_DEPOSIT    "deposit"
#define TRANSACTION_GROUP_TYPE_TRANSFER   "transfer"
#define TRANSACTION_GROUP_TYPE_CONTRACT   "contract"
#define TRANSACTION_GROUP_TYPE_WITHDRAWAL "withdrawal"

/**
 * A Deposit is used to move funds from an external source to Bitmunk Account.
 *
 * Deposit
 * {
 *    "id" : TransactionId (uint64),
 *    "type" : string,
 *    "date" : string (datetime 'YYYY-MM-DD HH:MM:SS' GMT),
 *    "source" : CreditCard,
 *    "gateway" : string,
 *    "payees" : [] Payees,
 *    "total" : string,
 *    "ip" : string,
 *    "result" : CreditCardProcessingResult
 *    "signature" : string,
 *    "signer" : Map
 *    {
 *       "userId" : UserId (uint64),
 *       "profileId" : ProfileId (uint32)
 *    }
 * }
 *
 * @member id the TransactionId associated with this deposit.
 * @member type the type of deposit ("creditcard").
 * @member date the date-time of the deposit.
 * @member source the CreditCard that is being used to deposit.
 * @member gateway the gateway that was used to perform the deposit.
 * @member payees the list of Payees to send money to, if this list is
 *                empty, then only a pre-auth will be peformed for a
 *                credit card (if process is true).
 * @member total the total amount set by a service that pre-processes
 *               the deposit.
 * @member ip the IP address of the person performing the deposit (this is
 *            only applicable when a delegate is performing the deposit).
 * @member result the result of the deposit.
 * @member signatureVersion the signature's version.
 * @member signature the signer's hex-encoded signature.
 * @member signer.userId the signer's user ID.
 * @member signer.profileId the signer's profile ID.
 */
typedef monarch::rt::DynamicObject Deposit;
typedef monarch::rt::DynamicObjectIterator DepositIterator;

/**
 * A Withdrawal is used to move funds from a Bitmunk Account to an external
 * destination.
 *
 * Withdrawal
 * {
 *    "id" : TransactionId (uint64),
 *    "type" : string,
 *    "date" : string (datetime 'YYYY-MM-DD HH:MM:SS' GMT),
 *    "sourceId" : AccountId (uint64),
 *    "destination" : PayPalThingy (implement me),
 *    "amount" : amount,
 *    "description" : string,
 *    "ip" : string
 * }
 *
 * @member id the TransactionId associated with this withdrawal.
 * @member type the type of withdrawal ("check", "electronic", "paypal").
 * @member date the date-time of the withdrawal.
 * @member sourceId the ID of the source Account.
 * @member destination the PayPalThingy (implement me).
 * @member amount the amount to withdraw.
 * @member description a description for the withdrawal.
 * @member ip the IP address of the person performing the withdrawal (this is
 *            only applicable when a delegate is performing the deposit).
 */
typedef monarch::rt::DynamicObject Withdrawal;
typedef monarch::rt::DynamicObjectIterator WithdrawalIterator;

/**
 * A CreditCard contains information about a real-world credit card.
 *
 * CreditCard
 * {
 *    "type" : string,
 *    "number" : string,
 *    "expMonth" : string,
 *    "expYear" : string,
 *    "cvm" : string,
 *    "name" : string,
 *    "firstName" : string,
 *    "lastName" : string
 *    "address" : Address
 * }
 *
 * @member type the type of credit card (i.e. visa, mastercard).
 * @member number the credit card number (i.e. 4000123400001234).
 * @member expMonth the 2-digit expiration month.
 * @member expYear the 2-digit expiration year.
 * @member cvm the credit card verification number.
 * @member name the full name on the credit card.
 * @member firstName the first name on the credit card (optional).
 * @member lastName the last name on the credit card (optional).
 * @member address the credit card holder's address.
 */
typedef monarch::rt::DynamicObject CreditCard;
typedef monarch::rt::DynamicObjectIterator CreditCardIterator;

typedef const char* CreditCardProcessingResult;
#define CREDIT_CARD_PROCESSING_RESULT_UNPROCESSED      "unprocessed"
#define CREDIT_CARD_PROCESSING_RESULT_APPROVED         "approved"
#define CREDIT_CARD_PROCESSING_RESULT_DECLINED         "declined"
#define CREDIT_CARD_PROCESSING_RESULT_FRAUD            "fraud"
#define CREDIT_CARD_PROCESSING_RESULT_DUPLICATE        "duplicate"
#define CREDIT_CARD_PROCESSING_RESULT_ERROR            "error"
#define CREDIT_CARD_PROCESSING_RESULT_INVALID_TYPE     "invalidType"
#define CREDIT_CARD_PROCESSING_RESULT_INVALID_NUMBER   "invalidNumber"
#define CREDIT_CARD_PROCESSING_RESULT_INVALID_EXP_DATE "invalidExpDate"
#define CREDIT_CARD_PROCESSING_RESULT_INVALID_CVM      "invalidCvm"
#define CREDIT_CARD_PROCESSING_RESULT_INVALID_ADDRESS  "invalidAddress"
#define CREDIT_CARD_PROCESSING_RESULT_INVALID_ZIP      "invalidZip"

/**
 * Defines a number of common exceptions used in Bitmunk.
 */
#define BITMUNK_NOT_IMPLEMENTED   "bitmunk.common.NotImplemented"
#define BITMUNK_PERMISSION_DENIED "bitmunk.permission.PermissionDenied"

} // end namespace common
} // end namespace bitmunk

#endif

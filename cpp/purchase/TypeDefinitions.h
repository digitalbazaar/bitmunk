/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_purchase_TypeDefinitions_H
#define bitmunk_purchase_TypeDefinitions_H

#include "monarch/rt/DynamicObject.h"
#include "monarch/rt/DynamicObjectIterator.h"

namespace bitmunk
{
namespace purchase
{

// ID type definitions:
typedef uint64_t DownloadStateId;

/**
 * A DownloadState is a collection of information about a Contract download.
 *
 * DownloadState
 * {
 *    "version" : string,
 *    "userId" : UserId (uint64),
 *    "id" : DownloadStateId (uint64),
 *    "ware" : Ware,
 *    "totalMinPrice" : string,
 *    "totalMedPrice" : string,
 *    "totalMaxPrice" : string,
 *    "totalPieceCount" : uint32,
 *    "totalMicroPaymentCost" : string
 *    "contract" : Contract,
 *    "preferences" : PurchasePreferences,
 *    "progress" : FileId => FileProgress,
 *    "remainingPieces" : uint32,
 *    "initialized" : Boolean,
 *    "licenseAcquired" : Boolean,
 *    "downloadStarted" : Boolean,
 *    "downloadPaused" : Boolean,
 *    "licensePurchased" : Boolean,
 *    "dataPurchased" : Boolean,
 *    "filesAssembled" : Boolean,
 *    "activeSellers" : "sellerId:serverId" => uint32,
 *    "blacklist" : "sellerId:serverId" => {
 *       "seller" : Seller,
 *       "time" : uint64
 *    },
 *    "processing" : Boolean,
 *    "processorId" : uint32,
 * }
 *
 * @member version the version ("3.0") for the DownloadState.
 * @member userId the ID of the user that owns the DownloadState.
 * @member id a unique ID for the DownloadState.
 * @member ware the peerbuy bundle ware to be purchased.
 * @member totalMinPrice the total min price for the entire ware bundle.
 * @member totalMedPrice the total median price for the entire ware bundle.
 * @member totalMaxPrice the total max price for the entire ware bundle.
 * @member totalPieceCount the total number of pieces.
 * @member totalMicroPaymentCost the total micro payment cost.
 * @member contract the Contract associated with the download, no sections set.
 * @member preferences the PurchasePreferences associated with the download.
 * @member progress a map of FileId to FileProgress.
 * @member remainingPieces a count of the number of pieces not yet downloaded.
 * @member initialized true if the download state has been initialized.
 * @member licenseAcquired true if a license has been acquired.
 * @member downloadStarted true if the download has ever been started.
 * @member downloadPaused true if the download has been paused.
 * @member licensePurchased true if a license has been purchased.
 * @member dataPurchased true if all the data has been purchased.
 * @member filesAssembled true if all the files have been assembled.
 * @member activeSellers seller key => # of active connections to that seller.
 * @member blacklist a map of blacklisted sellers that could not be negotiated
 *                   with.
 * @member processing true if the download state is being processed.
 * @member processorId the ID of the process that is processing the
 *                     download state, 0 indicates that the download state
 *                     is about to be processed but hasn't been assigned yet.
 */
typedef monarch::rt::DynamicObject DownloadState;
typedef monarch::rt::DynamicObject DownloadStateList;
typedef monarch::rt::DynamicObjectIterator DownloadStateIterator;

/**
 * A PurchasePreferences is a set of information that describes which sellers
 * should be selected (favored) for a particular purchase.
 *
 * PurchasePreferences
 * {
 *    "version" : string,
 *    "fast" : boolean,
 *    "accountId" : uint32,
 *    "price" : Map,
 *       "max" : string,
 *    "sellerLimit" : uint32,
 *    "sellers" : [] of Sellers
 * }
 *
 * @member version the version ("3.0") for the PurchasePreferences.
 * @member fast true to prefer faster downloads, false to prefer cheaper.
 * @member accountId the financial account ID that should be used to perform
 *                   the purchase.
 * @member price the price preferences.
 *    @member max the maximum price to pay.
 * @member sellerLimit the maximum number of sellers to download from.
 * @member sellers a list of preferred sellers.
 */
typedef monarch::rt::DynamicObject PurchasePreferences;
typedef monarch::rt::DynamicObjectIterator PurchasePreferencesIterator;

/**
 * A FileProgress is a collection of information about the download and
 * decryption progress of a particular file.
 *
 * FileProgress
 * {
 *    "fileInfo" : FileInfo,
 *    "bytesDownloaded" : uint64,
 *    "budget" : string,
 *    "sellerPool" : SellerPool,
 *    "sellerData" : csHash => SellerData,
 *    "sellers" : "sellerId:serverId" => Seller,
 *    "unassigned" : [] FilePieces,
 *    "assigned" : csHash => [] FilePieces,
 *    "downloaded" : csHash => [] FilePieces,
 *    "microPaymentCost" : string
 * }
 *
 * @member fileInfo the FileInfo for the file the progress information is for.
 * @member byteDownloaded the total bytes fully received so far
 *                        (completed pieces).
 * @member budget the maximum price allowed for a seller to be assigned a piece.
 * @member sellerPool the SellerPool associated with the file.
 * @member sellerData csHash => SellerData with negotiated contract sections.
 * @member sellers a lookup map with sellers that have been negotiated with.
 * @member unassigned list of pieces not yet assigned to a seller.
 * @member assigned csHash => pieces assigned to download (or are downloading).
 * @member downloaded csHash => pieces that have finished downloading.
 * @member microPaymentCost the micropayment cost for the file.
 */
typedef monarch::rt::DynamicObject FileProgress;
typedef monarch::rt::DynamicObjectIterator FileProgressIterator;

} // end namespace purchase
} // end namespace bitmunk

#endif

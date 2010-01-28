/*
 * Copyright (c) 2007-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_sell_ContractService_H
#define bitmunk_sell_ContractService_H

#include "bitmunk/bfp/Bfp.h"
#include "bitmunk/node/NodeService.h"
#include "bitmunk/sell/UploadThrottlerMap.h"

namespace bitmunk
{
namespace sell
{

/**
 * A ContractService services Contract-related btp actions.
 *
 * @author Dave Longley
 * @author Manu Sporny
 */
class ContractService :
public bitmunk::node::NodeService,
public bitmunk::common::PublicKeySource
{
protected:
   /**
    * A cache for storing seller keys.
    */
   bitmunk::common::PublicKeyCache mSellerKeyCache;

   /**
    * A map of upload throttlers for users.
    */
   UploadThrottlerMap* mUploadThrottlerMap;

public:
   /**
    * Creates a new ContractService.
    *
    * @param node the associated Bitmunk Node.
    * @param path the path this servicer handles requests for.
    */
   ContractService(bitmunk::node::Node* node, const char* path);

   /**
    * Destructs this ContractService.
    */
   virtual ~ContractService();

   /**
    * Initializes this BtpService.
    *
    * @return true if initialized, false if an Exception occurred.
    */
   virtual bool initialize();

   /**
    * Cleans up this BtpService.
    *
    * Must be called after initialize() regardless of its return value.
    */
   virtual void cleanup();

   /**
    * Negotiates a Contract with a buyer.
    *
    * HTTP equivalent: POST .../negotiate?nodeuser=<sellerId>
    *
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool negotiateContract(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);

   /**
    * Sends a file piece to a buyer.
    *
    * HTTP equivalent: POST .../filepiece?nodeuser=<sellerId>
    *
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool getFilePiece(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);

   /**
    * Gets the seller key for the given seller ID.
    *
    * @param sellerID the user ID of the seller to get the key for.
    *
    * @return the seller key or NULL if an exception occurred.
    */
   virtual monarch::crypto::PublicKeyRef getSellerKey(
      bitmunk::common::UserId sellerId);

   /**
    * Note: This is used for retrieving seller keys.
    *
    * Gets a PublicKey from this cache.
    *
    * @param uid the UserId associated with the PublicKey.
    * @param pid the ProfileId associated with the PublicKey.
    * @param isDelegate always set to false for seller keys.
    *
    * @return the PublicKey or NULL if none could be obtained.
    */
   virtual monarch::crypto::PublicKeyRef getPublicKey(
      bitmunk::common::UserId uid, bitmunk::common::ProfileId pid,
      bool* isDelegate);

protected:

   /**
    * Adds a peerbuy key (for downloading pieces) and a hash to the passed
    * contract section.
    *
    * @param cs the ContractSection to update.
    * @param profile the Profile to use to generate the key.
    */
   virtual void addPeerBuyKey(
      bitmunk::common::ContractSection& cs,
      bitmunk::common::ProfileRef& profile);

   /**
    * Verifies a peerbuy key.
    *
    * @param peerBuyKey the peerbuy key to verify.
    * @param csHash the contract section hash.
    * @param sellerId the ID of the seller.
    * @param sellerProfileId the ID of the seller's profile used for the key.
    *
    * @return true if verified, false if not.
    */
   virtual bool verifyPeerBuyKey(
      const char* peerBuyKey, const char* csHash,
      bitmunk::common::UserId sellerId,
      bitmunk::common::ProfileId sellerProfileId);

   /**
    * Sends a file piece to the connected client.
    *
    * @param action the associated action.
    * @param bfp the bfp to use to get the piece data.
    * @param sellerId the ID of the seller.
    * @param fi the FileInfo for the piece.
    * @param index the index of the piece.
    * @param size the size of the piece.
    * @param csHash the ContractSection hash.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool sendPiece(
      bitmunk::protocol::BtpAction* action,
      bitmunk::bfp::Bfp* bfp, bitmunk::common::UserId sellerId,
      bitmunk::common::FileInfo& fi, uint32_t index, uint32_t size,
      const char* csHash);
};

} // end namespace sell
} // end namespace bitmunk
#endif

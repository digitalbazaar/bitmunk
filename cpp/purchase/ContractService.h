/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_purchase_ContractService_H
#define bitmunk_purchase_ContractService_H

#include "bitmunk/purchase/TypeDefinitions.h"
#include "bitmunk/node/NodeService.h"

namespace bitmunk
{
namespace purchase
{

/**
 * A ContractService services Contract-related btp actions.
 *
 * @author Dave Longley
 */
class ContractService : public bitmunk::node::NodeService
{
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
    * Creates a new download state and returns its ID.
    *
    * HTTP equivalent: POST .../downloadstates
    *
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool createDownloadState(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);

   /**
    * Gets all of the download states given a nodeuser and a filter value.
    *
    * HTTP equivalent: GET .../downloadstates?nodeuser=123&incomplete=true
    *
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool getDownloadStates(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);

   /**
    * Deletes a given download state. The delete is allowed until the point at
    * which the license or data has been purchased. If a delete request is
    * made after license and data has been purchased, an exception is thrown.
    *
    * HTTP equivalent: POST .../downloadstates/delete?nodeuser=123
    * HTTP equivalent: DELETE .../downloadstates/delete/456
    *
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool deleteDownloadState(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);

   /**
    * Gets a download state by its ID.
    *
    * HTTP equivalent: GET .../downloadstates/<downloadStateId>
    *
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool getDownloadState(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);

   /**
    * Attempts to initialize a download state.
    *
    * HTTP equivalent: POST .../initialize
    *
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool initializeDownloadState(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);

   /**
    * Attempts to acquire a license for the data to be downloaded.
    *
    * HTTP equivalent: POST .../license
    *
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool acquireLicense(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);

   /**
    * Starts acquiring data for the specified media.
    *
    * HTTP equivalent: POST .../download
    *
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool downloadContractData(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);

   /**
    * Attempts to pause a running download.
    *
    * HTTP equivalent: POST .../pause
    *
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool pauseDownload(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);

   /**
    * Starts purchasing data for the specified media.
    *
    * HTTP equivalent: POST .../purchase
    *
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool purchaseContractData(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);

   /**
    * Starts assembling all purchased data for the specified media.
    *
    * HTTP equivalent: POST .../unlock
    *
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool assembleContractData(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);

protected:
   /**
    * Cleans up any file pieces associated with a particular download state.
    *
    * @param ds the download state to clean up the file pieces for.
    *
    * @return true if successful, false if not.
    */
   virtual bool cleanupFilePieces(DownloadState& ds);
};

} // end namespace purchase
} // end namespace bitmunk
#endif

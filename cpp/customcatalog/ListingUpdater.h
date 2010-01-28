/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_customcatalog_ListingUpdater_H
#define bitmunk_customcatalog_ListingUpdater_H

#include "bitmunk/node/NodeFiber.h"
#include "bitmunk/customcatalog/Catalog.h"

namespace bitmunk
{
namespace customcatalog
{

/**
 * A ListingUpdater keeps a user's catalog in sync with their listings on
 * bitmunk.
 *
 * @author Dave Longley
 */
class ListingUpdater : public bitmunk::node::NodeFiber
{
protected:
   /**
    * The catalog to use.
    */
   Catalog* mCatalog;

   /**
    * The remote update ID.
    */
   int64_t mRemoteUpdateId;

   /**
    * The possible states this updater can be in.
    */
   enum State
   {
      Busy, Idle
   } mState;

   /**
    * Saves access to a current operation.
    */
   monarch::modest::Operation mOperation;

   /**
    * Sets whether or an update request is pending.
    */
   bool mUpdateRequestPending;

   /**
    * Sets whether or a test net access request is pending.
    */
   bool mTestNetAccessPending;

public:
   /**
    * Creates a new ListingUpdater.
    *
    * @param node the Node associated with this fiber.
    * @param catalog the catalog to use.
    */
   ListingUpdater(bitmunk::node::Node* node, Catalog* catalog);

   /**
    * Destructs this ListingUpdater.
    */
   virtual ~ListingUpdater();

protected:
   /**
    * Processes messages, retrieved via getMessages(), and performs whatever
    * custom work is necessary.
    */
   virtual void processMessages();

   /**
    * Handles an update request message.
    *
    * @param msg the message.
    */
   virtual void handleUpdateRequest(monarch::rt::DynamicObject& msg);

   /**
    * Handles an update request message.
    *
    * @param msg the message.
    */
   virtual void handleUpdateResponse(monarch::rt::DynamicObject& msg);

   /**
    * Handles a register response message.
    *
    * @param msg the message.
    */
   virtual void handleRegisterResponse(monarch::rt::DynamicObject& msg);

   /**
    * Handles a test net access message.
    *
    * @param msg the message.
    */
   virtual void handleTestNetAccess(monarch::rt::DynamicObject& msg);

   /**
    * Performs a net access test and retrieves the public server URL for this
    * catalog upon success.
    *
    * @param serverUrl the URL to populate.
    *
    * @return true if successful, false if not.
    */
   virtual bool testNetAccess(std::string& serverUrl);

   /**
    * Registers for a new server ID via bitmunk.
    */
   virtual void registerServerId();

   /**
    * Runs a net access test and updates a catalog's server URL if it has
    * changed.
    *
    * @param seller the seller information containing the old server URL.
    */
   virtual void updateServerUrl(bitmunk::common::Seller& seller);

   /**
    * Sends a seller listing update to bitmunk.
    *
    * @param update the update to send.
    */
   virtual void sendUpdate(bitmunk::common::SellerListingUpdate& update);
};

} // end namespace customcatalog
} // end namespace bitmunk
#endif

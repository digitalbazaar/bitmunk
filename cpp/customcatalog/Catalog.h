/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_customcatalog_Catalog_H
#define bitmunk_customcatalog_Catalog_H

#include "bitmunk/common/CatalogInterface.h"
#include "bitmunk/node/Node.h"

namespace bitmunk
{
namespace customcatalog
{

/**
 * A Catalog is an API for a catalog that contains custom Wares for a seller.
 *
 * @author Dave Longley
 * @author Manu Sporny
 */
class Catalog : public bitmunk::common::CatalogInterface
{
public:
   /**
    * Creates a new Catalog.
    */
   Catalog() {};

   /**
    * Destructs this Catalog.
    */
   virtual ~Catalog() {};

   /**
    * Initializes this Catalog, creating its database and tables, etc.
    *
    * @param node the associated Bitmunk node.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool init(bitmunk::node::Node* node) = 0;

   /**
    * Cleans up this Catalog.
    *
    * @param node the associated Bitmunk node.
    */
   virtual void cleanup(bitmunk::node::Node* node) = 0;

   /**
    * Adds or updates a Ware in the catalog with the passed Ware.
    *
    * @param userId the ID of the user the ware belongs to.
    * @param w the Ware to add/update in the catalog.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool updateWare(
      bitmunk::common::UserId userId,
      bitmunk::common::Ware& w) = 0;

   /**
    * Removes a Ware from the catalog, if it exists.
    *
    * @param userId the ID of the user the ware belongs to.
    * @param w the Ware to remove from the catalog.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool removeWare(
      bitmunk::common::UserId userId,
      bitmunk::common::Ware& w) = 0;

   /**
    * Populates a Ware based on its ID. Any extraneous information not
    * in the catalog will be removed from the ware.
    *
    * @param userId the ID of the user the ware belongs to.
    * @param w the Ware to populate.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool populateWare(
      bitmunk::common::UserId userId,
      bitmunk::common::Ware& w) = 0;

   /**
    * Populates a set of wares that match the given query parameters.
    *
    * @param userId the ID of the user the ware belongs to.
    * @param w the Ware to populate.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool populateWareSet(
      bitmunk::common::UserId userId, monarch::rt::DynamicObject& query,
      bitmunk::common::ResourceSet& wareSet) = 0;

   /**
    * Populates a Ware bundle based on its media ID and the given
    * MediaPreferences. Any extraneous information not in the catalog
    * will be removed from the ware.
    *
    * @param userId the ID of the user the ware belongs to.
    * @param w the Ware to populate, with media ID set.
    * @param prefs the MediaPreferences for the Ware.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool populateWareBundle(
      bitmunk::common::UserId userId,
      bitmunk::common::Ware& w,
      bitmunk::common::MediaPreferenceList& prefs) = 0;

   /**
    * Populates a FileInfo based on its ID.
    *
    * @param fi the FileInfo to populate.
    *
   * @return true if successful, false if an Exception occurred.
    */
   virtual bool populateFileInfo(
      bitmunk::common::UserId userId,
      bitmunk::common::FileInfo& fi) = 0;

   /**
    * Populates the FileInfos for a Ware.
    *
    * @param w the Ware to populate the FileInfos of.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool populateFileInfos(
      bitmunk::common::UserId userId,
      bitmunk::common::Ware& w) = 0;

   /**
    * Populates a FileInfoList.
    *
    * @param userId the ID of the user the FileInfos belong to.
    * @param fil the FileInfoList with FileInfos that have their IDs set.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool populateFileInfoList(
      bitmunk::common::UserId userId,
      bitmunk::common::FileInfoList& fil) = 0;

   /**
    * Adds or updates a payee scheme.
    *
    * @param userId the ID of the user the scheme belongs to.
    * @param id the ID of the payee scheme, 0 to create a new one and it will
    *           be set, on success, to the new payee scheme ID.
    * @param description the description to use when describing the PayeeScheme.
    * @param payees the array of payees.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool updatePayeeScheme(
      bitmunk::common::UserId userId,
      bitmunk::common::PayeeSchemeId& psId, const char* description,
      bitmunk::common::PayeeList& payees) = 0;

   /**
    * Populates a set of payee schemes
    *
    * @param userId the ID of the user the scheme belongs to.
    * @param filters the filters to use when populating the result set.
    * @param rs the resource set to populate.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool populatePayeeSchemes(
      bitmunk::common::UserId userId,
      bitmunk::common::ResourceSet& rs,
      monarch::rt::DynamicObject& filters) = 0;

   /**
    * Populates a payee scheme that has its "id" field set.
    *
    * @param userId the ID of the user the scheme belongs to.
    * @param ps the payee scheme to populate.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool populatePayeeScheme(
      bitmunk::common::UserId userId,
      bitmunk::common::PayeeScheme& ps) = 0;

   /**
    * Populates a payee scheme that has its "id" field set and uses a set
    * of filters.
    *
    * @param userId the ID of the user the scheme belongs to.
    * @param filters the filters to use.
    * @param ps the payee scheme to populate.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool populatePayeeScheme(
      bitmunk::common::UserId userId,
      monarch::rt::DynamicObject& filters,
      bitmunk::common::PayeeScheme& ps) = 0;

   /**
    * Removes a payee scheme.
    *
    * @param userId the ID of the user the scheme belongs to.
    * @param id the ID of the payee scheme.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool removePayeeScheme(
      bitmunk::common::UserId userId,
      bitmunk::common::PayeeSchemeId psId) = 0;

   /**
    * Updates the seller configuration and server token for the catalog owned
    * by the given seller.
    *
    * @param userId the ID of the user the seller belongs to.
    * @param seller the seller object to use when populating the seller
    *               configuration.
    * @param serverToken the seller's associated server token.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool updateSeller(
      bitmunk::common::UserId userId,
      bitmunk::common::Seller& seller,
      bitmunk::common::ServerToken serverToken) = 0;

   /**
    * Populates the seller associated with the given user ID.
    *
    * @param userId the ID of the user.
    * @param seller the seller object to populate.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool populateSeller(
      bitmunk::common::UserId userId,
      bitmunk::common::Seller& seller) = 0;

   /**
    * Gets the seller object and server token that are associated with the
    * catalog.
    *
    * @param userId the ID of the user the seller object belongs to.
    * @param seller the seller object to use when configuring the object.
    * @param serverToken a string to store the server token in.
    *
    * @return true if successful, false otherwise.
    */
   virtual bool populateSeller(
      bitmunk::common::UserId userId,
      bitmunk::common::Seller& seller,
      std::string& serverToken) = 0;

   /**
    * Gets the current update ID.
    *
    * @param userId the ID of the user to get the current update ID for.
    * @param updateId the update ID to populate.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool getUpdateId(
      bitmunk::common::UserId userId,
      bitmunk::common::PayeeSchemeId& updateId) = 0;

   /**
    * Gets a heartbeat update.
    *
    * @param userId the ID of the user to get the heartbeat update for.
    * @param update the update to populate.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool populateHeartbeatListingUpdate(
      bitmunk::common::UserId userId,
      bitmunk::common::SellerListingUpdate& update) = 0;

   /**
    * Populates a SellerListingUpdate object with a list of listings
    * whose updates are currently pending. That is, the updates have been
    * flagged as being in-process (either en-route to bitmunk, or sent, but
    * not received by bitmunk).
    *
    * @param userId the ID associated with the user whose pending updates
    *               are to be retrieved.
    * @param update the seller listing update object that should be populated.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool populatePendingListingUpdate(
      bitmunk::common::UserId userId,
      bitmunk::common::SellerListingUpdate& update) = 0;

   /**
    * Populates a SellerListingUpdate object with a list of listings
    * that need to be sent to bitmunk. That is, the updates have been
    * flagged as needing to be sent to bitmunk and have not yet been sent.
    *
    * @param userId the ID associated with the user whose needed listing updates
    *               are to be retrieved.
    * @param update the seller listing update object that should be populated.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool populateNextListingUpdate(
      bitmunk::common::UserId userId,
      bitmunk::common::SellerListingUpdate& update) = 0;

   /**
    * Processes a SellerListingUpdate response that was received from
    * bitmunk.
    *
    * @param userId the ID associated with the user the response is for.
    * @param update the seller listing update object that was sent.
    * @param result the seller listing update object that contains any failed
    *               updates.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool processListingUpdateResponse(
      bitmunk::common::UserId userId,
      bitmunk::common::SellerListingUpdate update,
      bitmunk::common::SellerListingUpdate result) = 0;

   /**
    * Resets the seller listing update counter to completely re-sync all
    * wares with bitmunk. This is called if a seller's server has expired or
    * if catalog corruption has been detected.
    *
    * @param userId the ID of the user who owns the catalog.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool resetListingUpdateCounter(bitmunk::common::UserId userId) = 0;

   /**
    * Sets the configuration value for the name of the given configuration
    * setting.
    *
    * @param userId the ID of the user.
    * @param name the name of the configuration setting.
    * @param value the value of the configuration setting as a string.
    *
    * @return true if retrieving the value was successful, false otherwise.
    */
   virtual bool setConfigValue(
      bitmunk::common::UserId userId, const char* name, const char* value) = 0;

   /**
    * Gets the configuration value for the name of the given configuration
    * setting.
    *
    * @param userId the ID of the user.
    * @param name the name of the configuration setting.
    * @param value the value of the configuration setting as a string.
    *
    * @return true if retrieving the value was successful, false otherwise.
    */
   virtual bool getConfigValue(
      bitmunk::common::UserId userId, const char* name, std::string& value) = 0;
};

} // end namespace customcatalog
} // end namespace bitmunk
#endif

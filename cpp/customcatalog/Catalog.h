/*
 * Copyright (c) 2007-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_customcatalog_CustomCatalog_H
#define bitmunk_customcatalog_CustomCatalog_H

#include "bitmunk/common/CatalogInterface.h"
#include "bitmunk/customcatalog/CatalogDatabase.h"
#include "bitmunk/medialibrary/IMediaLibrary.h"
#include "monarch/event/Observer.h"
#include "monarch/kernel/MicroKernelModuleApi.h"

namespace bitmunk
{
namespace customcatalog
{

// forward declarations
class ListingUpdater;

/**
 * A Catalog contains custom Wares for a seller.
 *
 * @author Dave Longley
 * @author Manu Sporny
 */
class Catalog :
public monarch::kernel::MicroKernelModuleApi,
public bitmunk::common::CatalogInterface,
public bitmunk::medialibrary::IMediaLibraryExtension
{
protected:
   /**
    * The associated Bitmunk node.
    */
   bitmunk::node::Node* mNode;

   /**
    * The media library interface.
    */
   bitmunk::medialibrary::IMediaLibrary* mMediaLibrary;

   /**
    * A map of user ID to ListingUpdater fiber ID.
    */
   typedef std::map<bitmunk::common::UserId, monarch::fiber::FiberId>
   ListingUpdaterMap;
   ListingUpdaterMap mListingUpdaterMap;

   /**
    * Observer for handling user login.
    */
   monarch::event::ObserverRef mUserLoggedInObserver;

   /**
    * Observer for handling auto-sell.
    */
   monarch::event::ObserverRef mAutoSellObserver;

   /**
    * Observer that functions as a daemon for sending seller listing updates
    * to bitmunk.
    */
   monarch::event::ObserverRef mListingSyncDaemon;

   /**
    * Observer that functions as a daemon for running test access tests.
    */
   monarch::event::ObserverRef mNetAccessTestDaemon;

   /**
    * The catalog database that is used to modify the database entries for
    * the media library and custom catalog.
    */
   bitmunk::customcatalog::CatalogDatabase mCatalogDb;

public:
   /**
    * Creates a new Catalog.
    */
   Catalog();

   /**
    * Destructs this Catalog.
    */
   virtual ~Catalog();

   /**
    * Initializes this Catalog, creating its database and tables, etc.
    *
    * @param node the associated Bitmunk node.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool init(bitmunk::node::Node* node);

   /**
    * Cleans up this Catalog.
    *
    * @param node the associated Bitmunk node.
    */
   virtual void cleanup(bitmunk::node::Node* node);

   /**
    * Called when the media library has been initialized. This call may be
    * used to perform any initialization that is required by the media
    * library extension.
    *
    * This will be called when the first connection is requested to a user's
    * media library. It will be called for each different user.
    *
    * @param userId the ID of the user of the media library.
    * @param conn a connection to the media library to use to perform
    *             any custom initialization, *MUST NOT* be closed before
    *             returning.
    * @param dbc a DatabaseClient to use -- but it can only use the provided
    *            connection.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool mediaLibraryInitialized(
      bitmunk::common::UserId userId, monarch::sql::Connection* conn,
      monarch::sql::DatabaseClientRef& dbc);

   /**
    * Called when the media library is being cleaned up. This call may be
    * used to clean up any locally cached information that is required by
    * the media library extension.
    *
    * This will be called when a user logs out and their media library becomes
    * inaccessible. It will be called for each different user.
    *
    * @param userId the ID of the user of the media library.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual void mediaLibraryCleaningUp(bitmunk::common::UserId userId);

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
      bitmunk::common::Ware& w);

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
      bitmunk::common::Ware& w);

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
      bitmunk::common::Ware& w);

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
      bitmunk::common::ResourceSet& wareSet);

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
      bitmunk::common::MediaPreferenceList& prefs);

   /**
    * Populates a FileInfo based on its ID.
    *
    * @param fi the FileInfo to populate.
    *
   * @return true if successful, false if an Exception occurred.
    */
   virtual bool populateFileInfo(
      bitmunk::common::UserId userId,
      bitmunk::common::FileInfo& fi);

   /**
    * Populates the FileInfos for a Ware.
    *
    * @param w the Ware to populate the FileInfos of.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool populateFileInfos(
      bitmunk::common::UserId userId,
      bitmunk::common::Ware& w);

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
      bitmunk::common::FileInfoList& fil);

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
      bitmunk::common::PayeeList& payees);

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
      monarch::rt::DynamicObject& filters);

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
      bitmunk::common::PayeeScheme& ps);

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
      bitmunk::common::PayeeScheme& ps);

   /**
    * Removes a payee scheme.
    *
    * @param userId the ID of the user the scheme belongs to.
    * @param id the ID of the payee scheme.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool removePayeeScheme(
      bitmunk::common::UserId userId, bitmunk::common::PayeeSchemeId psId);

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
      bitmunk::common::ServerToken serverToken);

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
      bitmunk::common::Seller& seller);

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
      std::string& serverToken);

   /**
    * Gets the current update ID.
    *
    * @param userId the ID of the user to get the current update ID for.
    * @param updateId the update ID to populate.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool getUpdateId(
      bitmunk::common::UserId userId, bitmunk::common::PayeeSchemeId& updateId);

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
      bitmunk::common::SellerListingUpdate& update);

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
      bitmunk::common::SellerListingUpdate& update);

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
      bitmunk::common::SellerListingUpdate& update);

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
      bitmunk::common::SellerListingUpdate result);

   /**
    * Resets the seller listing update counter to completely re-sync all
    * wares with bitmunk. This is called if a seller's server has expired or
    * if catalog corruption has been detected.
    *
    * @param userId the ID of the user who owns the catalog.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool resetListingUpdateCounter(bitmunk::common::UserId userId);

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
      bitmunk::common::UserId userId, const char* name, const char* value);

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
      bitmunk::common::UserId userId, const char* name, std::string& value);

   /**
    * Called when a user logs in.
    *
    * @param e the event that occurred.
    */
   virtual void userLoggedIn(monarch::event::Event& e);

   /**
    * Called when a file is updated in the media library due to a purchase
    * and the auto-sell feature must run a check.
    *
    * @param e the event that occurred.
    */
   virtual void checkAutoSell(monarch::event::Event& e);

   /**
    * Called when it is time to send a seller listing update to bitmunk.
    *
    * @param e the event that occurred.
    */
   virtual void syncSellerListings(monarch::event::Event& e);

   /**
    * Called when it is time for the listing updating to ensure that the
    * user's catalog is accessible over the internet.
    *
    * @param e the event that occurred.
    */
   virtual void testNetAccess(monarch::event::Event& e);
};

} // end namespace customcatalog
} // end namespace bitmunk
#endif

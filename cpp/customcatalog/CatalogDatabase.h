/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_customcatalog_CatalogDatabase_H
#define bitmunk_customcatalog_CatalogDatabase_H

#include "bitmunk/common/TypeDefinitions.h"
#include "bitmunk/medialibrary/IMediaLibrary.h"
#include "bitmunk/node/Node.h"
#include "monarch/sql/Connection.h"

#include <string>

// These table names are used by the Catalog code
#define CC_TABLE_WARES          "bitmunk_customcatalog_wares"
#define CC_TABLE_PAYEE_SCHEMES  "bitmunk_customcatalog_payee_schemes"

namespace bitmunk
{
namespace customcatalog
{
/**
 * The Catalog Database class provides utility functions for performing
 * low-level operations on the custom catalog database.
 *
 * @author Manu Sporny
 * @author Dave Longley
 */
class CatalogDatabase
{
public:
   /**
    * Creates a new CatalogDatabase object.
    */
   CatalogDatabase();

   /**
    * Standard destructor for a CatalogDatabase object.
    */
   ~CatalogDatabase();

   /**
    * Initializes the database tables if they haven't already been initialized.
    *
    * @param c the database connection to use when initializing the database.
    *
    * @return true if successful, false otherwise.
    */
   virtual bool initialize(monarch::sql::Connection* c);

   /**
    * Sets the configuration value for the name of the given configuration
    * setting.
    *
    * @param userId the ID of the user.
    * @param name the name of the configuration setting.
    * @param value the value of the configuration setting as a string.
    * @param c a database connection.
    *
    * @return true if retrieving the value was successful, false otherwise.
    */
   virtual bool setConfigValue(
      bitmunk::common::UserId userId, const char* name,
      const char* value, monarch::sql::Connection* c);

   /**
    * Gets the configuration value for the name of the given configuration
    * setting.
    *
    * @param name the name of the configuration setting.
    * @param value the value of the configuration setting as a string.
    * @param c a database connection.
    *
    * @return true if retrieving the value was successful, false otherwise.
    */
   virtual bool getConfigValue(
      const char* name, std::string& value, monarch::sql::Connection* c);

   /**
    * Gets the configuration value for the name of the given configuration
    * setting.
    *
    * @param name the name of the configuration setting.
    * @param value the value of the configuration setting.
    * @param c a database connection.
    *
    * @return true if retrieving the value was successful, false otherwise.
    */
   virtual bool getConfigValue(
      const char* name, uint32_t& value, monarch::sql::Connection* c);

   /**
    * Sets flags for all entries in the given table to the values specified.
    *
    * @param tableName the name of the table to modify.
    * @param dirty true if the dirty flag should be set for all entries in
    *              the table, false otherwise.
    * @param updating true if the updating flag should be set for all entries
    *                 in the table, false otherwise.
    * @param c the connection to use when modifying the database.
    *
    * @return true if retrieving the value was successful, false if there was
    *         an Exception.
    */
   virtual bool setTableFlags(
      const char* tableName, bool dirty, bool updating,
      monarch::sql::Connection* c);

   /**
    * Purges entries marked as deleted in the given table.
    *
    * @param tableName the name of the table to modify.
    * @param c the connection to use when modifying the database.
    *
    * @return true if the purge was successful, false if there was
    *         an Exception.
    */
   virtual bool purgeDeletedEntries(
      const char* tableName, monarch::sql::Connection* c);

   /**
    * Clears the updating flags in the given table.
    *
    * @param tableName the name of the table to modify.
    * @param c the connection to use when modifying the database.
    *
    * @return true if the update was successful, false if there was
    *         an Exception.
    */
   virtual bool clearUpdatingFlags(
      const char* tableName, monarch::sql::Connection* c);

   /**
    * Helper function to populate a seller object and related server token.
    *
    * @param seller the seller object to populate.
    * @param serverToken the server token to populate.
    * @param c the database connection to use when populating the seller.
    *
    * @return true if successful, false if there was an Exception.
    */
   virtual bool populateSeller(
      bitmunk::common::UserId userId, bitmunk::common::Seller& seller,
      std::string& serverToken, monarch::sql::Connection* c);

   /**
    * Checks to ensure that a payee scheme is valid.
    *
    * @param psId The payee scheme ID to use when checking the database.
    * @param c A database connection.
    *
    * @return true if the payee scheme exists, false otherwise.
    */
   virtual bool isPayeeSchemeIdValid(
      bitmunk::common::PayeeSchemeId psId, monarch::sql::Connection* c);

   /**
    * Populates a given set of payee scheme updates and removals with the
    * current set of payee schemes that should be updated and removed.
    *
    * @param userId the ID of the user that owns the schemes.
    * @param serverId the ID of the server the schemes are associated with.
    * @param payeeSchemes the object that contains "updates" and "removals"
    *                     for payee schemes.
    * @param c the connection to use.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool populateUpdatingPayeeSchemes(
      bitmunk::common::UserId userId, bitmunk::common::ServerId serverId,
      bitmunk::common::PayeeScheme& payeeSchemes, monarch::sql::Connection* c);

   /**
    * A helper function to allocate a new payee scheme if one doesn't already
    * exist. This approach will always re-use PayeeSchemeIds that have been
    * returned to the list of available payee schemes.
    *
    * @param psId when the function exists, the payee scheme ID that was allocated
    *             or 0 if no payee scheme was allocated.
    * @param description the description associated with the payee scheme.
    * @param c The database connection to use when selecting and inserting a
    *          PayeeSchemeId.
    *
    * @return true if successful, false if there was an Exception.
    */
   virtual bool allocateNewPayeeScheme(
      bitmunk::common::PayeeSchemeId& psId, const char* description,
      monarch::sql::Connection* c);

   /**
    * Adds a payee scheme to the database.
    *
    * @param node the node associated with the give userId.
    * @param userId the user ID associated with the payee scheme.
    * @param psId the payee scheme ID to add to the database.
    * @param description a short description about the payee scheme in the
    *                    database.
    * @param psList the payee scheme list to populate.
    * @param c the connection to use when modifying the database.
    *
    * @return true if successful, false if there was an Exception.
    */
   virtual bool addPayeeScheme(
      bitmunk::node::Node* node, bitmunk::common::UserId userId,
      bitmunk::common::PayeeSchemeId& psId, const char* description,
      bitmunk::common::PayeeList& psList, monarch::sql::Connection* c);

   /**
    * Updates an existing payee scheme.
    *
    * @param ps the payee scheme to update.
    * @param c the connection to use.
    *
    * @return true if successful, false if there was an Exception.
    */
   virtual bool updatePayeeScheme(
      bitmunk::common::PayeeScheme& ps, monarch::sql::Connection* c);

   /**
    * Gets the first payee scheme ID for a user.
    *
    * @param psId the payee scheme ID to update, set to 0 if the user has
    *           no payee schemes.
    * @param c the connection to use.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool getFirstPayeeSchemeId(
      bitmunk::common::PayeeSchemeId& psId,
      monarch::sql::Connection* c);

   /**
    * Populates a payee scheme that has its "id" field set.
    *
    * @param payeeScheme the payee scheme object to populate.
    * @param filters the filters to use when populating the scheme.
    * @param c the connection to use.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool populatePayeeScheme(
      bitmunk::common::PayeeScheme& payeeScheme,
      monarch::rt::DynamicObject& filters, monarch::sql::Connection* c);

   /**
    * Populates a resource set containing payee schemes given a set of filters.
    *
    * @param userId the ID of the user the scheme belongs to.
    * @param filters the filters to use when populating the resource set.
    * @param rs the resource set to populate.
    * @param c the connection to use.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool populatePayeeSchemes(
      bitmunk::common::ResourceSet& rs,
      monarch::rt::DynamicObject& filters,
      monarch::sql::Connection* c);

   /**
    * Deletes a payee scheme from the database given a PayeeSchemeId.
    *
    * @param psId the ID associated with the payee scheme.
    * @param c the connection to use when modifying the database.
    *
    * @return true if successful, false if there was an Exception.
    */
   virtual bool deletePayeeScheme(
      bitmunk::common::PayeeSchemeId psId, monarch::sql::Connection* c);

   /**
    * Ensures that a payee scheme is not in use by a ware, and if it is,
    * sets an exception.
    *
    * @param psId the PayeeSchemeId associated with the PayeeScheme to check.
    * @param c the connection to use when modifying the database.
    *
    * @return true if successful, false if there was an Exception.
    */
   virtual bool ensurePayeeSchemeNotInUse(
      bitmunk::common::PayeeSchemeId psId, monarch::sql::Connection* c);

   /**
    * Marks a payee scheme for removal from the database. Note that this is
    * different from deleting the payee scheme, which deletes the entry from
    * the database.
    *
    * @param psId the ID associated with the payee scheme.
    * @param c the connection to use when modifying the database.
    *
    * @return true if successful, false if there was an Exception.
    */
   virtual bool removePayeeScheme(
      bitmunk::common::PayeeSchemeId psId, monarch::sql::Connection* c);

   /**
    * Updates the problem ID associated with a particular payee scheme.
    *
    * @param problemId the problem ID to associate with the given payee scheme.
    * @param psId the PayeeSchemeId associated with the given problem ID.
    * @param c the connection to use when retrieving the information from the
    *          database.
    *
    * @return true if successful, false if there was an Exception.
    */
   virtual bool updatePayeeSchemeProblemId(
      uint64_t problemId, bitmunk::common::PayeeSchemeId psId,
      monarch::sql::Connection* c);

   /**
    * Populates a ware given the UserId and a ware to populate.
    *
    * @param userId the UserId associated with the ware.
    * @param ware the ware to populate. The ware must have its "id" field
    *             specified.
    * @param mediaLibrary the medialibrary interface that contains the file
    *                     info objects from which to retrieve.
    * @param c the connection to use when retrieving the information.
    *
    * @return true if successful, false if there was an Exception.
    */
   virtual bool populateWare(
      bitmunk::common::UserId userId, bitmunk::common::Ware& ware,
      bitmunk::medialibrary::IMediaLibrary* mediaLibrary,
      monarch::sql::Connection* c);

   /**
    * Populates a list of wares given the UserId and a ware ids to populate.
    *
    * @param userId the UserId associated with the ware.
    * @param query the query parameters to use when retrieving the set of files.
    * @param wareSet the set of wares that were retrieved based on the query
    *                parameters.
    * @param mediaLibrary the medialibrary interface that contains the file
    *                     info objects from which to retrieve.
    * @param c the connection to use when retrieving the information.
    *
    * @return true if successful, false if there was an Exception.
    */
   virtual bool populateWareSet(
      bitmunk::common::UserId userId, monarch::rt::DynamicObject& query,
      bitmunk::common::ResourceSet& wareSet,
      bitmunk::medialibrary::IMediaLibrary* mediaLibrary,
      monarch::sql::Connection* c);

   /**
    * Populates the FileInfo that is associated with a given ware.
    *
    * @param userId the user ID that is associated with the ware.
    * @param ware the ware that should be populated. The incoming ware must
    *             have its "id" field specified.
    * @param mediaLibrary the medialibrary interface that contains the file
    *                     info objects from which to retrieve.
    * @param c the connection to use when modifying the database.
    *
    * @return true if successful, false if there was an Exception.
    */
   virtual bool populateWareFileInfo(
      bitmunk::common::UserId userId, bitmunk::common::Ware& ware,
      bitmunk::medialibrary::IMediaLibrary* mediaLibrary,
      monarch::sql::Connection* c);

   /**
    * Marks a ware for deletion, but does not delete it from the database.
    *
    * @param ware the ware to mark for deletion.
    * @param c the connection to use when modifying the database.
    *
    * @return true if successful, false if there was an Exception.
    */
   virtual bool removeWare(
      bitmunk::common::Ware& ware, monarch::sql::Connection* c);

   /**
    * Inserts or updates a ware in the ware table.
    *
    * @param ware the ware to insert/update in the ware table.
    * @param psId the payee scheme ID to associate with the ware.
    * @param c the database connection to use when modifying the database.
    * @param added set to true if the ware was added, false if it was updated.
    *
    * @return true if successful, false if there was an Exception.
    */
   virtual bool updateWare(
      bitmunk::common::Ware& ware, bitmunk::common::PayeeSchemeId psId,
      monarch::sql::Connection* c, bool* added);

   /**
    * Populates the given wares object with the set of wares that should
    * be updated in a future listings call.
    *
    * @param wares the object that contains the updates and
    *              removals of wares.
    * @param c the connection to use when modifying the database.
    *
    * @return true if successful, false if there was an Exception.
    */
   virtual bool populateUpdatingWares(
      bitmunk::common::UserId,
      bitmunk::medialibrary::IMediaLibrary* mediaLibrary,
      monarch::rt::DynamicObject& wares, monarch::sql::Connection* c);

   /**
    * Updates a ware problem ID with the given information.
    *
    * @param problemId The problem identifier.
    * @param wareId The identifier for the ware.
    * @param mediaId The identifier for the media associated with the ware.
    * @param c the connection to use when modifying the database.
    *
    * @return true if successful, false if there was an Exception.
    */
   virtual bool updateWareProblemId(
      uint64_t problemId, bitmunk::common::FileId fileId,
      bitmunk::common::MediaId mediaId, monarch::sql::Connection* c);

   /**
    * Populates the listings and payee schemes that should be updated or
    * removed from the remote catalog listing service.
    *
    * @param userId the user ID associated with the wares and payee schemes.
    * @param serverId the ID of the server the wares/schemes are on.
    * @param mediaLibrary the medialibrary interface associated with the given
    *                     userId.
    * @param wares the object that should be populated with all of
    *              the wares that should be updated or removed.
    * @param payeeSchemes the payee schemes object that will be updated with
    *                     all of the payee schemes that should be updated or
    *                     removed.
    * @param c the connection to use when modifying the database.
    *
    * @return true if successful, false if there was an Exception.
    */
   virtual bool populateUpdatingSellerListings(
      bitmunk::common::UserId userId,
      bitmunk::common::ServerId serverId,
      bitmunk::medialibrary::IMediaLibrary* mediaLibrary,
      monarch::rt::DynamicObject& wares,
      monarch::rt::DynamicObject& payeeSchemes,
      monarch::sql::Connection* c);

   /**
    * Marks the next set of wares and payee schemes that should be updated
    * or removed from the remote catalog listing service. After calling this
    * method, you can call the populateUpdatingSellerListings method to get
    * the next set of updates and removals.
    *
    * @param c the connection to use when modifying the database.
    *
    * @return true if successful, false if there was an Exception.
    */
   virtual bool markNextListingUpdate(monarch::sql::Connection* c);

   /**
    * Gets the problem ID associated with the given text. If the text does
    * not yet have a problem ID associated with it, it will be assigned one.
    *
    * @param text the json text for the problem.
    * @param problemId the problem ID to be set.
    * @param c the database connection to use.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool getProblemId(
      const char* text, uint64_t& problemId, monarch::sql::Connection* c);

   /**
    * Removes all problems that are not referenced by a ware or payee
    * scheme from the database.
    *
    * @param c the database connection to use.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool removeUnreferencedProblems(monarch::sql::Connection* c);
};

} // end namespace customcatalog
} // end namespace bitmunk
#endif

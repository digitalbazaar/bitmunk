/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_purchase_PurchaseDatabase_H
#define bitmunk_purchase_PurchaseDatabase_H

#include "bitmunk/peruserdb/IPerUserDBModule.h"
#include "bitmunk/peruserdb/PerUserDatabase.h"
#include "bitmunk/purchase/TypeDefinitions.h"
#include "bitmunk/node/Node.h"
#include "monarch/config/ConfigManager.h"

namespace bitmunk
{
namespace purchase
{

/**
 * The PurchaseDatabase stores the data a purchase module needs to accomplish
 * its tasks.
 *
 * @author Dave Longley
 */
class PurchaseDatabase : public bitmunk::peruserdb::PerUserDatabase
{
protected:
   /**
    * Node using this database.
    */
   bitmunk::node::Node* mNode;

   /**
    * A reference to the per-user database module's interface.
    */
   bitmunk::peruserdb::IPerUserDBModule* mPerUserDB;

   /**
    * The ID for a connection to the purchase database.
    */
   bitmunk::peruserdb::ConnectionGroupId mConnectionGroupId;

public:
   /**
    * Creates a new PurchaseDatabase.
    */
   PurchaseDatabase();

   /**
    * Destructs this PurchaseDatabase.
    */
   virtual ~PurchaseDatabase();

   /**
    * Gets the passed node's purchase module's purchase database.
    *
    * @param node the node to get the purchase database for.
    */
   static PurchaseDatabase* getInstance(bitmunk::node::Node* node);

   /**
    * Initializes this database for use.
    *
    * @param node the Node for this database.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool initialize(bitmunk::node::Node* node);

   /**
    * Gets the url and and maximum number of concurrent connections a
    * user can have to this database.
    *
    * This will be called when the first connection is requested for a
    * particular connection group.
    *
    * @param id the ID of the connection group.
    * @param userId the ID of the user.
    * @param url the url string to populate.
    * @param maxCount the maximum concurrent connection count to populate.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool getConnectionParams(
      bitmunk::peruserdb::ConnectionGroupId id, bitmunk::common::UserId userId,
      std::string& url, uint32_t& maxCount);

   /**
    * Initializes a database for a user.
    *
    * This will be called when the first connection is requested for a
    * particular connection group.
    *
    * @param id the ID of the connection group.
    * @param userId the ID of the user.
    * @param conn a connection to use to initialize the database, *must*
    *             be closed before returning.
    * @param dbc a DatabaseClient to use -- but only the given connection must
    *            be used with it.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool initializePerUserDatabase(
      bitmunk::peruserdb::ConnectionGroupId id, bitmunk::common::UserId userId,
      monarch::sql::Connection* conn, monarch::sql::DatabaseClientRef& dbc);

   /**
    * Stores a new DownloadState in the database and assigns it an ID.
    *
    * @param ds the new DownloadState to store.
    * @param conn the connection to use, NULL to open and close one.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool insertDownloadState(
      DownloadState& ds, monarch::sql::Connection* conn = NULL);

   /**
    * Deletes the given download state from the database, based on the ID in
    * the download state object.
    *
    * @param ds the DownloadState to delete, the ID must be set.
    * @param conn the connection to use, NULL to open and close one.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool deleteDownloadState(
      DownloadState& ds, monarch::sql::Connection* conn = NULL);

   /**
    * Marks a download state as being processed by a processor with the
    * given ID. This method will fail if the download state is already
    * being processed.
    *
    * @param ds the DownloadState to update.
    * @param processorId the ID of the processor.
    * @param conn the connection to use, NULL to open and close one.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool startProcessingDownloadState(
      DownloadState& ds, uint32_t processorId,
      monarch::sql::Connection* conn = NULL);

   /**
    * Changes a download state's processor ID from one to another. This method
    * will fail if the old processor ID is invalid.
    *
    * @param ds the DownloadState to update.
    * @param oldId the ID of the old processor.
    * @param newId the ID of the new processor.
    * @param conn the connection to use, NULL to open and close one.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool setDownloadStateProcessorId(
      DownloadState& ds, uint32_t oldId, uint32_t newId,
      monarch::sql::Connection* conn = NULL);

   /**
    * Marks a download state as no longer being processed if its processor
    * ID matches the passed ID, otherwise it is left alone (with no exception).
    *
    * @param ds the DownloadState to update.
    * @param processorId the ID of the processor that is finished.
    * @param conn the connection to use, NULL to open and close one.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool stopProcessingDownloadState(
      DownloadState& ds, uint32_t processorId,
      monarch::sql::Connection* conn = NULL);

   /**
    * Populates the processing flag and processor ID for a download state.
    *
    * @param ds the DownloadState to update.
    * @param conn the connection to use, NULL to open and close one.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool populateProcessingInfo(
      DownloadState& ds, monarch::sql::Connection* conn = NULL);

   /**
    * Clears all download states as not being processed.
    *
    * @param userId the ID of the user the DownloadStates belong to.
    * @param conn the connection to use, NULL to open and close one.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool clearProcessingDownloadStates(
      bitmunk::common::UserId userId, monarch::sql::Connection* conn = NULL);

   /**
    * Updates the preferences for an existing DownloadState in the database.
    *
    * @param ds the DownloadState to update.
    * @param conn the connection to use, NULL to open and close one.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool updatePreferences(
      DownloadState& ds, monarch::sql::Connection* conn = NULL);

   /**
    * Inserts the seller pools for a new DownloadState in the database. This
    * call is made only to initialize seller pools from a ware with its
    * file infos set in the download state.
    *
    * @param ds the new DownloadState with seller pools.
    * @param conn the connection to use, NULL to open and close one.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool insertSellerPools(
      DownloadState& ds, monarch::sql::Connection* conn = NULL);

   /**
    * Updates the seller pool for an existing DownloadState in the database.
    *
    * @param ds the DownloadState to update.
    * @param sp the SellerPool to update.
    * @param conn the connection to use, NULL to open and close one.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool updateSellerPool(
      DownloadState& ds, bitmunk::common::SellerPool& sp,
      monarch::sql::Connection* conn = NULL);

   /**
    * Populates the seller pools for an existing DownloadState.
    *
    * @param ds the DownloadState to populate.
    * @param conn the connection to use, NULL to open and close one.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool populateSellerPools(
      DownloadState& ds, monarch::sql::Connection* conn = NULL);

   /**
    * Updates the flags for an existing DownloadState in the database.
    *
    * @param ds the DownloadState to update.
    * @param conn the connection to use, NULL to open and close one.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool updateDownloadStateFlags(
      DownloadState& ds, monarch::sql::Connection* conn = NULL);

   /**
    * Inserts the contract for an existing DownloadState in the database.
    *
    * @param ds the DownloadState to update.
    * @param conn the connection to use, NULL to open and close one.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool insertContract(
      DownloadState& ds, monarch::sql::Connection* conn = NULL);

   /**
    * Updates the contract for an existing DownloadState in the database.
    *
    * @param ds the DownloadState to update.
    * @param conn the connection to use, NULL to open and close one.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool updateContract(
      DownloadState& ds, monarch::sql::Connection* conn = NULL);

   /**
    * Populates a DownloadState contract.
    *
    * @param ds the DownloadState to populate that has ds["id"] and
    *           ds["userId"] set.
    * @param conn the connection to use, NULL to open and close one.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool populateContract(
      DownloadState& ds, monarch::sql::Connection* conn = NULL);

   /**
    * Updates multiple file piece entries and the number of remaining
    * pieces in the download state.
    *
    * @param ds the DownloadState to update.
    * @param entries a map of database entries with update info:
    *                "fileId" => FileId,
    *                "csHash" => ContractSection hash,
    *                "status" => "unassigned","assigned","downloaded"
    *                "piece" => FilePiece
    * @param conn the connection to use, NULL to open and close one.
    */
   virtual bool updateFileProgress(
      DownloadState& ds, monarch::rt::DynamicObject& entries,
      monarch::sql::Connection* conn = NULL);

   /**
    * Populates each DownloadStates' file progresses.
    *
    * @param ds the download state to populate with "userId" and "id" set.
    * @param conn the connection to use, NULL to open and close one.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool populateFileProgress(
      DownloadState& ds, monarch::sql::Connection* conn = NULL);

   /**
    * Populates each DownloadStates' file path if assembled.
    *
    * @param ds the download state to populate with "userId" and "id" set.
    * @param conn the connection to use, NULL to open and close one.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool populateFilePaths(
      DownloadState& ds, monarch::sql::Connection* conn = NULL);

   /**
    * Inserts a SellerData for an existing DownloadState in the database.
    *
    * @param ds the DownloadState to update.
    * @param fileId the file ID associated with the seller data.
    * @param sd the SellerData to insert.
    * @param conn the connection to use, NULL to open and close one.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool insertSellerData(
      DownloadState& ds, bitmunk::common::FileId fileId,
      bitmunk::common::SellerData& sd,
      monarch::sql::Connection* conn = NULL);

   /**
    * Populates DownloadState seller data.
    *
    * @param ds the DownloadState to populate that has ds["id"] and
    *           ds["userId"] set.
    * @param conn the connection to use, NULL to open and close one.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool populateSellerData(
      DownloadState& ds, monarch::sql::Connection* conn = NULL);

   /**
    * Gets a specific DownloadState according to its IDs. Its contract,
    * seller data, and file progress will all be populated.
    *
    * @param ds the DownloadState to populate with its "id" and "userId" set.
    * @param conn the connection to use, NULL to open and close one.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool populateDownloadState(
      DownloadState& ds, monarch::sql::Connection* conn = NULL);

   /**
    * Gets specific DownloadStates according to their IDs. Their contracts,
    * seller data, and file progress will all be populated. Every download
    * state must have the same user ID.
    *
    * @param userId the ID of the user that owns the DownloadStates.
    * @param dsList the DownloadStateList with DownloadStates to populate that
    *               have ds["id"] and ds["userId"] set.
    * @param conn the connection to use, NULL to open and close one.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool populateDownloadStates(
      bitmunk::common::UserId userId, DownloadStateList& dsList,
      monarch::sql::Connection* conn = NULL);

   /**
    * Gets all DownloadStates for a user that haven't finished downloading yet.
    *
    * @param userId the ID of the user that owns the DownloadStates.
    * @param dsList the list of DownloadStates to populate.
    * @param conn the connection to use, NULL to open and close one.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool getIncompleteDownloadStates(
      bitmunk::common::UserId userId, DownloadStateList& dsList,
      monarch::sql::Connection* conn = NULL);

   /**
    * Gets all DownloadStates for a user that haven't been purchased yet.
    *
    * @param userId the ID of the user that owns the DownloadStates.
    * @param dsList the list of DownloadStates to populate.
    * @param conn the connection to use, NULL to open and close one.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool getUnpurchasedDownloadStates(
      bitmunk::common::UserId userId, DownloadStateList& dsList,
      monarch::sql::Connection* conn = NULL);

   /**
    * Inserts details on an assembed file in the database.
    *
    * FIXME: This will move to the media library in a future release.
    *
    * @param ds the DownloadState to update.
    * @param fileId the file ID associated with the seller data.
    * @param path the path of the assembled file.
    * @param conn the connection to use, NULL to open and close one.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool insertAssembledFile(
      DownloadState& ds, bitmunk::common::FileId fileId,
      const char* path, monarch::sql::Connection* conn = NULL);

protected:
   /**
    * Gets a connection from the connection pool for a specific userId. It
    * must be closed (but not deleted) when the caller is finished with it.
    *
    * @param userId the ID of the user that owns the DownloadStates.
    *
    * @return a connection or NULL if an exception occurred.
    */
   virtual monarch::sql::Connection* getConnection(bitmunk::common::UserId userId);
};

} // end namespace purchase
} // end namespace bitmunk
#endif

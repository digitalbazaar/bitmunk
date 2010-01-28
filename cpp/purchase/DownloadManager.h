/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc.  All rights reserved.
 */
#ifndef bitmunk_purchase_DownloadManager_H
#define bitmunk_purchase_DownloadManager_H

#include "bitmunk/purchase/DownloadStateFiber.h"
#include "bitmunk/purchase/FileCostCalculator.h"
#include "bitmunk/node/NodeFiber.h"
#include "monarch/event/ObserverList.h"

namespace bitmunk
{
namespace purchase
{

/**
 * The DownloadManager fiber is in charge of managing the entire download
 * process.
 *
 * This class will first ensure that its associated download state has a
 * valid ID and signed media license. Then it will iteratively check for
 * progress updates to piece downloads, negotiating with new sellers,
 * assigning pieces, and starting piece downloaders appropriately.
 *
 * @author Dave Longley
 */
class DownloadManager :
public bitmunk::node::NodeFiber,
public DownloadStateFiber
{
protected:
   /**
    * A FileCostCalculator used to calculate budgets for files.
    */
   FileCostCalculator mFileCostCalculator;

   /**
    * Set to true while the seller pools is being re-initialized.
    */
   bool mSellerPoolsInitializing;

   /**
    * Set to true while a seller is being negotiated with.
    */
   bool mNegotiating;

   /**
    * Set to true while seller pools are being updated.
    */
   bool mSellerPoolsUpdating;

   /**
    * Set to true if this download manager should try to assign an optional
    * piece.
    */
   bool mAssignOptionalPiece;

   /**
    * Set to true if this download manager has been interrupted.
    */
   bool mInterrupted;

   /**
    * The last assigned piece downloader ID.
    */
   uint32_t mPieceDownloaderId;

   /**
    * A map of unique piece downloader IDs to piece downloader info.
    */
   struct PieceDownloaderEntry
   {
      monarch::fiber::FiberId fiberId;
      std::string fileId;
      uint32_t pieceIndex;
      monarch::util::RateAverager* rateAverager;
   };
   typedef std::map<uint32_t, PieceDownloaderEntry> PieceDownloaderMap;
   PieceDownloaderMap mPieceDownloaders;

   /**
    * A RateAverager for just the related contract data.
    */
   monarch::util::RateAverager mDownloadRate;

public:
   /**
    * Creates a new DownloadManager.
    *
    * @param node the Node associated with this fiber.
    */
   DownloadManager(bitmunk::node::Node* node);

   /**
    * Destructs this DownloadManager.
    */
   virtual ~DownloadManager();

   /**
    * Handles a check completion event.
    *
    * @param e the event.
    */
   virtual void handleCheckCompletionEvent(monarch::event::Event& e);

   /**
    * Handles a seller pool timeout event.
    *
    * @param e the event.
    */
   virtual void handleSellerPoolTimeoutEvent(monarch::event::Event& e);

   /**
    * Handles a progress poll event.
    *
    * @param e the event.
    */
   virtual void handleProgressPollEvent(monarch::event::Event& e);

protected:
   /**
    * Processes messages, retrieved via getMessages(), and performs whatever
    * custom work is necessary.
    */
   virtual void processMessages();

   /**
    * Checks the download state to ensure it has been initialized.
    *
    * @return true if successful, false if not.
    */
   virtual bool checkInit();

   /**
    * Ensures the media license in the download state is signed and valid.
    *
    * @return true if successful, false if not.
    */
   virtual bool checkLicense();

   /**
    * Ensures that any previously assigned pieces that did not complete
    * downloading are unassigned so that they can be reassigned.
    *
    * @return true if successful, false if not.
    */
   virtual bool unassignPieces();

   /**
    * Registers all necessary event handlers and adds them to the passed
    * list for later unregistration.
    *
    * @param list the ObserverList to populate.
    */
   virtual void registerEventHandlers(monarch::event::ObserverList* list);

   /**
    * Handles any incoming messages until this DownloadManager is ready
    * to quit.
    *
    * @return true if no error, false if an error occurred.
    */
   virtual bool handleMessages();

   /**
    * Checks to see if the files are all complete, if not, sending off
    * appropriate message to obtain more sellers as necessary. True will be
    * passed to this function during an initialization step (prior to
    * message processing) to ensure that pieces that were assigned but
    * not completed will be unassigned so they can be reassigned later.
    *
    * @param unassign true to unassign assigned pieces, false not to.
    *
    * @return true if the files are all complete, false if not.
    */
   virtual bool isComplete();

   /**
    * Assigns a piece to be downloaded if appropriate. This method will
    * determine if a piece should be assigned, and if so, it will pick a
    * seller to assign it to. It may start negotiating with a particular
    * seller, in which case it will set mNegotiating to true and return, and
    * should not be called again until mNegotiating is set to false.
    *
    * @return true if successful or currently negotiating with a seller to
    *         assign a piece (check mNegotiating), false if error.
    */
   virtual bool tryAssignPiece();

   /**
    * Returns true if another piece should be assigned.
    *
    * @param must set to true if a piece *must* be assigned to continue
    *             the download.
    *
    * @return true if another piece should be assigned, false if not.
    */
   virtual bool shouldAssignPiece(bool& must);

   /**
    * Picks a seller to assign a piece to.
    *
    * @param sd the SellerData to set.
    * @param must true if a seller *must* be picked to continue the download.
    *
    * @return true if a seller was picked, false if not (if *must* is true,
    *         then an exception will be set).
    */
   virtual bool pickSeller(bitmunk::common::SellerData& sd, bool must);

   /**
    * Assigns a piece to a particular seller and starts its download.
    *
    * @param sd the SellerData of the seller to assign the piece to.
    *
    * @return true if successful, false if error.
    */
   virtual bool assignPiece(bitmunk::common::SellerData& sd);

   /**
    * Handles a message by calling its more specific message handling function.
    *
    * @param msg the message to handle.
    *
    * @return true if successful, false if error.
    */
   virtual bool handleMessage(monarch::rt::DynamicObject& msg);

   /**
    * Handles a piece update message.
    *
    * @param msg the message to handle.
    *
    * @return true if successful, false if error.
    */
   virtual bool pieceUpdate(monarch::rt::DynamicObject& msg);

   /**
    * Handles a seller pool timeout message.
    *
    * @param msg the message to handle.
    *
    * @return true if successful, false if error.
    */
   virtual bool poolTimeout(monarch::rt::DynamicObject& msg);

   /**
    * Handles a seller pool update message.
    *
    * @param msg the message to handle.
    *
    * @return true if successful, false if error.
    */
   virtual bool poolUpdate(monarch::rt::DynamicObject& msg);

   /**
    * Handles a negotiation complete message.
    *
    * @param msg the message to handle.
    *
    * @return true if successful, false if error.
    */
   virtual bool negotiationComplete(monarch::rt::DynamicObject& msg);

   /**
    * Handles a pause download message.
    *
    * @param msg the message to handle.
    *
    * @return true if successful, false if error.
    */
   virtual bool pauseDownload(monarch::rt::DynamicObject& msg);

   /**
    * Handles an interrupt message.
    *
    * @param msg the message to handle.
    *
    * @return true if successful, false if error.
    */
   virtual bool interrupt(monarch::rt::DynamicObject& msg);

   /**
    * Handles a progress poll message.
    *
    * @param msg the message to handle.
    *
    * @return true if successful, false if error.
    */
   virtual bool progressPolled(monarch::rt::DynamicObject& msg);

   /**
    * Formats a file piece filename given a base path, download state ID,
    * file ID, extension, and index of the file piece.
    *
    * @param path the base working path.
    * @param userId the user ID of the user that owns the download state.
    * @param dsId the download state that is associated with the filename.
    * @param fileId the ID of the file the piece is for.
    * @param extension the extension of the file that is being downloaded.
    * @param index the index of the file piece.
    *
    * @return the download filename as an absolute pathname.
    */
   virtual std::string generateFilePieceFilename(
      const char* path, bitmunk::common::UserId userId, DownloadStateId dsId,
      bitmunk::common::FileId fileId, const char* extension, uint32_t index);
};

} // end namespace purchase
} // end namespace bitmunk
#endif

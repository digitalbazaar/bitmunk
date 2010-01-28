/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_purchase_PieceDownloader_H
#define bitmunk_purchase_PieceDownloader_H

#include "bitmunk/purchase/DownloadStateFiber.h"
#include "bitmunk/node/NodeFiber.h"
#include "monarch/event/ObserverList.h"
#include "monarch/http/HttpConnection.h"

namespace bitmunk
{
namespace purchase
{

/**
 * A PieceDownloader is used to download a file piece from a seller that has
 * a negotiated contract section in the sqlite3 database. The data is pulled
 * using nonblocking IO and libev and then written to disk in a piece file.
 *
 * To read, the PieceDownloader spawns an Operation that reads as much as it
 * can from the seller and then goes inactive. The Operation may reach a
 * "wouldBlock" exception. If it does, then it simply returns and unloops the
 * libev event.
 *
 * When a new libev event occurs, the PieceDownloader will be activated by
 * an IOWatcher -- which, in turn, will cause another Operation to be spawned
 * to handle the IO. If the Operation actually finishes the piece download,
 * then the PieceDownloader exits.
 *
 * @author Dave Longley
 */
class PieceDownloader : public bitmunk::node::NodeFiber, public DownloadStateFiber
{
protected:
   /**
    * A unique ID assigned by this piece downloader's download manager.
    */
   uint32_t mUniqueId;

   /**
    * The standard piece size for the piece to be downloaded (may end up
    * being smaller).
    */
   uint32_t mPieceSize;

   /**
    * The contract section this piece downloader is assigned to.
    */
   bitmunk::common::ContractSection mSection;

   /**
    * The ID of the file to download.
    */
   char* mFileId;

   /**
    * The piece to download.
    */
   bitmunk::common::FilePiece mFilePiece;

   /**
    * The url to download from.
    */
   monarch::net::Url mUrl;

   /**
    * The connection being used to download.
    */
   monarch::http::HttpConnection* mConnection;

   /**
    * The http request.
    */
   monarch::http::HttpRequest* mRequest;

   /**
    * The http response.
    */
   monarch::http::HttpResponse* mResponse;

   /**
    * The input BtpMessage.
    */
   bitmunk::protocol::BtpMessage mInMessage;

   /**
    * The output BtpMessage.
    */
   bitmunk::protocol::BtpMessage mOutMessage;

   /**
    * The input stream being used to download.
    */
   monarch::io::InputStreamRef mInputStream;

   /**
    * The HttpTrailer to store trailers from the download.
    */
   monarch::http::HttpTrailerRef mTrailer;

   /**
    * The DigitalSignature to check content security.
    */
   monarch::crypto::DigitalSignatureRef mSignature;

   /**
    * The output stream used to write downloaded data.
    */
   monarch::io::OutputStreamRef mOutputStream;

   /**
    * A rate averager for receiving a piece.
    */
   monarch::util::RateAverager* mPieceDownloadRate;

   /**
    * A RateAverager for just the related contract data.
    */
   monarch::util::RateAverager* mDownloadRate;

   /**
    * Set to true when this piece downloader has been interrupted.
    */
   bool mInterrupted;

public:
   /**
    * Creates a new PieceDownloader.
    *
    * @param node the Node associated with this fiber.
    * @param parentId the ID of the parent fiber.
    */
   PieceDownloader(bitmunk::node::Node* node, monarch::fiber::FiberId parentId);

   /**
    * Destructs this PieceDownloader.
    */
   virtual ~PieceDownloader();

   /**
    * Assigns the passed information to this piece downloader. This should be
    * called before starting the piece downloader.
    *
    * @param id a unique ID assigned by a download manager.
    * @param pieceSize the standard (maximum) piece size.
    * @param cs the ContractSection to download pieces for.
    * @param fileId the ID of the file to download.
    * @param fp the FilePiece to download.
    * @param pa the piece download rate averager to use.
    * @param ra the contract download rate averager to use.
    */
   virtual void initialize(
      uint32_t id,
      uint32_t pieceSize,
      bitmunk::common::ContractSection& cs,
      bitmunk::common::FileId fileId,
      bitmunk::common::FilePiece& fp,
      monarch::util::RateAverager* pa,
      monarch::util::RateAverager* ra);

   /**
    * Downloads a file piece.
    */
   virtual void download();

   /**
    * Called when IO events occur.
    *
    * @param fd the file descriptor the events occurred on.
    * @param events the IO events.
    */
   virtual void fdUpdated(int fd, int events);

protected:
   /**
    * Processes messages, retrieved via getMessages(), and performs whatever
    * custom work is necessary.
    */
   virtual void processMessages();

   /**
    * Connects to the seller to download a file piece.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool connect();

   /**
    * Sends an event about piece download.
    *
    * @param error true if there was an error, false if not.
    * @param type the type of event to send, NULL when error is true.
    */
   virtual void sendEvent(bool error, const char* type);
};

} // end namespace purchase
} // end namespace bitmunk
#endif

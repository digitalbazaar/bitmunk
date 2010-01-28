/*
 * Copyright (c) 2007-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_bfp_Bfp_H
#define bitmunk_bfp_Bfp_H

#include "bitmunk/common/TypeDefinitions.h"

namespace bitmunk
{
namespace bfp
{

/**
 * The Bfp class is an interface to a Bitmunk File Processor (BFP).
 *
 * A Bfp is used to watermark, encrypt, and decrypt file pieces transferred
 * over Bitmunk.
 *
 * @author Dave Longley
 */
class Bfp
{
public:
   /**
    * The types of bfp modes.
    */
   enum BfpMode
   {
      PeerSell = 0, PeerBuy = 1, WebBuy = 2
   };

   /**
    * Creates a new Bfp.
    */
   Bfp() {};

   /**
    * Destructs this Bfp.
    */
   virtual ~Bfp() {};

   /**
    * Sets a FileInfo's ID and content size.
    *
    * @param fi the FileInfo to update.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool setFileInfoId(bitmunk::common::FileInfo& fi) = 0;

   /**
    * Initializes this Bfp to read Seller peerbuy data for the passed
    * ContractSection.
    *
    * @param csHash the hash of the ContractSection to initialize for.
    * @param sellerKey the seller's seal/open key in PEM format.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool initializePeerSell(
      const char* csHash, const char* sellerKey) = 0;

   /**
    * Initializes this Bfp to read Buyer peerbuy data.
    *
    * @param c the Contract to initialize for.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool initializePeerBuy(bitmunk::common::Contract& c) = 0;

   /**
    * Initializes this Bfp to read webbuy data.
    *
    * @param c the Contract to initialize for.
    * @param csHash the hash of the ContractSection to initialize for.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool initializeWebBuy(
      bitmunk::common::Contract& c, const char* csHash) = 0;

   /**
    * Prepares this Bfp to read the next file once it has been initialized
    * in PeerSell mode. This method *must* be called before startReading()
    * when the file to be read has changed.
    *
    * @param fi the FileInfo for the file.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool preparePeerSellFile(bitmunk::common::FileInfo& fi) = 0;

   /**
    * Prepares this Bfp to read the next file once it has been initialized
    * in PeerBuy mode. This method *must* be called before startReading()
    * when the file to be read has changed.
    *
    * If fileSize is provided, then the size of the output file will be
    * provided.
    *
    * @param fi the FileInfo for the file.
    * @param fileSize set to the new size of the output file.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool preparePeerBuyFile(
      bitmunk::common::FileInfo& fi, uint64_t* fileSize = NULL) = 0;

   /**
    * Prepares this Bfp to read the next file once it has been initialized
    * in WebBuy mode. This method *must* be called before startReading()
    * when the file to be read has changed.
    *
    * @param fi the FileInfo for the file.
    * @param fileSize set to the new size of the output file.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool prepareWebBuyFile(
      bitmunk::common::FileInfo& fi, uint64_t* fileSize = NULL) = 0;

   /**
    * Starts the reading process for a particular FilePiece that is a part
    * of the file that was previously prepared with prepareFile().
    *
    * The data that is read out from this bfp depend on the bfp's initialized
    * mode.
    *
    * PeerSell:
    *
    * The input stream must be the full file's raw data. The bfp's read()
    * method will produce the watermarked and encrypted file piece data for
    * a single piece.
    *
    * PeerBuy:
    *
    * The input stream must be the encrypted file piece data received from
    * a peer Seller. The bfp's read() method will produce the unencrypted,
    * receipt-embedded file piece data for a single piece. It must be sewn
    * together with the other file piece data produced via this method for
    * the other pieces.
    *
    * WebBuy:
    *
    * The input stream must be the full file's raw data. The bfp's read()
    * method will produce the full watermarked and receipt-embedded file data.
    *
    * @param fp the FilePiece.
    * @param is the input stream, which must point to the beginning of the file
    *           to be read. Note that this should be different from the input
    *           stream used in the prepare*File() methods above.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool startReading(bitmunk::common::FilePiece& fp) = 0;

   /**
    * The output of this bfp depends on the mode it was initialized in.
    *
    * PeerSell:
    *
    * The read data will be the watermarked and encrypted file piece data for
    * a single piece.
    *
    * PeerBuy:
    *
    * The read data will be the unencrypted, receipt-embedded file piece data
    * for a single piece. It must be sewn together with the other file piece
    * data produced via this method for the other pieces.
    *
    * WebBuy:
    *
    * The read data will be the full watermarked and receipt-embedded file data.
    *
    * @param b the buffer to read into.
    * @param length the length of the buffer.
    *
    * @return the actual number of bytes read, 0 if the end of the data
    *         has been reached, and -1 if an IOException occurred.
    */
   virtual int read(char* b, int length) = 0;

   /**
    * Gets the ID for this Bfp.
    *
    * @return the ID for this Bfp.
    */
   virtual bitmunk::common::BfpId getId() = 0;
};

} // end namespace bfp
} // end namespace bitmunk
#endif

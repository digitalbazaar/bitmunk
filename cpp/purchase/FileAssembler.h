/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_purchase_FileAssembler_H
#define bitmunk_purchase_FileAssembler_H

#include "bitmunk/bfp/Bfp.h"
#include "bitmunk/common/TypeDefinitions.h"
#include "bitmunk/purchase/DownloadStateFiber.h"
#include "bitmunk/node/NodeFiber.h"

namespace bitmunk
{
namespace purchase
{

/**
 * A FileAssembler fiber is used to assemble all of the files that are part
 * of a contract. Each file's pieces are decrypted, assembled into a
 * media file, and stored in the user's download path.
 * 
 * @author Manu Sporny
 * @author Dave Longley
 */
class FileAssembler :
public bitmunk::node::NodeFiber,
public DownloadStateFiber
{
protected:
   /**
    * The retrieved bfp.
    */
   bitmunk::bfp::Bfp* mBfp;
   
   /**
    * ID of the bfp to load.
    */
   bitmunk::common::BfpId mBfpId;
   
   /**
    * The file info of the file to verify.
    */
   bitmunk::common::FileInfo mFileInfo;
   
public:
   /**
    * Creates a new FileAssembler.
    * 
    * @param node the Node associated with this fiber.
    */
   FileAssembler(bitmunk::node::Node* node);
   
   /**
    * Destructs this FileAssembler.
    */
   virtual ~FileAssembler();
   
   /**
    * Acquires a bfp via an Operation.
    */
   virtual void acquireBfp();
   
   /**
    * Checks a file ID via an Operation.
    */
   virtual void checkFileId();
   
protected:
   /**
    * Processes messages, retrieved via getMessages(), and performs whatever
    * custom work is necessary.
    */
   virtual void processMessages();
   
   /**
    * Checks to ensure license and data have been purchased.
    * 
    * @return true if check passed, false if not.
    */
   virtual bool checkPurchase();
   
   /**
    * Loads a bfp interface.
    * 
    * @return true if successful, false if not.
    */
   virtual bool loadBfp();
   
   /**
    * Assembles a single file.
    * 
    * @param fi the FileInfo for the file.
    * @param downloadPath the path to write the output to.
    * 
    * @return true if successful, false if not.
    */
   virtual bool assembleFile(
      bitmunk::common::FileInfo& fi, const char* downloadPath);
   
   /**
    * Assembles all file pieces for a particular file.
    * 
    * @param fi the FileInfo for the file.
    * 
    * @return true if successful, false if error.
    */
   virtual bool assemblePieces(bitmunk::common::FileInfo& fi);
   
   /**
    * Verifies that the file is valid.
    * 
    * @param fi the FileInfo for the file.
    * 
    * @return true if successful, false if error.
    */
   virtual bool verifyFile(bitmunk::common::FileInfo& fi);
   
   /**
    * Cleans up temporary file pieces for the file.
    * 
    * @param fi the FileInfo for the file.
    * 
    * @return true if successful, false if error.
    */
   virtual bool cleanupFilePieces(bitmunk::common::FileInfo& fi);
   
   /**
    * Populates a media and collection details given a file info object. This
    * method is used to pick apart the contract such that the media and
    * collection details can be used to generate a proper file structure for
    * a downloaded file.
    * 
    * @param fi the file info object containing the mediaId of interest.
    * @param media the media object that will be populated during the call.
    * @param collection the collection object that will be populated during
    *                   the call.
    * @return true if the population was successful, false otherwise. 
    */
   virtual bool populateMediaAndCollectionDetails(
      bitmunk::common::FileInfo& fi, bitmunk::common::Media& media, 
      bitmunk::common::Media& collection);
};

} // end namespace purchase
} // end namespace bitmunk
#endif

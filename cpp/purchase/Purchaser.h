/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_purchase_Purchaser_H
#define bitmunk_purchase_Purchaser_H

#include "bitmunk/purchase/DownloadStateFiber.h"
#include "bitmunk/node/NodeFiber.h"

namespace bitmunk
{
namespace purchase
{

/**
 * A Purchaser fiber is used to purchase the license and data associated with
 * a DownloadState.
 * 
 * It will first attempt to purchase the license for the data, and if that's
 * successful or has already been performed, it will then attempt to purchase
 * the data.
 * 
 * @author Dave Longley
 * @author Manu Sporny
 */
class Purchaser :
public bitmunk::node::NodeFiber,
public DownloadStateFiber
{
protected:
   /**
    * A list of database entries to be processed to store decryption keys.
    */
   monarch::rt::DynamicObject mDbEntries;
   
public:
   /**
    * Creates a new Purchaser.
    * 
    * @param node the Node associated with this fiber.
    */
   Purchaser(bitmunk::node::Node* node);
   
   /**
    * Destructs this Purchaser.
    */
   virtual ~Purchaser();
   
   /**
    * Runs an operation to purchase the license.
    */
   virtual void runPurchaseLicenseOp();
   
   /**
    * Runs an operation to purchase the data.
    */
   virtual void runPurchaseDataOp();
   
protected:
   /**
    * Processes messages, retrieved via getMessages(), and performs whatever
    * custom work is necessary.
    */
   virtual void processMessages();
   
   /**
    * Attempts to purchase the license.
    * 
    * @return true if successful, false if not.
    */
   virtual bool purchaseLicense();
   
   /**
    * Attempts to purchase the data.
    * 
    * @return true if successful, false if not.
    */
   virtual bool purchaseData();
};

} // end namespace purchase
} // end namespace bitmunk
#endif

/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc.  All rights reserved.
 */
#ifndef bitmunk_purchase_Negotiator_H
#define bitmunk_purchase_Negotiator_H

#include "bitmunk/purchase/DownloadStateFiber.h"
#include "bitmunk/node/NodeFiber.h"
#include "monarch/crypto/BigDecimal.h"

namespace bitmunk
{
namespace purchase
{

/**
 * A Negotiator selects a new seller and negotiates a contract section with it.
 * 
 * @author Dave Longley
 */
class Negotiator :
public bitmunk::node::NodeFiber,
public DownloadStateFiber
{
protected:
   /**
    * The seller data with a contract section to negotiate.
    */
   bitmunk::common::SellerData mSellerData;
   
   /**
    * True if a seller must be found to continue downloading, false if not.
    */
   bool mMustFindSeller;
   
public:
   /**
    * Creates a new Negotiator.
    * 
    * @param node the Node associated with this fiber.
    * @param parentId the ID of the parent fiber.
    */
   Negotiator(bitmunk::node::Node* node, monarch::fiber::FiberId parentId);
   
   /**
    * Destructs this Negotiator.
    */
   virtual ~Negotiator();
   
   /**
    * Negotiates a contract section via an Operation.
    */
   virtual void negotiateSection();
   
   /**
    * Sets whether or not a seller *must* be found to continue downloading.
    * 
    * @param must true if a seller must be found, false if not.
    */
   virtual void setMustFindSeller(bool must);
   
protected:
   /**
    * Processes messages, retrieved via getMessages(), and performs whatever
    * custom work is necessary.
    */
   virtual void processMessages();
   
   /**
    * Picks files in the download state that have unassigned pieces and could
    * therefore benefit from the addition of a new seller. This method
    * populates the passed stack with those files' associated file progress
    * objects.
    * 
    * @param fpStack the stack to populate.
    */
   virtual void pickFiles(std::list<FileProgress>& fpStack);
   
   /**
    * Picks the new seller to negotiate with. Is called on each fiber iteration
    * until a seller is successfully negotiated with or until there are no
    * more available sellers (error case).
    * 
    * @param sp the SellerPool to pick from.
    * 
    * @return true if a seller was picked, false if not. 
    */
   virtual bool pickSeller(bitmunk::common::SellerPool& sp);
   
   /**
    * Starts an operation to negotiate with a seller.
    * 
    * @return 1 if a section was negotiated, 0 if the seller was bad and
    *         another should be picked, and -1 if there was an error.
    */
   virtual int negotiate();
   
   /**
    * Attempts to perform the actual negotiation with the seller, setting
    * whether or not the seller should be temporarily blacklisted due to
    * connection difficulties.
    * 
    * @param userId the ID of the buyer.
    * @param profile the profile to sign with.
    * @param cs the ContractSection to negotiate.
    * @param blacklist set to whether or not the seller should be blacklisted.
    */
   virtual bool negotiateWithSeller(
      bitmunk::common::UserId userId,
      bitmunk::common::ProfileRef& profile,
      bitmunk::common::ContractSection& cs,
      bool& blacklist);
   
   /**
    * Verifies the contract section received from the seller, signing it if
    * it is acceptable.
    * 
    * @param c the associated contract.
    * @param cs the contract section to verify.
    * @param profile the profile to sign with.
    * 
    * @return true if successful, false if not.
    */
   virtual bool verifySection(
      bitmunk::common::Contract& c,
      bitmunk::common::ContractSection& cs,
      bitmunk::common::ProfileRef& profile);
   
   /**
    * Checks the price of the contract section received from the seller to
    * ensure it is not over budget.
    * 
    * @param c the associated contract.
    * @param cs the contract section to check.
    * 
    * @return true if at or under budget, false if over.
    */
   virtual bool checkSectionPrice(
      bitmunk::common::Contract& c,
      bitmunk::common::ContractSection& cs);
   
   /**
    * Inserts a negotiated contract section into the database.
    * 
    * @return true if successful, false if not.
    */
   virtual bool insertSection();
};

} // end namespace purchase
} // end namespace bitmunk
#endif

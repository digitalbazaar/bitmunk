/*
 * Copyright (c) 2007-2008 Digital Bazaar, Inc.  All rights reserved.
 */
#ifndef bitmunk_common_NegotiateInterface_H
#define bitmunk_common_NegotiateInterface_H

#include "bitmunk/common/TypeDefinitions.h"

namespace bitmunk
{
namespace common
{

/**
 * A NegotiateInterface is a standard interface for negotiating
 * ContractSections. It is expected that various negotiate modules' interfaces
 * will implement this interface according to their own specifics.
 * 
 * @author Dave Longley
 */
class NegotiateInterface
{
public:
   /**
    * Creates a new NegotiateInterface.
    */
   NegotiateInterface() {};
   
   /**
    * Destructs this NegotiateInterface.
    */
   virtual ~NegotiateInterface() {};
   
   /**
    * Negotiates the given ContractSection of the passed Contract.
    * 
    * @param userId the ID of the user to negotiate as.
    * @param c the Contract to negotiate.
    * @param section the particular ContractSection to negotiate.
    * @param seller true to negotiate on behalf of the seller, false to
    *               negotiate on behalf of the buyer.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool negotiateContractSection(
      UserId userId, Contract& c, ContractSection& section, bool seller) = 0;
};

} // end namespace common
} // end namespace bitmunk
#endif

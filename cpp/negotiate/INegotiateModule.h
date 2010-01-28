/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_negotiate_INegotiateModule_H
#define bitmunk_negotiate_INegotiateModule_H

#include "bitmunk/common/NegotiateInterface.h"
#include "bitmunk/node/Node.h"
#include "monarch/kernel/MicroKernelModuleApi.h"

namespace bitmunk
{
namespace negotiate
{

/**
 * An INegotiateModule provides the interface for a NegotiateModule.
 * 
 * @author Dave Longley
 */
class INegotiateModule :
public monarch::kernel::MicroKernelModuleApi,
public bitmunk::common::NegotiateInterface
{
protected:
   /**
    * The associated node.
    */
   bitmunk::node::Node* mNode;
   
public:
   /**
    * Creates a new INegotiateModule.
    * 
    * @param node the associated node.
    */
   INegotiateModule(bitmunk::node::Node* node);
   
   /**
    * Destructs this INegotiateModule.
    */
   virtual ~INegotiateModule();
   
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
      bitmunk::common::UserId userId,
      bitmunk::common::Contract& c,
      bitmunk::common::ContractSection& section, bool seller);
};

} // end namespace negotiate
} // end namespace bitmunk
#endif

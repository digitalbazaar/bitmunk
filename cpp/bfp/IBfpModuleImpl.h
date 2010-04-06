/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_bfp_IBfpModuleImpl_H
#define bitmunk_bfp_IBfpModuleImpl_H

#include "bitmunk/bfp/IBfpModule.h"
#include "bitmunk/bfp/BfpFactory.h"
#include "bitmunk/node/Node.h"

namespace bitmunk
{
namespace bfp
{

/**
 * An IBfpModuleImpl provides the interface for a BfpModule. It allows bfps to
 * be created and freed.
 *
 * @author Dave Longley
 */
class IBfpModuleImpl : public bitmunk::bfp::IBfpModule
{
protected:
   /**
    * The Bitmunk Node associated with this interface.
    */
   bitmunk::node::Node* mNode;

   /**
    * A lock for creating a bfp.
    */
   monarch::rt::ExclusiveLock mLock;

   /**
    * Flag if a BFP lib check should be performed.
    */
   bool mCheckBfp;

public:
   /**
    * Creates a new IBfpModuleImpl that uses the passed Node.
    *
    * @param node the associated Bitmunk Node.
    */
   IBfpModuleImpl(bitmunk::node::Node* node);

   /**
    * Destructs this IBfpModuleImpl.
    */
   virtual ~IBfpModuleImpl();

   /**
    * Creates a Bfp object based on its version and ID. A created Bfp object
    * must be freed by calling freeBfp().
    *
    * @param id the BfpId for the Bfp object to create.
    *
    * @return the created Bfp object or NULL if an exception occurred.
    */
   virtual Bfp* createBfp(bitmunk::common::BfpId id);

   /**
    * Frees a Bfp object.
    *
    * @param bfp the Bfp object to free.
    */
   virtual void freeBfp(Bfp* bfp);

protected:
   /**
    * Gets the BfpFactory for Bfps with the given ID.
    *
    * @param id the BfpId of the BfpFactory to retrieve.
    *
    * @return the BfpFactory or NULL if none exists.
    */
   virtual BfpFactory* getBfpFactory(bitmunk::common::BfpId id);
};

} // end namespace bfp
} // end namespace bitmunk
#endif

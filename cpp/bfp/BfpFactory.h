/*
 * Copyright (c) 2007-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_bfp_BfpFactory_H
#define bitmunk_bfp_BfpFactory_H

#include "bitmunk/bfp/Bfp.h"

namespace bitmunk
{
namespace bfp
{

/**
 * A BfpFactory is used to create and free Bfp objects.
 * 
 * @author Dave Longley
 */
class BfpFactory
{
public:
   /**
    * Creates a new BfpFactory.
    */
   BfpFactory() {};
   
   /**
    * Destructs this BfpFactory.
    */
   virtual ~BfpFactory() {};
   
   /**
    * Creates a Bfp object based on its ID. A created Bfp object must be freed
    * by calling freeBfp().
    * 
    * @param id the BfpId for the Bfp object to create.
    * 
    * @return the created Bfp object or NULL if an exception occurred.
    */
   virtual Bfp* createBfp(bitmunk::common::BfpId id) = 0;
   
   /**
    * Frees a Bfp object.
    * 
    * @param bfp the Bfp object to free.
    */
   virtual void freeBfp(Bfp* bfp) = 0;
};

} // end namespace bfp
} // end namespace bitmunk
#endif

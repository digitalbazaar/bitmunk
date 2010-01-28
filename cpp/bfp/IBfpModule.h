/*
 * Copyright (c) 2007-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_bfp_IBfpModule_H
#define bitmunk_bfp_IBfpModule_H

#include "bitmunk/bfp/Bfp.h"
#include "monarch/kernel/MicroKernelModuleApi.h"

namespace bitmunk
{
namespace bfp
{

/**
 * An IBfpModule provides the interface for a BfpModule. It allows bfps to
 * be created and freed.
 * 
 * @author Dave Longley
 */
class IBfpModule : public monarch::kernel::MicroKernelModuleApi
{
public:
   /**
    * Creates a new IBfpModule.
    */
   IBfpModule() {};
   
   /**
    * Destructs this IBfpModule.
    */
   virtual ~IBfpModule() {};
   
   /**
    * Creates a Bfp object based on its version and ID. A created Bfp object
    * must be freed by calling freeBfp().
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

/*
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_gtkui_GtkUiModule_H
#define bitmunk_gtkui_GtkUiModule_H

#include "monarch/kernel/MicroKernelModule.h"

// module logging category
extern monarch::logging::Category* BM_GTKUI_CAT;

namespace bitmunk
{
namespace gtkui
{

/**
 * A GtkUiModule is a NodeModule that runs the main gtk loop so that other
 * modules can create gtk interfaces.
 * 
 * @author Dave Longley
 */
class GtkUiModule : public monarch::kernel::MicroKernelModule
{
protected:
   /**
    * The thread for the gtk main loop.
    */
   monarch::rt::Thread* mGtkThread;
   
public:
   /**
    * Creates a new GtkUiModule.
    */
   GtkUiModule();
   
   /**
    * Destructs this GtkUiModule.
    */
   virtual ~GtkUiModule();
   
   /**
    * Gets dependency information.
    *
    * @return the dependency information.
    */
   virtual monarch::rt::DynamicObject getDependencyInfo();

   /**
    * Initializes this Module with the passed MicroKernel.
    *
    * @param k the MicroKernel.
    *
    * @return true if initialized, false if an Exception occurred.
    */
   virtual bool initialize(monarch::kernel::MicroKernel* k);

   /**
    * Cleans up this Module just prior to its unloading.
    *
    * @param k the MicroKernel.
    */
   virtual void cleanup(monarch::kernel::MicroKernel* k);

   /**
    * Gets the API for this MicroKernelModule.
    *
    * @param k the MicroKernel that loaded this module.
    *
    * @return the API for this MicroKernelModule.
    */
   virtual monarch::kernel::MicroKernelModuleApi* getApi(
      monarch::kernel::MicroKernel* k);
};

} // end namespace gtkui
} // end namespace bitmunk
#endif

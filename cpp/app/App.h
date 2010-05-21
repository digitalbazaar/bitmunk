/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_app_App_H
#define bitmunk_app_App_H

#include "monarch/app/App.h"

namespace bitmunk
{
namespace app
{

/**
 * monarch::app::App with Bitmunk specific features.
 *
 * Author: David I. Lehn
 */
class App : public monarch::app::App
{
public:
   /**
    * Create an App instance.
    */
   App();

   /**
    * Deconstruct this App instance.
    */
   virtual ~App();

   /**
    * {@inheritDoc}
    */
   virtual bool initialize();

   /**
    * {@inheritDoc}
    */
   virtual void cleanup();

   /**
    * {@inheritDoc}
    */
   virtual bool initConfigs(monarch::config::Config& defaults);
};

} // end namespace app
} // end namespace bitmunk

#endif

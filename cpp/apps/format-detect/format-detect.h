/*
 * Copyright (c) 2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_test_Tester_H
#define bitmunk_test_Tester_H

#include "bitmunk/app/App.h"

namespace bitmunk
{
namespace apps
{
namespace tools
{

/**
 * A Bitmunk App that determines file formats via data inspection.
 *
 * @author Dave Longley
 */
class FormatDetectApp : public bitmunk::app::App
{
public:
   /**
    * Creates a FormatDetectApp.
    */
   FormatDetectApp();

   /**
    * Deconstructs this FormatDetectApp.
    */
   virtual ~FormatDetectApp();

   /**
    * {@inheritDoc}
    */
   virtual bool initialize();

   /**
    * {@inheritDoc}
    */
   virtual monarch::rt::DynamicObject getCommandLineSpec(
      monarch::config::Config& cfg);

   /**
    * Runs the format detector.
    *
    * @return true on success, false and exception set on failure.
    */
   virtual bool run();
};

} // end namespace tools
} // end namespace apps
} // end namespace bitmunk

#endif

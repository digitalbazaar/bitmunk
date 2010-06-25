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
 * A Bitmunk App that determines BFP file IDs.
 *
 * @author Dave Longley
 */
class BfpFileIdApp : public bitmunk::app::App
{
public:
   /**
    * Creates a BfpFileIdApp.
    */
   BfpFileIdApp();

   /**
    * Deconstructs this BfpFileIdApp.
    */
   virtual ~BfpFileIdApp();

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
    * Runs the BFP file ID calculator.
    *
    * @return true on success, false and exception set on failure.
    */
   virtual bool run();
};

} // end namespace tools
} // end namespace apps
} // end namespace bitmunk

#endif

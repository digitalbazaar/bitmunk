/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_test_Tester_H
#define bitmunk_test_Tester_H

#include "bitmunk/app/App.h"
#include "bitmunk/node/Node.h"
#include "bitmunk/test/Test.h"
#include "monarch/test/TestRunner.h"

#define BITMUNK_TESTER_CONFIG_ID   "bitmunk.test.Tester.config.base"

namespace bitmunk
{
namespace apps
{
namespace tester
{

/**
 * A Bitmunk App that runs tests in modules of type "monarch.test.TestModule".
 *
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 * @author Dave Longley
 */
class Tester : public bitmunk::app::App
{
public:
   /**
    * Creates a Tester.
    */
   Tester();

   /**
    * Deconstructs this Tester.
    */
   virtual ~Tester();

   /**
    * {@inheritDoc}
    */
   virtual bool initialize();

   /**
    * {@inheritDoc}
    */
   virtual bool initConfigs(monarch::config::Config& defaults);

   /**
    * {@inheritDoc}
    */
   virtual monarch::rt::DynamicObject getCommandLineSpec(
      monarch::config::Config& cfg);

   /**
    * Runs all tests or those specified via command line options.
    *
    * @return true on success, false and exception set on failure.
    */
   virtual bool run();
};

} // end namespace tester
} // end namespace apps
} // end namespace bitmunk

#endif

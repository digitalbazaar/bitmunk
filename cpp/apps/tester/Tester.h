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

   /**
    * Adds the Bitmunk test configuration. Optional config merge data may be
    * provided.
    *
    * @param tr the test's TestRunner.
    * @param extraMerge optional config data to add to the merge section.
    *
    * @return true if succesful, false if an exception occurred.
    */
   static bool loadConfig(
      monarch::test::TestRunner& tr,
      monarch::config::Config* extraMerge = NULL);

   /**
    * Removes the Bitmunk test configuration.
    *
    * @param tr the test's TestRunner.
    *
    * @return true if succesful, false if an exception occurred.
    */
   static bool unloadConfig(monarch::test::TestRunner& tr);

   /**
    * Loads a Node.
    *
    * @param tr the test's TestRunner.
    *
    * @return the loaded Node if successful, NULL if not.
    */
   static bitmunk::node::Node* loadNode(monarch::test::TestRunner& tr);

   /**
    * Unloads a previously loaded Node.
    *
    * @param tr the test's TestRunner.
    *
    * @return true if successful, false if an exception occurred.
    */
   static bool unloadNode(monarch::test::TestRunner& tr);
};

} // end namespace tester
} // end namespace apps
} // end namespace bitmunk

#endif

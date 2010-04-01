/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_test_Tester_H
#define bitmunk_test_Tester_H

#include "monarch/logging/Logger.h"
#include "monarch/apps/tester/Tester.h"
#include "monarch/rt/DynamicObject.h"
#include "monarch/rt/DynamicObjectIterator.h"
#include "bitmunk/node/Node.h"
#include "bitmunk/test/Test.h"

namespace bitmunk
{
namespace test
{

/**
 * Top-level bitmunk Tester.  Used to load standard configuration.
 * 
 * Author: David I. Lehn
 */
class Tester /*: public monarch::apps::tester::Tester*/
{
protected:
   /**
    * Kernel for this app.
    */
   monarch::kernel::MicroKernel* mKernel;

public:
   /**
    * Create a Tester.
    */
   Tester();
   
   /**
    * Deconstruct this Tester.
    */
   virtual ~Tester();
   
   /**
    * Setup the TestRunner for running node tests.
    * 
    * @param tr the test's TestRunner.
    *
    * @return true if succesful, false if an exception occurred.
    */
   static bool setup(monarch::test::TestRunner& tr);

   /**
    * Tear down the TestRunner's setup after running node tests.
    *
    * @param tr the test's TestRunner.
    *
    * @return true if succesful, false if an exception occurred.
    */
   static bool tearDown(monarch::test::TestRunner& tr);
   
   /**
    * Setup a node from this app.  Will read out the current config and use it
    * to set the Nodes config.
    *
    * @param node a Node to setup.
    *
    * @return true if succesful, false if an exception occurred.
    */
   //static bool setupNode(bitmunk::node::Node* node);

   /**
    * Setup the TestRunner for a peer node test. The merge config is only
    * merge data.
    *
    * @param tr the test's TestRunner.
    * @param merge optional config data to add to the merge section.
    *
    * @return true if succesful, false if an exception occurred.
    */
   static bool setupPeerNode(
      monarch::test::TestRunner& tr,
      monarch::config::Config* extraMerge = NULL);

   /**
    * Tear down the TestRunner's peer node setup.
    *
    * @param tr the test's TestRunner.
    *
    * @return true if succesful, false if an exception occurred.
    */
   static bool tearDownPeerNode(monarch::test::TestRunner& tr);
};

} // end namespace test
} // end namespace bitmunk

#endif

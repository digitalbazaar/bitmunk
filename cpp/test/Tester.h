/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_test_Tester_H
#define bitmunk_test_Tester_H

#include "bitmunk/node/Node.h"
#include "bitmunk/test/Test.h"
#include "monarch/test/TestRunner.h"

namespace bitmunk
{
namespace test
{

/**
 * Top-level bitmunk Tester. Used to handle Node configuration and loading.
 *
 * @author David I. Lehn
 * @author Dave Longley
 */
class Tester
{
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
    * Adds the configuration for a Node. Optional config merge data may be
    * provided.
    *
    * @param tr the test's TestRunner.
    * @param extraMerge optional config data to add to the merge section.
    *
    * @return true if succesful, false if an exception occurred.
    */
   static bool addNodeConfig(
      monarch::test::TestRunner& tr,
      monarch::config::Config* extraMerge = NULL);

   /**
    * Tear down the TestRunner's setup after running node tests.
    *
    * @param tr the test's TestRunner.
    *
    * @return true if succesful, false if an exception occurred.
    */
   static bool removeNodeConfig(monarch::test::TestRunner& tr);

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

} // end namespace test
} // end namespace bitmunk

#endif

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
    * Loads a Node. First the Node's test configuration will be loaded, then
    * the Node, then its modules. The Node must be started manually with a
    * call to start().
    *
    * An optional config file may be specified with additional configuration
    * options to be added to the config system before loading the node. The
    * config file is specified by passing the name of the unit test. The
    * unit test name concatenated with ".config" will be appended to the
    * "unitTestConfigPath" variable to find the associated file.
    *
    * The ConfigManager's state will be saved before loading any configs and
    * will be restored when unloadNode() is called.
    *
    * @param tr the test's TestRunner.
    * @param unitTest the name of the unit test if it has a custom config
    *           file that is to be loaded, otherwise NULL.
    *
    * @return the loaded Node if successful, NULL if not.
    */
   static bitmunk::node::Node* loadNode(
      monarch::test::TestRunner& tr, const char* unitTest = NULL);

   /**
    * Unloads a previously loaded Node and restores the ConfigManager's state.
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

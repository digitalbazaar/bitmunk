/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_test_Tester_H
#define bitmunk_test_Tester_H

#include "monarch/logging/Logger.h"
#include "monarch/test/Tester.h"
#include "monarch/rt/DynamicObject.h"
#include "monarch/rt/DynamicObjectIterator.h"
#include "bitmunk/node/App.h"
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
class Tester : public monarch::test::Tester
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
    * Setup before running tests.
    * 
    * @param tr the test's TestRunner.
    */
   virtual void setup(monarch::test::TestRunner& tr);
   
   /**
    * Setup a node from this app.  Will read out the current config and use it
    * to set the Nodes config.
    *
    * @param node a Node to setup.
    */
   virtual bool setupNode(bitmunk::node::Node* node);

   /**
    * Setup a node from this app as a peer.  Will read out the current config
    * and use it to set the Nodes config. The merge config is only merge data.
    *
    * @param node a Node to setup.
    * @param merge optional config data to add to the merge section.
    */
   virtual bool setupPeerNode(
      bitmunk::node::Node* node, monarch::config::Config* extraMerge = NULL);
};

/**
 * Macro to ease defining and starting a Bitmunk Tester.
 * NOTE: Surround this macro with #ifndef DB_TEST_NO_MAIN ... #endif.
 */
#define BM_TEST_MAIN(testClassName) BM_APP_PLUGIN_MAIN(testClassName)

} // end namespace test
} // end namespace bitmunk

#endif

/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk/node/NodeConfigManager.h"
#include "bitmunk/node/Node.h"
#include "bitmunk/test/Tester.h"
#include "monarch/test/Test.h"
#include "monarch/test/TestRunner.h"
#include "monarch/rt/DynamicObject.h"
#include "monarch/data/json/JsonWriter.h"

#include <inttypes.h>

using namespace std;
using namespace bitmunk::common;
using namespace bitmunk::node;
using namespace monarch::config;
using namespace monarch::data;
using namespace monarch::data::json;
using namespace monarch::io;
using namespace monarch::rt;
using namespace monarch::test;

void runConfigTest(Node& node, TestRunner& tr, bitmunk::test::Tester& tester)
{
   tr.group("config");
   
   tr.test("user configs");
   {
      /*{
         printf("config debug:\n");
         Config c = node.getConfigManager()->getDebugInfo();
         JsonWriter::writeToStdOut(c, false, false);
         assertNoException();
      }*/
      UserId userId;
      bool loggedin = node.login("devuser", "password", &userId);
      assertNoException();
      assert(loggedin);
      {
         printf("raw user %" PRIu64 " config:\n", userId);
         Config c = node.getConfigManager()->getUserConfig(userId, true);
         JsonWriter::writeToStdOut(c, false, false);
         assertNoException();
      }
      {
         printf("user %" PRIu64 " config:\n", userId);
         Config c = node.getConfigManager()->getUserConfig(userId);
         JsonWriter::writeToStdOut(c, false, false);
         assertNoException();
      }
      node.logout(userId);
      assertNoException();
   }
   tr.passIfNoException();
   
   tr.ungroup();
}

class BmConfigTester : public bitmunk::test::Tester
{
public:
   BmConfigTester()
   {
      setName("Config Tester");
   }

   /**
    * Run automatic unit tests.
    */
   virtual int runAutomaticTests(TestRunner& tr)
   {
      // FIXME:
#if 0
      // create a node
      Node node;
      {
         bool success;
         success = setupNode(&node);
         assertNoException();
         assert(success);
         success = setupPeerNode(&node);
         assertNoException();
         assert(success);
      }
      if(!node.start())
      {
         dumpException();
         exit(1);
      }
      
      // run test
      runConfigTest(node, tr, *this);
      
      // stop node
      node.stop();
#endif
      return 0;
   }

   /**
    * Runs interactive unit tests.
    */
   virtual int runInteractiveTests(TestRunner& tr)
   {
      return 0;
   }
};

#ifndef MO_TEST_NO_MAIN
BM_TEST_MAIN(BmConfigTester)
#endif

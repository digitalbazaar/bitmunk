/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk/node/NodeConfigManager.h"
#include "bitmunk/node/Node.h"
#include "bitmunk/test/Tester.h"
#include "monarch/test/Test.h"
#include "monarch/test/TestModule.h"
#include "monarch/data/json/JsonWriter.h"

using namespace std;
using namespace bitmunk::common;
using namespace bitmunk::node;
using namespace monarch::config;
using namespace monarch::data;
using namespace monarch::data::json;
using namespace monarch::io;
using namespace monarch::rt;
using namespace monarch::test;

static void runConfigTest(Node& node, TestRunner& tr)
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

static bool run(TestRunner& tr)
{
   if(tr.isDefaultEnabled())
   {
      // load bitmunk test config
      bool success = bitmunk::test::Tester::loadConfig(tr);
      assertNoException();
      assert(success);

      // load bitmunk node
      Node* node = bitmunk::test::Tester::loadNode(tr);
      assert(node != NULL);
      success = node->start();
      assertNoException();
      assert(success);

      // run config test
      runConfigTest(*node, tr);

      // unload bitmunk node
      node->stop();
      success = bitmunk::test::Tester::unloadNode(tr);
      assertNoException();
      assert(success);

      // unload bitmunk test config
      success = bitmunk::test::Tester::unloadConfig(tr);
      assertNoException();
      assert(success);
   }
   return true;
}

MO_TEST_MODULE_FN("bitmunk.tests.config.test", "1.0", run)

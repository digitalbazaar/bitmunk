/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk/test/Tester.h"
#include "monarch/test/Test.h"
#include "monarch/test/TestModule.h"


#include "bitmunk/node/NodeConfigManager.h"
#include "bitmunk/node/Node.h"
#include "bitmunk/test/Tester.h"
#include "monarch/test/Test.h"
#include "monarch/test/TestModule.h"
#include "monarch/data/json/JsonWriter.h"

using namespace std;
using namespace bitmunk::common;
using namespace bitmunk::node;
using namespace bitmunk::test;
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
      assertNoExceptionSet();
      assert(loggedin);
      {
         Config c = node.getConfigManager()->getUserConfig(userId, true);
         assert(!c.isNull());
         if(tr.getVerbosityLevel() > 1)
         {
            printf("raw user %" PRIu64 " config:\n", userId);
            JsonWriter::writeToStdOut(c, false, false);
            assertNoExceptionSet();
         }
      }
      {
         Config c = node.getConfigManager()->getUserConfig(userId);
         assert(!c.isNull());
         if(tr.getVerbosityLevel() > 1)
         {
            printf("user %" PRIu64 " config:\n", userId);
            JsonWriter::writeToStdOut(c, false, false);
            assertNoExceptionSet();
         }
      }
      node.logout(userId);
      assertNoExceptionSet();
   }
   tr.passIfNoException();

   tr.ungroup();
}

static bool run(TestRunner& tr)
{
   if(tr.isTestEnabled("login-required"))
   {
      // load bitmunk node
      Node* node = Tester::loadNode(tr);
      assertNoException(
         node->start());

      // run config test
      runConfigTest(*node, tr);

      // unload bitmunk node
      node->stop();
      Tester::unloadNode(tr);
      assertNoExceptionSet();
   }
   return true;
}

MO_TEST_MODULE_FN("bitmunk.tests.config.test", "1.0", run)

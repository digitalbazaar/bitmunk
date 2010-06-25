/*
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_CONSTANT_MACROS
#define __STDC_FORMAT_MACROS

#include "bitmunk/test/Tester.h"
#include "monarch/test/Test.h"
#include "monarch/test/TestModule.h"


#include "bitmunk/node/Node.h"
#include "bitmunk/test/Tester.h"
#include "monarch/data/json/JsonWriter.h"
#include "monarch/event/EventWaiter.h"
#include "monarch/io/File.h"
#include "monarch/rt/DynamicObject.h"
#include "monarch/rt/Exception.h"
#include "monarch/test/Test.h"
#include "monarch/test/TestModule.h"

using namespace std;
using namespace bitmunk::common;
using namespace bitmunk::node;
using namespace bitmunk::test;
using namespace monarch::config;
using namespace monarch::data::json;
using namespace monarch::event;
using namespace monarch::io;
using namespace monarch::net;
using namespace monarch::rt;
using namespace monarch::test;

#define TEST_USER_ID  UINT64_C(900)

namespace bm_tests_services_filesystem
{

static void fileBrowserTest(Node& node, TestRunner& tr)
{
   Messenger* messenger = node.getMessenger();
   string tempDirectory;
   Url filesUrl;
   // get the temporary directory location
   File::getTemporaryDirectory(tempDirectory);
   filesUrl.format("%s/api/3.2/filesystem/files?path=%s&nodeuser=%" PRIu64,
      messenger->getSelfUrl(true).c_str(), tempDirectory.c_str(), TEST_USER_ID);

   tr.test("get files (valid)");
   {
      // get a specific file from the media library
      DynamicObject receivedFileList;
      messenger->get(&filesUrl, receivedFileList, node.getDefaultUserId());
      assertNoException();

      // check to make sure there is at least one entry in the list of files
      //dumpDynamicObject(receivedFileList);
      assert(receivedFileList->length() > 1);
   }
   tr.passIfNoException();

   filesUrl.format("%s/api/3.2/filesystem/files?path=%s&nodeuser=%" PRIu64,
      messenger->getSelfUrl(true).c_str(), "/totallyInvalidDir", TEST_USER_ID);
   tr.test("get files (invalid)");
   {
      // get a specific file from the media library
      DynamicObject receivedFileList;
      messenger->get(&filesUrl, receivedFileList, node.getDefaultUserId());
      assertException();
   }
   tr.passIfException();
}

static bool run(TestRunner& tr)
{
   if(tr.isTestEnabled("fixme"))
   {
      // load and start node
      Node* node = Tester::loadNode(tr, "bpe");
      node->start();
      assertNoException();

      //config["bitmunk.catalog.CustomCatalog"]["uploadListings"] = false;

      // login customer support
      //node.logout(0);
      //node.login("BitmunkCustomerSupport", "password");
      //assertNoException();

      // run tests
      fileBrowserTest(*node, tr);

      // stop and unload node
      node->stop();
      Tester::unloadNode(tr);
   }

   return true;
};

} // end namespace

MO_TEST_MODULE_FN(
   "bitmunk.tests.services-filesystem.test", "1.0",
   bm_tests_services_filesystem::run)

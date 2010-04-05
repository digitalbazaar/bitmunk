/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_CONSTANT_MACROS
#define __STDC_FORMAT_MACROS

#include <inttypes.h>

#include "bitmunk/node/Node.h"
#include "bitmunk/test/Tester.h"
#include "monarch/data/json/JsonWriter.h"
#include "monarch/event/EventWaiter.h"
#include "monarch/io/File.h"
#include "monarch/rt/DynamicObject.h"
#include "monarch/rt/Exception.h"
#include "monarch/test/Test.h"
#include "monarch/test/TestRunner.h"

using namespace std;
using namespace bitmunk::common;
using namespace bitmunk::node;
using namespace monarch::config;
using namespace monarch::data::json;
using namespace monarch::event;
using namespace monarch::io;
using namespace monarch::net;
using namespace monarch::rt;
using namespace monarch::test;

#define TEST_USER_ID  UINT64_C(900)

void fileBrowserTest(Node& node, TestRunner& tr, bitmunk::test::Tester& tester)
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

class BmFileBrowserServicesTester : public bitmunk::test::Tester
{
public:
   BmFileBrowserServicesTester()
   {
      setName("Filesystem Services");
   }

   /**
    * Run automatic unit tests.
    */
   virtual int runAutomaticTests(TestRunner& tr)
   {
      // FIXME:
#if 0
      // create a client node for communicating
      Node node;
      {
         bool success;
         success = setupNode(&node);
         assertNoException();
         assert(success);
         Config config;
         config["node"]["modulePath"] = BPE_MODULES_DIRECTORY;
         config["bitmunk.catalog.CustomCatalog"]["uploadListings"] = false;
         success = setupPeerNode(&node, &config);
         assertNoException();
         assert(success);
      }
      if(!node.start())
      {
         dumpException();
         exit(1);
      }

      // login the devuser
      node.login("devuser", "password");
      assertNoException();

      // login the devuser
      //node.login("BitmunkCustomerSupport", "password");
      //assertNoException();

      // run tests
      fileBrowserTest(node, tr, *this);

      // logout of client node
      node.logout(0);
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
BM_TEST_MAIN(BmFileBrowserServicesTester)
#endif

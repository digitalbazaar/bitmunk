/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk-unit-tests.h"
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

#define TEST_USER_ID      (uint64_t)900
#define TEST_FILENAME     "2.mp3"
#define TEST_FILE_ID      "b79068aab92f78ba312d35286a77ea581037b109"
#define TEST_CONTENT_SIZE (uint64_t)2712402

static string sTestDataDir;

void mediaLibraryTest(Node& node, TestRunner& tr, bitmunk::test::Tester& tester)
{
   Messenger* messenger = node.getMessenger();
   Url filesUrl;
   filesUrl.format("%s/api/3.2/medialibrary/files?nodeuser=%" PRIu64,
      messenger->getSelfUrl(true).c_str(), TEST_USER_ID);
   Url fileUrl;
   fileUrl.format("%s/api/3.2/medialibrary/files/%s?nodeuser=%" PRIu64,
      messenger->getSelfUrl(true).c_str(), TEST_FILE_ID, TEST_USER_ID);

   // create the delete URL
   Url removeUrl;
   removeUrl.format(
      "%s/api/3.2/medialibrary/files/%s?nodeuser=%" PRIu64,
      messenger->getSelfUrl(true).c_str(), TEST_FILE_ID, TEST_USER_ID);

   // remove any previous files from the media library
   messenger->deleteResource(&removeUrl, NULL, node.getDefaultUserId());
   Exception::clear();

   tr.test("add file (valid)");
   {
      DynamicObject in;
      DynamicObject out;

      // create a FileInfo object
      out["id"] = TEST_FILE_ID;
      out["path"] = (sTestDataDir + TEST_FILENAME).c_str();
      out["mediaId"] = 2;
      out["contentType"] = "audio/mpeg";
      out["formatDetails"]->setType(Map);

      // add the file to the media library
      messenger->post(&filesUrl, &out, &in, node.getDefaultUserId());
   }
   tr.passIfNoException();

   tr.test("update pre-existing file (valid)");
   {
      DynamicObject in;
      DynamicObject out;

      // create a FileInfo object
      out["id"] = TEST_FILE_ID;
      out["path"] = (sTestDataDir + TEST_FILENAME).c_str();
      out["mediaId"] = 2;
      out["contentType"] = "audio/mpeg";
      out["formatDetails"]->setType(Map);

      // update file in the media library
      messenger->post(&filesUrl, &out, &in, node.getDefaultUserId());
   }
   tr.passIfNoException();

   tr.test("update file w/o file ID (invalid)");
   {
      DynamicObject in;
      DynamicObject out;

      // create a FileInfo object, do not set file ID
      out["path"] = "/tmp/notarealfile9832.mp3";
      out["mediaId"] = 2;

      // prepare event waiter
      EventWaiter ew(node.getEventController());
      ew.start("bitmunk.medialibrary.File.updated");
      ew.start("bitmunk.medialibrary.File.exception");

      // update file in the media library
      messenger->post(&filesUrl, &out, &in, node.getDefaultUserId());

      // wait for file ID set event
      assert(ew.waitForEvent(5*1000));

      // ensure it has an exception
      Event e = ew.popEvent();
      //dumpDynamicObject(e);
      if(e["details"]->hasMember("exception"))
      {
         ExceptionRef ex = Exception::convertToException(
            e["details"]["exception"]);
         Exception::set(ex);
      }
   }
   tr.passIfException();

   tr.test("add non-existent file (invalid)");
   {
      DynamicObject in;
      DynamicObject out;

      // create a FileInfo object
      out["path"] = (sTestDataDir + "this-file-should-not-exist.mp3").c_str();
      out["mediaId"] = 2;

      // prepare event waiter
      EventWaiter ew(node.getEventController());
      ew.start("bitmunk.medialibrary.File.updated");
      ew.start("bitmunk.medialibrary.File.exception");

      // try to add the invalid file to the media library
      messenger->post(&filesUrl, &out, &in, node.getDefaultUserId());

      // wait for file ID set event
      assert(ew.waitForEvent(5*1000));

      // ensure it has an exception
      Event e = ew.popEvent();
      //dumpDynamicObject(e);
      if(e["details"]->hasMember("exception"))
      {
         ExceptionRef ex = Exception::convertToException(
            e["details"]["exception"]);
         Exception::set(ex);
      }
   }
   tr.passIfException();

   tr.test("get file (valid)");
   {
      // Create the expected file info
      FileInfo expectedFileInfo;
      File file((sTestDataDir + TEST_FILENAME).c_str());
      expectedFileInfo["id"] = TEST_FILE_ID;
      expectedFileInfo["path"] = file->getAbsolutePath();
      expectedFileInfo["mediaId"] = 2;
      expectedFileInfo["extension"] = file->getExtension() + 1;
      expectedFileInfo["contentType"] = "audio/mpeg";
      expectedFileInfo["contentSize"] = TEST_CONTENT_SIZE;
      expectedFileInfo["size"] = (uint64_t)file->getLength();

      // get a specific file from the media library
      FileInfo receivedFileInfo;
      messenger->get(&fileUrl, receivedFileInfo, node.getDefaultUserId());
      assertNoException();

      // remove format details for comparison
      receivedFileInfo->removeMember("formatDetails");

      // check to make sure that the lists match and if not make it clear
      // why they don't
      if(expectedFileInfo != receivedFileInfo)
      {
         printf("\nExpected object: ");
         dumpDynamicObject(expectedFileInfo);
         printf("\nReceived object: ");
         dumpDynamicObject(receivedFileInfo);
      }
      assert(expectedFileInfo == receivedFileInfo);
   }
   tr.passIfNoException();

   // FIXME: Implement get multiple files as resource set test
   tr.test("get files via medialibrary search API (valid)");
   {
      Url searchFilesUrl;
      searchFilesUrl.format(
         "%s/api/3.2/medialibrary/files?nodeuser=%" PRIu64
         "&start=0&num=10&extension=mp3",
         messenger->getSelfUrl(true).c_str(), TEST_USER_ID);
      // Create the expected result set
      ResourceSet expectedFileSet;
      File file((sTestDataDir + TEST_FILENAME).c_str());
      expectedFileSet["resources"]->setType(Array);
      expectedFileSet["start"] = 0;
      expectedFileSet["num"] = 10;
      expectedFileSet["total"] = 1;
      DynamicObject& info = expectedFileSet["resources"]->append();
      FileInfo& expectedFileInfo = info["fileInfo"];
      expectedFileInfo->setType(Map);
      expectedFileInfo["id"] = TEST_FILE_ID;
      expectedFileInfo["path"] = file->getAbsolutePath();
      expectedFileInfo["mediaId"] = 2;
      expectedFileInfo["contentType"] = "audio/mpeg";
      expectedFileInfo["contentSize"] = TEST_CONTENT_SIZE;
      expectedFileInfo["size"] = (uint64_t)file->getLength();
      expectedFileInfo["extension"] = "mp3";

      // get a list of files from the media library
      FileInfo receivedFileSet;
      messenger->get(&searchFilesUrl, receivedFileSet, node.getDefaultUserId());
      assertNoException();

      // FIXME: this removes format details from the resources for an easier
      // comparison ... we probably want to actually check against these?
      DynamicObjectIterator i = receivedFileSet["resources"].getIterator();
      while(i->hasNext())
      {
         // FIXME: why do we have to key off of "fileInfo" here? that's not
         // how any of our other resource set stuff works...
         DynamicObject& next = i->next()["fileInfo"];
         next->removeMember("formatDetails");
      }

      // check to make sure that the resource sets match, and if not, make it
      // clear why they don't match
      if(expectedFileSet != receivedFileSet)
      {
         printf("\nExpected object: ");
         dumpDynamicObject(expectedFileSet);
         printf("\nReceived object: ");
         dumpDynamicObject(receivedFileSet);
      }
      assert(expectedFileSet == receivedFileSet);
   }
   tr.passIfNoException();

   tr.test("get files via medialibrary search API w/ media (valid)");
   {
      Url searchFilesUrl;
      searchFilesUrl.format(
         "%s/api/3.2/medialibrary/files?nodeuser=%" PRIu64 "&"
         "start=0&num=10&extension=mp3&media=true",
         messenger->getSelfUrl(true).c_str(), TEST_USER_ID);
      // Create the expected result set
      ResourceSet expectedFileSet;
      File file((sTestDataDir + "2.mp3").c_str());
      expectedFileSet["resources"]->setType(Array);
      expectedFileSet["start"] = 0;
      expectedFileSet["num"] = 10;
      expectedFileSet["total"] = 1;
      DynamicObject& info = expectedFileSet["resources"]->append();
      // File Info
      FileInfo& expectedFileInfo = info["fileInfo"];
      expectedFileInfo->setType(Map);
      expectedFileInfo["id"] = TEST_FILE_ID;
      expectedFileInfo["path"] = file->getAbsolutePath();
      expectedFileInfo["mediaId"] = 2;
      expectedFileInfo["contentType"] = "audio/mpeg";
      expectedFileInfo["contentSize"] = TEST_CONTENT_SIZE;
      expectedFileInfo["size"] = (uint64_t)file->getLength();
      expectedFileInfo["extension"] = "mp3";
      // Media Info
      DynamicObject& expectedMedia = info["media"];
      expectedMedia["id"] = 2;
      expectedMedia["type"] = "audio";
      expectedMedia["title"] = "Test Song 1";
      // Contributors
      DynamicObject& expectedContributors = expectedMedia["contributors"];
      expectedContributors["Composer"][0]["name"] = "Test Composer";
      expectedContributors["Copyright Owner"][0]["name"] = "Test Creator";
      expectedContributors["Performer"][0]["name"] = "Test Artist";
      expectedContributors["Publisher"][0]["name"] = "Test Publisher";

      // get a list of files from the media library
      FileInfo receivedFileSet;
      messenger->get(&searchFilesUrl, receivedFileSet, node.getDefaultUserId());
      assertNoException();

      // FIXME: this removes format details from the resources for an easier
      // comparison ... we probably want to actually check against these?
      DynamicObjectIterator i = receivedFileSet["resources"].getIterator();
      while(i->hasNext())
      {
         DynamicObject& next = i->next()["fileInfo"];
         next->removeMember("formatDetails");
      }

      // check to make sure that the resource sets match, and if not, make it
      // clear why they don't match
      if(expectedFileSet != receivedFileSet)
      {
         printf("\nExpected object: ");
         dumpDynamicObject(expectedFileSet);
         printf("\nReceived object: ");
         dumpDynamicObject(receivedFileSet);
      }
      assert(expectedFileSet == receivedFileSet);
   }
   tr.passIfNoException();

   tr.test("get files via medialibrary id API (valid)");
   {
      Url searchFilesUrl;
      searchFilesUrl.format(
         "%s/api/3.2/medialibrary/files?nodeuser=%" PRIu64 "&id=" TEST_FILE_ID,
         messenger->getSelfUrl(true).c_str(), TEST_USER_ID);
      // Create the expected result set
      ResourceSet expectedFileSet;
      File file((sTestDataDir + "2.mp3").c_str());
      expectedFileSet["resources"]->setType(Array);
      expectedFileSet["start"] = 0;
      expectedFileSet["num"] = 10;
      expectedFileSet["total"] = 1;
      DynamicObject& info = expectedFileSet["resources"]->append();
      // File Info
      FileInfo& expectedFileInfo = info["fileInfo"];
      expectedFileInfo->setType(Map);
      expectedFileInfo["id"] = TEST_FILE_ID;
      expectedFileInfo["path"] = file->getAbsolutePath();
      expectedFileInfo["mediaId"] = 2;
      expectedFileInfo["contentType"] = "audio/mpeg";
      expectedFileInfo["contentSize"] = TEST_CONTENT_SIZE;
      expectedFileInfo["size"] = (uint64_t)file->getLength();
      expectedFileInfo["extension"] = "mp3";

      // get a list of files from the media library
      FileInfo receivedFileSet;
      messenger->get(&searchFilesUrl, receivedFileSet, node.getDefaultUserId());
      assertNoException();

      // FIXME: this removes format details from the resources for an easier
      // comparison ... we probably want to actually check against these?
      DynamicObjectIterator i = receivedFileSet["resources"].getIterator();
      while(i->hasNext())
      {
         DynamicObject& next = i->next()["fileInfo"];
         next->removeMember("formatDetails");
      }

      // check to make sure that the resource sets match, and if not, make it
      // clear why they don't match
      if(expectedFileSet != receivedFileSet)
      {
         printf("\nExpected object: ");
         dumpDynamicObject(expectedFileSet);
         printf("\nReceived object: ");
         dumpDynamicObject(receivedFileSet);
      }
      assert(expectedFileSet == receivedFileSet);
   }
   tr.passIfNoException();

   tr.test("update file w/o file ID or media ID (valid)");
   {
      DynamicObject in;
      DynamicObject out;

      // create a FileInfo object, do not set file ID or media ID
      out["path"] = (sTestDataDir + TEST_FILENAME).c_str();
      out["mediaId"] = 0;

      // prepare event waiter
      EventWaiter ew(node.getEventController());
      ew.start("bitmunk.medialibrary.File.updated");
      ew.start("bitmunk.medialibrary.File.exception");

      // update file in the media library
      messenger->post(&filesUrl, &out, &in, node.getDefaultUserId());

      // wait for file update
      assert(ew.waitForEvent(5*1000));

      // ensure there was no exception
      Event e = ew.popEvent();
      //dumpDynamicObject(e);
      if(e["details"]->hasMember("exception"))
      {
         ExceptionRef ex = Exception::convertToException(
            e["details"]["exception"]);
         Exception::set(ex);
      }
      else
      {
         // ensure media ID was detected
         assert(e["details"]["fileInfo"]["mediaId"]->getUInt64() != 0);
      }
   }
   tr.passIfNoException();

   tr.test("remove new file (valid)");
   {
      // remove the file from the media library
      messenger->deleteResource(&removeUrl, NULL, node.getDefaultUserId());
   }
   tr.passIfNoException();
}

class BmMediaLibraryServicesTester : public bitmunk::test::Tester
{
public:
   BmMediaLibraryServicesTester()
   {
      setName("Media Library Services");
   }

   /**
    * Run automatic unit tests.
    */
   virtual int runAutomaticTests(TestRunner& tr)
   {
      printf("Note: You may see security breaches if the user's profile \n"
         "is not in the configured profile directory.\n");

      ConfigManager* cm = tr.getApp()->getConfigManager();
      Config cfg = cm->getConfig(BITMUNK_TESTER_CONFIG_ID);
      sTestDataDir = cfg["test"]["dataPath"]->getString();

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
      mediaLibraryTest(node, tr, *this);

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
BM_TEST_MAIN(BmMediaLibraryServicesTester)
#endif

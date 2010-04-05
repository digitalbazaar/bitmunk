/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk/node/Node.h"
#include "bitmunk/test/Tester.h"
#include "monarch/event/EventWaiter.h"
#include "monarch/rt/Exception.h"
#include "monarch/test/Test.h"
#include "monarch/test/TestRunner.h"
#include "monarch/rt/DynamicObject.h"
#include "monarch/data/json/JsonWriter.h"
#include "monarch/io/File.h"

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

#define TEST_USER_ID  (uint64_t)900

#define TEST_FILENAME_2     "2.mp3"
#define TEST_FILE_ID_2      "b79068aab92f78ba312d35286a77ea581037b109"
#define TEST_WARE_ID_2      "2-" TEST_FILE_ID_2
#define TEST_CONTENT_SIZE_2 (uint64_t)2712402

#define TEST_FILENAME_3 "3.mp3"
#define TEST_FILE_ID_3  "5E5A503FBD8217274819616929DA1C041C30B4FC"
#define TEST_WARE_ID_3  "3-" TEST_FILE_ID_3

static string sTestDataDir;

/**
 * This function resets the test environment by logging the user out,
 * deleting the database, and then logging the user back into the node.
 *
 * @param node the node to use when logging into and out of the node.
 */
static void resetTestEnvironment(Node& node)
{
   // get the path to the database file, dear sir
   Config config = node.getConfigManager()->getModuleUserConfig(
      "bitmunk.medialibrary.MediaLibrary", TEST_USER_ID);
   const char* url = config["database"]["url"]->getString();

   // assert that the translation happened correctly
   string fullDbPath;
   assert(node.getConfigManager()->translateUrlToUserFilePath(
      TEST_USER_ID, url, fullDbPath));

   // log the user out
   node.logout(TEST_USER_ID);
   assertNoException();

   // remove the file from the filesystem
   File file(fullDbPath.c_str());
   if(file->exists())
   {
      file->remove();
      assertNoException();
   }

   // log the user in
   node.login("devuser", "password");
   assertNoException();
}

void customCatalogTests(
   Node& node, TestRunner& tr, bitmunk::test::Tester& tester)
{
   resetTestEnvironment(node);

   Messenger* messenger = node.getMessenger();

   // generate the ware URLs that will be used throughout the test.
   Url waresUrl;
   waresUrl.format(
      "%s/api/3.0/catalog/wares?nodeuser=%" PRIu64,
      messenger->getSelfUrl(true).c_str(), TEST_USER_ID);
   Url waresNonDefaultListUrl;
   waresNonDefaultListUrl.format(
      "%s/api/3.0/catalog/wares?nodeuser=%" PRIu64 "&id=%s&default=false",
      messenger->getSelfUrl(true).c_str(), TEST_USER_ID, TEST_WARE_ID_2);
   Url wareUrl;
   wareUrl.format(
      "%s/api/3.0/catalog/wares/%s?nodeuser=%" PRIu64,
      messenger->getSelfUrl(true).c_str(), TEST_WARE_ID_2, TEST_USER_ID);

   // generate the files URL that will be used to prime the medialibrary
   Url filesUrl;
   filesUrl.format("%s/api/3.2/medialibrary/files?nodeuser=%" PRIu64,
      messenger->getSelfUrl(true).c_str(), TEST_USER_ID);
   Url removeUrl;
   removeUrl.format(
      "%s/api/3.2/medialibrary/files/%s?nodeuser=%" PRIu64,
      messenger->getSelfUrl(true).c_str(), TEST_FILE_ID_2, TEST_USER_ID);

   // Create basic and detailed test ware with id=2
   Ware basicWare2;
   Ware detailedWare2;
   {
      basicWare2["id"] = TEST_WARE_ID_2;
      basicWare2["description"] =
         "This ware was added by test-services-customcatalog";

      detailedWare2 = basicWare2.clone();
      detailedWare2["mediaId"] = 2;
      detailedWare2["fileInfos"]->setType(Array);
      FileInfo fi = detailedWare2["fileInfos"]->append();
      fi["id"] = TEST_FILE_ID_2;
      fi["mediaId"] = 2;
      File file((sTestDataDir + TEST_FILENAME_2).c_str());
      fi["extension"] = file->getExtension() + 1;
      fi["contentType"] = "audio/mpeg";
      fi["contentSize"] = TEST_CONTENT_SIZE_2;
      fi["size"] = (uint64_t)file->getLength();
      fi["path"] = file->getAbsolutePath();

      detailedWare2["payees"]->setType(Array);
      Payee p1 = detailedWare2["payees"]->append();
      Payee p2 = detailedWare2["payees"]->append();

      p1["id"] = 900;
      p1["amountType"] = PAYEE_AMOUNT_TYPE_FLATFEE;
      p1["amount"] = "0.10";
      p1["description"] = "This payee is for media ID 2";

      p2["id"] = 900;
      p2["amountType"] = PAYEE_AMOUNT_TYPE_PTOTAL;
      p2["percentage"] = "0.10";
      p2["description"] = "This payee is for media ID 2";
   }

   // remove any previous files from the media library
   messenger->deleteResource(&removeUrl, NULL, node.getDefaultUserId());
   // pass even if there is an exception - this is meant to just clear any
   // previous entries in the medialibrary database if they existed.
   Exception::clear();

   tr.group("customcatalog");

   tr.test("add file to medialibrary (valid)");
   {
      DynamicObject in;
      DynamicObject out;

      // create a FileInfo object
      string normalizedPath;
      File::normalizePath(
         (sTestDataDir + TEST_FILENAME_2).c_str(), normalizedPath);
      out["path"] = normalizedPath.c_str();
      out["mediaId"] = 2;

      // prepare event waiter
      EventWaiter ew(node.getEventController());
      ew.start("bitmunk.medialibrary.File.updated");
      ew.start("bitmunk.medialibrary.File.exception");

      // add the file to the media library
      messenger->post(&filesUrl, &out, &in, node.getDefaultUserId());
      assertNoException();

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
      else if(strcmp(
         e["type"]->getString(), "bitmunk.medialibrary.File.updated") == 0)
      {
         // add new mediaLibraryId to the basic and detailed wares
         basicWare2["mediaLibraryId"] = e["details"]["mediaLibraryId"];
         detailedWare2["mediaLibraryId"] = e["details"]["mediaLibraryId"];
      }
   }
   tr.passIfNoException();

   tr.test("add ware without payee-scheme (valid)");
   {
      DynamicObject in;
      DynamicObject out;

      // create the outgoing ware object
      FileInfo fi;
      fi["id"] = TEST_FILE_ID_2;

      out["id"] = TEST_WARE_ID_2;
      out["mediaId"] = 2;
      out["description"] = "This ware was added by test-services-customcatalog";
      out["fileInfo"] = fi;
      out["payees"]->setType(Array);
      Payee p1 = out["payees"]->append();
      Payee p2 = out["payees"]->append();

      p1["id"] = 900;
      p1["amountType"] = PAYEE_AMOUNT_TYPE_FLATFEE;
      p1["amount"] = "0.10";
      p1["description"] = "This payee is for media ID 2";

      p2["id"] = 900;
      p2["amountType"] = PAYEE_AMOUNT_TYPE_PTOTAL;
      p2["percentage"] = "0.10";
      p2["description"] = "This payee is for media ID 2";

      // add the ware to the custom catalog
      messenger->post(&waresUrl, &out, &in, node.getDefaultUserId());

      // set ware payeeSchemeIds
      basicWare2["payeeSchemeId"] = in["payeeSchemeId"];
      detailedWare2["payeeSchemeId"] = in["payeeSchemeId"];
   }
   tr.passIfNoException();

   tr.test("get ware (valid)");
   {
      // get a specific ware from the custom catalog
      Ware receivedWare;
      messenger->get(&wareUrl, receivedWare, node.getDefaultUserId());
      assertNoException();

      // remove format details for comparison
      if(receivedWare->hasMember("fileInfos") &&
         receivedWare["fileInfos"]->getType() == Array &&
         receivedWare["fileInfos"]->length() == 1)
      {
         receivedWare["fileInfos"][0]->removeMember("formatDetails");
      }

      // check to make sure that the wares match
      assertNamedDynoCmp(
         "Expected ware ResourceSet", detailedWare2,
         "Received ware ResourceSet", receivedWare);
   }
   tr.passIfNoException();

   tr.test("get wares");
   {
      // get a specific ware from the custom catalog
      Ware receivedWareSet;
      Url url;
      url.format(
         "%s/api/3.0/catalog/wares?nodeuser=%" PRIu64 "&id=%s",
         messenger->getSelfUrl(true).c_str(), TEST_USER_ID, TEST_WARE_ID_2);
      messenger->get(&url, receivedWareSet, node.getDefaultUserId());
      assertNoException();

      // check ware set
      Ware expectedWareSet;
      expectedWareSet["resources"][0] = basicWare2.clone();
      expectedWareSet["total"] = 1;
      expectedWareSet["num"] = 1;
      expectedWareSet["start"] = 0;

      assertNamedDynoCmp(
         "Expected ware ResourceSet", expectedWareSet,
         "Received ware ResourceSet", receivedWareSet);
   }
   tr.passIfNoException();

   tr.test("get wares by fileId");
   {
      // get a specific ware from the custom catalog
      Ware receivedWareSet;
      Url url;
      url.format(
         "%s/api/3.0/catalog/wares?nodeuser=%" PRIu64 "&fileId=%s",
         messenger->getSelfUrl(true).c_str(), TEST_USER_ID, TEST_FILE_ID_2);
      messenger->get(&url, receivedWareSet, node.getDefaultUserId());
      assertNoException();

      // check ware set
      Ware expectedWareSet;
      expectedWareSet["resources"][0] = basicWare2.clone();
      expectedWareSet["total"] = 1;
      expectedWareSet["num"] = 1;
      expectedWareSet["start"] = 0;

      assertNamedDynoCmp(
         "Expected ware ResourceSet", expectedWareSet,
         "Received ware ResourceSet", receivedWareSet);
   }
   tr.passIfNoException();

   tr.test("get specific unknown wares");
   {
      // get a specific ware from the custom catalog
      Ware receivedWareSet;
      Url url;
      url.format(
         "%s/api/3.0/catalog/wares?nodeuser=%" PRIu64 "&id=%s&id=INVALID",
         messenger->getSelfUrl(true).c_str(), TEST_USER_ID, TEST_WARE_ID_2);
      messenger->get(&url, receivedWareSet, node.getDefaultUserId());

      // check ware set
      Ware expectedWareSet;
      expectedWareSet["resources"][0] = basicWare2.clone();
      expectedWareSet["total"] = 1;
      expectedWareSet["num"] = 2;
      expectedWareSet["start"] = 0;

      assertNamedDynoCmp(
         "Expected ware ResourceSet", expectedWareSet,
         "Received ware ResourceSet", receivedWareSet);
   }
   tr.passIfNoException();

   tr.test("get wares by dup ware+file ids");
   {
      // This test gets the same ware by both ware id and file id and returns
      // duplicate results.

      // get a specific ware from the custom catalog
      Ware receivedWareSet;
      Url url;
      url.format(
         "%s/api/3.0/catalog/wares?nodeuser=%" PRIu64 "&id=%s&fileId=%s",
         messenger->getSelfUrl(true).c_str(), TEST_USER_ID,
         TEST_WARE_ID_2, TEST_FILE_ID_2);
      messenger->get(&url, receivedWareSet, node.getDefaultUserId());

      // check ware set
      Ware expectedWareSet;
      expectedWareSet["resources"][0] = basicWare2.clone();
      expectedWareSet["total"] = 1;
      expectedWareSet["num"] = 2;
      expectedWareSet["start"] = 0;

      assertNamedDynoCmp(
         "Expected ware ResourceSet", expectedWareSet,
         "Received ware ResourceSet", receivedWareSet);
   }
   tr.passIfNoException();

   tr.test("remove ware (valid)");
   {
      // remove the ware
      messenger->deleteResource(&wareUrl, NULL, node.getDefaultUserId());
   }
   tr.passIfNoException();

   /*************************** Payee Scheme tests **************************/
   // generate the payee schemes URL
   Url payeeSchemesUrl;
   payeeSchemesUrl.format("%s/api/3.0/catalog/payees/schemes?nodeuser=%" PRIu64,
      messenger->getSelfUrl(true).c_str(), TEST_USER_ID);
   PayeeSchemeId psId = 0;

   tr.test("add payee scheme (valid)");
   {
      DynamicObject in;
      DynamicObject out;

      // create the outgoing list of payees to organize into a payee scheme
      out["description"] = "Payee scheme description 1";
      out["payees"]->setType(Array);
      PayeeList payees = out["payees"];
      Payee p1 = payees->append();
      Payee p2 = payees->append();

      p1["id"] = 900;
      p1["amountType"] = PAYEE_AMOUNT_TYPE_FLATFEE;
      p1["amount"] = "0.10";
      p1["description"] = "test-services-customcatalog test payee 1";

      p2["id"] = 900;
      p2["amountType"] = PAYEE_AMOUNT_TYPE_PTOTAL;
      p2["percentage"] = "0.10";
      p2["description"] = "test-services-customcatalog test payee 2";

      // add the ware to the custom catalog
      messenger->post(&payeeSchemesUrl, &out, &in, node.getDefaultUserId());

      if(in->hasMember("payeeSchemeId"))
      {
         psId = in["payeeSchemeId"]->getUInt32();
      }
   }
   tr.passIfNoException();

   // generate the payee scheme URL
   Url payeeSchemeIdUrl;
   payeeSchemeIdUrl.format("%s/api/3.0/catalog/payees/schemes/%u?nodeuser=%"
      PRIu64,
      messenger->getSelfUrl(true).c_str(), psId, TEST_USER_ID);
   tr.test("get payee scheme (valid)");
   {
      // Create the expected payee scheme
      PayeeScheme expectedPayeeScheme;
      expectedPayeeScheme["id"] = psId;
      expectedPayeeScheme["userId"] = node.getDefaultUserId();
      expectedPayeeScheme["description"] = "Payee scheme description 1";
      expectedPayeeScheme["payees"]->setType(Array);
      Payee p1 = expectedPayeeScheme["payees"]->append();
      Payee p2 = expectedPayeeScheme["payees"]->append();

      p1["id"] = 900;
      p1["amountType"] = PAYEE_AMOUNT_TYPE_FLATFEE;
      p1["amount"] = "0.10";
      p1["description"] = "test-services-customcatalog test payee 1";

      p2["id"] = 900;
      p2["amountType"] = PAYEE_AMOUNT_TYPE_PTOTAL;
      p2["percentage"] = "0.10";
      p2["description"] = "test-services-customcatalog test payee 2";

      // get a specific payee scheme from the custom catalog
      PayeeScheme receivedPayeeScheme;
      messenger->get(
         &payeeSchemeIdUrl, receivedPayeeScheme, node.getDefaultUserId());

      // check payee schemes
      assertNamedDynoCmp(
         "Expected payee scheme", expectedPayeeScheme,
         "Received payee scheme", receivedPayeeScheme);
   }
   tr.passIfNoException();

   tr.test("get all payee schemes (valid)");
   {
      // Create the result set
      ResourceSet expectedResults;
      expectedResults["total"] = 2;
      expectedResults["start"] = 0;
      expectedResults["num"] = 2;

      PayeeScheme ps = expectedResults["resources"]->append();
      ps["id"] = 1;
      ps["userId"] = node.getDefaultUserId();
      ps["description"] = "This ware was added by test-services-customcatalog";
      ps["payees"]->setType(Array);
      Payee p1 = ps["payees"]->append();
      Payee p2 = ps["payees"]->append();

      p1["id"] = 900;
      p1["amountType"] = PAYEE_AMOUNT_TYPE_FLATFEE;
      p1["amount"] = "0.10";
      p1["description"] = "This payee is for media ID 2";

      p2["id"] = 900;
      p2["amountType"] = PAYEE_AMOUNT_TYPE_PTOTAL;
      p2["percentage"] = "0.10";
      p2["description"] = "This payee is for media ID 2";

      ps = expectedResults["resources"]->append();
      ps["id"] = 2;
      ps["userId"] = node.getDefaultUserId();
      ps["description"] = "Payee scheme description 1";
      ps["payees"]->setType(Array);
      Payee p3 = ps["payees"]->append();
      Payee p4 = ps["payees"]->append();

      p3["id"] = 900;
      p3["amountType"] = PAYEE_AMOUNT_TYPE_FLATFEE;
      p3["amount"] = "0.10";
      p3["description"] = "test-services-customcatalog test payee 1";

      p4["id"] = 900;
      p4["amountType"] = PAYEE_AMOUNT_TYPE_PTOTAL;
      p4["percentage"] = "0.10";
      p4["description"] = "test-services-customcatalog test payee 2";

      // get all payee schemes from the custom catalog
      ResourceSet receivedResults;
      messenger->get(
         &payeeSchemesUrl, receivedResults, node.getDefaultUserId());

      // check payee schemes
      assertNamedDynoCmp(
         "Expected payee scheme ResourceSet", expectedResults,
         "Received payee scheme ResourceSet", receivedResults);
   }
   tr.passIfNoException();

   tr.test("update payee scheme (valid)");
   {
      DynamicObject in;
      DynamicObject out;

      // create the outgoing list of payees to organize into a payee scheme
      out["description"] = "Payee scheme description 2";
      Payee p1 = out["payees"]->append();
      Payee p2 = out["payees"]->append();

      p1["id"] = 900;
      p1["amountType"] = PAYEE_AMOUNT_TYPE_FLATFEE;
      p1["amount"] = "0.15";
      p1["description"] = "test-services-customcatalog test payee 1 (updated)";

      p2["id"] = 900;
      p2["amountType"] = PAYEE_AMOUNT_TYPE_PTOTAL;
      p2["percentage"] = "0.14";
      p2["description"] = "test-services-customcatalog test payee 2 (updated)";

      // update a pre-existing payee scheme
      messenger->post(&payeeSchemeIdUrl, &out, &in, node.getDefaultUserId());
      assertNoException();

      // ensure that the payee scheme was updated
      assert(in["payeeSchemeId"]->getUInt32() == 2);

      if(in->hasMember("payeeSchemeId"))
      {
         psId = in["payeeSchemeId"]->getUInt32();
      }
   }
   tr.passIfNoException();

   tr.test("add ware with payee scheme (valid)");
   {
      DynamicObject in;
      DynamicObject out;

      // create the outgoing ware object
      FileInfo fi;
      fi["id"] = TEST_FILE_ID_2;
      out["id"] = TEST_WARE_ID_2;
      out["mediaId"] = 2;
      out["description"] = "This ware was added by test-services-customcatalog";
      out["fileInfo"] = fi;
      out["payeeSchemeId"] = psId;

      // add the ware to the custom catalog
      messenger->post(&waresUrl, &out, &in, node.getDefaultUserId());
   }
   tr.passIfNoException();

   tr.test("remove associated payee scheme (invalid)");
   {
      messenger->deleteResource(
         &payeeSchemeIdUrl, NULL, node.getDefaultUserId());
   }
   tr.passIfException();

   tr.test("remove ware associated w/ payee scheme (valid)");
   {
      messenger->deleteResource(
         &wareUrl, NULL, node.getDefaultUserId());
   }
   tr.passIfNoException();

   tr.test("remove payee scheme (valid)");
   {
      messenger->deleteResource(
         &payeeSchemeIdUrl, NULL, node.getDefaultUserId());
   }
   tr.passIfNoException();

   tr.test("create ware w/ invalid payee scheme (invalid)");
   {
      DynamicObject in;
      DynamicObject out;

      // create the outgoing ware object
      FileInfo fi;
      fi["id"] = TEST_FILE_ID_2;
      PayeeScheme ps;
      ps["id"] = psId;

      out["id"] = TEST_WARE_ID_2;
      out["mediaId"] = 2;
      out["description"] = "This ware was added by test-services-customcatalog";
      out["fileInfo"] = fi;
      out["payeeScheme"] = ps;

      // add the ware to the custom catalog
      messenger->post(&waresUrl, &out, &in, node.getDefaultUserId());
   }
   tr.passIfException();

   tr.test("remove ware (invalid)");
   {
      // remove a ware that doesn't exist
      messenger->deleteResource(&wareUrl, NULL, node.getDefaultUserId());
   }
   tr.passIfException();

   tr.ungroup();
}

void interactiveCustomCatalogTests(
   Node& node, TestRunner& tr, bitmunk::test::Tester& tester)
{
   Messenger* messenger = node.getMessenger();

   tr.group("customcatalog+listing updater");

   // generate the ware URLs that will be used throughout the test.
   Url waresUrl;
   waresUrl.format("%s/api/3.0/catalog/wares?nodeuser=%" PRIu64,
      messenger->getSelfUrl(true).c_str(), TEST_USER_ID);
   Url wareUrl;
   wareUrl.format("%s/api/3.0/catalog/wares/%s?nodeuser=%" PRIu64,
      messenger->getSelfUrl(true).c_str(), TEST_WARE_ID_2, TEST_USER_ID);

   // generate the files URL that will be used to prime the medialibrary
   Url filesUrl;
   filesUrl.format("%s/api/3.2/medialibrary/files?nodeuser=%" PRIu64,
      messenger->getSelfUrl(true).c_str(), TEST_USER_ID);
   Url removeUrl;
   removeUrl.format(
      "%s/api/3.2/medialibrary/files/%s?nodeuser=%" PRIu64,
      messenger->getSelfUrl(true).c_str(), TEST_FILE_ID_2, TEST_USER_ID);

   // remove any previous files from the media library
   messenger->deleteResource(&removeUrl, NULL, node.getDefaultUserId());
   // pass even if there is an exception - this is meant to just clear any
   // previous entries in the medialibrary database if they existed.
   Exception::clear();

   tr.test("add file to medialibrary (valid)");
   {
      DynamicObject in;
      DynamicObject out;

      // create a FileInfo object
      string normalizedPath;
      File::normalizePath(
         (sTestDataDir + TEST_FILENAME_2).c_str(), normalizedPath);
      out["path"] = normalizedPath.c_str();
      out["mediaId"] = 2;

      // prepare event waiter
      EventWaiter ew(node.getEventController());
      ew.start("bitmunk.medialibrary.File.updated");
      ew.start("bitmunk.medialibrary.File.exception");

      // add the file to the media library
      messenger->post(&filesUrl, &out, &in, node.getDefaultUserId());
      assertNoException();

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
   tr.passIfNoException();

   tr.test("add ware without payee-scheme (valid)");
   {
      DynamicObject in;
      DynamicObject out;

      // create the outgoing ware object
      FileInfo fi;
      fi["id"] = TEST_FILE_ID_2;

      out["id"] = TEST_WARE_ID_2;
      out["mediaId"] = 2;
      out["description"] = "This ware was added by test-services-customcatalog";
      out["fileInfo"] = fi;
      out["payees"]->setType(Array);
      Payee p1 = out["payees"]->append();
      Payee p2 = out["payees"]->append();

      p1["id"] = 900;
      p1["amountType"] = PAYEE_AMOUNT_TYPE_FLATFEE;
      p1["amount"] = "0.10";
      p1["description"] = "This payee is for media ID 2";

      p2["id"] = 900;
      p2["amountType"] = PAYEE_AMOUNT_TYPE_PTOTAL;
      p2["percentage"] = "0.10";
      p2["description"] = "This payee is for media ID 2";

      // add the ware to the custom catalog
      messenger->post(&waresUrl, &out, &in, node.getDefaultUserId());
   }
   tr.passIfNoException();

   printf("\nWaiting for server info to update...\n");
   Thread::sleep(2*1000);

   while(true)
   {
      tr.test("get server info");
      {
         Url serverUrl;
         serverUrl.format("%s/api/3.0/catalog/server?nodeuser=%" PRIu64,
            messenger->getSelfUrl(true).c_str(), TEST_USER_ID);
         DynamicObject in;
         messenger->get(&serverUrl, in, node.getDefaultUserId());
         dumpDynamicObject(in);
      }
      tr.passIfNoException();

      printf(
         "\nSleeping to allow listing updater to run. Hit CTRL+C to quit.\n");
      Thread::sleep(30*1000);
   }

   tr.ungroup();
}

class BmCustomCatalogServicesTester : public bitmunk::test::Tester
{
public:
   BmCustomCatalogServicesTester()
   {
      setName("Services");
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

      // run tests
      customCatalogTests(node, tr, *this);

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
      ConfigManager* cm = tr.getApp()->getConfigManager();
      Config cfg = cm->getConfig(BITMUNK_TESTER_CONFIG_ID);
      sTestDataDir = cfg["test"]["dataPath"]->getString();

      printf("Note: You may see security breaches if the user's profile \n"
         "is not in the configured profile directory.\n");
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
         config["bitmunk.catalog.CustomCatalog"]["uploadListings"] = true;
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

      // run tests
      interactiveCustomCatalogTests(node, tr, *this);

      // logout of client node
      node.logout(0);
      node.stop();
#endif
      return 0;
   }
};

#ifndef MO_TEST_NO_MAIN
BM_TEST_MAIN(BmCustomCatalogServicesTester)
#endif

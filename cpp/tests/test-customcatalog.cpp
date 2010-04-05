/*
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk-unit-tests.h"
#include "bitmunk/common/Logging.h"
#include "bitmunk/customcatalog/Catalog.h"
#include "bitmunk/data/FormatDetectorInputStream.h"
#include "bitmunk/node/Node.h"
#include "bitmunk/test/Tester.h"
#include "monarch/event/EventWaiter.h"
#include "monarch/io/FileInputStream.h"
#include "monarch/net/Url.h"
#include "monarch/rt/DynamicObject.h"
#include "monarch/rt/Exception.h"
#include "monarch/test/Test.h"
#include "monarch/test/TestRunner.h"
#include "monarch/util/StringTools.h"

using namespace std;
using namespace bitmunk::common;
using namespace bitmunk::customcatalog;
using namespace bitmunk::data;
using namespace bitmunk::node;
using namespace monarch::config;
using namespace monarch::data;
using namespace monarch::data::json;
using namespace monarch::event;
using namespace monarch::io;
using namespace monarch::net;
using namespace monarch::rt;
using namespace monarch::test;
using namespace monarch::util;

#define TEST_USER_ID      (uint64_t)900
#define TEST_ACCOUNT_ID   (uint64_t)9000
#define TEST_SERVER_ID    5
#define TEST_FILENAME     "2.mp3"
#define TEST_FILE_ID      "b79068aab92f78ba312d35286a77ea581037b109"
#define TEST_SERVER_TOKEN "123"
#define TEST_CONTENT_SIZE (uint64_t)2712402
#define TEST_PS_ID        1

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
   file->remove();

   // log the user in
   node.login("devuser", "password");
   assertNoException();
}

static void populateFormatDetails(FileInfo& fi)
{
   File file(fi["path"]->getString());
   FileInputStream* fis = new FileInputStream(file);
   InspectorInputStream* iis = new InspectorInputStream(fis, true);
   FormatDetectorInputStream* fdis =
      new FormatDetectorInputStream(iis, true);

   // fully inspect mp3 files
   fdis->getDataFormatInspector("bitmunk.data.MpegAudioDetector")->
      setKeepInspecting(true);

   // detect format details
   assert(fdis->detect());
   assert(fdis->isFormatRecognized());
   // get format details
   DynamicObject details = fdis->getFormatDetails();
   fi["formatDetails"] = details.first();
   fdis->close();

   assertNoException();

   delete fdis;
}

static Seller buildSeller()
{
   Seller seller;
   seller["userId"] = TEST_USER_ID;
   seller["serverId"] = TEST_SERVER_ID;
   seller["url"] = "http://localhost:19200/";
   return seller;
}

static SellerListingUpdate buildUpdate(uint32_t updateId, uint32_t newUpdateId)
{
   SellerListingUpdate update;
   update["seller"] = buildSeller();
   update["listings"]["updates"]->setType(Array);
   update["listings"]["removals"]->setType(Array);
   update["payeeSchemes"]["updates"]->setType(Array);
   update["payeeSchemes"]["removals"]->setType(Array);
   update["updateId"] = updateId;
   update["updateId"]->setType(String);
   update["newUpdateId"] = newUpdateId;
   update["newUpdateId"]->setType(String);
   update["serverToken"] = TEST_SERVER_TOKEN;
   return update;
}

static SellerListingUpdate buildResponse(uint32_t updateId)
{
   SellerListingUpdate update;
   update["seller"] = buildSeller();
   update["listings"]["updates"]->setType(Array);
   update["listings"]["removals"]->setType(Array);
   update["payeeSchemes"]["updates"]->setType(Array);
   update["payeeSchemes"]["removals"]->setType(Array);
   update["updateId"] = updateId;
   update["updateId"]->setType(String);
   update["serverToken"] = TEST_SERVER_TOKEN;
   return update;
}

static PayeeScheme buildPayeeScheme()
{
   PayeeScheme ps;
   ps["id"] = TEST_PS_ID;
   ps["description"] = "Test payee scheme";
   ps["userId"] = TEST_USER_ID;
   ps["serverId"] = TEST_SERVER_ID;

   PayeeList payees = ps["payees"];
   Payee p1 = payees->append();
   Payee p2 = payees->append();

   p1["id"] = TEST_ACCOUNT_ID;
   p1["amountType"] = PAYEE_AMOUNT_TYPE_FLATFEE;
   p1["amount"] = "0.10";
   p1["description"] = "This is a test-customcatalog payee 1.";

   p2["id"] = TEST_ACCOUNT_ID;
   p2["amountType"] = PAYEE_AMOUNT_TYPE_PTOTAL;
   p2["percentage"] = "0.10";
   p2["description"] = "This is a test-customcatalog payee 2.";

   return ps;
}

static Ware buildWare()
{
   File file((sTestDataDir + TEST_FILENAME).c_str());
   FileInfo fi;
   fi["id"] = TEST_FILE_ID;
   fi["path"] = file->getAbsolutePath();
   fi["mediaId"] = 2;
   fi["contentType"] = "audio/mpeg";
   fi["contentSize"] = TEST_CONTENT_SIZE;
   fi["size"] = (uint64_t)file->getLength();
   fi["extension"] = "mp3";
   populateFormatDetails(fi);

   Ware ware;
   ware["id"] = StringTools::format(
      "bitmunk:file:2-%s", fi["id"]->getString()).c_str();
   ware["mediaId"] = 2;
   ware["description"] = "This ware was added by test-customcatalog";
   ware["fileInfos"]->append(fi);
   ware["payeeSchemeId"] = TEST_PS_ID;

   return ware;
}

static DynamicObject buildListing(bool removal)
{
   File file((sTestDataDir + TEST_FILENAME).c_str());
   FileInfo fi;
   fi["id"] = TEST_FILE_ID;
   fi["mediaId"] = 2;
   if(!removal)
   {
      fi["path"] = file->getAbsolutePath();
      fi["contentType"] = "audio/mpeg";
      fi["contentSize"] = TEST_CONTENT_SIZE;
      fi["size"] = (uint64_t)file->getLength();
      fi["extension"] = "mp3";
      populateFormatDetails(fi);
      fi->removeMember("path");
   }

   DynamicObject listing;
   listing["fileInfo"] = fi;
   if(!removal)
   {
      listing["payeeSchemeId"] = TEST_PS_ID;
   }

   return listing;
}

#define dirtyPayeeScheme(cat) \
{ \
   PayeeScheme ps = buildPayeeScheme(); \
   PayeeSchemeId psId = TEST_PS_ID; \
   cat->updatePayeeScheme( \
      TEST_USER_ID, psId, "Test payee scheme", ps["payees"]); \
   assertNoException(); \
}

#define dirtyWare(cat) \
{ \
   Ware ware = buildWare(); \
   cat->updateWare(TEST_USER_ID, ware); \
   assertNoException();\
}

#define deletePayeeScheme(cat) \
{ \
   cat->removePayeeScheme(TEST_USER_ID, TEST_PS_ID); \
   assertNoException(); \
}

#define deleteWare(cat) \
{ \
   Ware ware = buildWare(); \
   cat->removeWare(TEST_USER_ID, ware); \
   assertNoException(); \
}

#define assertUpdate(update, expected) \
{ \
   DynamicObject differences; \
   uint32_t flags = \
      DynamicObject::DiffDefault | DynamicObject::DiffDoublesAsStrings; \
   if(expected.diff(update, differences, flags)) \
   { \
      printf("\nSellerListingUpdates do not match."); \
      printf("\nDifferences: "); \
      dumpDynamicObject(differences); \
      printf("\nExpected: "); \
      dumpDynamicObject(expected); \
      printf("\nReturned: "); \
      dumpDynamicObject(update); \
      assert(false); \
   } \
}

void runCustomCatalogTest(
   Node& node, TestRunner& tr, bitmunk::test::Tester& tester)
{
   // reset the test environment
   resetTestEnvironment(node);

   // get the custom catalog interface
   Catalog* cat = dynamic_cast<Catalog*>(
      node.getModuleApiByType("bitmunk.catalog"));
   assert(cat != NULL);
   // get the current user ID
   UserId userId = node.getDefaultUserId();

   tr.group("Database");

   // generate all of the wares that will be used in this test and register
   // them with the catalog
   Messenger* messenger = node.getMessenger();
   DynamicObject files;
   files->setType(Array);
   tr.test("populating media library");
   {
      Url filesUrl;
      filesUrl.format("%s/api/3.2/medialibrary/files?nodeuser=%" PRIu64,
         messenger->getSelfUrl(true).c_str(), TEST_USER_ID);

      // prepare event waiter
      EventWaiter ew(node.getEventController());
      ew.start("bitmunk.medialibrary.File.updated");
      ew.start("bitmunk.medialibrary.File.exception");

      // add each file ID 2-10, and print a dot after each successful iteration
      printf("\b");
      for(uint32_t i = 2; i <= 10; i++)
      {
         DynamicObject in;
         DynamicObject out;
         File file(
            StringTools::format("%s%u.mp3", sTestDataDir.c_str(), i).c_str());
         out["path"] = file->getAbsolutePath();
         out["mediaId"] = i;

         // add the file to the media library
         messenger->post(&filesUrl, &out, &in, node.getDefaultUserId());
         assertNoException();

         // print a dot for every file processed and flush stdout
         printf(".");
         fflush(0);
      }

      while(files->length() < 9)
      {
         // wait for event
         assert(ew.waitForEvent(5*1000));

         Event e = ew.popEvent();
         while(!e.isNull())
         {
            // ensure event has no exception
            if(e["details"]->hasMember("exception"))
            {
               ExceptionRef ex = Exception::convertToException(
                  e["details"]["exception"]);
               Exception::set(ex);
               assertNoException();
            }
            else
            {
               // get file info from event
               files->append(e["details"]["fileInfo"]);

               // get next event
               e = ew.popEvent();
            }
         }
      }
   }
   tr.passIfNoException();

   // populate the wares catalog
   DynamicObject wares;
   wares->setType(Array);
   tr.test("populating customcatalog");
   {
      printf("\b");
      FileInfoIterator fii = files.getIterator();
      while(fii->hasNext())
      {
         FileInfo fi = fii->next();

         // create ware object
         Ware ware;
         ware["id"] = StringTools::format(
            "bitmunk:file:%" PRIu64 "-%s",
            BM_MEDIA_ID(fi["mediaId"]),
            BM_FILE_ID(fi["id"])).c_str();
         BM_ID_SET(ware["mediaId"], BM_MEDIA_ID(fi["mediaId"]));
         ware["description"] = "This ware was added by test-customcatalog";
         ware["fileInfos"]->append(fi);
         ware["payees"]->setType(Array);
         Payee p1 = ware["payees"]->append();
         Payee p2 = ware["payees"]->append();

         BM_ID_SET(p1["id"], TEST_ACCOUNT_ID);
         p1["amountType"] = PAYEE_AMOUNT_TYPE_FLATFEE;
         p1["amount"] = "0.10";
         p1["description"] = "This is a test-customcatalog payee 1.";

         BM_ID_SET(p2["id"], TEST_ACCOUNT_ID);
         p2["amountType"] = PAYEE_AMOUNT_TYPE_PTOTAL;
         p2["percentage"] = "0.10";
         p1["description"] = "This is a test-customcatalog payee 2.";

         // add the ware to the custom catalog
         cat->updateWare(TEST_USER_ID, ware);
         assertNoException();

         wares->append(ware);

         // print a dot for every file processed and flush stdout
         printf(".");
         fflush(0);
      }
   }
   tr.passIfNoException();

   // FIXME: Remove this code once stuff is working - dump the files and wares
   //printf("------------------- FILES --------------------");
   //JsonWriter::writeToStdOut(files);
   //printf("------------------- WARES --------------------");
   //JsonWriter::writeToStdOut(wares);

   // create the default seller configuration
   Seller seller = buildSeller();
   const char* serverToken = TEST_SERVER_TOKEN;

   tr.test("set the seller configuration");
   {
      if(!cat->updateSeller(userId, seller, serverToken))
      {
         ExceptionRef e = new Exception(
            "The seller configuration failed to be updated.",
            "bitmunk.test.customcatalog.UpdateSellerConfigFailed");
         Exception::push(e);
      }
   }
   tr.passIfNoException();

   tr.test("get the seller configuration");
   {
      Seller receivedSeller;
      string receivedServerToken;

      // check to see if the seller information was stored correctly
      if(cat->populateSeller(userId, receivedSeller, receivedServerToken))
      {
         if(seller != receivedSeller)
         {
            printf("\nExpected object: ");
            dumpDynamicObject(seller);
            printf("\nReceived object: ");
            dumpDynamicObject(receivedSeller);
         }
         assert(seller == receivedSeller);
         assertStrCmp(serverToken, receivedServerToken.c_str());
      }
      else
      {
         ExceptionRef e = new Exception(
            "Failed to populate the seller of the catalog.",
            "bitmunk.test.customcatalog.PopulateSellerConfigFailed");
         Exception::push(e);
      }
   }
   tr.passIfNoException();

   tr.test("get listing updates (10 payee schemes)");
   {
      // build the expected update
      SellerListingUpdate expectedUpdate = buildUpdate(0, 1);

      // build each expected PayeeScheme in the Seller Listing updates array
      for(uint32_t psId = 1; psId < 10; psId++)
      {
         PayeeScheme& ps = expectedUpdate["payeeSchemes"]["updates"]->append();

         ps["description"] = "This ware was added by test-customcatalog";
         ps["id"] = psId;
         ps["serverId"] = TEST_SERVER_ID;
         ps["userId"] = TEST_USER_ID;
         ps["payees"]->setType(Array);
         Payee& p1 = ps["payees"]->append();
         p1["amount"] = "0.10";
         p1["amountType"] = "flatFee";
         p1["description"] = "This is a test-customcatalog payee 2.";
         p1["id"] = TEST_ACCOUNT_ID;
         Payee& p2 = ps["payees"]->append();
         p2["amountType"] = "percentOfTotal";
         p2["description"] = "";
         p2["id"] = TEST_ACCOUNT_ID;
         p2["percentage"] = "0.10";
      }

      SellerListingUpdate update;
      if(cat->populateNextListingUpdate(userId, update))
      {
         assertUpdate(update, expectedUpdate);

         // build an update response
         SellerListingUpdate response = buildResponse(1);

         // process update response
         cat->processListingUpdateResponse(userId, update, response);
         assertNoException();
      }
      else
      {
         ExceptionRef e = new Exception(
            "The database failed to generate the next listings update.",
            "bitmunk.test.customcatalog.PopulateNextListingUpdateFailed");
         Exception::push(e);
      }
   }
   tr.passIfNoException();

   tr.test("get listing updates (10 listings)");
   {
      // build the expected update
      SellerListingUpdate expectedUpdate = buildUpdate(1, 2);

      // build each expected Ware in the SellerListing update array
      uint32_t psId = 1;
      FileInfoIterator fii = files.getIterator();
      while(fii->hasNext())
      {
         FileInfo& fi = fii->next();
         FileInfo clonedFileInfo;
         SellerListing& sl = expectedUpdate["listings"]["updates"]->append();

         // copy only the fields necessary from the file info
         clonedFileInfo["id"] = fi["id"].clone();
         clonedFileInfo["path"] = fi["path"].clone();
         clonedFileInfo["mediaId"] = fi["mediaId"].clone();

         // populate the format details for the file info
         populateFormatDetails(clonedFileInfo);

         // modify the exactMpegAudioTime to round correctly because the
         // comparison function below will fail due to rounding issues if
         // we don't do this.
         clonedFileInfo["formatDetails"]["exactMpegAudioTime"]->setType(String);
         clonedFileInfo["formatDetails"]["exactMpegAudioTime"]->setType(Double);

         // populate some of the other details for the file info
         clonedFileInfo["contentType"] = "audio/mpeg";
         clonedFileInfo["contentSize"] = fi["contentSize"]->getUInt64();
         clonedFileInfo["extension"] = "mp3";
         File file(clonedFileInfo["path"]->getString());
         clonedFileInfo["size"] = (uint64_t)file->getLength();

         // remove the path, which isn't reported in the get next seller listing
         // call
         clonedFileInfo->removeMember("path");

         // update the SellerListing item
         sl["fileInfo"] = clonedFileInfo;
         sl["payeeSchemeId"] = psId++;
      }

      SellerListingUpdate update;
      if(cat->populateNextListingUpdate(userId, update))
      {
         assertUpdate(update, expectedUpdate);
      }
      else
      {
         ExceptionRef e = new Exception(
            "The database failed to generate the next listings update.",
            "bitmunk.test.customcatalog.PopulateNextListingUpdateFailed");
         Exception::push(e);
      }
   }
   tr.passIfNoException();

   tr.test("get pending listing updates");
   {
      // build the expected update
      SellerListingUpdate expectedUpdate = buildUpdate(2, 2);

      // build each expected Ware in the SellerListing update array
      uint32_t psId = 1;
      FileInfoIterator fii = files.getIterator();
      while(fii->hasNext())
      {
         FileInfo& fi = fii->next();
         FileInfo clonedFileInfo;
         SellerListing& sl = expectedUpdate["listings"]["updates"]->append();

         // copy only the fields necessary from the file info
         clonedFileInfo["id"] = fi["id"].clone();
         clonedFileInfo["path"] = fi["path"].clone();
         clonedFileInfo["mediaId"] = fi["mediaId"].clone();

         // populate the format details for the file info
         populateFormatDetails(clonedFileInfo);

         // modify the exactMpegAudioTime to round correctly because the
         // comparison function below will fail due to rounding issues if
         // we don't do this.
         clonedFileInfo["formatDetails"]["exactMpegAudioTime"]->setType(String);
         clonedFileInfo["formatDetails"]["exactMpegAudioTime"]->setType(Double);

         // populate some of the other details for the file info
         clonedFileInfo["contentType"] = "audio/mpeg";
         clonedFileInfo["contentSize"] = fi["contentSize"]->getUInt64();
         clonedFileInfo["extension"] = "mp3";
         File file(clonedFileInfo["path"]->getString());
         clonedFileInfo["size"] = (uint64_t)file->getLength();

         // remove the path, which isn't reported in the get next seller listing
         // call
         clonedFileInfo->removeMember("path");

         // update the SellerListing item
         sl["fileInfo"] = clonedFileInfo;
         sl["payeeSchemeId"] = psId++;
      }

      SellerListingUpdate update;
      if(cat->populatePendingListingUpdate(userId, update))
      {
         assertUpdate(update, expectedUpdate);
      }
      else
      {
         ExceptionRef e = new Exception(
            "The database failed to generate the pending listings update.",
            "bitmunk.test.customcatalog.PopulatePendingListingUpdateFailed");
         Exception::push(e);
      }
   }
   tr.passIfNoException();

   tr.test("process listing updates (no errors)");
   {
      SellerListingUpdate update = buildUpdate(2, 2);
      SellerListingUpdate response = buildResponse(2);

      if(!cat->processListingUpdateResponse(userId, update, response))
      {
         ExceptionRef e = new Exception(
            "The database failed to process the listings update response.",
            "bitmunk.test.customcatalog.ProcessListingUpdateResponseFailed");
         Exception::push(e);
      }
   }
   tr.passIfNoException();

   tr.test("get listing updates (heartbeat)");
   {
      // build the expected update
      SellerListingUpdate expectedUpdate = buildUpdate(2, 2);

      SellerListingUpdate update;
      if(cat->populateNextListingUpdate(userId, update))
      {
         assertUpdate(update, expectedUpdate);
      }
      else
      {
         ExceptionRef e = new Exception(
            "The database failed to generate the next listings update.",
            "bitmunk.test.customcatalog.PopulateNextListingUpdateFailed");
         Exception::push(e);
      }
   }
   tr.passIfNoException();

   tr.test("process listing updates (ware failure)");
   {
      // dirty a ware and get the update for it
      dirtyWare(cat);
      SellerListingUpdate update;
      cat->populateNextListingUpdate(userId, update);
      assertNoException();

      // ensure there's only one ware in the update
      SellerListingUpdate expectedUpdate = buildUpdate(
         update["updateId"]->getUInt32(),
         update["newUpdateId"]->getUInt32());
      expectedUpdate["listings"]["updates"]->append() = buildListing(false);
      assertUpdate(update, expectedUpdate);

      // pretend there was a failure with the ware
      SellerListingUpdate response = buildResponse(
         update["newUpdateId"]->getUInt32());
      SellerListing& listing = response["listings"]["updates"]->append();
      listing["fileInfo"]["mediaId"] = 2;
      listing["fileInfo"]["id"] = TEST_FILE_ID;
      ExceptionRef e = new Exception(
         "Fake exception generated due to testing catalog sync failure.",
         "bitmunk.mastercatalog.FakeException");
      listing["exception"] = Exception::convertToDynamicObject(e);

      if(!cat->processListingUpdateResponse(userId, update, response))
      {
         ExceptionRef e = new Exception(
            "The database failed to process the listings update response.",
            "bitmunk.test.customcatalog.ProcessListingUpdateResponseFailed");
         Exception::push(e);
      }
   }
   tr.passIfNoException();

   tr.test("process listing updates (payee scheme failure)");
   {
      // dirty a payee scheme and get the update for it
      dirtyPayeeScheme(cat);
      SellerListingUpdate update;
      cat->populateNextListingUpdate(userId, update);
      assertNoException();

      // ensure there's only one payee scheme in the update
      SellerListingUpdate expectedUpdate = buildUpdate(
         update["updateId"]->getUInt32(),
         update["newUpdateId"]->getUInt32());
      expectedUpdate["payeeSchemes"]["updates"]->append() = buildPayeeScheme();
      assertUpdate(update, expectedUpdate);

      // pretend there was a failure with the payee scheme
      SellerListingUpdate response = buildResponse(
         update["newUpdateId"]->getUInt32());
      PayeeScheme& ps = response["payeeSchemes"]["updates"]->append();
      ps["userId"] = TEST_USER_ID;
      ps["serverId"] = 1;
      ps["id"] = 1;
      ExceptionRef e = new Exception(
         "Fake exception generated due to testing catalog sync failure.",
         "bitmunk.mastercatalog.FakeException");
      ps["exception"] = Exception::convertToDynamicObject(e);

      if(!cat->processListingUpdateResponse(userId, update, response))
      {
         ExceptionRef e = new Exception(
            "The database failed to process the listings update response.",
            "bitmunk.test.customcatalog.ProcessListingUpdateResponseFailed");
         Exception::push(e);
      }
   }
   tr.passIfNoException();

   tr.ungroup();
}

/* All update simulated crash test states:
 *
 * STARTING CONDITION => (UPDATE,CRASH) => ACTIONS => (DO ANOTHER UPDATE)
 *
 * clean ps, clean w => no action
 * clean ps, clean w => dirty ps
 * clean ps, clean w => delete ps (IMPOSSIBLE)
 * clean ps, clean w => dirty w
 * clean ps, clean w => delete w
 * clean ps, clean w => dirty ps, dirty w
 * clean ps, clean w => dirty ps, delete w
 * clean ps, clean w => delete ps, dirty w (IMPOSSIBLE)
 * clean ps, clean w => delete ps, delete w
 *
 * clean ps, dirty w => no action
 * clean ps, dirty w => dirty ps
 * clean ps, dirty w => delete ps (IMPOSSIBLE)
 * clean ps, dirty w => dirty w
 * clean ps, dirty w => delete w
 * clean ps, dirty w => dirty ps, dirty w
 * clean ps, dirty w => dirty ps, delete w
 * clean ps, dirty w => delete ps, dirty w (IMPOSSIBLE)
 * clean ps, dirty w => delete ps, delete w
 *
 * clean ps, deleted w => no action
 * clean ps, deleted w => dirty ps
 * clean ps, deleted w => delete ps
 * clean ps, deleted w => dirty w
 * clean ps, deleted w => delete w (IMPOSSIBLE)
 * clean ps, deleted w => dirty ps, dirty w
 * clean ps, deleted w => dirty ps, delete w (IMPOSSIBLE)
 * clean ps, deleted w => delete ps, dirty w (IMPOSSIBLE)
 * clean ps, deleted w => delete ps, delete w (IMPOSSIBLE)
 *
 * dirty ps, clean w => no action
 * dirty ps, clean w => dirty ps
 * dirty ps, clean w => delete ps (IMPOSSIBLE)
 * dirty ps, clean w => dirty w
 * dirty ps, clean w => delete w
 * dirty ps, clean w => dirty ps, dirty w
 * dirty ps, clean w => dirty ps, delete w
 * dirty ps, clean w => delete ps, dirty w (IMPOSSIBLE)
 * dirty ps, clean w => delete ps, delete w
 *
 * dirty ps, dirty w => no action
 * dirty ps, dirty w => dirty ps
 * dirty ps, dirty w => delete ps (IMPOSSIBLE)
 * dirty ps, dirty w => dirty w
 * dirty ps, dirty w => delete w
 * dirty ps, dirty w => dirty ps, dirty w
 * dirty ps, dirty w => dirty ps, delete w
 * dirty ps, dirty w => delete ps, dirty w (IMPOSSIBLE)
 * dirty ps, dirty w => delete ps, delete w
 *
 * dirty ps, deleted w => no action
 * dirty ps, deleted w => dirty ps
 * dirty ps, deleted w => delete ps
 * dirty ps, deleted w => dirty w
 * dirty ps, deleted w => delete w (IMPOSSIBLE)
 * dirty ps, deleted w => dirty ps, dirty w
 * dirty ps, deleted w => dirty ps, delete w (IMPOSSIBLE)
 * dirty ps, deleted w => delete ps, dirty w (IMPOSSIBLE)
 * dirty ps, deleted w => delete ps, delete w (IMPOSSIBLE)
 *
 * deleted ps, clean w (IMPOSSIBLE)
 * deleted ps, dirty w (IMPOSSIBLE)
 *
 * deleted ps, deleted w => no action
 * deleted ps, deleted w => dirty ps (IMPOSSIBLE)
 * deleted ps, deleted w => delete ps (IMPOSSIBLE)
 * deleted ps, deleted w => dirty w (IMPOSSIBLE)
 * deleted ps, deleted w => delete w (IMPOSSIBLE)
 * deleted ps, deleted w => dirty ps, dirty w (IMPOSSIBLE)
 * deleted ps, deleted w => dirty ps, delete w (IMPOSSIBLE)
 * deleted ps, deleted w => delete ps, clean w (IMPOSSIBLE)
 * deleted ps, deleted w => delete ps, dirty w (IMPOSSIBLE)
 * deleted ps, deleted w => delete ps, delete w (IMPOSSIBLE)
 *
 * Actually possible simulated update crash tests:
 *
 * clean ps, clean w => no action
 * clean ps, clean w => dirty ps
 * clean ps, clean w => dirty w
 * clean ps, clean w => delete w
 * clean ps, clean w => dirty ps, dirty w
 * clean ps, clean w => dirty ps, delete w
 * clean ps, clean w => delete ps, delete w
 *
 * clean ps, dirty w => no action
 * clean ps, dirty w => dirty ps
 * clean ps, dirty w => dirty w
 * clean ps, dirty w => delete w
 * clean ps, dirty w => dirty ps, dirty w
 * clean ps, dirty w => dirty ps, delete w
 * clean ps, dirty w => delete ps, delete w
 *
 * clean ps, deleted w => no action
 * clean ps, deleted w => dirty ps
 * clean ps, deleted w => delete ps
 * clean ps, deleted w => dirty w
 * clean ps, deleted w => dirty ps, dirty w
 *
 * dirty ps, clean w => no action
 * dirty ps, clean w => dirty ps
 * dirty ps, clean w => dirty w
 * dirty ps, clean w => delete w
 * dirty ps, clean w => dirty ps, dirty w
 * dirty ps, clean w => dirty ps, delete w
 * dirty ps, clean w => delete ps, delete w
 *
 * dirty ps, dirty w => no action
 * dirty ps, dirty w => dirty ps
 * dirty ps, dirty w => dirty w
 * dirty ps, dirty w => delete w
 * dirty ps, dirty w => dirty ps, delete w
 * dirty ps, dirty w => delete ps, delete w
 *
 * dirty ps, deleted w => no action
 * dirty ps, deleted w => dirty ps
 * dirty ps, deleted w => delete ps
 * dirty ps, deleted w => dirty w
 * dirty ps, deleted w => dirty ps, dirty w
 *
 * deleted ps, deleted w => no action
 */

/* Needs:
 *
 * initCleanPsCleanW()
 * initCleanPsDirtyW()
 * initCleanPsDeletedW()
 * initDirtyPsCleanW()
 * initDirtyPsDirtyW()
 * initDirtyPsDeletedW()
 * initDeletedPsDeletedW()
 *
 * dirtyPs()
 * dirtyW()
 * deletePs()
 * deleteW()
 */

void initializeCatalog(Node& node, Catalog* cat)
{
   // delete the customcatalog and medialibrary database and re-initialize them
   resetTestEnvironment(node);

   Messenger* messenger = node.getMessenger();
   Url filesUrl;
   filesUrl.format("%s/api/3.2/medialibrary/files?nodeuser=%" PRIu64,
      messenger->getSelfUrl(true).c_str(), TEST_USER_ID);

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
   assertNoException();

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
      assert(BM_MEDIA_ID_VALID(
         BM_MEDIA_ID(e["details"]["fileInfo"]["mediaId"])));
   }
   assertNoException();

   // create the default seller configuration
   Seller seller;
   BM_ID_SET(seller["userId"], node.getDefaultUserId());
   BM_ID_SET(seller["serverId"], TEST_SERVER_ID);
   seller["url"] = "http://localhost:19200/";
   const char* serverToken = TEST_SERVER_TOKEN;
   cat->updateSeller(TEST_USER_ID, seller, serverToken);
   assertNoException();
}

static void insertPayeeScheme(Catalog* cat)
{
   PayeeScheme ps = buildPayeeScheme();
   PayeeSchemeId psId = 0;
   cat->updatePayeeScheme(
      TEST_USER_ID, psId, "Test payee scheme", ps["payees"]);
   assertNoException();
}

static void insertWare(Catalog* cat)
{
   Ware ware = buildWare();
   cat->updateWare(TEST_USER_ID, ware);
   assertNoException();
}

static void cleanPayeeScheme(Catalog* cat, uint32_t& updateId)
{
   // get the next update
   SellerListingUpdate update;
   cat->populateNextListingUpdate(TEST_USER_ID, update);
   assertNoException();

   // build the expected update
   SellerListingUpdate expected = buildUpdate(updateId, updateId + 1);
   expected["payeeSchemes"]["updates"]->append() = buildPayeeScheme();
   assertUpdate(update, expected);

   // build the response
   updateId++;
   SellerListingUpdate response = buildResponse(updateId);

   // process the response
   cat->processListingUpdateResponse(TEST_USER_ID, update, response);
   assertNoException();
}

static void cleanWare(Catalog* cat, uint32_t& updateId)
{
   // get the next update
   SellerListingUpdate update;
   cat->populateNextListingUpdate(TEST_USER_ID, update);
   assertNoException();

   // build the expected update
   SellerListingUpdate expected = buildUpdate(updateId, updateId + 1);
   expected["listings"]["updates"]->append() = buildListing(false);
   assertUpdate(update, expected);

   // build the response
   updateId++;
   SellerListingUpdate response = buildResponse(updateId);

   // process the response
   cat->processListingUpdateResponse(TEST_USER_ID, update, response);
   assertNoException();

   // get the next update (should be empty now)
   SellerListingUpdate empty;
   cat->populateNextListingUpdate(TEST_USER_ID, empty);
   assertNoException();

   SellerListingUpdate expectedEmpty = buildUpdate(updateId, updateId);
   assertUpdate(empty, expectedEmpty);
}

void initCleanPsCleanW(Catalog* cat, uint32_t& updateId)
{
   // insert a payee scheme and a ware
   insertPayeeScheme(cat);
   insertWare(cat);

   // clean the payee scheme and the ware
   cleanPayeeScheme(cat, updateId);
   cleanWare(cat, updateId);

   // get the next update
   SellerListingUpdate update;
   cat->populateNextListingUpdate(TEST_USER_ID, update);
   assertNoException();

   // expect an empty update
   SellerListingUpdate expected = buildUpdate(updateId, updateId);
   assertUpdate(update, expected);
}

void initCleanPsDirtyW(Catalog* cat, uint32_t& updateId)
{
   // insert a payee scheme and a ware
   insertPayeeScheme(cat);
   insertWare(cat);

   // clean the payee scheme
   cleanPayeeScheme(cat, updateId);

   // get the next update
   SellerListingUpdate update;
   cat->populateNextListingUpdate(TEST_USER_ID, update);
   assertNoException();

   // expect an update with a listing
   SellerListingUpdate expected = buildUpdate(updateId, updateId + 1);
   expected["listings"]["updates"]->append() = buildListing(false);
   assertUpdate(update, expected);
}

void initCleanPsDeletedW(Catalog* cat, uint32_t& updateId)
{
   // insert a payee scheme and a ware
   insertPayeeScheme(cat);
   insertWare(cat);

   // clean the payee scheme and delete the ware
   cleanPayeeScheme(cat, updateId);
   deleteWare(cat);

   // get the next update
   SellerListingUpdate update;
   cat->populateNextListingUpdate(TEST_USER_ID, update);
   assertNoException();

   // expect an update with a listing
   SellerListingUpdate expected = buildUpdate(updateId, updateId + 1);
   expected["listings"]["removals"]->append() = buildListing(true);
   assertUpdate(update, expected);
}

void initDirtyPsCleanW(Catalog* cat, uint32_t& updateId)
{
   // insert a payee scheme and a ware
   insertPayeeScheme(cat);
   insertWare(cat);

   // clean the payee scheme and the ware
   cleanPayeeScheme(cat, updateId);
   cleanWare(cat, updateId);

   // dirty the payee scheme
   dirtyPayeeScheme(cat);

   // get the next update
   SellerListingUpdate update;
   cat->populateNextListingUpdate(TEST_USER_ID, update);
   assertNoException();

   // expect an update with a payee scheme
   SellerListingUpdate expected = buildUpdate(updateId, updateId + 1);
   expected["payeeSchemes"]["updates"]->append() = buildPayeeScheme();
   assertUpdate(update, expected);
}

void initDirtyPsDirtyW(Catalog* cat, uint32_t& updateId)
{
   // insert a payee scheme and a ware
   insertPayeeScheme(cat);
   insertWare(cat);

   // get the next update
   SellerListingUpdate update;
   cat->populateNextListingUpdate(TEST_USER_ID, update);
   assertNoException();

   // expect an update with a payee scheme
   SellerListingUpdate expected = buildUpdate(updateId, updateId + 1);
   expected["payeeSchemes"]["updates"]->append() = buildPayeeScheme();
   assertUpdate(update, expected);
}

void initDirtyPsDeletedW(Catalog* cat, uint32_t& updateId)
{
   // insert a payee scheme and a ware
   insertPayeeScheme(cat);
   insertWare(cat);

   // delete the ware
   deleteWare(cat);

   // get the next update
   SellerListingUpdate update;
   cat->populateNextListingUpdate(TEST_USER_ID, update);
   assertNoException();

   // expect an update with a listing
   SellerListingUpdate expected = buildUpdate(updateId, updateId + 1);
   expected["listings"]["removals"]->append() = buildListing(true);
   assertUpdate(update, expected);
}

void initDeletedPsDeletedW(Catalog* cat, uint32_t& updateId)
{
   // insert a payee scheme and a ware
   insertPayeeScheme(cat);
   insertWare(cat);

   // delete the ware and the payee scheme
   deleteWare(cat);
   deletePayeeScheme(cat);

   // get the next update
   SellerListingUpdate update;
   cat->populateNextListingUpdate(TEST_USER_ID, update);
   assertNoException();

   // expect an update with a listing
   SellerListingUpdate expected = buildUpdate(updateId, updateId + 1);
   expected["listings"]["removals"]->append() = buildListing(true);
   assertUpdate(update, expected);
}

void runUpdateCrashTest(
   Node& node, Catalog* cat, TestRunner& tr, bitmunk::test::Tester& tester)
{
   tr.group("update crash test");

   tr.group("clean ps, clean w");

   tr.test("=> no action");
   {
      uint32_t updateId = 0;
      initializeCatalog(node, cat);
      initCleanPsCleanW(cat, updateId);

      // *FAKE CRASH BEFORE SERVER HIT, NO RESPONSE*

      // get the next update again
      {
         SellerListingUpdate update;
         cat->populateNextListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an empty update
         SellerListingUpdate expected = buildUpdate(updateId, updateId);
         assertUpdate(update, expected);
      }

      // *FAKE CRASH AFTER SERVER HIT, NO RESPONSE*

      // no pending update here
   }
   tr.passIfNoException();

   tr.test("=> dirty ps");
   {
      uint32_t updateId = 0;
      initializeCatalog(node, cat);
      initCleanPsCleanW(cat, updateId);

      // *FAKE CRASH BEFORE SERVER HIT, NO RESPONSE*

      // actions:
      dirtyPayeeScheme(cat);

      // get the next update again
      {
         SellerListingUpdate update;
         cat->populateNextListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an update with a payee scheme
         SellerListingUpdate expected = buildUpdate(updateId, updateId + 1);
         expected["payeeSchemes"]["updates"]->append() = buildPayeeScheme();
         assertUpdate(update, expected);
      }

      // *FAKE CRASH AFTER SERVER HIT, NO RESPONSE*

      // get the pending update
      {
         SellerListing update;
         cat->populatePendingListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an update with a payee scheme
         SellerListingUpdate expected = buildUpdate(updateId + 1, updateId + 1);
         expected["payeeSchemes"]["updates"]->append() = buildPayeeScheme();
         assertUpdate(update, expected);
      }
   }
   tr.passIfNoException();

   tr.test("=> dirty w");
   {
      uint32_t updateId = 0;
      initializeCatalog(node, cat);
      initCleanPsCleanW(cat, updateId);

      // *FAKE CRASH BEFORE SERVER HIT, NO RESPONSE*

      // actions:
      dirtyWare(cat);

      // get the next update again
      {
         SellerListingUpdate update;
         cat->populateNextListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an update with a listing
         SellerListingUpdate expected = buildUpdate(updateId, updateId + 1);
         expected["listings"]["updates"]->append() = buildListing(false);
         assertUpdate(update, expected);
      }

      // *FAKE CRASH AFTER SERVER HIT, NO RESPONSE*

      // get the pending update
      {
         SellerListing update;
         cat->populatePendingListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an update with a listing
         SellerListingUpdate expected = buildUpdate(updateId + 1, updateId + 1);
         expected["listings"]["updates"]->append() = buildListing(false);
         assertUpdate(update, expected);
      }
   }
   tr.passIfNoException();

   tr.test("=> delete w");
   {
      uint32_t updateId = 0;
      initializeCatalog(node, cat);
      initCleanPsCleanW(cat, updateId);

      // *FAKE CRASH BEFORE SERVER HIT, NO RESPONSE*

      // actions:
      deleteWare(cat);

      // get the next update again
      {
         SellerListingUpdate update;
         cat->populateNextListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an update with a listing
         SellerListingUpdate expected = buildUpdate(updateId, updateId + 1);
         expected["listings"]["removals"]->append() = buildListing(true);
         assertUpdate(update, expected);
      }

      // *FAKE CRASH AFTER SERVER HIT, NO RESPONSE*

      // get the pending update
      {
         SellerListing update;
         cat->populatePendingListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an update with a listing
         SellerListingUpdate expected = buildUpdate(updateId + 1, updateId + 1);
         expected["listings"]["removals"]->append() = buildListing(true);
         assertUpdate(update, expected);
      }
   }
   tr.passIfNoException();

   tr.test("=> dirty ps, dirty w");
   {
      uint32_t updateId = 0;
      initializeCatalog(node, cat);
      initCleanPsCleanW(cat, updateId);

      // *FAKE CRASH BEFORE SERVER HIT, NO RESPONSE*

      // actions:
      dirtyWare(cat);
      dirtyPayeeScheme(cat);

      // get the next update again
      {
         SellerListingUpdate update;
         cat->populateNextListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an update with a payee scheme
         SellerListingUpdate expected = buildUpdate(updateId, updateId + 1);
         expected["payeeSchemes"]["updates"]->append() = buildPayeeScheme();
         assertUpdate(update, expected);
      }

      // *FAKE CRASH AFTER SERVER HIT, NO RESPONSE*

      // get the pending update
      {
         SellerListing update;
         cat->populatePendingListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an update with a payee scheme
         SellerListingUpdate expected = buildUpdate(updateId + 1, updateId + 1);
         expected["payeeSchemes"]["updates"]->append() = buildPayeeScheme();
         assertUpdate(update, expected);
      }
   }
   tr.passIfNoException();

   tr.test("=> dirty ps, delete w");
   {
      uint32_t updateId = 0;
      initializeCatalog(node, cat);
      initCleanPsCleanW(cat, updateId);

      // *FAKE CRASH BEFORE SERVER HIT, NO RESPONSE*

      // actions:
      dirtyPayeeScheme(cat);
      deleteWare(cat);

      // get the next update again
      {
         SellerListingUpdate update;
         cat->populateNextListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an update with a listing
         SellerListingUpdate expected = buildUpdate(updateId, updateId + 1);
         expected["listings"]["removals"]->append() = buildListing(true);
         assertUpdate(update, expected);
      }

      // *FAKE CRASH AFTER SERVER HIT, NO RESPONSE*

      // get the pending update
      {
         SellerListing update;
         cat->populatePendingListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an update with a listing
         SellerListingUpdate expected = buildUpdate(updateId + 1, updateId + 1);
         expected["listings"]["removals"]->append() = buildListing(true);
         assertUpdate(update, expected);
      }
   }
   tr.passIfNoException();

   tr.test("=> delete ps, delete w");
   {
      uint32_t updateId = 0;
      initializeCatalog(node, cat);
      initCleanPsCleanW(cat, updateId);

      // *FAKE CRASH BEFORE SERVER HIT, NO RESPONSE*

      // actions:
      deleteWare(cat);
      deletePayeeScheme(cat);

      // get the next update again
      {
         SellerListingUpdate update;
         cat->populateNextListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an update with a listing
         SellerListingUpdate expected = buildUpdate(updateId, updateId + 1);
         expected["listings"]["removals"]->append() = buildListing(true);
         assertUpdate(update, expected);
      }

      // *FAKE CRASH AFTER SERVER HIT, NO RESPONSE*

      // get the pending update
      {
         SellerListing update;
         cat->populatePendingListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an update with a listing
         SellerListingUpdate expected = buildUpdate(updateId + 1, updateId + 1);
         expected["listings"]["removals"]->append() = buildListing(true);
         assertUpdate(update, expected);
      }
   }
   tr.passIfNoException();

   tr.ungroup();

   tr.group("clean ps, dirty w");

   tr.test("=> no action");
   {
      uint32_t updateId = 0;
      initializeCatalog(node, cat);
      initCleanPsDirtyW(cat, updateId);

      // *FAKE CRASH BEFORE SERVER HIT, NO RESPONSE*

      // get the next update again
      {
         SellerListingUpdate update;
         cat->populateNextListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an update with a listing
         SellerListingUpdate expected = buildUpdate(updateId, updateId + 1);
         expected["listings"]["updates"]->append() = buildListing(false);
         assertUpdate(update, expected);
      }

      // *FAKE CRASH AFTER SERVER HIT, NO RESPONSE*

      // get the pending update
      {
         SellerListing update;
         cat->populatePendingListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an update with a listing
         SellerListingUpdate expected = buildUpdate(updateId + 1, updateId + 1);
         expected["listings"]["updates"]->append() = buildListing(false);
         assertUpdate(update, expected);
      }
   }
   tr.passIfNoException();

   tr.test("=> dirty ps");
   {
      uint32_t updateId = 0;
      initializeCatalog(node, cat);
      initCleanPsDirtyW(cat, updateId);

      // *FAKE CRASH BEFORE SERVER HIT, NO RESPONSE*

      // actions:
      dirtyPayeeScheme(cat);

      // get the next update again
      {
         SellerListingUpdate update;
         cat->populateNextListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an update with a listing
         SellerListingUpdate expected = buildUpdate(updateId, updateId + 1);
         expected["listings"]["updates"]->append() = buildListing(false);
         assertUpdate(update, expected);
      }

      // *FAKE CRASH AFTER SERVER HIT, NO RESPONSE*

      // get the pending update
      {
         SellerListing update;
         cat->populatePendingListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an update with a listing
         SellerListingUpdate expected = buildUpdate(updateId + 1, updateId + 1);
         expected["listings"]["updates"]->append() = buildListing(false);
         assertUpdate(update, expected);
      }
   }
   tr.passIfNoException();

   tr.test("=> dirty w");
   {
      uint32_t updateId = 0;
      initializeCatalog(node, cat);
      initCleanPsDirtyW(cat, updateId);

      // *FAKE CRASH BEFORE SERVER HIT, NO RESPONSE*

      // actions:
      dirtyWare(cat);

      // get the next update again
      {
         SellerListingUpdate update;
         cat->populateNextListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an empty update
         SellerListingUpdate expected = buildUpdate(updateId, updateId);
         assertUpdate(update, expected);
      }

      // *FAKE CRASH AFTER SERVER HIT, NO RESPONSE*

      // no pending update
   }
   tr.passIfNoException();

   tr.test("=> delete w");
   {
      uint32_t updateId = 0;
      initializeCatalog(node, cat);
      initCleanPsDirtyW(cat, updateId);

      // *FAKE CRASH BEFORE SERVER HIT, NO RESPONSE*

      // actions:
      deleteWare(cat);

      // get the next update again
      {
         SellerListingUpdate update;
         cat->populateNextListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an empty update
         SellerListingUpdate expected = buildUpdate(updateId, updateId);
         assertUpdate(update, expected);
      }

      // *FAKE CRASH AFTER SERVER HIT, NO RESPONSE*

      // no pending update
   }
   tr.passIfNoException();

   tr.test("=> dirty ps, dirty w");
   {
      uint32_t updateId = 0;
      initializeCatalog(node, cat);
      initCleanPsDirtyW(cat, updateId);

      // *FAKE CRASH BEFORE SERVER HIT, NO RESPONSE*

      // actions:
      dirtyWare(cat);
      dirtyPayeeScheme(cat);

      // get the next update again
      {
         SellerListingUpdate update;
         cat->populateNextListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an empty update
         SellerListingUpdate expected = buildUpdate(updateId, updateId);
         assertUpdate(update, expected);
      }

      // *FAKE CRASH AFTER SERVER HIT, NO RESPONSE*

      // no pending update
   }
   tr.passIfNoException();

   tr.test("=> dirty ps, delete w");
   {
      uint32_t updateId = 0;
      initializeCatalog(node, cat);
      initCleanPsDirtyW(cat, updateId);

      // *FAKE CRASH BEFORE SERVER HIT, NO RESPONSE*

      // actions:
      dirtyPayeeScheme(cat);
      deleteWare(cat);

      // get the next update again
      {
         SellerListingUpdate update;
         cat->populateNextListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an empty update
         SellerListingUpdate expected = buildUpdate(updateId, updateId);
         assertUpdate(update, expected);
      }

      // *FAKE CRASH AFTER SERVER HIT, NO RESPONSE*

      // no pending update
   }
   tr.passIfNoException();

   tr.test("=> delete ps, delete w");
   {
      uint32_t updateId = 0;
      initializeCatalog(node, cat);
      initCleanPsDirtyW(cat, updateId);

      // *FAKE CRASH BEFORE SERVER HIT, NO RESPONSE*

      // actions:
      deleteWare(cat);
      deletePayeeScheme(cat);

      // get the next update again
      {
         SellerListingUpdate update;
         cat->populateNextListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an empty update
         SellerListingUpdate expected = buildUpdate(updateId, updateId);
         assertUpdate(update, expected);
      }

      // *FAKE CRASH AFTER SERVER HIT, NO RESPONSE*

      // no pending update
   }
   tr.passIfNoException();

   tr.ungroup();

   tr.group("clean ps, deleted w");

   tr.test("=> no action");
   {
      uint32_t updateId = 0;
      initializeCatalog(node, cat);
      initCleanPsDeletedW(cat, updateId);

      // *FAKE CRASH BEFORE SERVER HIT, NO RESPONSE*

      // get the next update again
      {
         SellerListingUpdate update;
         cat->populateNextListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an update with a listing
         SellerListingUpdate expected = buildUpdate(updateId, updateId + 1);
         expected["listings"]["removals"]->append() = buildListing(true);
         assertUpdate(update, expected);
      }

      // *FAKE CRASH AFTER SERVER HIT, NO RESPONSE*

      // get the pending update
      {
         SellerListing update;
         cat->populatePendingListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an update with a listing
         SellerListingUpdate expected = buildUpdate(updateId + 1, updateId + 1);
         expected["listings"]["removals"]->append() = buildListing(true);
         assertUpdate(update, expected);
      }
   }
   tr.passIfNoException();

   tr.test("=> dirty ps");
   {
      uint32_t updateId = 0;
      initializeCatalog(node, cat);
      initCleanPsDeletedW(cat, updateId);

      // *FAKE CRASH BEFORE SERVER HIT, NO RESPONSE*

      // actions:
      dirtyPayeeScheme(cat);

      // get the next update again
      {
         SellerListingUpdate update;
         cat->populateNextListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an update with a listing
         SellerListingUpdate expected = buildUpdate(updateId, updateId + 1);
         expected["listings"]["removals"]->append() = buildListing(true);
         assertUpdate(update, expected);
      }

      // *FAKE CRASH AFTER SERVER HIT, NO RESPONSE*

      // get the pending update
      {
         SellerListing update;
         cat->populatePendingListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an update with a listing
         SellerListingUpdate expected = buildUpdate(updateId + 1, updateId + 1);
         expected["listings"]["removals"]->append() = buildListing(true);
         assertUpdate(update, expected);
      }
   }
   tr.passIfNoException();

   tr.test("=> delete ps");
   {
      uint32_t updateId = 0;
      initializeCatalog(node, cat);
      initCleanPsDeletedW(cat, updateId);

      // *FAKE CRASH BEFORE SERVER HIT, NO RESPONSE*

      // actions:
      deletePayeeScheme(cat);

      // get the next update again
      {
         SellerListingUpdate update;
         cat->populateNextListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an update with a listing
         SellerListingUpdate expected = buildUpdate(updateId, updateId + 1);
         expected["listings"]["removals"]->append() = buildListing(true);
         assertUpdate(update, expected);
      }

      // *FAKE CRASH AFTER SERVER HIT, NO RESPONSE*

      // get the pending update
      {
         SellerListing update;
         cat->populatePendingListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an update with a listing
         SellerListingUpdate expected = buildUpdate(updateId + 1, updateId + 1);
         expected["listings"]["removals"]->append() = buildListing(true);
         assertUpdate(update, expected);
      }
   }
   tr.passIfNoException();

   tr.test("=> dirty w");
   {
      uint32_t updateId = 0;
      initializeCatalog(node, cat);
      initCleanPsDeletedW(cat, updateId);

      // *FAKE CRASH BEFORE SERVER HIT, NO RESPONSE*

      // actions:
      dirtyWare(cat);

      // get the next update again
      {
         SellerListingUpdate update;
         cat->populateNextListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an empty update
         SellerListingUpdate expected = buildUpdate(updateId, updateId);
         assertUpdate(update, expected);
      }

      // *FAKE CRASH AFTER SERVER HIT, NO RESPONSE*

      // no pending update
   }
   tr.passIfNoException();

   tr.test("=> dirty ps, dirty w");
   {
      uint32_t updateId = 0;
      initializeCatalog(node, cat);
      initCleanPsDeletedW(cat, updateId);

      // *FAKE CRASH BEFORE SERVER HIT, NO RESPONSE*

      // actions:
      dirtyPayeeScheme(cat);
      dirtyWare(cat);

      // get the next update again
      {
         SellerListingUpdate update;
         cat->populateNextListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an empty update
         SellerListingUpdate expected = buildUpdate(updateId, updateId);
         assertUpdate(update, expected);
      }

      // *FAKE CRASH AFTER SERVER HIT, NO RESPONSE*

      // no pending update
   }
   tr.passIfNoException();

   tr.ungroup();

   tr.group("dirty ps, clean w");

   tr.test("=> no action");
   {
      uint32_t updateId = 0;
      initializeCatalog(node, cat);
      initDirtyPsCleanW(cat, updateId);

      // *FAKE CRASH BEFORE SERVER HIT, NO RESPONSE*

      // get the next update again
      {
         SellerListingUpdate update;
         cat->populateNextListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an update with a payee scheme
         SellerListingUpdate expected = buildUpdate(updateId, updateId + 1);
         expected["payeeSchemes"]["updates"]->append() = buildPayeeScheme();
         assertUpdate(update, expected);
      }

      // *FAKE CRASH AFTER SERVER HIT, NO RESPONSE*

      // get the pending update
      {
         SellerListing update;
         cat->populatePendingListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an update with a payee scheme
         SellerListingUpdate expected = buildUpdate(updateId + 1, updateId + 1);
         expected["payeeSchemes"]["updates"]->append() = buildPayeeScheme();
         assertUpdate(update, expected);
      }
   }
   tr.passIfNoException();

   tr.test("=> dirty ps");
   {
      uint32_t updateId = 0;
      initializeCatalog(node, cat);
      initDirtyPsCleanW(cat, updateId);

      // *FAKE CRASH BEFORE SERVER HIT, NO RESPONSE*

      // actions:
      dirtyPayeeScheme(cat);

      // get the next update again
      {
         SellerListingUpdate update;
         cat->populateNextListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an empty update
         SellerListingUpdate expected = buildUpdate(updateId, updateId);
         assertUpdate(update, expected);
      }

      // *FAKE CRASH AFTER SERVER HIT, NO RESPONSE*

      // no pending update
   }
   tr.passIfNoException();

   tr.test("=> dirty w");
   {
      uint32_t updateId = 0;
      initializeCatalog(node, cat);
      initDirtyPsCleanW(cat, updateId);

      // *FAKE CRASH BEFORE SERVER HIT, NO RESPONSE*

      // actions:
      dirtyWare(cat);

      // get the next update again
      {
         SellerListingUpdate update;
         cat->populateNextListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an update with a payee scheme
         SellerListingUpdate expected = buildUpdate(updateId, updateId + 1);
         expected["payeeSchemes"]["updates"]->append() = buildPayeeScheme();
         assertUpdate(update, expected);
      }

      // *FAKE CRASH AFTER SERVER HIT, NO RESPONSE*

      // get the pending update
      {
         SellerListing update;
         cat->populatePendingListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an update with a payee scheme
         SellerListingUpdate expected = buildUpdate(updateId + 1, updateId + 1);
         expected["payeeSchemes"]["updates"]->append() = buildPayeeScheme();
         assertUpdate(update, expected);
      }
   }
   tr.passIfNoException();

   tr.test("=> delete w");
   {
      uint32_t updateId = 0;
      initializeCatalog(node, cat);
      initDirtyPsCleanW(cat, updateId);

      // *FAKE CRASH BEFORE SERVER HIT, NO RESPONSE*

      // actions:
      deleteWare(cat);

      // get the next update again
      {
         SellerListingUpdate update;
         cat->populateNextListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an update with a payee scheme
         SellerListingUpdate expected = buildUpdate(updateId, updateId + 1);
         expected["payeeSchemes"]["updates"]->append() = buildPayeeScheme();
         assertUpdate(update, expected);
      }

      // *FAKE CRASH AFTER SERVER HIT, NO RESPONSE*

      // get the pending update
      {
         SellerListing update;
         cat->populatePendingListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an update with a payee scheme
         SellerListingUpdate expected = buildUpdate(updateId + 1, updateId + 1);
         expected["payeeSchemes"]["updates"]->append() = buildPayeeScheme();
         assertUpdate(update, expected);
      }
   }
   tr.passIfNoException();

   tr.test("=> dirty ps, dirty w");
   {
      uint32_t updateId = 0;
      initializeCatalog(node, cat);
      initDirtyPsCleanW(cat, updateId);

      // *FAKE CRASH BEFORE SERVER HIT, NO RESPONSE*

      // actions:
      dirtyPayeeScheme(cat);
      dirtyWare(cat);

      // get the next update again
      {
         SellerListingUpdate update;
         cat->populateNextListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an empty update
         SellerListingUpdate expected = buildUpdate(updateId, updateId);
         assertUpdate(update, expected);
      }

      // *FAKE CRASH AFTER SERVER HIT, NO RESPONSE*

      // no pending update
   }
   tr.passIfNoException();

   tr.test("=> dirty ps, delete w");
   {
      uint32_t updateId = 0;
      initializeCatalog(node, cat);
      initDirtyPsCleanW(cat, updateId);

      // *FAKE CRASH BEFORE SERVER HIT, NO RESPONSE*

      // actions:
      dirtyPayeeScheme(cat);
      deleteWare(cat);

      // get the next update again
      {
         SellerListingUpdate update;
         cat->populateNextListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an empty update
         SellerListingUpdate expected = buildUpdate(updateId, updateId);
         assertUpdate(update, expected);
      }

      // *FAKE CRASH AFTER SERVER HIT, NO RESPONSE*

      // no pending update
   }
   tr.passIfNoException();

   tr.test("=> delete ps, delete w");
   {
      uint32_t updateId = 0;
      initializeCatalog(node, cat);
      initDirtyPsCleanW(cat, updateId);

      // *FAKE CRASH BEFORE SERVER HIT, NO RESPONSE*

      // actions:
      deleteWare(cat);
      deletePayeeScheme(cat);

      // get the next update again
      {
         SellerListingUpdate update;
         cat->populateNextListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an empty update
         SellerListingUpdate expected = buildUpdate(updateId, updateId);
         assertUpdate(update, expected);
      }

      // *FAKE CRASH AFTER SERVER HIT, NO RESPONSE*

      // no pending update
   }
   tr.passIfNoException();

   tr.ungroup();

   tr.group("dirty ps, dirty w");

   tr.test("=> no action");
   {
      uint32_t updateId = 0;
      initializeCatalog(node, cat);
      initDirtyPsDirtyW(cat, updateId);

      // *FAKE CRASH BEFORE SERVER HIT, NO RESPONSE*

      // actions:

      // get the next update again
      {
         SellerListingUpdate update;
         cat->populateNextListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an update with a payee scheme
         SellerListingUpdate expected = buildUpdate(updateId, updateId + 1);
         expected["payeeSchemes"]["updates"]->append() = buildPayeeScheme();
         assertUpdate(update, expected);
      }

      // *FAKE CRASH AFTER SERVER HIT, NO RESPONSE*

      // get the pending update
      {
         SellerListing update;
         cat->populatePendingListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an update with a payee scheme
         SellerListingUpdate expected = buildUpdate(updateId + 1, updateId + 1);
         expected["payeeSchemes"]["updates"]->append() = buildPayeeScheme();
         assertUpdate(update, expected);
      }
   }
   tr.passIfNoException();

   tr.test("=> dirty w");
   {
      uint32_t updateId = 0;
      initializeCatalog(node, cat);
      initDirtyPsDirtyW(cat, updateId);

      // *FAKE CRASH BEFORE SERVER HIT, NO RESPONSE*

      // actions:
      dirtyWare(cat);

      // get the next update again
      {
         SellerListingUpdate update;
         cat->populateNextListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an update with a payee scheme
         SellerListingUpdate expected = buildUpdate(updateId, updateId + 1);
         expected["payeeSchemes"]["updates"]->append() = buildPayeeScheme();
         assertUpdate(update, expected);
      }

      // *FAKE CRASH AFTER SERVER HIT, NO RESPONSE*

      // get the pending update
      {
         SellerListing update;
         cat->populatePendingListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an update with a payee scheme
         SellerListingUpdate expected = buildUpdate(updateId + 1, updateId + 1);
         expected["payeeSchemes"]["updates"]->append() = buildPayeeScheme();
         assertUpdate(update, expected);
      }
   }
   tr.passIfNoException();

   tr.test("=> delete w");
   {
      uint32_t updateId = 0;
      initializeCatalog(node, cat);
      initDirtyPsDirtyW(cat, updateId);

      // *FAKE CRASH BEFORE SERVER HIT, NO RESPONSE*

      // actions:
      deleteWare(cat);

      // get the next update again
      {
         SellerListingUpdate update;
         cat->populateNextListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an update with a payee scheme
         SellerListingUpdate expected = buildUpdate(updateId, updateId + 1);
         expected["payeeSchemes"]["updates"]->append() = buildPayeeScheme();
         assertUpdate(update, expected);
      }

      // *FAKE CRASH AFTER SERVER HIT, NO RESPONSE*

      // get the pending update
      {
         SellerListing update;
         cat->populatePendingListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an update with a payee scheme
         SellerListingUpdate expected = buildUpdate(updateId + 1, updateId + 1);
         expected["payeeSchemes"]["updates"]->append() = buildPayeeScheme();
         assertUpdate(update, expected);
      }
   }
   tr.passIfNoException();

   tr.test("=> dirty ps, dirty w");
   {
      uint32_t updateId = 0;
      initializeCatalog(node, cat);
      initDirtyPsDirtyW(cat, updateId);

      // *FAKE CRASH BEFORE SERVER HIT, NO RESPONSE*

      // actions:
      dirtyPayeeScheme(cat);
      dirtyWare(cat);

      // get the next update again
      {
         SellerListingUpdate update;
         cat->populateNextListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an empty update
         SellerListingUpdate expected = buildUpdate(updateId, updateId);
         assertUpdate(update, expected);
      }

      // *FAKE CRASH AFTER SERVER HIT, NO RESPONSE*

      // no pending update
   }
   tr.passIfNoException();

   tr.test("=> dirty ps, delete w");
   {
      uint32_t updateId = 0;
      initializeCatalog(node, cat);
      initDirtyPsDirtyW(cat, updateId);

      // *FAKE CRASH BEFORE SERVER HIT, NO RESPONSE*

      // actions:
      dirtyPayeeScheme(cat);
      deleteWare(cat);

      // get the next update again
      {
         SellerListingUpdate update;
         cat->populateNextListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an empty update
         SellerListingUpdate expected = buildUpdate(updateId, updateId);
         assertUpdate(update, expected);
      }

      // *FAKE CRASH AFTER SERVER HIT, NO RESPONSE*

      // no pending update
   }
   tr.passIfNoException();

   tr.test("=> delete ps, delete w");
   {
      uint32_t updateId = 0;
      initializeCatalog(node, cat);
      initDirtyPsDirtyW(cat, updateId);

      // *FAKE CRASH BEFORE SERVER HIT, NO RESPONSE*

      // actions:
      deleteWare(cat);
      deletePayeeScheme(cat);

      // get the next update again
      {
         SellerListingUpdate update;
         cat->populateNextListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an empty update
         SellerListingUpdate expected = buildUpdate(updateId, updateId);
         assertUpdate(update, expected);
      }

      // *FAKE CRASH AFTER SERVER HIT, NO RESPONSE*

      // no pending update
   }
   tr.passIfNoException();

   tr.ungroup();

   tr.group("dirty ps, deleted w");

   tr.test("=> no action");
   {
      uint32_t updateId = 0;
      initializeCatalog(node, cat);
      initDirtyPsDeletedW(cat, updateId);

      // *FAKE CRASH BEFORE SERVER HIT, NO RESPONSE*

      // get the next update again
      {
         SellerListingUpdate update;
         cat->populateNextListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an update with a listing
         SellerListingUpdate expected = buildUpdate(updateId, updateId + 1);
         expected["listings"]["removals"]->append() = buildListing(true);
         assertUpdate(update, expected);
      }

      // *FAKE CRASH AFTER SERVER HIT, NO RESPONSE*

      // get the pending update
      {
         SellerListing update;
         cat->populatePendingListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an update with a listing
         SellerListingUpdate expected = buildUpdate(updateId + 1, updateId + 1);
         expected["listings"]["removals"]->append() = buildListing(true);
         assertUpdate(update, expected);
      }
   }
   tr.passIfNoException();

   tr.test("=> dirty ps");
   {
      uint32_t updateId = 0;
      initializeCatalog(node, cat);
      initDirtyPsDeletedW(cat, updateId);

      // *FAKE CRASH BEFORE SERVER HIT, NO RESPONSE*

      // actions:
      dirtyPayeeScheme(cat);

      // get the next update again
      {
         SellerListingUpdate update;
         cat->populateNextListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an update with a listing
         SellerListingUpdate expected = buildUpdate(updateId, updateId + 1);
         expected["listings"]["removals"]->append() = buildListing(true);
         assertUpdate(update, expected);
      }

      // *FAKE CRASH AFTER SERVER HIT, NO RESPONSE*

      // get the pending update
      {
         SellerListing update;
         cat->populatePendingListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an update with a listing
         SellerListingUpdate expected = buildUpdate(updateId + 1, updateId + 1);
         expected["listings"]["removals"]->append() = buildListing(true);
         assertUpdate(update, expected);
      }
   }
   tr.passIfNoException();

   tr.test("=> delete ps");
   {
      uint32_t updateId = 0;
      initializeCatalog(node, cat);
      initDirtyPsDeletedW(cat, updateId);

      // *FAKE CRASH BEFORE SERVER HIT, NO RESPONSE*

      // actions:
      deletePayeeScheme(cat);

      // get the next update again
      {
         SellerListingUpdate update;
         cat->populateNextListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an update with a listing
         SellerListingUpdate expected = buildUpdate(updateId, updateId + 1);
         expected["listings"]["removals"]->append() = buildListing(true);
         assertUpdate(update, expected);
      }

      // *FAKE CRASH AFTER SERVER HIT, NO RESPONSE*

      // get the pending update
      {
         SellerListing update;
         cat->populatePendingListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an update with a listing
         SellerListingUpdate expected = buildUpdate(updateId + 1, updateId + 1);
         expected["listings"]["removals"]->append() = buildListing(true);
         assertUpdate(update, expected);
      }
   }
   tr.passIfNoException();

   tr.test("=> dirty w");
   {
      uint32_t updateId = 0;
      initializeCatalog(node, cat);
      initDirtyPsDeletedW(cat, updateId);

      // *FAKE CRASH BEFORE SERVER HIT, NO RESPONSE*

      // actions:
      dirtyWare(cat);

      // get the next update again
      {
         SellerListingUpdate update;
         cat->populateNextListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an empty update
         SellerListingUpdate expected = buildUpdate(updateId, updateId);
         assertUpdate(update, expected);
      }

      // *FAKE CRASH AFTER SERVER HIT, NO RESPONSE*

      // no pending update
   }
   tr.passIfNoException();

   tr.test("=> dirty ps, dirty w");
   {
      uint32_t updateId = 0;
      initializeCatalog(node, cat);
      initDirtyPsDeletedW(cat, updateId);

      // *FAKE CRASH BEFORE SERVER HIT, NO RESPONSE*

      // actions:
      dirtyPayeeScheme(cat);
      dirtyWare(cat);

      // get the next update again
      {
         SellerListingUpdate update;
         cat->populateNextListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an empty update
         SellerListingUpdate expected = buildUpdate(updateId, updateId);
         assertUpdate(update, expected);
      }

      // *FAKE CRASH AFTER SERVER HIT, NO RESPONSE*

      // no pending update
   }
   tr.passIfNoException();

   tr.ungroup();

   tr.group("deleted ps, deleted w");

   tr.test("=> no action");
   {
      uint32_t updateId = 0;
      initializeCatalog(node, cat);
      initDeletedPsDeletedW(cat, updateId);

      // *FAKE CRASH BEFORE SERVER HIT, NO RESPONSE*

      // get the next update again
      {
         SellerListingUpdate update;
         cat->populateNextListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect an update with a listing
         SellerListingUpdate expected = buildUpdate(updateId, updateId + 1);
         expected["listings"]["removals"]->append() = buildListing(true);
         assertUpdate(update, expected);
      }

      // *FAKE CRASH AFTER SERVER HIT, NO RESPONSE*

      // get the pending update
      {
         SellerListing update;
         cat->populatePendingListingUpdate(TEST_USER_ID, update);
         assertNoException();

         // expect and update with a listing
         SellerListingUpdate expected = buildUpdate(updateId + 1, updateId + 1);
         expected["listings"]["removals"]->append() = buildListing(true);
         assertUpdate(update, expected);
      }
   }
   tr.passIfNoException();

   tr.ungroup();

   tr.ungroup();
}

class BmCustomCatalogTester : public bitmunk::test::Tester
{
public:
   BmCustomCatalogTester()
   {
      setName("Custom Catalog");
   }

   /**
    * Run automatic unit tests.
    */
   virtual int runAutomaticTests(TestRunner& tr)
   {
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

      // Note: always print this out to avoid confusion
      const char* profileFile = "devuser.profile";
      string profileDir =
         getApp()->getConfig()["node"]["profilePath"]->getString();
      string prof = File::join(profileDir.c_str(), profileFile);
      File file(prof.c_str());
      printf(
         "You must copy '%s' from pki to '%s' to run this. "
         "If you're seeing security breaches, your copy may "
         "be out of date.\n", profileFile, profileDir.c_str());
      if(!file->exists())
      {
         exit(1);
      }

      // login the devuser
      node.login("devuser", "password");
      assertNoException();

      // run test(s):
      runCustomCatalogTest(node, tr, *this);

      // get the custom catalog interface
      Catalog* cat = dynamic_cast<Catalog*>(
         node.getModuleApiByType("bitmunk.catalog"));
      assert(cat != NULL);
      runUpdateCrashTest(node, cat, tr, *this);

      // logout of client node
      node.logout(0);

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
         success = setupPeerNode(&node);
         assertNoException();
         assert(success);
      }

      if(!node.start())
      {
         dumpException();
         exit(1);
      }

      // Note: always print this out to avoid confusion
      const char* profileFile = "devuser.profile";
      const char* profileDir =
         getApp()->getConfig()["node"]["profilePath"]->getString();
      string prof = File::join(profileDir, profileFile);
      File file(prof.c_str());
      printf(
         "You must copy '%s' from pki to '%s' to run this. "
         "If you're seeing security breaches, your copy may "
         "be out of date.\n", profileFile, profileDir);
      if(!file->exists())
      {
         exit(1);
      }

      // login the devuser
      node.login("devuser", "password");
      assertNoException();

      // run test(s)
      runCustomCatalogTest(node, tr, *this);

      // logout of client node
      node.logout(0);

      // stop node
      node.stop();
#endif
      return 0;
   }
};

#ifndef MO_TEST_NO_MAIN
BM_TEST_MAIN(BmCustomCatalogTester)
#endif

/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include <iostream>
#include <sstream>

#include "bitmunk/common/Logging.h"
#include "bitmunk/purchase/TypeDefinitions.h"
#include "bitmunk/node/Node.h"
#include "bitmunk/test/Tester.h"
#include "monarch/event/EventWaiter.h"
#include "monarch/rt/Exception.h"
#include "monarch/test/Test.h"
#include "monarch/test/TestRunner.h"
#include "monarch/rt/DynamicObject.h"
#include "monarch/data/json/JsonWriter.h"
#include "monarch/io/File.h"
#include "monarch/io/FileOutputStream.h"
#include "monarch/io/OStreamOutputStream.h"

using namespace std;
using namespace bitmunk::common;
using namespace bitmunk::protocol;
using namespace bitmunk::purchase;
using namespace bitmunk::node;
using namespace monarch::config;
using namespace monarch::data::json;
using namespace monarch::event;
using namespace monarch::io;
using namespace monarch::net;
using namespace monarch::rt;
using namespace monarch::test;

#define DEVUSER_ID    900
#define TEST_MEDIA_ID 2ULL

void runPieceUploadTest(
   Node& node, TestRunner& tr, bitmunk::test::Tester& tester)
{
   tr.group("Piece upload");

   Messenger* messenger = node.getMessenger();

   // create contract
   Contract c;
   c["version"] = "3.0";
   c["id"] = 0;
   c["buyer"]["userId"] = DEVUSER_ID;
   c["buyer"]["profileId"] = 1;

   // create contract media
   Media& media = c["media"];
   media["id"] = TEST_MEDIA_ID;

   // create contract section
   ContractSection cs = c["sections"]["1"]->append();
   BM_ID_SET(cs["contractId"], 0);
   BM_ID_SET(cs["buyer"]["userId"], BM_USER_ID(c["buyer"]["userId"]));
   BM_ID_SET(cs["buyer"]["profileId"], BM_PROFILE_ID(c["buyer"]["profileId"]));
   cs["webbuy"] = false;
   BM_ID_SET(cs["ware"]["id"], TEST_MEDIA_ID);

   Seller& seller = cs["seller"];
   BM_ID_SET(seller["userId"], 1);
   BM_ID_SET(seller["profileId"], 1);
   BM_ID_SET(seller["serverId"], 1);
   seller["url"] =
      node.getMessenger()->getSecureBitmunkUrl()->toString().c_str();

   tr.test("get license");
   {
      // contact sva's POST media service to create a signed media
      Url url;
      url.format("/api/3.0/sva/contracts/media/%" PRIu64, TEST_MEDIA_ID);
      messenger->postSecureToBitmunk(&url, NULL, &media, DEVUSER_ID);
   }
   tr.passIfNoException();

   tr.test("negotiate");
   {
      // negotiate contract with seller's server
      Url url("/api/3.0/sales/contract/negotiate?nodeuser=1");
      messenger->postSecureToBitmunk(&url, &c, &c, DEVUSER_ID);
   }
   tr.passIfNoException();

   tr.test("receive piece");
   {
      // get section
      cs = c["sections"]["1"][0];

      // create piece request
      DynamicObject pieceRequest;
      pieceRequest["csHash"] = cs["hash"]->getString();
      BM_ID_SET(
         pieceRequest["fileId"],
         BM_FILE_ID(cs["ware"]["fileInfos"][0]["id"]));
      pieceRequest["index"] = 0;
      pieceRequest["size"] = 250000;
      pieceRequest["peerbuyKey"] = cs["peerbuyKey"]->getString();
      BM_ID_SET(
         pieceRequest["sellerProfileId"],
         BM_PROFILE_ID(cs["seller"]["profileId"]));
      BM_ID_SET(pieceRequest["bfpId"], 1);

      // create stream for writing piece
      File file("/tmp/bmtestpiece.piece");
      FileOutputStream fos(file);

      // setup btp messages
      BtpMessage out;
      BtpMessage in;
      out.setDynamicObject(pieceRequest);
      out.setType(BtpMessage::Post);
      in.setContentSink(&fos, true);

      // post request to seller's server
      Url url;
      url.format("%s/api/3.0/sales/contract/filepiece?nodeuser=1",
         node.getMessenger()->getSecureBitmunkUrl()->toString().c_str());
      messenger->exchange(1, &url, &out, &in, DEVUSER_ID);
      assertNoException();

      // print trailer
      printf("Received trailer:\n%s\n", in.getTrailer()->toString().c_str());
   }
   tr.passIfNoException();

   tr.ungroup();
}

class BmPieceUploadTester :
public bitmunk::test::Tester,
public monarch::event::Observer
{
public:
   BmPieceUploadTester()
   {
      setName("Piece Upload Tester");
   }

   virtual void eventOccurred(Event& e)
   {
      MO_CAT_DEBUG(BM_TEST_CAT, "Got event: \n%s",
         JsonWriter::writeToString(e).c_str());
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
      const char* profilePath =
         getApp()->getConfig()["node"]["profilePath"]->getString();
      string prof = File::join(profilePath, profileFile);
      File file(prof.c_str());
      printf(
         "You must copy '%s' from pki to '%s' to run this. "
         "If you're seeing security breaches, your copy may "
         "be out of date.\n", profileFile, profilePath);
      if(!file->exists())
      {
         exit(1);
      }

      // login the devuser
      node.login("devuser", "password");
      assertNoException();

      // register self as event observer of all events
      node.getEventController()->registerObserver(this, "*");

      // run test
      runPieceUploadTest(node, tr, *this);

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
      return 0;
   }
};

#ifndef MO_TEST_NO_MAIN
BM_TEST_MAIN(BmPieceUploadTester)
#endif

/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include <iostream>
#include <sstream>

#include "bitmunk/common/Logging.h"
#include "bitmunk/common/Tools.h"
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
#define TEST_SINGLE_MEDIA_ID 2
#define TEST_COLLECTION_MEDIA_ID 1

// The download state test type specifies the type of media to purchase
enum DownloadStateTestType 
{
   Single,
   Collection
};

void runDownloadStatesTest(
   Node& node, TestRunner& tr, bitmunk::test::Tester& tester, 
   DownloadStateTestType testType)
{
   MediaId mediaId = 0;
   DownloadStateId dsId = 0;
   const char* price;
   
   // check the type of peerbuy test to perform and output the type to the
   // tester
   if(testType == Single)
   {
      tr.group("Download State Single");
      mediaId = TEST_SINGLE_MEDIA_ID;
      price = "1.00";
   }
   else
   {
      tr.group("Download State Collection");
      mediaId = TEST_COLLECTION_MEDIA_ID;
      price = "8.00";
   }
   
   tr.test("create download state 1");
   {
      DynamicObject in;
      DynamicObject out;
      out["mediaId"] = mediaId;
      // FIXME: id needs to be a hexadecimal ware ID
      out["wareId"] = out["mediaId"]->getString();
      
      // hit self to create download state
      Url url;
      url.format(
         "%s/api/3.0/purchase/contracts/downloadstates?nodeuser=900",
         "https://localhost:19200");
      
      // start download
      Messenger* messenger = node.getMessenger();
      messenger->post(&url, &out, &in, node.getDefaultUserId());
      
      // get the download state ID
      dsId = in["downloadStateId"]->getUInt64();
   }
   tr.passIfNoException();

   tr.test("delete download state (HTTP POST)");
   {
      DynamicObject out;
      out["downloadStateId"] = dsId;
      
      // hit self to delete the download state
      Url url;
      url.format(
         "%s/api/3.0/purchase/contracts/downloadstates/delete?nodeuser=900",
         "https://localhost:19200");
      
      // delete the download
      Messenger* messenger = node.getMessenger();
      messenger->post(&url, &out, NULL, node.getDefaultUserId());
   }
   tr.passIfNoException();

   tr.test("create download state 2");
   {
      DynamicObject in;
      DynamicObject out;
      out["mediaId"] = mediaId;
      // FIXME: id needs to be a hexadecimal ware ID
      out["wareId"] = out["mediaId"]->getString();
      
      // hit self to create download state
      Url url;
      url.format(
         "%s/api/3.0/purchase/contracts/downloadstates?nodeuser=900",
         "https://localhost:19200");
      
      // start download
      Messenger* messenger = node.getMessenger();
      messenger->post(&url, &out, &in, node.getDefaultUserId());
      
      // get the download state ID
      dsId = in["downloadStateId"]->getUInt64();
   }
   tr.passIfNoException();

   tr.test("delete download state (HTTP DELETE)");
   {
      // hit self to delete the download state using HTTP DELETE
      Url url;
      url.format(
         "%s/api/3.0/purchase/contracts/downloadstates/%" PRIu64
         "?nodeuser=900",
         "https://localhost:19200", dsId);
      
      // delete the download
      Messenger* messenger = node.getMessenger();
      messenger->deleteResource(&url, NULL, node.getDefaultUserId());
   }
   tr.passIfNoException();

   tr.ungroup();
}

class BmDownloadStatesTester :
public bitmunk::test::Tester,
public monarch::event::Observer
{
public:
   BmDownloadStatesTester()
   {
      setName("Download States Tester");
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
      runDownloadStatesTest(node, tr, *this, Single);
      //runDownloadStatesTest(node, tr, *this, Collection);
      
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
BM_TEST_MAIN(BmDownloadStatesTester)
#endif

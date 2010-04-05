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

#define DEVUSER_ID            900
struct testIds_s
{
   const char* fileId;
   uint64_t mediaId;
};
static struct testIds_s testIds[] =
{
   // mediaId 1 is collection of 2-9
   {"b79068aab92f78ba312d35286a77ea581037b109", 2},
   {"5e5a503fbd8217274819616929da1c041c30b4fc", 3},
   {"1ab6dc46818c4f497b32edf8842047f78fa448b7", 4},
   {"944c8d2db1e709f1e438ba5f5644c9485b6b6291", 5},
   {"ed10444dc6d9c36fdd21556192d2ca2f574cda7c", 6},
   {"252e18e46d73f6481f98ff5952ff4b8cfbd30781", 7},
   {"7fe75342e25ba9fdd478614d3d3267585acdfcbc", 8},
   {"aa36394f53644e8c2c9aaf5cab3da877b2f03c8f", 9},
   {NULL, 0}
};

// The peerbuy test type specifies the type of media to purchase
enum PeerBuyTestType 
{
   Single,
   Collection
};

/**
 * @param testIdIndex index for use with Single type
 */
void runPeerBuyTest(
   Node& node, TestRunner& tr, bitmunk::test::Tester& tester, 
   PeerBuyTestType testType, int testIdIndex = 0)
{
   DownloadStateId dsId = 0;
   const char* price;
   // for tester
   DynamicObject groupName;
   
   // check the type of peerbuy test to perform and output type to tester
   Ware ware;
   ware["id"] = "bitmunk:bundle";
   // FIXME: does not test single item NOT in a shopping cart "bundle"
   switch(testType)
   {
      case Single:
      {
         groupName->format("PeerBuy Single (mediaId:%" PRIu64 ")",
            testIds[testIdIndex].mediaId);
         price = "0.90";//1.00";
         
         ware["mediaId"] = testIds[testIdIndex].mediaId;
         FileInfo& fi = ware["fileInfos"]->append();
         fi["id"] = testIds[testIdIndex].fileId;
         fi["mediaId"] = testIds[testIdIndex].mediaId;
         break;
      }
      case Collection:
      {
         groupName = "PeerBuy Collection";
         price = "8.00";
         
         ware["mediaId"] = 1;
         for(int i = 0; testIds[i].fileId != NULL; i++)
         {
            FileInfo& fi = ware["fileInfos"]->append();
            fi["id"] = testIds[i].fileId;
            fi["mediaId"] = testIds[i].mediaId;
         }
         break;
      }
      default:
      {
         groupName = "?";
         price = "0.00";
      }
   }
   
   tr.group(groupName->getString());
   
   tr.test("create download state");
   {
      DynamicObject in;
      DynamicObject out;
      out["ware"] = ware;
      
      // hit self to create download state
      Url url;
      url.format(
         "%s/api/3.0/purchase/contracts/downloadstates?nodeuser=900",
         "https://localhost:19200");
      
      // create download state
      Messenger* messenger = node.getMessenger();
      messenger->post(&url, &out, &in, node.getDefaultUserId());
      assertNoException();
      
      // get the download state ID
      dsId = in["downloadStateId"]->getUInt64();
      
      // FIXME: temp code to test download progress polling
      /*
      {
         Event e;
         e["type"] = "bitmunk.purchase.DownloadState.progressPoll";
         e["details"]["downloadStateId"] = dsId;
         node.getEventDaemon()->add(e, 1000, 60);
      }
      */
   }
   tr.passIfNoException();
   
   tr.test("initialize download state");
   if(!Exception::isSet())
   {
      // create event waiter
      EventWaiter ew(node.getEventController());
      
      // create specific conditional filter to wait for
      EventFilter ef;
      ef["details"]["downloadStateId"] = dsId;
      
      // start listening to events
      ew.start("bitmunk.purchase.DownloadState.initialized", &ef);
      ew.start("bitmunk.purchase.DownloadState.exception", &ef);
      
      // hit self to initialize download state
      Url url;
      url.format("%s/api/3.0/purchase/contracts/initialize?nodeuser=900",
         "https://localhost:19200");
      
      // initialize state by posting download state ID
      DynamicObject out;
      out["downloadStateId"] = dsId;
      Messenger* messenger = node.getMessenger();
      messenger->post(&url, &out, NULL, node.getDefaultUserId());
      assertNoException();
      
      // wait for event
      ew.waitForEvent();
      
      // handle exception event
      Event e = ew.popEvent();
      if(strstr(e["type"]->getString(), "exception") != NULL)
      {
         ExceptionRef ex =
            Exception::convertToException(e["details"]["exception"]);
         Exception::set(ex);
      }
      
      tr.passIfNoException();
   }
   else
   {
      tr.fail("Exception in previous test.");
   }
   
   tr.test("acquire license");
   if(!Exception::isSet())
   {
      // create event waiter
      EventWaiter ew(node.getEventController());
      
      // create specific conditional filter to wait for
      EventFilter ef;
      ef["details"]["downloadStateId"] = dsId;
      
      // start listening to events
      ew.start("bitmunk.purchase.DownloadState.licenseAcquired", &ef);
      ew.start("bitmunk.purchase.DownloadState.exception", &ef);
      
      // hit self to acquire license
      Url url;
      url.format("%s/api/3.0/purchase/contracts/license?nodeuser=900",
         "https://localhost:19200");
      
      // acquire license by posting download state ID
      DynamicObject out;
      out["downloadStateId"] = dsId;
      Messenger* messenger = node.getMessenger();
      messenger->post(&url, &out, NULL, node.getDefaultUserId());
      assertNoException();
      
      // wait for event
      ew.waitForEvent();
      
      // handle exception event
      Event e = ew.popEvent();
      if(strstr(e["type"]->getString(), "exception") != NULL)
      {
         ExceptionRef ex =
            Exception::convertToException(e["details"]["exception"]);
         Exception::set(ex);
      }
      
      tr.passIfNoException();
   }
   else
   {
      tr.fail("Exception in previous test.");
   }
   
   tr.test("download");
   if(!Exception::isSet())
   {
      // create event waiter
      EventWaiter ew(node.getEventController());
      
      // create specific conditional filter to wait for
      EventFilter ef;
      ef["details"]["downloadStateId"] = dsId;
      
      // start listening to events
      ew.start("bitmunk.purchase.DownloadState.downloadCompleted", &ef);
      ew.start("bitmunk.purchase.DownloadState.downloadInterrupted", &ef);
      ew.start("bitmunk.purchase.DownloadState.exception", &ef);
      
      // hit self to start download
      Url url;
      url.format("%s/api/3.0/purchase/contracts/download?nodeuser=900",
         "https://localhost:19200");
      
      // start download by posting download state ID and preferences
      DynamicObject out;
      out["downloadStateId"] = dsId;
      out["preferences"]["price"]["max"] = price;
      out["preferences"]["fast"] = false;//true;
      out["preferences"]["accountId"] = 9000;
      out["preferences"]["sellerLimit"] = 1;
      Messenger* messenger = node.getMessenger();
      messenger->post(&url, &out, NULL, node.getDefaultUserId());
      assertNoException();
      
      // FIXME: here to test pausing a download
      if(false)
      {
         // hit self to pause download
         Url url2;
         url2.format("%s/api/3.0/purchase/contracts/pause?nodeuser=900",
            "https://localhost:19200");
         
         // pause download by posting download state ID
         DynamicObject out2;
         out2["downloadStateId"] = dsId;
         messenger->post(&url2, &out2, NULL, node.getDefaultUserId());
         assertNoException();
      }
      
      // wait for event
      ew.waitForEvent();
      
      // handle exception event
      Event e = ew.popEvent();
      if(strstr(e["type"]->getString(), "exception") != NULL)
      {
         ExceptionRef ex =
            Exception::convertToException(e["details"]["exception"]);
         Exception::set(ex);
      }
      else if(strstr(e["type"]->getString(), "downloadInterrupted") != NULL)
      {
         ExceptionRef ex =
            new Exception("Download interrupted via 'pause' in peerbuy test.");
         Exception::set(ex);
      }
      
      tr.passIfNoException();
   }
   else
   {
      tr.fail("Exception in previous test.");
   }
   
   tr.test("purchase");
   if(!Exception::isSet())
   {
      // create event waiter
      EventWaiter ew(node.getEventController());
      
      // create specific conditional filter to wait for
      EventFilter ef;
      ef["details"]["downloadStateId"] = dsId;
      
      // start listening to events
      ew.start("bitmunk.purchase.DownloadState.purchaseCompleted", &ef);
      ew.start("bitmunk.purchase.DownloadState.exception", &ef);
      
      // hit self to start purchase
      Url url;
      url.format("%s/api/3.0/purchase/contracts/purchase?nodeuser=900",
         "https://localhost:19200");
      
      // start purchase by posting download state ID
      DynamicObject out;
      out["downloadStateId"] = dsId;
      Messenger* messenger = node.getMessenger();
      messenger->post(&url, &out, NULL, node.getDefaultUserId());
      assertNoException();
      
      // wait for event
      ew.waitForEvent();
      
      // handle exception event
      Event e = ew.popEvent();
      if(strstr(e["type"]->getString(), "exception") != NULL)
      {
         ExceptionRef ex =
            Exception::convertToException(e["details"]["exception"]);
         Exception::set(ex);
      }
      
      tr.passIfNoException();
   }
   else
   {
      tr.fail("Exception in previous test.");
   }

   tr.test("assemble");
   if(!Exception::isSet())
   {
      // create event waiter
      EventWaiter ew(node.getEventController());
      
      // create specific conditional filter to wait for
      EventFilter ef;
      ef["details"]["downloadStateId"] = dsId;
      
      // start listening to events
      ew.start("bitmunk.purchase.DownloadState.assemblyCompleted", &ef);
      ew.start("bitmunk.purchase.DownloadState.exception", &ef);
      
      // hit self to start file assembly
      Url url;
      url.format("%s/api/3.0/purchase/contracts/assemble?nodeuser=900",
         "https://localhost:19200");
      
      // start file assembly by posting download state ID
      DynamicObject out;
      out["downloadStateId"] = dsId;
      Messenger* messenger = node.getMessenger();
      messenger->post(&url, &out, NULL, node.getDefaultUserId());
      assertNoException();
      
      // wait for event
      ew.waitForEvent();
      
      // handle exception event
      Event e = ew.popEvent();
      if(strstr(e["type"]->getString(), "exception") != NULL)
      {
         ExceptionRef ex =
            Exception::convertToException(e["details"]["exception"]);
         Exception::set(ex);
      }
      
      tr.passIfNoException();
   }
   else
   {
      tr.fail("Exception in previous test.");
   }
   
   tr.ungroup();
}

class BmPeerBuyTester :
public bitmunk::test::Tester,
public monarch::event::Observer
{
public:
   BmPeerBuyTester()
   {
      setName("PeerBuy Tester");
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
         Config config;
         config["node"]["modulePath"] = BPE_MODULES_DIRECTORY;
         config["bitmunk.webui.WebUi"]["flash"]["policyServer"]["port"] = 19844;
         setupPeerNode(&node, &config);
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
      
      // register self as event observer of all events
      node.getEventController()->registerObserver(this, "*");
      
      // buy just one
      //runPeerBuyTest(node, tr, *this, Single);
      
      // buy all individual singles
      for(int i = 0; testIds[i].fileId != NULL; i++)
      {
         runPeerBuyTest(node, tr, *this, Single, i);
      }
      
      // buy all as a collection
      runPeerBuyTest(node, tr, *this, Collection);
      
      // logout of client node
      node.logout(0);
      
      // stop node
      node.stop();
#endif
      return 0;
   }
};

#ifndef MO_TEST_NO_MAIN
BM_TEST_MAIN(BmPeerBuyTester)
#endif

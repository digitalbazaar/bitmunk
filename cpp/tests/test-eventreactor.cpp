/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_CONSTANT_MACROS
#define __STDC_FORMAT_MACROS

#include <iostream>
#include <sstream>
#include <inttypes.h>

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

#define DEVUSER_ID        UINT64_C(900)
#define SINGLE_MEDIA_ID   UINT64_C(2)
#define FILE_ID           "b79068aab92f78ba312d35286a77ea581037b109"

void runEventReactorTest(
   Node& node, TestRunner& tr, bitmunk::test::Tester& tester)
{
   tr.group("EventReactor");
   
   tr.test("create event reactor");
   {
      DynamicObject in;
      
      // hit self to create event reactor
      Url url;
      url.format(
         "%s/api/3.0/eventreactor/users/900/downloadstate?nodeuser=900",
         "https://localhost:19200");
      
      Messenger* messenger = node.getMessenger();
      messenger->post(&url, NULL, &in, node.getDefaultUserId());
      // FIXME: remove me
      dumpDynamicObject(in);
   }
   tr.passIfNoException();

   tr.test("create peerbuy directive");
   if(!Exception::isSet())
   {
      // create event waiters to notify that events are successfully being
      // processed through the system via the event reactor
      
      // create event waiter
      EventWaiter ew(node.getEventController());
      
      // start listening to events
      ew.start("bitmunk.system.Directive.processed");
      ew.start("bitmunk.purchase.DownloadState.initialized");
      ew.start("bitmunk.purchase.DownloadState.downloadStarted");
      ew.start("bitmunk.purchase.DownloadState.progressUpdate");
      ew.start("bitmunk.purchase.DownloadState.downloadCompleted");
      ew.start("bitmunk.purchase.DownloadState.licenseAcquired");
      ew.start("bitmunk.purchase.DownloadState.purchaseCompleted");
      ew.start("bitmunk.purchase.DownloadState.assemblyCompleted");
      ew.start("bitmunk.purchase.DownloadState.exception");
      
      // create bitmunk peerbuy directive
      DynamicObject directive;
      directive["version"] = "3.0";
      directive["type"] = "peerbuy";
      Ware& ware = directive["data"]["ware"];
      ware["id"] = "bitmunk:bundle";
      ware["mediaId"] = SINGLE_MEDIA_ID;
      FileInfo& fi = ware["fileInfos"]->append();
      fi["id"] = FILE_ID;
      fi["mediaId"] = SINGLE_MEDIA_ID;
      
      DynamicObject in;
      DynamicObject out;
      out = directive;
      
      // FIXME: use this loop to run concurrent downloads,
      // simply change "count" to the number of concurrent downloads
      int count = 1;
      for(int i = 0; i < count; ++i)
      {
         // hit self to create directive
         Url url;
         url.format(
            "%s/api/3.0/system/directives?nodeuser=900",
            "https://localhost:19200");
         
         Messenger* messenger = node.getMessenger();
         messenger->post(&url, &out, &in, node.getDefaultUserId());
      }
      
      while(true)
      {
         // wait for event
         ew.waitForEvent();
         
         // handle exception event
         Event e = ew.popEvent();
         if(strstr(e["type"]->getString(), "exception") != NULL)
         {
            ExceptionRef ex =
               Exception::convertToException(e["details"]["exception"]);
            Exception::set(ex);
            break;
         }
         else
         {
            if(strstr(e["type"]->getString(), "progressUpdate") == NULL)
            {
               printf("EVENT OCCURRED: '%s'\n", e["type"]->getString());
            }
            
            if(strstr(e["type"]->getString(), "licenseAcquired") != NULL)
            {
               // start download manually here
               Messenger* messenger = node.getMessenger();
               
               Url url;
               url.format("%s/api/3.0/purchase/contracts/%s?nodeuser=%" PRIu64,
                  messenger->getSelfUrl(true).c_str(), "download", DEVUSER_ID);
               
               // start download by posting download state ID and preferences
               DynamicObject out;
               out["downloadStateId"] = e["details"]["downloadStateId"];
               out["preferences"]["price"]["max"] = "1.00";
               out["preferences"]["fast"] = false;//true;
               out["preferences"]["accountId"] = 9000;
               out["preferences"]["sellerLimit"] = 1;
               messenger->post(&url, &out, NULL, DEVUSER_ID);
            }
            else if(strstr(e["type"]->getString(), "assemblyCompleted") != NULL)
            {
               if(--count == 0)
               {
                  // done
                  break;
               }
            }
            else if(strstr(e["type"]->getString(), "progressUpdate") != NULL)
            {
               double percent =
                  e["details"]["downloaded"]->getDouble() /
                  e["details"]["size"]->getDouble() * 100.;
               printf("Downloaded: %" PRIu64 "/%" PRIu64 " %.2f%%\r",
                  e["details"]["downloaded"]->getUInt64(),
                  e["details"]["size"]->getUInt64(), percent);
               fflush(0);
            }
         }
      }
      
      tr.passIfNoException();
   }
   else
   {
      tr.fail("Exception in previous test.");
   }

   tr.test("delete event reactor");
   {
      DynamicObject in;
      
      // hit self to delete event reactor
      Url url;
      url.format(
         "%s/api/3.0/eventreactor/users/900?nodeuser=900",
         "https://localhost:19200");
      
      Messenger* messenger = node.getMessenger();
      messenger->deleteResource(&url, &in, node.getDefaultUserId());
      // FIXME: remove me
      dumpDynamicObject(in);
   }
   tr.passIfNoException();
   
   tr.ungroup();
}

void createEventReactor(
   Node& node, TestRunner& tr, bitmunk::test::Tester& tester)
{
   tr.test("create event reactor");
   {
      DynamicObject in;
      
      // hit self to create event reactor
      Url url;
      url.format(
         "/api/3.0/eventreactor/users/900/downloadstate?nodeuser=900");
      
      printf("\nCreating DownloadStateEventReactor:\n");
      Messenger* messenger = node.getMessenger();
      if(messenger->postSecureToBitmunk(
         &url, NULL, &in, node.getDefaultUserId()))
      {
         dumpDynamicObject(in);
         printf("Done. Now use it for something.\n");
      }
   }
   tr.passIfNoException();
}

class BmEventReactorTester :
public bitmunk::test::Tester,
public monarch::event::Observer
{
public:
   BmEventReactorTester()
   {
      setName("EventReactor Tester");
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
      
      // run test(s)
      runEventReactorTest(node, tr, *this);
      
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
      
      // register self as event observer of all events
      node.getEventController()->registerObserver(this, "*");
      
      // run test(s)
      createEventReactor(node, tr, *this);
      
      // logout of client node
      node.logout(0);
      
      // stop node
      node.stop();      
#endif
      return 0;
   }
};

#ifndef MO_TEST_NO_MAIN
BM_TEST_MAIN(BmEventReactorTester)
#endif

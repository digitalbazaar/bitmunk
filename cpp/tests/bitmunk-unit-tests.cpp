/*
 * Copyright (c) 2007-2009 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include <iostream>
#include <sstream>

#include "monarch/config/ConfigManager.h"
#include "monarch/crypto/AsymmetricKeyFactory.h"
#include "monarch/data/DynamicObjectInputStream.h"
#include "monarch/data/DynamicObjectOutputStream.h"
#include "monarch/data/json/JsonReader.h"
#include "monarch/data/json/JsonWriter.h"
#include "monarch/data/xml/XmlReader.h"
#include "monarch/data/xml/XmlWriter.h"
#include "monarch/event/Event.h"
#include "monarch/event/EventController.h"
#include "monarch/event/Observer.h"
#include "monarch/event/ObserverDelegate.h"
#include "monarch/io/FileInputStream.h"
#include "monarch/io/FileOutputStream.h"
#include "monarch/io/MutatorInputStream.h"
#include "monarch/io/MutatorOutputStream.h"
#include "monarch/io/OStreamOutputStream.h"
#include "monarch/logging/OutputStreamLogger.h"
#include "monarch/modest/Kernel.h"
#include "monarch/net/Server.h"
#include "monarch/http/HttpConnectionServicer.h"
#include "monarch/test/Test.h"
#include "monarch/test/Tester.h"
#include "monarch/test/TestRunner.h"
#include "monarch/util/Convert.h"
#include "bitmunk/common/Logging.h"
#include "bitmunk/common/Profile.h"
#include "bitmunk/common/Tools.h"
#include "bitmunk/data/Id3v2TagWriter.h"
#include "bitmunk/data/MpegAudioTimeParser.h"
#include "bitmunk/node/Node.h"
#include "bitmunk/common/CatalogInterface.h"
#include "bitmunk/test/Tester.h"

#include "bitmunk-unit-tests.h"

using namespace std;
using namespace monarch::config;
using namespace monarch::crypto;
using namespace monarch::data;
using namespace monarch::data::json;
using namespace monarch::data::xml;
using namespace monarch::event;
using namespace monarch::http;
using namespace monarch::io;
using namespace monarch::logging;
using namespace monarch::modest;
using namespace monarch::net;
using namespace monarch::rt;
using namespace monarch::test;
using namespace monarch::util;
using namespace bitmunk::common;
using namespace bitmunk::data;
using namespace bitmunk::protocol;
using namespace bitmunk::node;

void runNodeTest(TestRunner& tr, bitmunk::test::Tester& tester)
{
   tr.test("Node Initialization");
   // FIXME:
#if 0
   Node node;
   {
      bool setup = tester.setupNode(&node);
      assertNoException();
      assert(setup);
      bool started = node.start();
      assertNoException();
      assert(started);
   }
#endif
   tr.passIfNoException();
}

void runNodeConfigTest(TestRunner& tr, bitmunk::test::Tester& tester)
{
   tr.test("Node config");
   // FIXME:
#if 0
   Node node;
   {
      bool setup = tester.setupNode(&node);
      assertNoException();
      assert(setup);
      bool started = node.start();
      assertNoException();
      assert(started);
   }
#endif
   tr.passIfNoException();
}

void assertSameExceptions(ExceptionRef& e0, ExceptionRef& e1)
{
   // assert both are NULL or not NULL
   assert((e0.isNull() && e1.isNull()) ||
          (!e0.isNull() && !e1.isNull()));

   // check contents if both not NULL
   if(!e0.isNull() && !e1.isNull())
   {
      // compare basic elements
      assertStrCmp(e0->getMessage(), e1->getMessage());
      assertStrCmp(e0->getType(), e1->getType());
      assert(e0->getCode() == e1->getCode());
      
      // recursively check cause chain
      // FIXME enable cause checking
      assertSameExceptions(e0->getCause(), e1->getCause());
   }
}

void runBtpExceptionTest_XML_1(ExceptionRef& e)
{
   // write out exception
   DynamicObject dyno = Exception::convertToDynamicObject(e);
   ostringstream oss;
   OStreamOutputStream os(&oss);
   XmlWriter writer;
   writer.setIndentation(0, 1);
   DynamicObjectInputStream dois(dyno, &writer, false);
   
   char b[1024];
   int numBytes;
   while((numBytes = dois.read(b, 1024)) > 0)
   {
      os.write(b, numBytes);
      assertNoException();
   }
   assertNoException();
   
   string xml1 = oss.str();
   //cout << "XML1=\n" << oss.str() << endl;
   
   // read exception back in
   DynamicObject dyno2;
   XmlReader reader;
   DynamicObjectOutputStream doos(dyno2, &reader, false);
   doos.write(oss.str().c_str(), oss.str().length());
   assertNoException();
   ExceptionRef e2 = Exception::convertToException(dyno2);
   
   // write exception back out
   DynamicObject dyno3 = Exception::convertToDynamicObject(e2);
   DynamicObjectInputStream dois2(dyno3, &writer, false);
   ostringstream oss2;
   OStreamOutputStream os2(&oss2);
   
   char b2[1024];
   int numBytes2;
   while((numBytes2 = dois2.read(b2, 1024)) > 0)
   {
      os2.write(b2, numBytes2);
      assertNoException();
   }
   assertNoException();
   
   string xml2 = oss2.str();
   //cout << endl << "XML2=\n" << xml2 << endl;
   
   assertStrCmp(xml1.c_str(), xml2.c_str());
   assertSameExceptions(e, e2);
}

void runBtpExceptionTest_JSON_1(ExceptionRef& e)
{
   // write out exception
   DynamicObject dyno = Exception::convertToDynamicObject(e);
   ostringstream oss;
   OStreamOutputStream os(&oss);
   JsonWriter writer;
   writer.setIndentation(0, 1);
   DynamicObjectInputStream dois(dyno, &writer, false);
   
   char b[1024];
   int numBytes;
   while((numBytes = dois.read(b, 1024)) > 0)
   {
      os.write(b, numBytes);
      assertNoException();
   }
   assertNoException();
   
   string json1 = oss.str();
   //cout << "JSON1=\n" << oss.str() << endl;
   
   // read exception back in
   DynamicObject dyno2;
   JsonReader reader;
   DynamicObjectOutputStream doos(dyno2, &reader, false);
   doos.write(oss.str().c_str(), oss.str().length());
   assertNoException();
   ExceptionRef e2 = Exception::convertToException(dyno2);
   
   // write exception back out
   DynamicObject dyno3 = Exception::convertToDynamicObject(e2);
   DynamicObjectInputStream dois2(dyno3, &writer, false);
   ostringstream oss2;
   OStreamOutputStream os2(&oss2);
   
   char b2[1024];
   int numBytes2;
   while((numBytes2 = dois2.read(b2, 1024)) > 0)
   {
      os2.write(b2, numBytes2);
      assertNoException();
   }
   assertNoException();
   
   string json2 = oss2.str();
   //cout << endl << "JSON2=\n" << json2 << endl;
   
   assertStrCmp(json1.c_str(), json2.c_str());
   assertSameExceptions(e, e2);
}

void runBtpExceptionTypeTest(TestRunner& tr,
   const char* type, void (*runTestFunc)(ExceptionRef&))
{
   tr.group(type);
   
   tr.test("simple serialize/deserialize");
   ExceptionRef e = new Exception("e name", "e type", 0);
   runTestFunc(e);
   tr.pass();
   
   tr.test("simple serialize/deserialize w/ a cause");
   ExceptionRef e2 = new Exception("e2 name", "e2 type", 0);
   ExceptionRef e0 = new Exception("e0 name", "e0 type", 0);
   e2->setCause(e0);
   runTestFunc(e2);
   tr.pass();
   
   tr.ungroup();
}

void runBtpExceptionTest(TestRunner& tr)
{
   tr.group("Btp exception");

   runBtpExceptionTypeTest(tr, "XML", &runBtpExceptionTest_XML_1);
   runBtpExceptionTypeTest(tr, "JSON", &runBtpExceptionTest_JSON_1);

   tr.ungroup();
}

void runLoginTest(TestRunner& tr, bitmunk::test::Tester& tester)
{
   tr.group("Login");
   // FIXME:
#if 0
   Node node;
   {
      bool setup = tester.setupNode(&node);
      assertNoException();
      assert(setup);
      bool started = node.start();
      assertNoException();
      assert(started);
   }
   
   tr.test("login success");
   node.login("devuser", "password");
   tr.passIfNoException();
   
   // logout
   node.logout(0);
   
   tr.test("login failure (invalid short password)");
   node.login("devuser", "");
   tr.passIfException();
   
   tr.test("login failure (invalid long password)");
   node.login("devuser", "badpassword");
   tr.passIfException();
   
   // logout
   node.logout(0);
#endif
   tr.ungroup();
}

void runLoadModulesTest(TestRunner &tr, bitmunk::test::Tester& tester)
{
   tr.test("load modules");
   // FIXME:
#if 0
   Node node;
   {
      bool setup = tester.setupNode(&node);
      assertNoException();
      assert(setup);
      bool started = node.start();
      assertNoException();
      assert(started);
   }
   
   // login
   node.login("devuser", "password");
   assertNoException();
   
   // logout
   node.logout(0);
#endif
   tr.pass();
}

class NodeStopEventObserver : public Observer
{
protected:
   ExclusiveLock* mWaiter;
public:
   NodeStopEventObserver(ExclusiveLock* waiter)
   {
      mWaiter = waiter;
   }
   virtual ~NodeStopEventObserver()
   {
   }
   virtual void eventOccurred(Event& e)
   {
      mWaiter->notifyAll();
   }
};

void runNodeStopEventTest(TestRunner& tr, bitmunk::test::Tester& tester)
{
   tr.test("NodeStopEvent bug check");
   // FIXME:
#if 0
   // heap allocate so we can check cleanup worked
   Node* node = new Node();
   {
      bool setup = tester.setupNode(node);
      assertNoException();
      assert(setup);
      bool started = node->start();
      assertNoException();
      assert(started);
   }

   ExclusiveLock waiter;
   NodeStopEventObserver o(&waiter);

   EventController* ec = node->getEventController();
   const char* name = "NodeStopEvent";

   ec->registerObserver(&o, name);

   Event e;
   e["type"] = name;
   ec->schedule(e);

   // wait for event 
   waiter.wait();

   delete node;
#endif
   tr.passIfNoException();
}

void runSampleTest(TestRunner& tr, bitmunk::test::Tester& tester)
{
   tr.group("Sample");
   
   const char* format = "mp3";
   char filename[100];
   snprintf(filename, 100, "/tmp/bmtestfile.%s", format);
   File file(filename);
   if(!file->exists())
   {
      string temp = filename;
      temp.append(" does not exist, not running test!");
      tr.warning(temp.c_str());
   }
   else
   {
      tr.test("30 second sample via InputStream");
      {
         // create time parser to produce 30 second sample
         MpegAudioTimeParser matp;
         matp.addTimeSet(10, 40);
         
         File inputFile("/tmp/bmtestfile.mp3");
         FileInputStream fis(inputFile);
         
         File sampleFile("/tmp/bmsample-mis.mp3");
         FileOutputStream fos(sampleFile);
         
         Id3v2TagWriter stripper(NULL);
         MutatorInputStream stripStream(&fis, false, &stripper, false);
         MutatorInputStream mis(&stripStream, false, &matp, false);
         
         char b[2048];
         int numBytes;
         while((numBytes = mis.read(b, 2048)) > 0)
         {
            fos.write(b, numBytes);
         }
         mis.close();
         
         assert(sampleFile->getLength() > 100000);
      }
      tr.passIfNoException();

      tr.test("30 second sample via OutputStream");
      {
         // create time parser to produce 30 second sample
         MpegAudioTimeParser matp;
         matp.addTimeSet(10, 40);
         
         File inputFile("/tmp/bmtestfile.mp3");
         FileInputStream fis(inputFile);
         
         File sampleFile("/tmp/bmsample-mos.mp3");
         FileOutputStream fos(sampleFile);
         
         Id3v2TagWriter stripper(NULL);
         MutatorOutputStream mos(&fos, false, &matp, false);
         MutatorOutputStream stripStream(&mos, false, &stripper, false);
         
         char b[2048];
         int numBytes;
         while((numBytes = fis.read(b, 2048)) > 0)
         {
            stripStream.write(b, numBytes);
         }
         fis.close();
         stripStream.close();

         assert(sampleFile->getLength() > 100000);
      }
      tr.passIfNoException();

   }
   
   tr.ungroup();
}

#define MO_TEST_NO_MAIN
#include "test-common.cpp"
#include "test-config.cpp"
#include "test-statemonitor.cpp"
#undef MO_TEST_NO_MAIN

class BmAllTester : public bitmunk::test::Tester
{
public:
   BmAllTester()
   {
      setName("bitmunk");
      addTester(new BmCommonTester());
      addTester(new BmStateMonitorTester());
   }

   ~BmAllTester() {}

   /**
    * Runs automatic unit tests.
    */
   virtual int runAutomaticTests(TestRunner& tr)
   {
      runNodeTest(tr, *this);
      runNodeConfigTest(tr, *this);
      runBtpExceptionTest(tr);
      runLoginTest(tr, *this);
      runNodeStopEventTest(tr, *this);
      //runSampleTest(tr, *this);
      
      return 0;
   }

   /**
    * Runs interactive unit tests.
    */
   virtual int runInteractiveTests(TestRunner& tr)
   {
      //runLoadModulesTest(tr, *this);
      return 0;
   }
};

BM_TEST_MAIN(BmAllTester)

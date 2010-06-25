/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include <iostream>
#include <sstream>

#include "bitmunk/common/CatalogInterface.h"
#include "bitmunk/common/Logging.h"
#include "bitmunk/common/Profile.h"
#include "bitmunk/common/Tools.h"
#include "bitmunk/data/Id3v2TagWriter.h"
#include "bitmunk/data/MpegAudioTimeParser.h"
#include "bitmunk/node/Node.h"
#include "bitmunk/test/Tester.h"
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
#include "monarch/http/HttpConnectionServicer.h"
#include "monarch/io/FileInputStream.h"
#include "monarch/io/FileOutputStream.h"
#include "monarch/io/MutatorInputStream.h"
#include "monarch/io/MutatorOutputStream.h"
#include "monarch/io/OStreamOutputStream.h"
#include "monarch/logging/OutputStreamLogger.h"
#include "monarch/modest/Kernel.h"
#include "monarch/net/Server.h"
#include "monarch/test/Test.h"
#include "monarch/test/TestModule.h"
#include "monarch/util/Convert.h"

using namespace std;
using namespace bitmunk::common;
using namespace bitmunk::data;
using namespace bitmunk::node;
using namespace bitmunk::protocol;
using namespace bitmunk::test;
using namespace monarch::config;
using namespace monarch::crypto;
using namespace monarch::data::json;
using namespace monarch::data::xml;
using namespace monarch::data;
using namespace monarch::event;
using namespace monarch::http;
using namespace monarch::io;
using namespace monarch::logging;
using namespace monarch::modest;
using namespace monarch::net;
using namespace monarch::rt;
using namespace monarch::test;
using namespace monarch::util;

namespace bm_tests_unit
{

static void runNodeTest(TestRunner& tr)
{
   tr.test("Node Initialization");
   {
      // load and start node
      Node* node = Tester::loadNode(tr);
      node->start();
      assertNoException();

      // stop and unload node
      node->stop();
      Tester::unloadNode(tr);
   }
   tr.passIfNoException();
}

void runNodeConfigTest(TestRunner& tr)
{
   tr.test("Node config");
   {
      // load and start node
      Node* node = Tester::loadNode(tr, "common");
      node->start();
      assertNoException();

      // stop and unload node
      node->stop();
      Tester::unloadNode(tr);
   }
   tr.passIfNoException();
}

static void assertSameExceptions(ExceptionRef& e0, ExceptionRef& e1)
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

static void runBtpExceptionTest_XML_1(ExceptionRef& e)
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

static void runBtpExceptionTest_JSON_1(ExceptionRef& e)
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

static void runBtpExceptionTypeTest(TestRunner& tr,
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

static void runBtpExceptionTest(TestRunner& tr)
{
   tr.group("Btp exception");

   runBtpExceptionTypeTest(tr, "XML", &runBtpExceptionTest_XML_1);
   runBtpExceptionTypeTest(tr, "JSON", &runBtpExceptionTest_JSON_1);

   tr.ungroup();
}

static void runLoginTest(TestRunner& tr)
{
   tr.group("Login");
   {
      bool success;
      // load and start node
      Node* node = Tester::loadNode(tr, "common");
      node->start();
      assertNoException();

      // NOTE: some of these are redundant with what load/unloadNode do.

      // logout
      node->logout(0);

      tr.test("login success");
      success = node->login("devuser", "password");
      assert(success);
      tr.passIfNoException();

      // logout
      node->logout(0);

      tr.test("login failure (invalid short password)");
      success = node->login("devuser", "");
      assert(!success);
      tr.passIfException();

      tr.test("login failure (invalid long password)");
      success = node->login("devuser", "badpassword");
      assert(!success);
      tr.passIfException();
 
      // logout
      node->logout(0);

      // stop and unload node
      node->stop();
      Tester::unloadNode(tr);
   }
   tr.ungroup();
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

static void runNodeStopEventTest(TestRunner& tr)
{
   tr.test("NodeStopEvent bug check");
   {
      // load and start node
      Node* node = Tester::loadNode(tr, "common");
      node->start();
      assertNoException();

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

      // stop and unload node
      node->stop();
      Tester::unloadNode(tr);
   }
   tr.passIfNoException();
}

static void runSampleTest(TestRunner& tr)
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

static bool run(TestRunner& tr)
{
   if(tr.isDefaultEnabled())
   {
      runNodeTest(tr);
      runNodeConfigTest(tr);
      runBtpExceptionTest(tr);
      runLoginTest(tr);
      runNodeStopEventTest(tr);
   }

   if(tr.isTestEnabled("sample-test"))
   {
      runSampleTest(tr);
   }

   return true;
};

} // end namespace

MO_TEST_MODULE_FN(
   "bitmunk.tests.unit.test", "1.0", bm_tests_unit::run)

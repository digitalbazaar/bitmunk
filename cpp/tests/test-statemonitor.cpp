/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/node/Node.h"
#include "bitmunk/test/Tester.h"
#include "monarch/test/Test.h"
#include "monarch/test/TestRunner.h"
#include "monarch/rt/DynamicObject.h"
#include "monarch/data/json/JsonWriter.h"
#include "bitmunk/node/NodeMonitor.h"

using namespace std;
using namespace bitmunk::common;
using namespace bitmunk::node;
using namespace monarch::config;
using namespace monarch::data::json;
using namespace monarch::io;
using namespace monarch::rt;
using namespace monarch::test;

static void runNodeMonitorTest(TestRunner& tr)
{
   tr.group("NodeMonitor");
   
   tr.test("empty");
   {
      NodeMonitor nm;
      DynamicObject expect;
      expect->setType(Map);
      DynamicObject all = nm.getAll();
      assertDynoCmp(expect, all);
   }
   tr.passIfNoException();
   
   tr.test("add1 init");
   {
      bool r;
      NodeMonitor nm;
      DynamicObject si;
      si["init"] = "foo";
      r = nm.addState("s", si);
      assert(r);
      
      DynamicObject expect;
      expect["s"] = "foo";
      
      DynamicObject all = nm.getAll();
      assertDynoCmp(expect, all);
   }
   tr.passIfNoException();
   
   tr.test("addN init");
   {
      bool r;
      NodeMonitor nm;
      DynamicObject si;
      si["s"]["init"] = "foo";
      si["u"]["init"] = (uint64_t)0;
      si["i"]["init"] = (int64_t)0;
      si["d"]["init"] = 0.;
      r = nm.addStates(si);
      assert(r);
      
      DynamicObject expect;
      expect["s"] = "foo";
      expect["u"] = (uint64_t)0;
      expect["i"] = (int64_t)0;
      expect["d"] = 0.;
      
      DynamicObject all = nm.getAll();
      assertDynoCmp(expect, all);
   }
   tr.passIfNoException();
   
   tr.test("remove");
   {
      bool r;
      NodeMonitor nm;
      DynamicObject si;
      si["n"]["init"] = (uint64_t)0;
      si["rem"]["init"] = (uint64_t)0;
      r = nm.addStates(si);
      assert(r);
      
      DynamicObject s;
      s["rem"].setNull();
      r = nm.removeStates(s);
      assert(r);
      
      DynamicObject expect;
      expect["n"] = (uint64_t)0;
      
      DynamicObject all = nm.getAll();
      assertDynoCmp(expect, all);
   }
   tr.passIfNoException();
   
   tr.test("set");
   {
      bool r;
      NodeMonitor nm;
      DynamicObject si;
      si["n"]["init"] = (uint64_t)0;
      r = nm.addStates(si);
      assert(r);
      
      DynamicObject s;
      s["n"] = (uint64_t)123;
      r = nm.setStates(s);
      assert(r);
      
      DynamicObject expect;
      expect["n"] = (uint64_t)123;
      
      DynamicObject all = nm.getAll();
      assertDynoCmp(expect, all);
   }
   tr.passIfNoException();
   
   tr.test("adj");
   {
      bool r;
      NodeMonitor nm;
      DynamicObject si;
      si["n"]["init"] = (uint64_t)0;
      r = nm.addStates(si);
      assert(r);
      
      DynamicObject s;
      s["n"] = (uint64_t)1;
      r = nm.adjustStates(s);
      assert(r);
      r = nm.adjustStates(s);
      assert(r);
      r = nm.adjustStates(s);
      assert(r);
      
      DynamicObject expect;
      expect["n"] = (uint64_t)3;
      
      DynamicObject all = nm.getAll();
      assertDynoCmp(expect, all);
   }
   tr.passIfNoException();
   
   tr.test("reset");
   {
      bool r;
      NodeMonitor nm;
      DynamicObject si;
      si["n"]["init"] = (uint64_t)100;
      r = nm.addStates(si);
      assert(r);
      
      DynamicObject s;
      s["n"] = (uint64_t)123;
      r = nm.setStates(s);
      assert(r);
      r = nm.resetStates(s);
      assert(r);
      
      DynamicObject expect;
      expect["n"] = (uint64_t)100;
      
      DynamicObject all = nm.getAll();
      assertDynoCmp(expect, all);
   }
   tr.passIfNoException();
   
   tr.test("adj neg");
   {
      bool r;
      NodeMonitor nm;
      DynamicObject si;
      si["n"]["init"] = (uint64_t)100;
      r = nm.addStates(si);
      assert(r);
      
      DynamicObject s;
      s["n"] = (int64_t)-1;
      r = nm.adjustStates(s);
      assert(r);
      
      DynamicObject expect;
      expect["n"] = (uint64_t)99;
      
      DynamicObject all = nm.getAll();
      assertDynoCmp(expect, all);
   }
   tr.passIfNoException();
   
   tr.test("getN");
   {
      bool r;
      NodeMonitor nm;
      DynamicObject si;
      si["a"]["init"] = "a";
      si["b"]["init"] = "b";
      si["c"]["init"] = "c";
      r = nm.addStates(si);
      assert(r);
      
      DynamicObject s;
      s["b"].setNull();
      s["c"].setNull();
      r = nm.getStates(s);
      assert(r);
      
      DynamicObject expect;
      expect["b"] = "b";
      expect["c"] = "c";
      
      assertDynoCmp(expect, s);
   }
   tr.passIfNoException();
   
   tr.test("add ex");
   {
      bool r;
      NodeMonitor nm;
      DynamicObject si;
      si["bad"] = 0;
      r = nm.addStates(si);
      assert(!r);
      assertException();
      Exception::clear();
   }
   tr.passIfNoException();
   
   tr.test("rem ex");
   {
      bool r;
      NodeMonitor nm;
      DynamicObject s;
      s["a"] = 0;
      r = nm.removeStates(s);
      assert(!r);
      assertException();
      Exception::clear();
   }
   tr.passIfNoException();
   
   // Not for use with async StateMonitor implementation.
   /*
   tr.test("set ex");
   {
      bool r;
      NodeMonitor nm;
      DynamicObject s;
      s["a"] = 0;
      r = nm.setStates(s);
      assert(!r);
      assertException();
      Exception::clear();
   }
   tr.passIfNoException();
   */
   
   tr.test("get ex");
   {
      bool r;
      NodeMonitor nm;
      DynamicObject s;
      s["a"] = 0;
      r = nm.getStates(s);
      assert(!r);
      assertException();
      Exception::clear();
   }
   tr.passIfNoException();
   
   tr.test("adj ex");
   {
      bool r;
      NodeMonitor nm;
      DynamicObject s;
      s["a"] = 1;
      r = nm.adjustStates(s);
      assert(!r);
      assertException();
      Exception::clear();
   }
   tr.passIfNoException();
   
   tr.test("adj type ex");
   {
      bool r;
      NodeMonitor nm;
      DynamicObject si;
      si["a"]["init"] = (uint64_t)0;
      r = nm.addStates(si);
      assert(r);
      
      // Not for use with async StateMonitor implementation.
      /*
      DynamicObject s;
      s["a"] = 0.;
      r = nm.adjustStates(s);
      assert(!r);
      assertException();
      Exception::clear();
      */
   }
   tr.passIfNoException();
   
   tr.test("txn adj ex");
   {
      // check bad transaction doesn't change values
      bool r;
      NodeMonitor nm;
      DynamicObject si;
      si["a"]["init"] = (uint64_t)10;
      si["b"]["init"] = (uint64_t)10;
      r = nm.addStates(si);
      assert(r);
      
      // Not for use with async StateMonitor implementation.
      /*
      DynamicObject s;
      s["a"] = (int64_t)-1;
      s["b"] = -1.;
      r = nm.adjustStates(s);
      assert(!r);
      assertException();
      Exception::clear();
      */
      
      DynamicObject expect;
      expect["a"] = (uint64_t)10;
      expect["b"] = (uint64_t)10;
      
      DynamicObject all = nm.getAll();
      assertDynoCmp(expect, all);
   }
   tr.passIfNoException();
   
   tr.test("no ex");
   {
      bool r;
      NodeMonitor nm;
      DynamicObject si;
      si["a"]["init"] = (uint64_t)10;
      r = nm.addStates(si);
      assert(r);
      
      DynamicObject s;
      s["bad"] = (int64_t)-1;
      r = nm.adjustStates(s);
      assert(!r);
      assertException();
      Exception::clear();
      
      DynamicObject expect;
      expect["a"] = (uint64_t)10;
      
      DynamicObject all = nm.getAll();
      assertDynoCmp(expect, all);
   }
   tr.passIfNoException();
   
   tr.ungroup();
}

class BmStateMonitorTester : public bitmunk::test::Tester
{
public:
   BmStateMonitorTester()
   {
      setName("StateMonitor");
   }

   /**
    * Run automatic unit tests.
    */
   virtual int runAutomaticTests(TestRunner& tr)
   {
      // run test
      runNodeMonitorTest(tr);
      
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
BM_TEST_MAIN(BmStateMonitorTester)
#endif

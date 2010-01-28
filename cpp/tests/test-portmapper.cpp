/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/portmapper/IPortMapper.h"
#include "bitmunk/test/Tester.h"
#include "monarch/rt/DynamicObject.h"
#include "monarch/rt/Exception.h"
#include "monarch/test/Test.h"
#include "monarch/test/TestRunner.h"

using namespace std;
using namespace bitmunk::common;
using namespace bitmunk::portmapper;
using namespace bitmunk::node;
using namespace monarch::config;
using namespace monarch::io;
using namespace monarch::rt;
using namespace monarch::test;

void runPortMapperTest(IPortMapper* ipm, TestRunner& tr)
{
   tr.group("portmapper");
   
   tr.test("add port mapping");
   {
      // FIXME:
   }
   tr.passIfNoException();
   
   tr.ungroup();
}

class BmPortMapperTester : public bitmunk::test::Tester
{
public:
   BmPortMapperTester()
   {
      setName("portmapper");
   }
   
   /**
    * Run automatic unit tests.
    */
   virtual int runAutomaticTests(TestRunner& tr)
   {
      // FIXME:
#if 0
      // create a client node
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
      
      // login the devuser
      node.login("devuser", "password");
      assertNoException();
      
      // get port mapper interface
      IPortMapper* ipm = dynamic_cast<IPortMapper*>(
         node.getModuleInterfaceByType("bitmunk.portmapper"));
      assert(ipm != NULL);
      
      // run test
      runPortMapperTest(ipm, tr);
      
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
BM_TEST_MAIN(BmPortMapperTester)
#endif

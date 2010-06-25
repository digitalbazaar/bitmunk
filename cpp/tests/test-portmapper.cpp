/*
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/portmapper/IPortMapper.h"
#include "bitmunk/test/Tester.h"
#include "monarch/rt/DynamicObject.h"
#include "monarch/rt/Exception.h"
#include "monarch/test/Test.h"
#include "monarch/test/TestModule.h"

using namespace std;
using namespace bitmunk::common;
using namespace bitmunk::portmapper;
using namespace bitmunk::node;
using namespace bitmunk::test;
using namespace monarch::config;
using namespace monarch::io;
using namespace monarch::rt;
using namespace monarch::test;

namespace bm_tests_portmapper
{

static void runPortMapperTest(IPortMapper* ipm, TestRunner& tr)
{
   tr.group("portmapper");
   
   tr.test("add port mapping");
   {
      // FIXME:
   }
   tr.passIfNoException();
   
   tr.ungroup();
}

static bool run(TestRunner& tr)
{
   if(tr.isTestEnabled("portmapper"))
   {
      // load and start node
      Node* node = Tester::loadNode(tr, "common");
      node->start();
      assertNoException();

      // get port mapper interface
      IPortMapper* ipm = dynamic_cast<IPortMapper*>(
         node->getModuleApiByType("bitmunk.portmapper"));
      assert(ipm != NULL);
      
      // run test
      runPortMapperTest(ipm, tr);

      // stop and unload node
      node->stop();
      Tester::unloadNode(tr);
   }

   return true;
};

} // end namespace

MO_TEST_MODULE_FN(
   "bitmunk.tests.portmapper.test", "1.0", bm_tests_portmapper::run)

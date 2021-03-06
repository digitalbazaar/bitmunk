/*
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk/test/Tester.h"
#include "monarch/test/Test.h"
#include "monarch/test/TestModule.h"


#include "bitmunk/test/Tester.h"
#include "monarch/rt/DynamicObject.h"
#include "monarch/rt/Exception.h"
#include "monarch/test/Test.h"
#include "monarch/test/TestModule.h"

#include <gtk/gtk.h>

using namespace std;
using namespace bitmunk::common;
using namespace bitmunk::node;
using namespace bitmunk::test;
using namespace monarch::config;
using namespace monarch::io;
using namespace monarch::rt;
using namespace monarch::test;

static gboolean createWindow(gpointer data)
{
   // create builder
   GtkBuilder* builder = gtk_builder_new();

   // load interface file
   gtk_builder_add_from_file(builder, "cpp/tests/test-gtkbuilder.xml", NULL);

   // connect any signals (NULL = user data which will be passed to signal
   // handlers... so we can use it to pass in a class that will store state)
   gtk_builder_connect_signals(builder, NULL);

   // get window
   GtkWidget* window = GTK_WIDGET(gtk_builder_get_object(builder, "window1"));

   // destroy builder (will not destroy top-level windows)
   g_object_unref(G_OBJECT(builder));

   // show window
   gtk_widget_show(window);

   // FIXME: must call this to clean up the window
   //gtk_widget_destroy(window);

   return FALSE;
}

static void runGtkBuilderTest(TestRunner& tr)
{
   tr.group("GtkBuilder");

   tr.test("create window");
   {
      gdk_threads_add_idle_full(
         G_PRIORITY_DEFAULT_IDLE,
         createWindow, NULL, NULL);
   }
   tr.passIfNoException();

   tr.ungroup();
}


static bool run(TestRunner& tr)
{
   if(tr.isTestEnabled("gtkbuilder"))
   {
      // load and start node
      Node* node = Tester::loadNode(tr);
      node->start();
      assertNoException();

      // register event observer of all events
      BmEventReactorTesterObserver obs;
      node->getEventController()->registerObserver(&obs, "*");

      // run test
      runGtkBuilderTest(tr);

      printf("Hit CTRL+C to end test...\n");

      // wait forever
      Thread::sleep(0);

      // stop and unload node
      node->stop();
      Tester::unloadNode(tr);
   }

   return true;
};

} // end namespace

MO_TEST_MODULE_FN(
   "bitmunk.tests.eventreactor.test", "1.0", bm_tests_eventreactor::run)

/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/test/Tester.h"
#include "monarch/rt/DynamicObject.h"
#include "monarch/rt/Exception.h"
#include "monarch/test/Test.h"
#include "monarch/test/TestRunner.h"

#include <gtk/gtk.h>

using namespace std;
using namespace bitmunk::common;
using namespace bitmunk::node;
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

class BmGtkBuilderTester : public bitmunk::test::Tester
{
public:
   BmGtkBuilderTester()
   {
      setName("GtkBuilder");
   }

   /**
    * Run automatic unit tests.
    */
   virtual int runAutomaticTests(TestRunner& tr)
   {
      return 0;
   }

   /**
    * Runs interactive unit tests.
    */
   virtual int runInteractiveTests(TestRunner& tr)
   {
      // create a client node
      Node node;
      {
         bool success;
         success = setupNode(&node);
         assertNoException();
         assert(success);
         Config config;
         config["node"]["modulePath"] = BPE_MODULES_DIRECTORY;
         success = setupPeerNode(&node, &config);
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

      // run test
      runGtkBuilderTest(tr);

      printf("Hit CTRL+C to end test...\n");

      // wait forever
      Thread::sleep(0);

      // logout of client node
      node.logout(0);
      node.stop();

      return 0;
   }
};

#ifndef MO_TEST_NO_MAIN
BM_TEST_MAIN(BmGtkBuilderTester)
#endif

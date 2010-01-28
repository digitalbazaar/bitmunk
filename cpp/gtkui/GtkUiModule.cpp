/*
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/gtkui/GtkUiModule.h"

#include <gtk/gtk.h>

using namespace monarch::kernel;
using namespace monarch::logging;
using namespace monarch::modest;
using namespace monarch::rt;
using namespace bitmunk::gtkui;
using namespace bitmunk::node;

// Logging category initialized during module initialization.
Category* BM_GTKUI_CAT;

GtkUiModule::GtkUiModule() :
   MicroKernelModule("bitmunk.gtkui.GtkUi", "1.0"),
   mGtkThread(NULL)
{
}

GtkUiModule::~GtkUiModule()
{
}

DynamicObject GtkUiModule::getDependencyInfo()
{
   DynamicObject rval;

   // set name, version, and type for this module
   rval["name"] = mId.name;
   rval["version"] = mId.version;
   rval["type"] = "bitmunk.gtkui";

   // no dependencies
   rval["dependencies"]->setType(Array);

   return rval;
}

// runnable for the main gtk loop
class GtkMainLoop : public Runnable
{
public:
   GtkMainLoop() {};
   virtual ~GtkMainLoop() {};
   virtual void run()
   {
      gdk_threads_enter();
      gtk_main();
      gdk_threads_leave();
   };
};

bool GtkUiModule::initialize(MicroKernel* k)
{
   bool rval = true;

   BM_GTKUI_CAT = new Category(
      "BM_GTKUI",
      "Bitmunk GTK UI",
      NULL);

   // initialize gtk thread support
   g_thread_init(NULL);
   gdk_threads_init();

   // initialize gtk
   int argc = 0;
   char** argv = NULL;
   gtk_init(&argc, &argv);

   // start gtk main loop
   RunnableRef r = new GtkMainLoop();
   mGtkThread = new Thread(r);
   rval = mGtkThread->start();
   if(!rval)
   {
      delete mGtkThread;
      mGtkThread = NULL;
      ExceptionRef e = new Exception(
         "Could not start gtk main loop.",
         "bitmunk.gtkui.GtkMainLoopFailure");
      Exception::push(e);
   }

   return rval;
}

// called to exit the main loop
static gboolean exitMainLoop(gpointer data)
{
   gtk_main_quit();
   return FALSE;
}

void GtkUiModule::cleanup(MicroKernel* k)
{
   if(mGtkThread != NULL)
   {
      gdk_threads_add_idle_full(
         G_PRIORITY_DEFAULT_IDLE,
         exitMainLoop, NULL, NULL);

      // wait for main loop to finish
      mGtkThread->join();
      delete mGtkThread;
   }

   delete BM_GTKUI_CAT;
   BM_GTKUI_CAT = NULL;
}

MicroKernelModuleApi* GtkUiModule::getApi(MicroKernel* k)
{
   // no API
   return NULL;
}

Module* createModestModule()
{
   return new GtkUiModule();
}

void freeModestModule(Module* m)
{
   delete m;
}

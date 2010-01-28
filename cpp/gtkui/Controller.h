/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_gtkui_Controller_H
#define bitmunk_gtkui_Controller_H

#include "bitmunk/gtkui/SignalHandler.h"
#include "bitmunk/node/Node.h"
#include "monarch/rt/Runnable.h"
#include "monarch/util/StringTools.h"

#include <gtk/gtk.h>
#include <map>

namespace bitmunk
{
namespace gtkui
{

/**
 * A Controller is used to create and destroy UI widgets and to their signals.
 *
 * @author Dave Longley
 */
class Controller
{
protected:
   /**
    * The associated Bitmunk node.
    */
   bitmunk::node::Node* mNode;

   /**
    * The signal handler map used by this controller.
    */
   typedef std::map<const char*, SignalHandler*, monarch::util::StringComparator>
      HandlerMap;
   HandlerMap mHandlerMap;

   /**
    * A lock used to send an receive signals between the gtk thread and
    * any other thread.
    */
   monarch::rt::ExclusiveLock mSignalLock;

public:
   /**
    * Creates a new Controller.
    */
   Controller();

   /**
    * Destructs this Controller.
    */
   virtual ~Controller();

   /**
    * Initializes this Controller, setting up signal handlers, etc.
    *
    * @param node the associated Bitmunk node.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool init(bitmunk::node::Node* node) = 0;

   /**
    * Cleans up this Controller.
    */
   virtual void cleanup() = 0;

   /**
    * Gets a signal handler.
    *
    * @param name the name of the signal handler to retrieve.
    *
    * @return the signal handler or NULL.
    */
   virtual SignalHandler* getSignalHandler(const char* name);

protected:
   /**
    * Adds a new signal handler.
    *
    * @param name the name of the signal handler.
    * @param handler the signal handler.
    */
   virtual void addSignalHandler(const char* name, SignalHandler* handler);

   /**
    * Connects signal handlers for all gtk objects loaded by the given
    * GtkBuilder.
    *
    * @param builder the builder to connect signal handlers for.
    */
   virtual void connectSignalHandlers(GtkBuilder* builder);

   /**
    * Runs GTK code on the GTK thread and returns when its finished. This call
    * will block until the GTK code has been run.
    *
    * @param runnable the Runnable to execute on the GTK thread.
    *
    * @return true if successful, false if an Exception was set by the runnable.
    */
   virtual bool runGtkCode(monarch::rt::RunnableRef runnable);

   /**
    * Gets a GtkWidget by name from a builder.
    *
    * @param builder the builder to get the widget from.
    * @param name the name of the GtkWidget.
    *
    * @return the GtkWidget or NULL if none found.
    */
   virtual GtkWidget* getWidget(GtkBuilder* builder, const char* name);
};

} // end namespace gtkui
} // end namespace bitmunk
#endif

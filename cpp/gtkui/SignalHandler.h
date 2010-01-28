/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_gtkui_SignalHandler_H
#define bitmunk_gtkui_SignalHandler_H

#include <gtk/gtk.h>

namespace bitmunk
{
namespace gtkui
{

/**
 * A SignalHandler handles a signal or an event that is sent by a gtk object.
 *
 * @author Dave Longley
 */
class SignalHandler
{
public:
   enum SignalType
   {
      WidgetSignal,
      WidgetEvent,
      DialogResponse
   };

public:
   /**
    * Creates a new SignalHandler.
    */
   SignalHandler() {};

   /**
    * Destructs this SignalHandler.
    */
   virtual ~SignalHandler() {};

   /**
    * Returns the type of signal that can be handled.
    *
    * @return the type of signal that can be handled.
    */
   virtual SignalType getSignalType() = 0;

   /**
    * Handles the signal sent by the given gtk widget.
    *
    * @param widget the gtk widget that sent the signal.
    */
   virtual void handleWidgetSignal(GtkWidget* widget) {};

   /**
    * Handles the event sent by the given gtk widget.
    *
    * @param widget the gtk widget that sent the event.
    * @param event the gdk event.
    *
    * @return true to stop propagating events, false to continue.
    */
   virtual bool handleWidgetEvent(GtkWidget* widget, GdkEvent* event)
   {
      return false;
   };

   /**
    * Handles a response from a dialog.
    *
    * @param dialog the gtk dialog that sent the response.
    * @param responseId the ID of the response.
    */
   virtual void handleDialogResponse(GtkDialog* dialog, gint responseId) {};
};

} // end namespace gtkui
} // end namespace bitmunk
#endif

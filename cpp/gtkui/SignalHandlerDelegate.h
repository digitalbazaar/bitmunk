/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_gtkui_SignalHandlerDelegate_H
#define bitmunk_gtkui_SignalHandlerDelegate_H

#include "bitmunk/gtkui/SignalHandler.h"

namespace bitmunk
{
namespace gtkui
{

/**
 * A SignalHandlerDelegate handles a signal that is sent by a gtk object via
 * a Controller's class member method.
 *
 * @author Dave Longley
 */
template<typename Controller>
class SignalHandlerDelegate : public SignalHandler
{
protected:
   /**
    * Typedef for signal handler member method.
    */
   typedef void (Controller::*SignalMethod)(GtkWidget* widget);

   /**
    * Typedef for event handler member method.
    */
   typedef bool (Controller::*EventMethod)(GtkWidget* widget, GdkEvent* event);

   /**
    * Typedef for dialog response handler member method.
    */
   typedef void (Controller::*DialogResponseMethod)
      (GtkDialog* dialog, gint responseId);

   /**
    * The controller instance.
    */
   Controller* mController;

   /**
    * The type of signal handling method to call.
    */
   SignalHandler::SignalType mSignalType;

   /**
    * The signal handler member method to call.
    */
   union
   {
      SignalMethod mSignalMethod;
      EventMethod mEventMethod;
      DialogResponseMethod mDialogResponseMethod;
   };

public:
   /**
    * Creates a new SignalHandlerDelegate.
    *
    * @param controller the controller instance.
    * @param method the controller's signal handler method to invoke.
    */
   SignalHandlerDelegate(Controller* controller, SignalMethod method);
   SignalHandlerDelegate(Controller* controller, EventMethod method);
   SignalHandlerDelegate(Controller* controller, DialogResponseMethod method);

   /**
    * Destructs this SignalHandlerDelegate.
    */
   virtual ~SignalHandlerDelegate();

   /**
    * Returns the type of signal that can be handled.
    *
    * @return the type of signal that can be handled.
    */
   virtual SignalHandler::SignalType getSignalType();

   /**
    * Handles the signal sent by the given gtk widget.
    *
    * @param widget the gtk widget that sent the signal.
    */
   virtual void handleWidgetSignal(GtkWidget* widget);

   /**
    * Handles the event sent by the given gtk widget.
    *
    * @param widget the gtk widget that sent the event.
    * @param event the gdk event.
    *
    * @return true to continue propagating events, false to stop.
    */
   virtual bool handleWidgetEvent(GtkWidget* widget, GdkEvent* event);

   /**
    * Handles a response from a dialog.
    *
    * @param dialog the gtk dialog that sent the response.
    * @param responseId the ID of the response.
    */
   virtual void handleDialogResponse(GtkDialog* dialog, gint responseId);
};

template<typename Controller>
SignalHandlerDelegate<Controller>::SignalHandlerDelegate(
   Controller* controller, SignalMethod method) :
   mController(controller),
   mSignalType(SignalHandler::WidgetSignal),
   mSignalMethod(method)
{
}

template<typename Controller>
SignalHandlerDelegate<Controller>::SignalHandlerDelegate(
   Controller* controller, EventMethod method) :
   mController(controller),
   mSignalType(SignalHandler::WidgetEvent),
   mEventMethod(method)
{
}

template<typename Controller>
SignalHandlerDelegate<Controller>::SignalHandlerDelegate(
   Controller* controller, DialogResponseMethod method) :
   mController(controller),
   mSignalType(SignalHandler::DialogResponse),
   mDialogResponseMethod(method)
{
}

template<typename Controller>
SignalHandlerDelegate<Controller>::~SignalHandlerDelegate()
{
}

template<typename Controller>
SignalHandler::SignalType SignalHandlerDelegate<Controller>::getSignalType()
{
   return mSignalType;
}

template<typename Controller>
void SignalHandlerDelegate<Controller>::handleWidgetSignal(GtkWidget* widget)
{
   (mController->*mSignalMethod)(widget);
}

template<typename Controller>
bool SignalHandlerDelegate<Controller>::handleWidgetEvent(
   GtkWidget* widget, GdkEvent* event)
{
   return (mController->*mEventMethod)(widget, event);
}

template<typename Controller>
void SignalHandlerDelegate<Controller>::handleDialogResponse(
   GtkDialog* dialog, gint responseId)
{
   (mController->*mDialogResponseMethod)(dialog, responseId);
}

} // end namespace gtkui
} // end namespace bitmunk
#endif

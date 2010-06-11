/*
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/gtkui/Controller.h"

#include "bitmunk/common/Logging.h"
#include "bitmunk/common/TypeDefinitions.h"

using namespace std;
using namespace monarch::config;
using namespace monarch::io;
using namespace monarch::rt;
using namespace bitmunk::gtkui;
using namespace bitmunk::common;
using namespace bitmunk::node;

#define GOBJECT_KEY_HANDLER "_gobject_key_handler"

Controller::Controller() :
   mNode(NULL)
{
}

Controller::~Controller()
{
   // clean up handlers
   for(HandlerMap::iterator i = mHandlerMap.begin();
       i != mHandlerMap.end(); ++i)
   {
      free((char*)i->first);
      delete i->second;
   }
}

SignalHandler* Controller::getSignalHandler(const char* name)
{
   SignalHandler* rval = NULL;

   HandlerMap::iterator i = mHandlerMap.find(name);
   if(i != mHandlerMap.end())
   {
      rval = i->second;
   }

   return rval;
}

void Controller::addSignalHandler(const char* name, SignalHandler* handler)
{
   // Note: Will leak memory if handler names are reused (programmer error).

   // insert handler name
   mHandlerMap.insert(make_pair(strdup(name), handler));
}

/**
 * Handles all gtk widget signals that include connect object by passing them
 * off to a SignalHandler.
 *
 * @param widget the widget to handle the signal for.
 * @param userData the user data pointer to the connect object to use.
 */
static void _handleWidgetSignalWithConnectObject(
   GtkWidget* widget, gpointer userData)
{
   // get signal handler out of connect object's user data map
   GObject* connectObject = static_cast<GObject*>(userData);
   gpointer ptr = g_object_get_data(connectObject, GOBJECT_KEY_HANDLER);
   SignalHandler* h = static_cast<SignalHandler*>(ptr);
   h->handleWidgetSignal(widget);
}

/**
 * Handles all gtk widget events that include connect object by passing them
 * off to a SignalHandler.
 *
 * @param widget the widget to handle the event for.
 * @param event the event to handle.
 * @param userData the user data pointer to the connect object to use.
 */
static gboolean _handleWidgetEventWithConnectObject(
   GtkWidget* widget, GdkEvent* event, gpointer userData)
{
   // get signal handler out of connect object's user data map
   GObject* connectObject = static_cast<GObject*>(userData);
   gpointer ptr = g_object_get_data(connectObject, GOBJECT_KEY_HANDLER);
   SignalHandler* h = static_cast<SignalHandler*>(ptr);
   return h->handleWidgetEvent(widget, event) ? TRUE : FALSE;
}

/**
 * Handles all gtk dialog responses that include connect object by passing them
 * off to a SignalHandler.
 *
 * @param dialog the dialog that sent the response.
 * @param responseId the response ID.
 * @param userData the user data pointer to the connect object to use.
 */
static void _handleDialogResponseWithConnectObject(
   GtkDialog* dialog, gint responseId,  gpointer userData)
{
   // get signal handler out of connect object's user data map
   GObject* connectObject = static_cast<GObject*>(userData);
   gpointer ptr = g_object_get_data(connectObject, GOBJECT_KEY_HANDLER);
   SignalHandler* h = static_cast<SignalHandler*>(ptr);
   h->handleDialogResponse(dialog, responseId);
}

/**
 * Handles all gtk widget signals by passing them off to a SignalHandler.
 *
 * @param widget the widget to handle the signal for.
 * @param userData the user data pointer to the SignalHandler to use.
 */
static void _handleWidgetSignal(GtkWidget* widget, gpointer userData)
{
   SignalHandler* h = static_cast<SignalHandler*>(userData);
   h->handleWidgetSignal(widget);
}

/**
 * Handles all gtk widget events by passing them off to a SignalHandler.
 *
 * @param widget the widget to handle the event for.
 * @param event the event to handle.
 * @param userData the user data pointer to the SignalHandler to use.
 */
static gboolean _handleWidgetEvent(
   GtkWidget* widget, GdkEvent* event, gpointer userData)
{
   SignalHandler* h = static_cast<SignalHandler*>(userData);
   return h->handleWidgetEvent(widget, event) ? TRUE : FALSE;
}

/**
 * Handles all gtk dialog responses by passing them off to a SignalHandler.
 *
 * @param dialog the dialog that sent the response.
 * @param responseId the response ID.
 * @param userData the user data pointer to the SignalHandler to use.
 */
static void _handleDialogResponse(
   GtkDialog* dialog, gint responseId,  gpointer userData)
{
   SignalHandler* h = static_cast<SignalHandler*>(userData);
   h->handleDialogResponse(dialog, responseId);
}

/**
 * Connect an object's signals to the appropriate handlers.
 *
 * @param builder the builder that created the object.
 * @param object the object to connect.
 * @param signalName the name of the signal to connect.
 * @param handlerName the name of the handler to use.
 * @param connectObject if non-NULL, use g_signal_connect_object.
 * @param flags the connection flags to use.
 * @param userData a pointer to mapping of signals to handler pointers.
 */
static void _connectSignals(
   GtkBuilder* builder, GObject* object,
   const gchar* signalName, const gchar* handlerName,
   GObject* connectObject, GConnectFlags flags,
   gpointer userData)
{
   const char* hn = (const char*)handlerName;

   // get controller's signal handler associated with handler name, log
   // cases where there is no controller signal handler found
   Controller* controller = static_cast<Controller*>(userData);
   SignalHandler* h = controller->getSignalHandler(hn);
   if(h != NULL)
   {
      SignalHandler::SignalType t = h->getSignalType();
      GCallback cb = NULL;

      if(connectObject != NULL)
      {
         switch(t)
         {
            case SignalHandler::WidgetSignal:
               cb = G_CALLBACK(_handleWidgetSignalWithConnectObject);
               break;
            case SignalHandler::WidgetEvent:
               cb = G_CALLBACK(_handleWidgetEventWithConnectObject);
               break;
            case SignalHandler::DialogResponse:
               cb = G_CALLBACK(_handleDialogResponseWithConnectObject);
               break;
         }

         // save handler in connect object
         g_object_set_data(connectObject, GOBJECT_KEY_HANDLER, h);
         g_signal_connect_object(object, signalName, cb, connectObject, flags);
      }
      else
      {
         switch(t)
         {
            case SignalHandler::WidgetSignal:
               cb = G_CALLBACK(_handleWidgetSignal);
               break;
            case SignalHandler::WidgetEvent:
               cb = G_CALLBACK(_handleWidgetEvent);
               break;
            case SignalHandler::DialogResponse:
               cb = G_CALLBACK(_handleDialogResponse);
               break;
         }

         // pass handler as data
         g_signal_connect_data(object, signalName, cb, h, NULL, flags);
      }
   }
   else
   {
      MO_CAT_WARNING(BM_GTKUI_CAT,
         "No gtk signal handler found for '%s'", hn);
   }
}

void Controller::connectSignalHandlers(GtkBuilder* builder)
{
   // connect any signals and pass self to them
   gtk_builder_connect_signals_full(builder, _connectSignals, this);
}

/**
 * This struct is used when arbitrary code must be executed on the gtk thread.
 */
struct GtkThreadUserData
{
   monarch::rt::Runnable* runnable;
   monarch::rt::ExclusiveLock* signalLock;
   monarch::rt::ExceptionRef exception;
};

/**
 * Runs arbitrary code inside of the gtk thread.
 *
 * @param userData the associated user data.
 *
 * @return false to indicate this method should only be called once.
 */
static gboolean _runGtkCode(gpointer userData)
{
   // clear any existing exceptions
   Exception::clear();

   // interpret the user data as a GtkThreadUserData
   GtkThreadUserData* d = static_cast<GtkThreadUserData*>(userData);

   // execute the runnable
   d->runnable->run();

   // save any exception that occurred
   if(Exception::isSet())
   {
      d->exception = Exception::get();
   }

   // send signal that the code has completed
   d->signalLock->lock();
   d->signalLock->notifyAll();
   d->signalLock->unlock();

   return FALSE;
}

bool Controller::runGtkCode(RunnableRef runnable)
{
   bool rval = true;

   // create user data to be made available on gtk thread
   GtkThreadUserData d;
   d.runnable = &(*runnable);
   d.signalLock = &mSignalLock;

   // lock to receive signal, schedule gtk code, wait for signal
   mSignalLock.lock();
   gdk_threads_add_idle_full(G_PRIORITY_DEFAULT_IDLE, _runGtkCode, &d, NULL);
   mSignalLock.wait();
   mSignalLock.unlock();

   // set any exception that occurred on the gtk thread
   if(!d.exception.isNull())
   {
      Exception::set(d.exception);
      rval = false;
   }

   return rval;
}

GtkWidget* Controller::getWidget(GtkBuilder* builder, const char* name)
{
   GtkWidget* rval = GTK_WIDGET(gtk_builder_get_object(builder, name));
   return rval;
}

/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_apps_Bitmunk_h
#define bitmunk_apps_Bitmunk_h

#include "bitmunk/app/App.h"
#include "monarch/event/ObserverDelegate.h"
#include "monarch/io/File.h"

namespace bitmunk
{
namespace apps
{

/**
 * Top-level Bitmunk App.
 *
 * @author David I. Lehn
 * @author Dave Longley
 */
class Bitmunk : public bitmunk::app::App
{
protected:
   /**
    * Observer for Node restart event.
    */
   monarch::event::ObserverRef mNodeRestartObserver;

   /**
    * Observer for Node shutdown event.
    */
   monarch::event::ObserverRef mNodeShutdownObserver;

public:
   /**
    * Create a Bitmunk instance.
    */
   Bitmunk();

   /**
    * Deconstruct this Bitmunk instance.
    */
   virtual ~Bitmunk();

   /**
    * {@inheritDoc}
    */
   virtual bool initialize();

   /**
    * {@inheritDoc}
    */
   virtual void cleanup();

   /**
    * {@inheritDoc}
    */
   virtual bool initConfigs(monarch::config::Config& defaults);

   /**
    * {@inheritDoc}
    */
   virtual monarch::rt::DynamicObject getCommandLineSpec(
      monarch::config::Config& cfg);

   /**
    * {@inheritDoc}
    */
   virtual monarch::rt::DynamicObject getWaitEvents();

   /**
    * Runs the main application.
    */
   virtual bool run();

protected:
   /**
    * Get a list of module path the kernel should load to start a node.
    *
    * @return a list of modules to load.
    */
   virtual monarch::io::FileList getModulePaths();

   /**
    * Load node modules and setup related events.
    *
    * @return true on success, false on failure and exception set.
    */
   virtual bool startNode();

   /**
    * Stop node, unload node modules, and teardown related events.
    *
    * @param restarting true if attempting a restart
    *
    * @return true on success, false on failure and exception set.
    */
   virtual bool stopNode(bool restarting = false);

   /**
    * Restart a node with stopNode() and startNode().
    *
    * @return true on success, false on failure and exception set.
    */
   virtual bool restartNode();

   /**
    * Event handler for restart events.
    */
   virtual void restartNodeHandler(monarch::event::Event& e);

   /**
    * Event handler for shutdown events.
    */
   virtual void shutdownNodeHandler(monarch::event::Event& e);
};

} // end namespace apps
} // end namespace bitmunk

#endif

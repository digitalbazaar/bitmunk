/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "bitmunk.h"

#include "monarch/app/App.h"
#include "monarch/app/AppFactory.h"
#include "monarch/app/AppRunner.h"
#include "monarch/config/ConfigManager.h"
#include "monarch/data/json/JsonReader.h"
#include "monarch/data/json/JsonWriter.h"
#include "monarch/modest/Module.h"
#include "monarch/util/Macros.h"
#include "monarch/rt/DynamicObject.h"
#include "bitmunk/app/App.h"
#include "bitmunk/common/Logging.h"
#include "bitmunk/node/Node.h"

using namespace std;
using namespace monarch::app;
using namespace monarch::config;
using namespace monarch::data::json;
using namespace monarch::event;
using namespace monarch::fiber;
using namespace monarch::io;
using namespace monarch::kernel;
using namespace monarch::logging;
using namespace monarch::modest;
using namespace monarch::net;
using namespace monarch::rt;
using namespace bitmunk::apps;
using namespace bitmunk::common;
using namespace bitmunk::node;

#define APP_NAME "bitmunk.apps.Bitmunk"

#define BITMUNK_NODE_RESTART_EVENT "bitmunk.node.Node.restart"
#define BITMUNK_NODE_SHUTDOWN_EVENT "bitmunk.node.Node.shutdown"

#define BITMUNK_NAME "Bitmunk"

Bitmunk::Bitmunk()
{
}

Bitmunk::~Bitmunk()
{
}

bool Bitmunk::initialize()
{
   bool rval = bitmunk::app::App::initialize();
   if(rval)
   {
      setName(BITMUNK_NAME);
      setVersion(PACKAGE_VERSION);
   }
   return rval;
}

bool Bitmunk::initConfigs(Config& defaults)
{
   bool rval = bitmunk::app::App::initConfigs(defaults);
   if(rval)
   {
      // set defaults
      Config& cm = defaults[ConfigManager::MERGE];
      cm["node"]["handlers"]->setType(Map);
      cm["node"]["events"]->setType(Map);

      // add to config manager
      rval = getConfigManager()->addConfig(defaults);
   }

   return rval;
}

DynamicObject Bitmunk::getCommandLineSpec(Config& cfg)
{
   // initialize config
   Config& options = cfg[ConfigManager::MERGE];
   options->setType(Map);

   DynamicObject spec;
   spec["help"] =
"Bitmunk options\n"
"      --bitmunk-module-path PATH\n"
"                      A colon separated list of Node modules or directories\n"
"                      where Node modules are stored. May be specified multiple\n"
"                      times. Appends to BITMUNK_MODULE_PATH.\n"
"      --profile-path PATH\n"
"                      The directory where profiles are stored.\n"
"  -p, --port PORT     The server port to use for all incoming connections.\n"
"  -a, --auto-login    Auto-login to the node.\n"
"  -U, --user USER     Node user for auto-login.\n"
"  -P, --password PASSWORD\n"
"                      Node password for auto-login.\n"
"\n";

   DynamicObject opt;

   opt = spec["options"]->append();
   opt["short"] = "-a";
   opt["long"] = "--auto-login";
   opt["setTrue"]["root"] = options;
   opt["setTrue"]["path"] = "node.login.auto";

   opt = spec["options"]->append();
   opt["short"] = "-p";
   opt["long"] = "--port";
   opt["arg"]["root"] = options;
   opt["arg"]["path"] = "node.port";
   opt["arg"]["type"] = (uint64_t)0;
   opt["argError"] = "No port specified.";

   opt = spec["options"]->append();
   opt["long"] = "--bitmunk-module-path";
   opt["append"]["root"] = options;
   opt["append"]["path"] = "node.modulePath";
   opt["argError"] = "No node module path specified.";

   opt = spec["options"]->append();
   opt["long"] = "--profile-dir";
   opt["arg"]["root"] = options;
   opt["arg"]["path"] = "node.profilePath";
   opt["argError"] = "No path specified.";

   opt = spec["options"]->append();
   opt["short"] = "-U";
   opt["long"] = "--user";
   opt["arg"]["root"] = options;
   opt["arg"]["path"] = "node.login.username";
   opt["argError"] = "No user specified.";

   opt = spec["options"]->append();
   opt["short"] = "-P";
   opt["long"] = "--password";
   opt["arg"]["root"] = options;
   opt["arg"]["path"] = "node.login.password";
   opt["argError"] = "No password specified.";

   return spec;
}

FileList Bitmunk::getModulePaths()
{
   FileList rval;

   Config cfg = getConfig()["node"];
   ConfigIterator mpi = cfg["modulePath"].getIterator();
   while(mpi->hasNext())
   {
      const char* path = mpi->next()->getString();
      FileList pathList = File::parsePath(path);
      rval->concat(*pathList);
   }

   return rval;
}

bool Bitmunk::startNode()
{
   bool rval;
   // setup app wait event
   // The bitmunk app run() call will finish normally but we must setup a
   // wait event so the kernel knows bitmunk is still running.
   //...

   // observe node events
   mNodeRestartObserver =
      new ObserverDelegate<Bitmunk>(this, &Bitmunk::restartNodeHandler);
   getKernel()->getEventController()->registerObserver(
      &(*mNodeRestartObserver), BITMUNK_NODE_RESTART_EVENT);
   MO_CAT_DEBUG(BM_APP_CAT,
      "Bitmunk registered for " BITMUNK_NODE_RESTART_EVENT);

   /*
   Currently shutdown event will cause kernel to stop everything so no need
   to handle it here.  At some point need node vs kernel shutdown handling.

   mNodeShutdownObserver =
      new ObserverDelegate<Bitmunk>(this, &Bitmunk::shutdownNodeHandler);
   getKernel()->getEventController()->registerObserver(
      &(*mNodeShutdownObserver), BITMUNK_NODE_SHUTDOWN_EVENT);
   MO_CAT_DEBUG(BM_APP_CAT,
      "Bitmunk registered for " BITMUNK_NODE_SHUTDOWN_EVENT);
   */

   // FIXME: change to use the builtin module path stuff? won't want to do
   // that if we want to be able to restart the node without restarting
   // the kernel...

   // get node and other related module paths
   FileList modulePaths = getModulePaths();
   // load all module paths at once
   rval = getKernel()->loadModules(modulePaths);
   if(rval)
   {
      Node* node = dynamic_cast<Node*>(
         getKernel()->getModuleApi("bitmunk.node.Node"));
      if(node == NULL)
      {
         ExceptionRef e = new Exception(
            "Could not load Node interface. No compatible Node module found.",
            APP_NAME ".InvalidNodeModule");
         Exception::set(e);
         rval = false;
      }
      else
      {
         rval = node->start();
      }
   }

   // bitmunk ready
   if(rval)
   {
      // show start up time
      MO_CAT_INFO(BM_APP_CAT, "Started in %" PRIu64 " ms",
         getAppRunner()->getTimer()->getElapsedMilliseconds());

      // send ready event
      MO_CAT_INFO(BM_APP_CAT, "Ready.");
      Event e;
      e["type"] = "bitmunk.app.Bitmunk.ready";
      getKernel()->getEventController()->schedule(e);
   }

   return rval;
}

bool Bitmunk::stopNode(bool restarting)
{
   bool rval = true;

   // stop watching for node events
   getKernel()->getEventController()->unregisterObserver(
      &(*mNodeRestartObserver), BITMUNK_NODE_RESTART_EVENT);
   mNodeRestartObserver.setNull();

   /*
   No need to implement this until node can be shutdown without shutting down
   the kernel.

   if(!restarting)
   {
      undo app wait event
   }
   */

   Node* node = dynamic_cast<Node*>(
      getKernel()->getModuleApi("bitmunk.node.Node"));
   if(node != NULL)
   {
      node->stop();
      // unload node module and all modules depending on it
      rval = getKernel()->unloadModule("bitmunk.node.Node");
   }

   return rval;
}

bool Bitmunk::restartNode()
{
   return stopNode(true) && startNode();
}

void Bitmunk::restartNodeHandler(Event& e)
{
   MO_CAT_INFO(BM_APP_CAT, "Handling node restart event.");
   //bool success =
   restartNode();
   // FIXME: handle exceptions
}

void Bitmunk::shutdownNodeHandler(Event& e)
{
   MO_CAT_INFO(BM_APP_CAT, "Handling node shutdown event.");
   //bool success =
   stopNode();
   // FIXME: handle exceptions
}

DynamicObject Bitmunk::getWaitEvents()
{
   DynamicObject rval;
   DynamicObject event;
   event["id"] = APP_NAME;
   event["type"] = APP_NAME ".wait";
   rval->append(event);
   return rval;
}

bool Bitmunk::run()
{
   bool rval = true;

   // Dump preliminary start-up information
   MO_CAT_INFO(BM_APP_CAT, "Bitmunk");
   MO_CAT_INFO(BM_APP_CAT, "Version: %s", PACKAGE_VERSION);

   Config cfg = getConfig()["node"];

   MO_CAT_INFO(BM_APP_CAT, "Modules path: %s",
      JsonWriter::writeToString(cfg["modulePath"]).c_str());

   rval = startNode();

   return rval;
}

class BitmunkFactory : public AppFactory
{
public:
   BitmunkFactory() : AppFactory(APP_NAME, "1.0")
   {
   }

   virtual ~BitmunkFactory() {}

   virtual App* createApp()
   {
      return new Bitmunk();
   }
};

Module* createModestModule()
{
   return new BitmunkFactory();
}

void freeModestModule(Module* m)
{
   delete m;
}

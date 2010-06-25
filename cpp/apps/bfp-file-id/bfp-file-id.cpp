/*
 * Copyright (c) 2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/apps/bfp-file-id/bfp-file-id.h"

#include "bitmunk/bfp/BfpFactory.h"
#include "bitmunk/common/Logging.h"
#include "monarch/app/AppFactory.h"
#include "monarch/data/json/JsonWriter.h"
#include "monarch/config/ConfigManager.h"
#include "monarch/io/File.h"

#include <cstdio>

using namespace std;
using namespace bitmunk::apps::tools;
using namespace bitmunk::bfp;
using namespace bitmunk::common;
using namespace monarch::app;
using namespace monarch::data::json;
using namespace monarch::config;
using namespace monarch::io;
using namespace monarch::kernel;
using namespace monarch::modest;
using namespace monarch::rt;

#define APP_NAME "bitmunk.apps.tools.BfpFileId"

BfpFileIdApp::BfpFileIdApp()
{
}

BfpFileIdApp::~BfpFileIdApp()
{
}

bool BfpFileIdApp::initialize()
{
   bool rval = bitmunk::app::App::initialize();
   if(rval)
   {
      setName(APP_NAME);
      setVersion("1.0");
   }
   return rval;
}

DynamicObject BfpFileIdApp::getCommandLineSpec(Config& cfg)
{
   // initialize config
   Config& c = cfg[ConfigManager::MERGE][APP_NAME];
   c["details"] = false;
   c["files"]->setType(Array);

   // create command line spec
   DynamicObject spec;
   spec["help"] =
"BfpFileId options\n"
"      --bfp-module   The path to the BFP factory module to use.\n"
"      --bfp-id       The BFP ID to use.\n"
"      --show-details Print file info details.\n"
"\n";

   DynamicObject opt;

   opt = spec["options"]->append();
   opt["long"] = "--show-details";
   opt["setTrue"]["root"] = c;
   opt["setTrue"]["path"] = "details";

   opt = spec["options"]->append();
   opt["long"] = "--bfp-module";
   opt["arg"]["root"] = c;
   opt["arg"]["path"] = "bfpModule";
   opt["argError"] = "No BFP factory module filename specified.";

   opt = spec["options"]->append();
   opt["long"] = "--bfp-id";
   opt["arg"]["root"] = c;
   opt["arg"]["path"] = "bfpId";
   opt["arg"]["type"] = (uint32_t)0;
   opt["argError"] = "No BFP ID specified.";

   // use extra options as files to process
   opt = spec["options"]->append();
   opt["extra"]["root"] = c;
   opt["extra"]["path"] = "files";

   return spec;
}

static bool _processFile(Bfp* bfp, const char* filename, bool showDetails)
{
   bool rval = true;

   string _;
   string ext;
   File::splitext(filename, _, ext);
   if(ext.size() <= 1)
   {
      ExceptionRef e = new Exception(
         "No file extension.",
         APP_NAME ".MissingFileExtension");
      e->getDetails()["filename"] = filename;
      Exception::set(e);
      rval = false;
   }
   else
   {
      FileInfo fi;
      fi["path"] = filename;
      // skip '.' in ext
      fi["extension"] = ext.c_str() + 1;
      if((rval = bfp->setFileInfoId(fi)))
      {
         if(showDetails)
         {
            JsonWriter::writeToStdOut(fi);
         }
         else
         {
            printf("%s %s\n", fi["id"]->getString(), filename);
         }
      }
   }

   return rval;
};

bool BfpFileIdApp::run()
{
   // get app config
   Config cfg = getConfig()[APP_NAME];

   // get bfp library to use
   const char* bfpModule = cfg["bfpModule"]->getString();

   // load bfp module
   MicroKernel* k = getKernel();
   Module* m = k->loadModule(bfpModule);
   if(m == NULL)
   {
      ExceptionRef e = new Exception(
         "Could not load BFP module.",
         APP_NAME ".InvalidBfpModule");
      e->getDetails()["filename"] = bfpModule;
      Exception::push(e);
   }
   else
   {
      BfpFactory* f = dynamic_cast<BfpFactory*>(m->getInterface());
      if(f == NULL)
      {
         // could not load bfp factory
         ExceptionRef e = new Exception(
            "Could not load BFP module.",
            APP_NAME ".InvalidBfpModule");
         e->getDetails()["filename"] = bfpModule;
         e->getDetails()["error"] = "Module API is not a BfpFactory.";
         Exception::set(e);
      }
      else
      {
         // create bfp
         BfpId id = cfg["bfpId"]->getUInt32();
         Bfp* bfp = f->createBfp(id);
         if(bfp == NULL)
         {
            ExceptionRef e = new Exception(
               "Could not create BFP instance.",
               APP_NAME ".BfpError");
            Exception::push(e);
         }
         else
         {
            DynamicObject& files = cfg["files"];
            if(files->length() == 0)
            {
               ExceptionRef e = new Exception(
                  "No files specified.",
                  APP_NAME ".NoFiles");
               Exception::set(e);
            }
            else
            {
               bool details = cfg["details"]->getBoolean();
               DynamicObjectIterator i = files.getIterator();
               bool success = true;
               while(success && i->hasNext())
               {
                  success = _processFile(bfp, i->next()->getString(), details);
               }
            }

            // clean up bfp
            f->freeBfp(bfp);
         }
      }

      // unload module
      k->unloadModule(&m->getId());
   }

   return !Exception::isSet();
}

class BfpFileIdAppFactory : public AppFactory
{
public:
   BfpFileIdAppFactory() : AppFactory(APP_NAME, "1.0")
   {
   }

   virtual ~BfpFileIdAppFactory() {}

   virtual App* createApp()
   {
      return new BfpFileIdApp();
   }
};

Module* createModestModule()
{
   return new BfpFileIdAppFactory();
}

void freeModestModule(Module* m)
{
   delete m;
}

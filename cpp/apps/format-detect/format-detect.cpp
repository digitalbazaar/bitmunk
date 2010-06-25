/*
 * Copyright (c) 2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/apps/format-detect/format-detect.h"

#include "bitmunk/common/Logging.h"
#include "bitmunk/data/FormatDetectorInputStream.h"
#include "monarch/app/AppFactory.h"
#include "monarch/data/json/JsonWriter.h"
#include "monarch/config/ConfigManager.h"
#include "monarch/io/FileInputStream.h"

#include <cstdio>

using namespace std;
using namespace bitmunk::apps::tools;
using namespace bitmunk::data;
using namespace monarch::app;
using namespace monarch::config;
using namespace monarch::data;
using namespace monarch::data::json;
using namespace monarch::io;
using namespace monarch::modest;
using namespace monarch::rt;

#define APP_NAME "bitmunk.apps.tools.FormatDetect"

FormatDetectApp::FormatDetectApp()
{
}

FormatDetectApp::~FormatDetectApp()
{
}

bool FormatDetectApp::initialize()
{
   bool rval = bitmunk::app::App::initialize();
   if(rval)
   {
      setName(APP_NAME);
      setVersion("1.0");
   }
   return rval;
}

DynamicObject FormatDetectApp::getCommandLineSpec(Config& cfg)
{
   // initialize config
   Config& c = cfg[ConfigManager::MERGE][APP_NAME];
   c["quick"] = false;
   c["files"]->setType(Array);

   // create command line spec
   DynamicObject spec;
   spec["help"] =
"Format detect options\n"
"      --quick-detect\n"
"                     Do not fully inspect files, do quick detection.\n"
"\n";

   DynamicObject opt;

   opt = spec["options"]->append();
   opt["long"] = "--quick-detect";
   opt["setTrue"]["root"] = c;
   opt["setTrue"]["path"] = "quick";

   // use extra options as files to process
   opt = spec["options"]->append();
   opt["extra"]["root"] = c;
   opt["extra"]["path"] = "files";

   return spec;
}

static bool _processFile(const char* filename, bool quick)
{
   bool rval = true;

   MO_INFO("Processing file: \"%s\".", filename);
   printf("Processing file: \"%s\".\n", filename);
   File file(filename);
   FileInputStream fis(file);
   InspectorInputStream iis(&fis);
   FormatDetectorInputStream fdis(&iis);

   if(!quick)
   {
      // fully inspect mp3 files
      fdis.getDataFormatInspector("bitmunk.data.MpegAudioDetector")->
         setKeepInspecting(true);
   }

   uint64_t total = 0;
   fdis.detect(&total);
   bool complete = fdis.isFormatRecognized();
   MO_INFO("Done. Format detected=%s", complete ? "true" : "false");
   printf("Done. Format detected=%s\n", complete ? "true" : "false");
   if(complete)
   {
      DynamicObject dyno = fdis.getFormatDetails();
      string str = JsonWriter::writeToString(dyno);
      MO_INFO("Format details: %s", str.c_str());
      printf("Format details: %s\n", str.c_str());
   }
   fdis.close();

   return rval;
};

bool FormatDetectApp::run()
{
   // get app config
   Config cfg = getConfig()[APP_NAME];

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
      bool quick = cfg["quick"]->getBoolean();
      DynamicObjectIterator i = files.getIterator();
      bool success = true;
      while(success && i->hasNext())
      {
         success = _processFile(i->next()->getString(), quick);
      }
   }

   return !Exception::isSet();
}

class FormatDetectAppFactory : public AppFactory
{
public:
   FormatDetectAppFactory() : AppFactory(APP_NAME, "1.0")
   {
   }

   virtual ~FormatDetectAppFactory() {}

   virtual App* createApp()
   {
      return new FormatDetectApp();
   }
};

Module* createModestModule()
{
   return new FormatDetectAppFactory();
}

void freeModestModule(Module* m)
{
   delete m;
}

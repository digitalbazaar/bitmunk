/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#include <iostream>
#include <sstream>

#include "bitmunk/common/Logging.h"
#include "bitmunk/data/FormatDetectorInputStream.h"
#include "bitmunk/node/App.h"
#include "monarch/app/App.h"
#include "monarch/io/FileInputStream.h"
#include "monarch/io/OStreamOutputStream.h"
#include "monarch/logging/OutputStreamLogger.h"
#include "monarch/test/Test.h"

using namespace std;
using namespace monarch::data;
using namespace monarch::io;
using namespace monarch::logging;
using namespace monarch::test;
using namespace monarch::rt;
using namespace bitmunk::common;
using namespace bitmunk::data;

class FormatDetectorApp :
   public monarch::app::AppPlugin
{
protected:
   /**
    * Options from the command line.
    * "allInfo": output all file info
    * "files": files to process
    */
   DynamicObject mOptions;

public:
   FormatDetectorApp()
   {
      mInfo["id"] = "bitmunk.test.FormatDetector";
      mInfo["dependencies"]->append() = "bitmunk.app.App";

      mOptions["allInfo"] = false;
      mOptions["files"]->setType(Array);
   };

   virtual ~FormatDetectorApp() {};

   virtual DynamicObject getCommandLineSpecs()
   {
      DynamicObject spec;
      spec["help"] =
"FormatDetector options\n"
"  -a, --all           Print all file info.\n"
"\n";

      DynamicObject opt;

      opt = spec["options"]->append();
      opt["short"] = "-a";
      opt["long"] = "--all";
      opt["argError"] = "No files specified.";
      opt["setTrue"]["target"] = mOptions["allInfo"];

      spec["args"] = mOptions["files"];

      DynamicObject specs = AppPlugin::getCommandLineSpecs();
      specs->append(spec);
      return specs;
   };

   virtual bool processFile(const char* filename)
   {
      bool rval = true;

      MO_INFO("Processing file: \"%s\".", filename);
      File file(filename);
      FileInputStream fis(file);
      InspectorInputStream iis(&fis);
      FormatDetectorInputStream fdis(&iis);

      // fully inspect mp3 files
      fdis.getDataFormatInspector("bitmunk.data.MpegAudioDetector")->
         setKeepInspecting(true);

      uint64_t total = 0;
      fdis.detect(&total);
      bool complete = fdis.isFormatRecognized();
      MO_INFO("Done read. Complete=%d", complete);
      if(complete)
      {
         DynamicObject ddyno = fdis.getFormatDetails();
         string dstr;
         dynamicObjectToString(ddyno, dstr);
         MO_INFO("Format details: %s", dstr.c_str());
      }
      fdis.close();
      assertNoException();

      return rval;
   };

   virtual bool run()
   {
      DynamicObject& files = mOptions["files"];
      if(files->length() > 0)
      {
         DynamicObjectIterator i = mOptions["files"].getIterator();
         bool success = true;
         while(success && i->hasNext())
         {
            success = processFile(i->next()->getString());
         }
      }
      return !Exception::isSet();
   };
};

/**
 * Create main() for the FormatDetector app
 */
BM_APP_PLUGIN_MAIN(FormatDetectorApp);

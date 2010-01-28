/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#include <iostream>
#include <cstdio>

#include "bitmunk/bfp/BfpFactory.h"
#include "bitmunk/common/TypeDefinitions.h"
#include "bitmunk/node/App.h"
#include "monarch/app/App.h"
#include "monarch/data/json/JsonWriter.h"
#include "monarch/io/File.h"
#include "monarch/modest/Kernel.h"

using namespace std;
using namespace bitmunk::common;
using namespace bitmunk::bfp;
using namespace monarch::data::json;
using namespace monarch::io;
using namespace monarch::modest;
using namespace monarch::rt;

class BfpFileIdApp :
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
   BfpFileIdApp()
   {
      mInfo["id"] = "bitmunk.test.BfpFileId";
      mInfo["dependencies"]->append() = "bitmunk.app.App";

      mOptions["allInfo"] = false;
      mOptions["files"]->setType(Array);
   };
   
   virtual ~BfpFileIdApp() {};

   virtual DynamicObject getCommandLineSpecs()
   {
      DynamicObject spec;
      spec["help"] =
"BfpFileId options\n"
"  -a, --all           Print all file info.\n"
"\n";
      
      DynamicObject opt;
      
      opt = spec["options"]->append();
      opt["short"] = "-a";
      opt["long"] = "--all";
      opt["argError"] = "No files specified.";
      opt["setTrue"]["target"] = mOptions["allInfo"];
      
      spec["args"] = mOptions["files"];
      
      DynamicObject specs;
      specs->setType(Array);
      specs->append(spec);
      return specs;
   };
   
   virtual bool processFile(Bfp* bfp, const char* filename)
   {
      bool rval = true;
      
      string _;
      string ext;
      File::splitext(filename, _, ext);
      if(ext.size() > 1)
      {
         FileInfo fi;
         fi["path"] = filename;
         // skip '.' in ext
         fi["extension"] = ext.c_str() + 1;
         if((rval = bfp->setFileInfoId(fi)))
         {
            if(mOptions["allInfo"]->getBoolean())
            {
               JsonWriter::writeToStdOut(fi);
            }
            else
            {
               printf("%s %s\n", fi["id"]->getString(), filename);
            }
         }
      }
      else
      {
         ExceptionRef e = new Exception("No file extension.");
         e->getDetails()["filename"] = filename;
         Exception::set(e);
         rval = false;
      }
      
      return rval;
   };
   
   virtual bool run()
   {
      // bfp library to use
      const char* bfplib = "libbmbfp-1.so";
   
      // load bfp module
      Kernel k;
      Module* m = k.getModuleLibrary()->loadModule(bfplib);
      if(m != NULL)
      {
         BfpFactory* f = dynamic_cast<BfpFactory*>(m->getInterface());
         if(f == NULL)
         {
            // could not load bfp factory
            ExceptionRef e = new Exception(
               "Could not load bfp module.", "bitmunk.bfp.tools.BadBfpModule");
            e->getDetails()["filename"] = bfplib;
            e->getDetails()["error"] = "Module interface is not a BfpFactory.";
            Exception::set(e);
         }
         else
         {
            // create bfp
            Bfp* bfp = f->createBfp(1);
            if(bfp != NULL)
            {
               DynamicObject& files = mOptions["files"];
               if(files->length() > 0)
               {
                  DynamicObjectIterator i = mOptions["files"].getIterator();
                  bool success = true;
                  while(success && i->hasNext())
                  {
                     success = processFile(bfp, i->next()->getString());
                  }
               }
               
               // clean up bfp
               f->freeBfp(bfp);
            }
            else
            {
               ExceptionRef e = new Exception("Could not create Bfp.");
               Exception::set(e);
            }
         }
         
         // unload module
         k.getModuleLibrary()->unloadAllModules();
      }
      else
      {
         ExceptionRef e = new Exception("Could not load Bfp library.");
         e->getDetails()["filename"] = bfplib;
         Exception::set(e);
      }

      return !Exception::isSet();
   };
};

/**
 * Create main() for the BfpFileId app
 */
BM_APP_PLUGIN_MAIN(BfpFileIdApp);

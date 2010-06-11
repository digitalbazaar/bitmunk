/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/data/FormatDetectorInputStream.h"

#include "bitmunk/data/AviDetector.h"
#include "bitmunk/data/Id3v2TagReader.h"
#include "bitmunk/data/MpegAudioDetector.h"
#include "bitmunk/common/Logging.h"

using namespace std;
using namespace monarch::data;
using namespace monarch::rt;
using namespace bitmunk::common;
using namespace bitmunk::data;

FormatDetectorInputStream::FormatDetectorInputStream(
   InspectorInputStream* is, bool cleanup) :
   FilterInputStream(is, cleanup)
{
   mDataFormatRecognized = false;
   createInspectors();
}

FormatDetectorInputStream::~FormatDetectorInputStream()
{
}

void FormatDetectorInputStream::createInspectors()
{
   // add inspectors:

   // document inspectors:

   // pdf detector
//   addDataFormatInspector(new PdfDetector(), Document, true);

   // image inspectors:

   // video inspectors:

   // avi detector
   {
      AviDetector* ad = new AviDetector();
      addDataFormatInspector("bitmunk.data.AviDetector", ad, true);
   }

   // audio inspectors:

   // mpeg audio detector
   {
      MpegAudioDetector* mad = new MpegAudioDetector();
      mad->setKeepInspecting(true);
      mad->setKeepInspecting(false);
      addDataFormatInspector("bitmunk.data.MpegAudioDetector", mad, true);
   }

   // id3 tag reader
   {
      Id3v2TagRef tag = new Id3v2Tag();
      Id3v2TagReader* itw = new Id3v2TagReader(tag);
      addDataFormatInspector("bitmunk.data.Id3v2TagReader", itw, true);
   }
}

bool FormatDetectorInputStream::detect(uint64_t* total)
{
   InspectorInputStream* iis = static_cast<InspectorInputStream*>(mInputStream);
   return iis->inspect(total);
}

bool FormatDetectorInputStream::addDataFormatInspector(
   const char* name, DataFormatInspector* dfi, bool cleanup)
{
   bool rval = false;

   // cast underlying input stream appropriately
   InspectorInputStream* iis = static_cast<InspectorInputStream*>(mInputStream);
   if(iis->getInspector(name) == NULL)
   {
      rval = true;
      mInspectors.push_back(dfi);
      if(rval)
      {
         iis->addInspector(name, dfi, cleanup);
      }
   }

   return rval;
}

DataFormatInspector* FormatDetectorInputStream::getDataFormatInspector(
   const char* name)
{
   DataFormatInspector* rval = NULL;

   // cast underlying input stream appropriately
   InspectorInputStream* iis = static_cast<InspectorInputStream*>(mInputStream);
   rval = dynamic_cast<DataFormatInspector*>(iis->getInspector(name));

   return rval;
}

vector<DataFormatInspector*>* FormatDetectorInputStream::getAllInspectors()
{
   return &mInspectors;
}

vector<DataFormatInspector*> FormatDetectorInputStream::getInspectors()
{
   vector<DataFormatInspector*> dfiInspectors;

   list<DataInspector*> inspectors;
   InspectorInputStream* iis = static_cast<InspectorInputStream*>(mInputStream);
   iis->getInspectors(inspectors);
   for(list<DataInspector*>::iterator i = inspectors.begin();
       i != inspectors.end(); ++i)
   {
      DataFormatInspector* dfi = dynamic_cast<DataFormatInspector*>(*i);
      if(dfi != NULL && dfi->isFormatRecognized())
      {
         dfiInspectors.push_back(dfi);
      }
   }

   return dfiInspectors;
}

bool FormatDetectorInputStream::isFormatRecognized()
{
   if(!mDataFormatRecognized)
   {
      // check all inspectors for satisfaction
      list<DataInspector*> inspectors;
      InspectorInputStream* iis =
         static_cast<InspectorInputStream*>(mInputStream);
      iis->getInspectors(inspectors);
      for(list<DataInspector*>::iterator i = inspectors.begin();
          !mDataFormatRecognized && i != inspectors.end(); ++i)
      {
         DataFormatInspector* dfi = dynamic_cast<DataFormatInspector*>(*i);
         if(dfi != NULL)
         {
            mDataFormatRecognized = dfi->isFormatRecognized();
         }
      }
   }

   return mDataFormatRecognized;
}

DynamicObject FormatDetectorInputStream::getFormatDetails()
{
   DynamicObject details;
   details->setType(Array);

   for(vector<DataFormatInspector*>::iterator i = mInspectors.begin();
       i != mInspectors.end(); ++i)
   {
      DataFormatInspector* dfi = dynamic_cast<DataFormatInspector*>(*i);
      if(dfi != NULL && dfi->isFormatRecognized())
      {
         DynamicObject d = dfi->getFormatDetails();
         details->append(d);
      }
   }

   return details;
}

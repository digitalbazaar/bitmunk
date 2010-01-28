/*
 * Copyright (c) 2007-2008 Digital Bazaar, Inc.  All rights reserved.
 */
#ifndef bitmunk_data_PdfDetector_H
#define bitmunk_data_PdfDetector_H

#include "monarch/data/AbstractDataFormatInspector.h"

namespace bitmunk
{
namespace data
{

/**
 * A PdfDetector attempts to detect the PDF format in a stream of data.
 *  
 * @author Dave Longley
 */
class PdfDetector : public monarch::data::AbstractDataFormatInspector
{
protected:
   
public:
   /**
    * Creates a new PdfDetector.
    */
   PdfDetector();
   
   /**
    * Destructs this PdfDetector.
    */
   virtual ~PdfDetector();
   
   /**
    * Gets a string identifier for the format that was detected.  Use
    * getInspectionReport() for format and stream details.
    * 
    * @return a string identifier for the format that was detected or NULL.
    */
   virtual const char* getFormat();
};

} // end namespace data
} // end namespace bitmunk
#endif

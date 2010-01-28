/*
 * Copyright (c) 2007-2009 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/data/PdfDetector.h"

using namespace bitmunk::data;

PdfDetector::PdfDetector()
{
}

PdfDetector::~PdfDetector()
{
}

const char* PdfDetector::getFormat()
{
   return isFormatRecognized() ? "application/pdf" : NULL;
}

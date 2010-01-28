/*
 * Copyright (c) 2008 Digital Bazaar, Inc.  All rights reserved.
 */

#include "bitmunk/common/Logging.h"

using namespace bitmunk::common;
 
void Logging::initialize()
{
   LoggingCategories::initialize();
}

void Logging::cleanup()
{
   LoggingCategories::cleanup();
}

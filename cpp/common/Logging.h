/*
 * Copyright (c) 2008 Digital Bazaar, Inc.  All rights reserved.
 */
#ifndef bitmunk_common_Logging_H
#define bitmunk_common_Logging_H

// Include DB logging system and Bitmunk categories for convienience:
#include "monarch/logging/Logging.h"
#include "bitmunk/common/LoggingCategories.h"

namespace bitmunk
{
namespace common
{
   
/**
 * Pseudo-class to initialize and cleanup the bitmunk logging framework.
 * 
 * @author David I. Lehn
 */
class Logging
{
public:
   /**
    * Initializes the DB logging system and Bitmunk categories.
    * This MUST be called during application start-up.
    */
   static void initialize();

   /**
    * Cleans up the Bitmunk categories and DB logging system.
    * This MUST be called during application tear-down.
    */
   static void cleanup();
};

} // end namespace common
} // end namespace bitmunk
#endif

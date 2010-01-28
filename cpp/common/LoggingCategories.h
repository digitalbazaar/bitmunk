/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_common_LoggingCategories_H
#define bitmunk_common_LoggingCategories_H

#include "monarch/logging/Category.h"

// This is a list of pre-defined common Bitmunk logging categories that can be
// used by any module that would like to log messages to a specific category.
// See individual modules for per-module categories.

extern monarch::logging::Category* BM_APP_CAT;
extern monarch::logging::Category* BM_COMMON_CAT;
extern monarch::logging::Category* BM_DATA_CAT;
extern monarch::logging::Category* BM_PROTOCOL_CAT;
extern monarch::logging::Category* BM_TEST_CAT;

namespace bitmunk
{
namespace common
{

/**
 * Pseudo-class to contain category initialization/cleanup.
 *
 * FIXME: Move these to modules?
 */
class LoggingCategories
{
public:
   /**
    * Initializes the static categories. This static method is called by
    * bitmunk::commmon::Logging::initialize() which MUST be called during
    * application start-up.
    */
   static void initialize();

   /**
    * Frees static categories. This static method is called from
    * bitmunk::commmon::Logging::cleanup() and MUST be called during
    * application tear-down.
    */
   static void cleanup();
};

} // end namespace logging
} // end namespace db

#endif

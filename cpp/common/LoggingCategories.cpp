/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/common/LoggingCategories.h"

#include "monarch/util/AnsiEscapeCodes.h"

using namespace monarch::logging;
using namespace bitmunk::common;

// DO NOT INITIALIZE THESE VARIABLES!
// These are not initialized on purpose due to initialization code issues.
Category* BM_APP_CAT;
Category* BM_COMMON_CAT;
Category* BM_DATA_CAT;
Category* BM_PROTOCOL_CAT;
Category* BM_TEST_CAT;

void LoggingCategories::initialize()
{
   BM_APP_CAT = new Category(
      "BM_APP",
      "Bitmunk Application",
      "Bitmunk application level messages.");
   BM_APP_CAT->setAnsiEscapeCodes(
      MO_ANSI_CSI MO_ANSI_BOLD MO_ANSI_SEP MO_ANSI_FG_HI_GREEN MO_ANSI_SGR);
   BM_COMMON_CAT = new Category(
      "BM_COMMON",
      "Bitmunk Common",
      NULL);
   BM_DATA_CAT = new Category(
      "BM_DATA",
      "Bitmunk Data",
      NULL);
   BM_PROTOCOL_CAT = new Category(
      "BM_PROTOCOL",
      "Bitmunk Protocol",
      NULL);
   BM_PROTOCOL_CAT->setAnsiEscapeCodes(
      MO_ANSI_CSI MO_ANSI_BOLD MO_ANSI_SEP MO_ANSI_FG_HI_MAGENTA MO_ANSI_SGR);
   BM_TEST_CAT = new Category(
      "BM_TEST",
      "Bitmunk Test",
      NULL);
}

void LoggingCategories::cleanup()
{
   delete BM_APP_CAT;
   BM_APP_CAT = NULL;

   delete BM_COMMON_CAT;
   BM_COMMON_CAT = NULL;

   delete BM_DATA_CAT;
   BM_DATA_CAT = NULL;

   delete BM_PROTOCOL_CAT;
   BM_PROTOCOL_CAT = NULL;

   delete BM_TEST_CAT;
   BM_TEST_CAT = NULL;
}

/*
 * Copyright (c) 2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/common/BitmunkId.h"

#include <cstdlib>
#include <cstring>

using namespace bitmunk::common;

BitmunkId::BitmunkId() :
   mId(NULL)
{
}

BitmunkId::BitmunkId(const char* id)
{
   mId = (id == NULL) ? NULL : strdup(id);
}

BitmunkId::BitmunkId(const BitmunkId& rhs) :
   mId(NULL)
{
   *this = rhs;
}

BitmunkId::~BitmunkId()
{
   if(mId != NULL)
   {
      free(mId);
   }
}

inline const BitmunkId& BitmunkId::operator=(const BitmunkId& rhs)
{
   if(mId != NULL)
   {
      free(mId);
   }
   mId = rhs.isInvalid() ? NULL : strdup(rhs.id());
   return *this;
}

inline bool BitmunkId::operator==(const BitmunkId& rhs) const
{
   return strcmp(id(), rhs.id()) == 0;
}

inline bool BitmunkId::operator!=(const BitmunkId& rhs) const
{
   return strcmp(id(), rhs.id()) != 0;
}

inline bool BitmunkId::isInvalid() const
{
   return strcmp(id(), BM_ID_INVALID) == 0;
}

inline bool BitmunkId::isNone() const
{
   return strcmp(id(), BM_ID_NONE) == 0;
}

inline bool BitmunkId::isAny() const
{
   return strcmp(id(), BM_ID_ANY) == 0;
}

inline bool BitmunkId::isUnknown() const
{
   return strcmp(id(), BM_ID_UNKNOWN) == 0;
}

inline const char* BitmunkId::id() const
{
   return (mId == NULL) ? BM_ID_INVALID : mId;
}

/*
 * Copyright (c) 2007-2009 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/common/PublicKeyCache.h"

using namespace std;
using namespace monarch::crypto;
using namespace bitmunk::common;

PublicKeyCache::PublicKeyCache(unsigned int capacity)
{
   mCapacity = capacity;
   mKeySource = NULL;
}

PublicKeyCache::~PublicKeyCache()
{
}

void PublicKeyCache::addPublicKey(
   UserId uid, ProfileId pid, PublicKeyRef& key, bool isDelegate)
{
   mCacheLock.lock();
   {
      // remove ids from map and list
      IdPair p = make_pair(uid, pid);
      mKeyMap.erase(p);
      mIdList.remove(p);
      
      // add ids to map and front of list
      mKeyMap[p] = make_pair(key, isDelegate);
      mIdList.push_front(p);
      
      if(mIdList.size() > mCapacity)
      {
         // remove oldest (last in list) id pair
         p = mIdList.back();
         mIdList.pop_back();
         mKeyMap.erase(p);
      }
   }
   mCacheLock.unlock();
}

PublicKeyRef PublicKeyCache::getPublicKey(
   UserId uid, ProfileId pid, bool* isDelegate)
{
   PublicKeyRef rval(NULL);
   bool delegate = false;
   
   mCacheLock.lock();
   {
      IdPair p = make_pair(uid, pid);
      KeyMap::iterator i = mKeyMap.find(p);
      if(i != mKeyMap.end())
      {
         rval = i->second.first;
         delegate = i->second.second;
      }
      else if(mKeySource != NULL)
      {
         // try to obtain public key from source
         mCacheLock.unlock();
         rval = mKeySource->getPublicKey(uid, pid, &delegate);
         mCacheLock.lock();
         if(!rval.isNull())
         {
            // add public key to cache
            addPublicKey(uid, pid, rval, delegate);
         }
      }
   }
   mCacheLock.unlock();
   
   // return whether or not public key belongs to a delegate
   if(isDelegate != NULL)
   {
      *isDelegate = delegate;
   }
   
   return rval;
}

void PublicKeyCache::setCapacity(unsigned int capacity)
{
   mCacheLock.lock();
   {
      mCapacity = capacity;
   }
   mCacheLock.unlock();
}

void PublicKeyCache::setPublicKeySource(PublicKeySource* source)
{
   mCacheLock.lock();
   {
      mKeySource = source;
   }
   mCacheLock.unlock();
}

void PublicKeyCache::clear()
{
   mCacheLock.lock();
   {
      mKeyMap.clear();
   }
   mCacheLock.unlock();
}

/*
 * Copyright (c) 2007-2008 Digital Bazaar, Inc.  All rights reserved.
 */
#ifndef bitmunk_common_PublicKeyCache_H
#define bitmunk_common_PublicKeyCache_H

#include "monarch/rt/ExclusiveLock.h"
#include "monarch/crypto/PublicKey.h"
#include "bitmunk/common/PublicKeySource.h"

#include <map>
#include <list>

namespace bitmunk
{
namespace common
{

/**
 * A PublicKeyCache is used to cache PublicKeys for quick retrieval.
 * 
 * @author Dave Longley
 */
class PublicKeyCache : public PublicKeySource
{
protected:
   /**
    * A map of <UserId,ProfileId> pairs to <PublicKey,isDelegate> pairs.
    */
   typedef std::pair<UserId, ProfileId> IdPair;
   typedef std::pair<monarch::crypto::PublicKeyRef, bool> PublicKeyPair;
   typedef std::map<IdPair, PublicKeyPair> KeyMap;
   KeyMap mKeyMap;
   
   /**
    * A list of <UserId,ProfileId> pairs that have PublicKeys in this cache,
    * in the order they were added.
    */
   typedef std::list<IdPair> IdList;
   IdList mIdList;
   
   /**
    * The capacity for this cache.
    */
   unsigned int mCapacity;
   
   /**
    * A PublicKeySource used to obtain PublicKeys when they cannot
    * be found in this cache. 
    */
   PublicKeySource* mKeySource;
   
   /**
    * A lock used for manipulation the cache.
    */
   monarch::rt::ExclusiveLock mCacheLock;
   
public:
   /**
    * Creates a new PublicKeyCache with the given capacity.
    * 
    * @param capacity the maximum number of keys to store in this cache before
    *                 removing the oldest key when adding a new one.
    */
   PublicKeyCache(unsigned int capacity = 100);
   
   /**
    * Destructs this PublicKeyCache.
    */
   virtual ~PublicKeyCache();
   
   /**
    * Adds a PublicKey to this cache.
    * 
    * @param uid the UserId associated with the PublicKey.
    * @param pid the ProfileId associated with the PublicKey.
    * @param key the PublicKey.
    * @param isDelegate true if the PublicKey belongs to a delegate,
    *                   false if not.
    */
   virtual void addPublicKey(
      UserId uid, ProfileId pid,
      monarch::crypto::PublicKeyRef& key, bool isDelegate);
   
   /**
    * Gets a PublicKey from this cache.
    * 
    * @param uid the UserId associated with the PublicKey.
    * @param pid the ProfileId associated with the PublicKey.
    * @param isDelegate to be set to true if the PublicKey belongs to
    *                   a delegate, false if not (NULL for don't care).
    * 
    * @return the PublicKey or NULL if none could be obtained.
    */
   virtual monarch::crypto::PublicKeyRef getPublicKey(
      UserId uid, ProfileId pid, bool* isDelegate);
   
   /**
    * Sets the capacity for this cache. If the capacity of this cache is
    * reached when trying to add a new public key to it, the public key
    * that was added first (the oldest entry) will be removed to accomodate
    * the new key.
    * 
    * @param capacity the capacity for this cache.
    */
   virtual void setCapacity(unsigned int capacity);
   
   /**
    * Sets the PublicKey source for this cache. This is the source that
    * PublicKeys will be retrieved from if they cannot be found in this
    * cache.
    * 
    * @param source the PublicKeySource to retrieve keys from when they
    *               cannot be found in this cache.
    */
   virtual void setPublicKeySource(PublicKeySource* source);
   
   /**
    * Clears the cache.
    */
   virtual void clear();
};

} // end namespace common
} // end namespace bitmunk
#endif

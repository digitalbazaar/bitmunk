/*
 * Copyright (c) 2007-2008 Digital Bazaar, Inc.  All rights reserved.
 */
#ifndef bitmunk_common_PublicKeySource_H
#define bitmunk_common_PublicKeySource_H

#include "monarch/crypto/PublicKey.h"
#include "bitmunk/common/TypeDefinitions.h"

namespace bitmunk
{
namespace common
{

/**
 * A PublicKeySource is an interface that provides a public key for
 * a given UserId.
 * 
 * @author Dave Longley
 */
class PublicKeySource
{
public:
   /**
    * Creates a new PublicKeySource.
    */
   PublicKeySource() {};
   
   /**
    * Destructs this PublicKeySource.
    */
   virtual ~PublicKeySource() {};
   
   /**
    * Gets the PublicKey for the given UserId and ProfileId.
    * 
    * @param uid the UserId to get the PublicKey for.
    * @param pid the ProfileId associated with the PublicKey.
    * @param isDelegate to be set to true if the PublicKey belongs to
    *                   a delegate, false if not (NULL for don't care).
    * 
    * @return the PublicKey for the UserId or NULL if none is available.
    */
   virtual monarch::crypto::PublicKeyRef getPublicKey(
      UserId uid, ProfileId pid, bool* isDelegate) = 0;
};

} // end namespace common
} // end namespace bitmunk
#endif

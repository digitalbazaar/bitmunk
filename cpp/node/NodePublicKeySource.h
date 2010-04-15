/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_node_NodePublicKeySource_H
#define bitmunk_node_NodePublicKeySource_H

#include "bitmunk/common/PublicKeyCache.h"
#include "bitmunk/node/Messenger.h"

namespace bitmunk
{
namespace node
{

/**
 * A NodePublicKeySource is a Node's source for PublicKeys.
 *
 * @author Dave Longley
 */
class NodePublicKeySource : public bitmunk::common::PublicKeySource
{
protected:
   /**
    * The Node's Messenger.
    */
   MessengerRef mMessenger;

public:
   /**
    * Creates a new NodePublicKeySource.
    */
   NodePublicKeySource();

   /**
    * Destructs this NodePublicKeySource.
    */
   virtual ~NodePublicKeySource();

   /**
    * Sets the messenger to use.
    *
    * @param m the messenger to use.
    */
   virtual void setMessenger(MessengerRef& m);

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
      bitmunk::common::UserId uid,
      bitmunk::common::ProfileId pid,
      bool* isDelegate);
};

} // end namespace node
} // end namespace bitmunk
#endif

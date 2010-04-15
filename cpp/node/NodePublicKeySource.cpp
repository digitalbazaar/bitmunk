/*
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk/node/NodePublicKeySource.h"

#include "monarch/crypto/AsymmetricKeyFactory.h"

using namespace monarch::crypto;
using namespace monarch::net;
using namespace monarch::rt;
using namespace bitmunk::common;
using namespace bitmunk::node;

NodePublicKeySource::NodePublicKeySource()
{
}

NodePublicKeySource::~NodePublicKeySource()
{
}

void NodePublicKeySource::setMessenger(MessengerRef& m)
{
   mMessenger = m;
}

PublicKeyRef NodePublicKeySource::getPublicKey(
   UserId uid, ProfileId pid, bool* isDelegate)
{
   PublicKeyRef rval(NULL);
   bool delegate = false;

   // create url for retrieving public key
   Url url;
   url.format("/api/3.0/users/keys/public/%" PRIu64 "/%u", uid, pid);

   // get public key
   DynamicObject in;
   if(mMessenger->getSecureFromBitmunk(&url, in))
   {
      // use factory to read public key from PEM string
      const char* pem = in["publicKey"]->getString();
      delegate = in["delegate"]->getBoolean();
      AsymmetricKeyFactory afk;
      rval = afk.loadPublicKeyFromPem(pem, strlen(pem));
   }

   // return whether or not public key belongs to a delegate
   if(isDelegate != NULL)
   {
      *isDelegate = delegate;
   }

   return rval;
}

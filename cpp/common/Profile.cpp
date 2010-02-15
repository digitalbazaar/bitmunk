/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/common/Profile.h"

#include "monarch/crypto/AsymmetricKeyFactory.h"
#include "monarch/data/json/JsonWriter.h" 
#include "monarch/data/json/JsonReader.h"

using namespace monarch::crypto;
using namespace monarch::data::json;
using namespace monarch::io;
using namespace monarch::rt;
using namespace monarch::util;
using namespace bitmunk::common;

Profile::Profile() :
   mUserId(0),
   mId(0)
{
}

Profile::~Profile()
{
}

void Profile::clear()
{
   mId = 0;
   mUserId = 0;
   mPrivateKey.setNull();
   mCertificate.setNull();
}

PublicKeyRef Profile::generate()
{
   PublicKeyRef rval(NULL);
   
   // clear old private key, if any
   mPrivateKey.setNull();
   
   // create asymmetric key pair
   // (default to RSA because it is more compatible with various softwares)
   AsymmetricKeyFactory afk;
   afk.createKeyPair("RSA", mPrivateKey, rval);
   
   // clear old certificate, if any
   // new certificate needs to be set elsewhere
   mCertificate.setNull();

   return rval;
}

bool Profile::save(const char* password, OutputStream* os)
{
   // create dyno to store profile
   DynamicObject profile;
   BM_ID_SET(profile["id"], mId);
   BM_ID_SET(profile["userId"], mUserId);
   
   // use factory to write private key to PEM string
   AsymmetricKeyFactory afk;
   profile["privateKey"] = afk.writePrivateKeyToPem(
      mPrivateKey, password).c_str();
   
   // only save certificate if it exists
   if(!mCertificate.isNull())
   {
      // use factory to write certificate to PEM string
      profile["certificate"] = afk.writeCertificateToPem(mCertificate).c_str();
   }

   // write out pretty-print json-formatted profile
   JsonWriter writer;
   writer.setCompact(false);
   writer.setIndentation(0, 1);
   return writer.write(profile, os);
}

bool Profile::load(const char* password, InputStream* is)
{
   bool rval = true;
   
   // create dyno to store profile
   DynamicObject profile;
   
   // read in json-formatted profile
   JsonReader reader;
   reader.start(profile);
   rval = reader.read(is) && reader.finish();
   
   if(rval && !BM_USER_ID_EQUALS(BM_USER_ID(profile["userId"]), mUserId))
   {
      ExceptionRef e = new Exception(
         "Could not load Profile, invalid user ID.",
         "bitmunk.InvalidProfileUserId");
      BM_ID_SET(e->getDetails()["loadedUserId"], BM_USER_ID(profile["userId"]));
      BM_ID_SET(e->getDetails()["expectedUserId"], mUserId);
      Exception::set(e);
      rval = false;
   }
   
   if(rval)
   {
      if(profile->hasMember("id"))
      {
         // store ID
         setId(BM_PROFILE_ID(profile["id"]));

         // use factory to read private key from PEM string
         const char* pem = profile["privateKey"]->getString();
         AsymmetricKeyFactory afk;
         mPrivateKey = afk.loadPrivateKeyFromPem(pem, strlen(pem), password);
         rval = !mPrivateKey.isNull();
      }
      else
      {
         ExceptionRef e = new Exception(
            "Could not load Profile, missing profile ID.",
            "bitmunk.InvalidProfileId");
         Exception::set(e);
         rval = false;
      }
   }

   // only try to load certificate if it exists
   if(rval && profile->hasMember("certificate"))
   {
      // read in certificate
      X509CertificateRef cert;
      AsymmetricKeyFactory akf;
      cert = akf.loadCertificateFromPem(
         profile["certificate"]->getString(),
         profile["certificate"]->length());
      setCertificate(cert);
      rval = !mPrivateKey.isNull();
   }
   
   return rval;
}

void Profile::setUserId(UserId id)
{
   mUserId = id;
}

UserId Profile::getUserId() const
{
   return mUserId;
}

void Profile::setId(ProfileId id)
{
   mId = id;
}

ProfileId Profile::getId() const
{
   return mId;
}

void Profile::setPrivateKey(PrivateKeyRef& key)
{
   mPrivateKey = key;
}

PrivateKeyRef& Profile::getPrivateKey()
{
   return mPrivateKey;
}

DigitalSignature* Profile::createSignature()
{
   // create signature if private key is available, else return NULL
   return (!mPrivateKey.isNull() ? new DigitalSignature(mPrivateKey) : NULL);
}

void Profile::setCertificate(monarch::crypto::X509CertificateRef& cert)
{
   mCertificate = cert;
}

monarch::crypto::X509CertificateRef& Profile::getCertificate()
{
   return mCertificate;
}

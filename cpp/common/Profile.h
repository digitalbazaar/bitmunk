/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc.  All rights reserved.
 */
#ifndef bitmunk_common_Profile_H
#define bitmunk_common_Profile_H

#include "bitmunk/common/TypeDefinitions.h"
#include "monarch/crypto/PrivateKey.h"
#include "monarch/crypto/PublicKey.h"
#include "monarch/crypto/DigitalSignature.h"
#include "monarch/crypto/X509Certificate.h"
#include "monarch/io/OutputStream.h"
#include "monarch/io/InputStream.h"
#include "monarch/rt/Exception.h"

namespace bitmunk
{
namespace common
{

/**
 * A Profile is a user's Profile on Bitmunk. It contains an ID and a
 * PrivateKey. 
 * 
 * @author Dave Longley
 */
class Profile
{
protected:
   /**
    * The ID of the user this Profile belongs to.
    */
   UserId mUserId;
   
   /**
    * This Profile's ID.
    */
   ProfileId mId;
   
   /**
    * This Profile's private key.
    */
   monarch::crypto::PrivateKeyRef mPrivateKey;
   
   /**
    * This Profile's communication certificate.
    */
   monarch::crypto::X509CertificateRef mCertificate;

public:
   /**
    * Creates a new Profile.
    */
   Profile();
   
   /**
    * Destructs this Profile.
    */
   virtual ~Profile();
   
   /**
    * Clears this profile, erasing its private key from memory and resetting
    * its IDs.
    */
   virtual void clear();
   
   /**
    * Generates a new key-pair for this Profile. The generated PrivateKey is
    * stored and memory-managed internally by this class.
    * 
    * @return the generated PublicKey or NULL if an exception occurred.
    */
   virtual monarch::crypto::PublicKeyRef generate();
   
   /**
    * Saves this Profile to disk.
    * 
    * @param password the user's password.
    * @param os the OutputStream to save the profile to.
    * 
    * @return true if successful, false if an Exception occurred. 
    */
   virtual bool save(const char* password, monarch::io::OutputStream* os);
   
   /**
    * Loads this Profile from disk. The profile on disk must have a matching
    * user ID.
    * 
    * @param password the user's password.
    * @param is the InputStream to load the profile from.
    * 
    * @return true if successful, false if an Exception occurred. 
    */
   virtual bool load(const char* password, monarch::io::InputStream* is);
   
   /**
    * Sets this Profile's user ID.
    * 
    * @param id the user ID for this Profile.
    */
   virtual void setUserId(UserId id);
   
   /**
    * Gets this Profile's user ID.
    * 
    * @return the user ID for this Profile.
    */
   virtual UserId getUserId() const;
   
   /**
    * Sets this Profile's ID.
    * 
    * @param id the ID for this Profile.
    */
   virtual void setId(ProfileId id);
   
   /**
    * Gets this Profile's ID.
    * 
    * @return the ID for this Profile.
    */
   virtual ProfileId getId() const;
   
   /**
    * Sets this Profile's private key.
    * 
    * @param key the Profile's private key.
    */
   virtual void setPrivateKey(monarch::crypto::PrivateKeyRef& key);
   
   /**
    * Gets this Profile's private key. The createSignature() method should be
    * used to get a Signature for signing data, not this method.
    * 
    * @return the Profile's private key.
    */
   virtual monarch::crypto::PrivateKeyRef& getPrivateKey();
   
   /**
    * Gets a DigitalSignature from this Profile for signing data.
    * 
    * The caller of this method is responsible for deleting the
    * DigitalSignature.
    * 
    * @return a DigitalSignature for signing data.
    */ 
   virtual monarch::crypto::DigitalSignature* createSignature();

   /**
    * Sets this Profile's communication certificate.
    *
    * @param key the Profile's communication certificate.
    */
   virtual void setCertificate(monarch::crypto::X509CertificateRef& cert);

   /**
    * Gets this Profile's communication certificate.
    *
    * @return the Profile's private key.
    */
   virtual monarch::crypto::X509CertificateRef& getCertificate();
};

// type definition for reference counted Profile
typedef monarch::rt::Collectable<Profile> ProfileRef;

} // end namespace common
} // end namespace bitmunk
#endif

/*
 * Copyright (c) 2007-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_protocol_BtpTrailer_H
#define bitmunk_protocol_BtpTrailer_H

#include "monarch/http/HttpTrailer.h"
#include "monarch/crypto/DigitalSignature.h"

namespace bitmunk
{
namespace protocol
{

/**
 * A BtpTrailer is an HttpTrailer that includes a Btp-Signature header field.
 * 
 * @author Dave Longley
 */
class BtpTrailer : public monarch::http::HttpTrailer
{
protected:
   /**
    * The DigitalSignature to obtain the signature data from.
    */
   monarch::crypto::DigitalSignature* mSignature;
   
public:
   /**
    * Creates a new BtpTrailer that will obtain its signature data from the
    * passed DigitalSignature.
    * 
    * @param ds the DigitalSignature to use.
    */
   BtpTrailer(monarch::crypto::DigitalSignature* ds);
   
   /**
    * Destructs this BtpTrailer.
    */
   virtual ~BtpTrailer();
   
   /**
    * Updates this trailer according to some proprietary implementation. This
    * method will be called before a trailer is sent out or after it has been
    * received.
    * 
    * This method will set the content length and Btp-Signature field.
    * 
    * @param contentLength the length of the content that was sent or received.
    */
   virtual void update(unsigned long long contentLength);
};

} // end namespace protocol
} // end namespace bitmunk
#endif

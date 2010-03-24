/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/protocol/BtpTrailer.h"

#include "monarch/util/Convert.h"

using namespace std;
using namespace monarch::crypto;
using namespace monarch::http;
using namespace monarch::util;
using namespace bitmunk::protocol;

BtpTrailer::BtpTrailer(DigitalSignature* ds) :
   mSignature(ds)
{
}

BtpTrailer::~BtpTrailer()
{
}

void BtpTrailer::update(int64_t contentLength)
{
   // call parent implementation
   HttpTrailer::update(contentLength);

   // add content-signature if applicable
   if(mSignature != NULL && contentLength > 0 && mSignature->getSignMode())
   {
      // get signature data
      char s[mSignature->getValueLength()];
      unsigned int length;
      mSignature->getValue(s, length);

      // hex-encode signature
      string data = Convert::bytesToHex(s, length);

      // add Btp-Content-Signature field
      setField("Btp-Content-Signature", data.c_str());
   }
}

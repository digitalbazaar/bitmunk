/*
 * Copyright (c) 2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/node/CertificateCreator.h"

#include "monarch/crypto/AsymmetricKeyFactory.h"
#include "monarch/io/File.h"
#include "bitmunk/common/Logging.h"

using namespace std;
using namespace monarch::config;
using namespace monarch::crypto;
using namespace monarch::io;
using namespace monarch::rt;
using namespace monarch::util;
using namespace bitmunk::common;
using namespace bitmunk::node;

CertificateCreator::CertificateCreator(Node* node) :
   mNode(node)
{
}

CertificateCreator::~CertificateCreator()
{

}

bool CertificateCreator::createCertificate(File& pkeyFile, File& certFile)
{
   bool rval;

   // first generate a new RSA key pair
   AsymmetricKeyFactory akf;
   PrivateKeyRef privateKey;
   PublicKeyRef publicKey;
   if((rval = akf.createKeyPair("RSA", privateKey, publicKey)))
   {
      // use x.509 v3 and serial number 0
      uint32_t version = 0x2;
      BigInteger serial(0);

      // create the subject
      DynamicObject subject;
      // country
      {
         DynamicObject& item = subject->append();
         item["type"] = "C";
         item["value"] = "US";
      }
      // organization
      {
         DynamicObject& item = subject->append();
         item["type"] = "O";
         item["value"] = "Digital Bazaar, Inc.";
      }
      // organizational unit
      {
         DynamicObject& item = subject->append();
         item["type"] = "OU";
         item["value"] =
            "Bitmunk localhost-only Certificates - Authorization via BTP";
      }
      // common name
      {
         DynamicObject& item = subject->append();
         item["type"] = "CN";
         item["value"] = "localhost";
      }

      // certificate is valid from yesterday until 10 years from now
      // Note: yesterday is chosen in case time is off a little on
      // the client and also in case flash as3crypto library is used
      // which has some date accuracy issues
      Date startDate;
      startDate.addSeconds(-1 * 24 * 60 * 60);
      Date endDate;
      endDate.addSeconds(10 * 365 * 24 * 60 * 60);

      // set extensions
      DynamicObject extensions;
      extensions[0]["type"] = "basicConstraints";
      extensions[0]["value"] = "critical,CA:FALSE";
      extensions[1]["type"] = "keyUsage";
      extensions[1]["value"] =
         "critical,digitalSignature,nonRepudiation,"
         "keyEncipherment,dataEncipherment";
      extensions[2]["type"] = "extendedKeyUsage";
      extensions[2]["value"] = "serverAuth,clientAuth";

      // create the self-signed certificate
      X509CertificateRef cert = akf.createCertificate(
         version, privateKey, publicKey, subject, subject,
         &startDate, &endDate, serial, &extensions, NULL);
      if(cert.isNull())
      {
         rval = false;
      }
      else
      {
         // write out the certificate and private key (unencrypted)
         string privatePem = akf.writePrivateKeyToPem(privateKey, NULL);
         string certPem = akf.writeCertificateToPem(cert);
         if(privatePem.length() > 0 && certPem.length() > 0)
         {
            // make directories key and certificate
            rval = pkeyFile->mkdirs() && certFile->mkdirs();
            if(rval)
            {
               // write out private key
               ByteBuffer buf(
                  (char*)privatePem.c_str(), 0, privatePem.length(),
                  privatePem.length(), false);
               rval = pkeyFile.writeBytes(&buf, false);
            }
            if(rval)
            {
               // write out certificate
               ByteBuffer buf(
                  (char*)certPem.c_str(), 0, certPem.length(),
                  certPem.length(), false);
               rval = certFile.writeBytes(&buf, false);
            }
         }
      }
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Could not create self-signed localhost certificate.",
         "bitmunk.node.SelfSignedCertificateError");
      Exception::push(e);
   }

   return rval;
}

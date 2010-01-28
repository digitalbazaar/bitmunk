/*
 * Copyright (c) 2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_node_CertificateCreator_H
#define bitmunk_node_CertificateCreator_H

#include "bitmunk/node/Node.h"

namespace bitmunk
{
namespace node
{

/**
 * A CertificateCreator creates self-signed X.509 certificates for Bitmunk
 * peers.
 *
 * @author Dave Longley
 */
class CertificateCreator
{
protected:
   /**
    * The associated Node.
    */
   Node* mNode;

public:
   /**
    * Creates a new CertificateCreator.
    *
    * @param node the associated Bitmunk node.
    */
   CertificateCreator(Node* node);

   /**
    * Destructs this CertificateCreator.
    */
   virtual ~CertificateCreator();

   /**
    * Creates a new self-signed localhost certificate, writes its private
    * key and certificate to the given files.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool createCertificate(
      monarch::io::File& pkeyFile, monarch::io::File& certFile);
};

} // end namespace node
} // end namespace bitmunk
#endif

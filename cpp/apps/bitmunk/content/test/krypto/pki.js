/**
 * Javscript implementation of a basic Public Key Infrastructure, including
 * support for RSA public and private keys.
 *
 * @author Dave Longley
 *
 * Copyright (c) 2010 Digital Bazaar, Inc. All rights reserved.
 * 
 * The ASN.1 representation of an X.509v3 certificate is as follows:
 * 
 * Certificate ::= SEQUENCE {
 *    tbsCertificate       TBSCertificate,
 *    signatureAlgorithm   AlgorithmIdentifier,
 *    signatureValue       BIT STRING
 * }
 *
 * TBSCertificate ::= SEQUENCE {
 *    version         [0]  EXPLICIT Version DEFAULT v1,
 *    serialNumber         CertificateSerialNumber,
 *    signature            AlgorithmIdentifier,
 *    issuer               Name,
 *    validity             Validity,
 *    subject              Name,
 *    subjectPublicKeyInfo SubjectPublicKeyInfo,
 *    issuerUniqueID  [1]  IMPLICIT UniqueIdentifier OPTIONAL,
 *                         -- If present, version shall be v2 or v3
 *    subjectUniqueID [2]  IMPLICIT UniqueIdentifier OPTIONAL,
 *                         -- If present, version shall be v2 or v3
 *    extensions      [3]  EXPLICIT Extensions OPTIONAL
 *                         -- If present, version shall be v3
 * }
 *
 * Version ::= INTEGER  { v1(0), v2(1), v3(2) }
 *
 * CertificateSerialNumber ::= INTEGER
 *
 * Name ::= CHOICE {
 *    // only one possible choice for now
 *    RDNSequence
 * }
 *
 * RDNSequence ::= SEQUENCE OF RelativeDistinguishedName
 *
 * RelativeDistinguishedName ::= SET OF AttributeTypeAndValue
 *
 * AttributeTypeAndValue ::= SEQUENCE {
 *    type     AttributeType,
 *    value    AttributeValue
 * }
 * AttributeType ::= OBJECT IDENTIFIER
 * AttributeValue ::= ANY DEFINED BY AttributeType
 *
 * Validity ::= SEQUENCE {
 *    notBefore      Time,
 *    notAfter       Time
 * }
 *
 * Time ::= CHOICE {
 *    utcTime        UTCTime,
 *    generalTime    GeneralizedTime
 * }
 *
 * UniqueIdentifier ::= BIT STRING
 *
 * SubjectPublicKeyInfo ::= SEQUENCE {
 *    algorithm            AlgorithmIdentifier,
 *    subjectPublicKey     BIT STRING
 * }
 *
 * Extensions ::= SEQUENCE SIZE (1..MAX) OF Extension
 *
 * Extension ::= SEQUENCE {
 *    extnID      OBJECT IDENTIFIER,
 *    critical    BOOLEAN DEFAULT FALSE,
 *    extnValue   OCTET STRING
 * }
 * 
 * The only algorithm currently supported for PKI is RSA.
 * 
 * An RSA key is often stored in ASN.1 DER format. The SubjectPublicKeyInfo
 * ASN.1 structure is composed of an algorithm of type AlgorithmIdentifier
 * and a subjectPublicKey of type bit string.
 * 
 * The AlgorithmIdentifier contains an Object Identifier (OID) and parameters
 * for the algorithm, if any. In the case of RSA, there aren't any.
 * 
 * SubjectPublicKeyInfo ::= SEQUENCE
 * {
 *    algorithm AlgorithmIdentifier,
 *    subjectPublicKey BIT STRING
 * }
 * 
 * AlgorithmIdentifer ::= SEQUENCE
 * {
 *    algorithm OBJECT IDENTIFIER,
 *    parameters ANY DEFINED BY algorithm OPTIONAL
 * }
 * 
 * For an RSA public key, the subjectPublicKey is:
 * 
 * RSAPublicKey ::= SEQUENCE {
 *    modulus            INTEGER,    -- n
 *    publicExponent     INTEGER     -- e
 * }
 * 
 * The OID for the RSA key algorithm is: 1.2.840.113549.1.1.1
 */
(function()
{
   // local aliases for krypto stuff
   var krypto = window.krypto;
   var asn1 = krypto.asn1;
   
   /**
    * Public Key Infrastructure (PKI) implementation.
    */
   krypto.pki = {};
   var pki = krypto.pki;
   krypto.oids = krypto.oids || {};
   var oids = krypto.oids;
   
   // algorithm OIDs
   oids['1.2.840.113549.1.1.1'] = 'rsaEncryption';
   oids['rsaEncryption'] = '1.2.840.113549.1.1.1';
   // Note: md2 & md4 not implemented
   //oids['1.2.840.113549.1.1.1'] = 'md2withRSAEncryption';
   //oids['md2withRSAEncryption'] = '1.2.840.113549.1.1.1';
   //oids['1.2.840.113549.1.1.1'] = 'md4withRSAEncryption';
   //oids['md4withRSAEncryption'] = '1.2.840.113549.1.1.1';
   oids['1.2.840.113549.1.1.1'] = 'md5withRSAEncryption';
   oids['md5withRSAEncryption'] = '1.2.840.113549.1.1.1';
   oids['1.2.840.113549.1.1.1'] = 'sha1withRSAEncryption';
   oids['sha1withRSAEncryption'] = '1.2.840.113549.1.1.1';
   
   // certificate issuer/subject OIDs
   oids['2.5.4.3'] = 'commonName';
   oids['commonName'] = '2.5.4.3';
   oids['2.5.4.5'] = 'serialName';
   oids['serialName'] = '2.5.4.5';
   oids['2.5.4.6'] = 'countryName';
   oids['countryName'] = '2.5.4.6';
   oids['2.5.4.7'] = 'localityName';
   oids['localityName'] = '2.5.4.7';
   oids['2.5.4.8'] = 'stateOrProvinceName';
   oids['stateOrProvinceName'] = '2.5.4.9';
   oids['2.5.4.10'] = 'organizationName';
   oids['organizationName'] = '2.5.4.10';
   oids['2.5.4.11'] = 'organizationalUnitName';
   oids['organizationalUnitName'] = '2.5.4.11';
   
   // short name OID mappings
   var _shortNames = {};
   _shortNames['CN'] = oids['commonName'];
   _shortNames['commonName'] = 'CN';
   _shortNames['C'] = oids['countryName'];
   _shortNames['countryName'] = 'C';
   _shortNames['L'] = oids['localityName'];
   _shortNames['localityName'] = 'L';
   _shortNames['ST'] = oids['stateOrProvinceName'];
   _shortNames['stateOrProvinceName'] = 'ST';
   _shortNames['O'] = oids['organizationName'];
   _shortNames['organizationName'] = 'O';
   _shortNames['OU'] = oids['organizationalUnitName'];
   _shortNames['organizationalUnitName'] = 'OU';
   
   // X.509 extension OIDs
   oids['2.5.29.1'] = 'authorityKeyIdentifier'; // deprecated, use .35
   oids['2.5.29.2'] = 'keyAttributes'; // obsolete use .37 or .15
   oids['2.5.29.3'] = 'certificatePolicies'; // deprecated, use .32
   oids['2.5.29.4'] = 'keyUsageRestriction'; // obsolete use .37 or .15
   oids['2.5.29.5'] = 'policyMapping'; // deprecated use .33
   oids['2.5.29.6'] = 'subtreesConstraint'; // obsolete use .30 
   oids['2.5.29.7'] = 'subjectAltName'; // deprecated use .17
   oids['2.5.29.8'] = 'issuerAltName'; // deprecated use .18
   oids['2.5.29.9'] = 'subjectDirectoryAttributes'; 
   oids['2.5.29.10'] = 'basicConstraints'; // deprecated use .19
   oids['2.5.29.11'] = 'nameConstraints'; // deprecated use .30
   oids['2.5.29.12'] = 'policyConstraints'; // deprecated use .36
   oids['2.5.29.13'] = 'basicConstraints'; // deprecated use .19 
   oids['2.5.29.14'] = 'subjectKeyIdentifier';
   oids['2.5.29.15'] = 'keyUsage';
   oids['2.5.29.16'] = 'privateKeyUsagePeriod';
   oids['2.5.29.17'] = 'subjectAltName';
   oids['2.5.29.18'] = 'issuerAltName';
   oids['2.5.29.19'] = 'basicConstraints';
   oids['2.5.29.20'] = 'cRLNumber';
   oids['2.5.29.21'] = 'cRLReason';
   oids['2.5.29.22'] = 'expirationDate';
   oids['2.5.29.23'] = 'instructionCode';
   oids['2.5.29.24'] = 'invalidityDate';
   oids['2.5.29.25'] = 'cRLDistributionPoints'; // deprecated use .31
   oids['2.5.29.26'] = 'issuingDistributionPoint'; // deprecated use .28
   oids['2.5.29.27'] = 'deltaCRLIndicator';
   oids['2.5.29.28'] = 'issuingDistributionPoint';
   oids['2.5.29.29'] = 'certificateIssuer';
   oids['2.5.29.30'] = 'nameConstraints';
   oids['2.5.29.31'] = 'cRLDistributionPoints';
   oids['2.5.29.32'] = 'certificatePolicies';
   oids['2.5.29.33'] = 'policyMappings';
   oids['2.5.29.34'] = 'policyConstraints'; // deprecated use .36
   oids['2.5.29.35'] = 'authorityKeyIdentifier';
   oids['2.5.29.36'] = 'policyConstraints';
   oids['2.5.29.37'] = 'extKeyUsage';
   oids['2.5.29.46'] = 'freshestCRL';
   oids['2.5.29.54'] = 'inhibitAnyPolicy';
   
   // validator for an SubjectPublicKeyInfo structure
   // Note: Currently only works with an RSA public key 
   var publicKeyValidator = {
      name: 'SubjectPublicKeyInfo',
      tagClass: asn1.Class.UNIVERSAL,
      type: asn1.Type.SEQUENCE,
      constructed: true,
      captureAsn1: 'subjectPublicKeyInfo',
      value: [{
         name: 'SubjectPublicKeyInfo.AlgorithmIdentifier',
         tagClass: asn1.Class.UNIVERSAL,
         type: asn1.Type.SEQUENCE,
         constructed: true,
         value: [{
            name: 'AlgorithmIdentifier.algorithm',
            tagClass: asn1.Class.UNIVERSAL,
            type: asn1.Type.OID,
            constructed: false,
            capture: 'publicKeyOid'
         }]
      }, {
         // subjectPublicKey
         name: 'SubjectPublicKeyInfo.subjectPublicKey',
         tagClass: asn1.Class.UNIVERSAL,
         type: asn1.Type.BITSTRING,
         constructed: false,
         value: [{
            // RSAPublicKey
            name: 'SubjectPublicKeyInfo.subjectPublicKey.RSAPublicKey',
            tagClass: asn1.Class.UNIVERSAL,
            type: asn1.Type.SEQUENCE,
            constructed: true,
            optional: true,
            captureAsn1: 'rsaPublicKey'
         }]
      }]
   };
   
   // validator for an RSA public key
   var rsaPublicKeyValidator = {
      // RSAPublicKey
      name: 'RSAPublicKey',
      tagClass: asn1.Class.UNIVERSAL,
      type: asn1.Type.SEQUENCE,
      constructed: true,
      value: [{
         // modulus (n)
         name: 'RSAPublicKey.modulus',
         tagClass: asn1.Class.UNIVERSAL,
         type: asn1.Type.INTEGER,
         constructed: false,
         capture: 'publicKeyModulus'
      }, {
         // publicExponent (e)
         name: 'RSAPublicKey.exponent',
         tagClass: asn1.Class.UNIVERSAL,
         type: asn1.Type.INTEGER,
         constructed: false,
         capture: 'publicKeyExponent'
      }]
   };
   
   // validator for an X.509v3 certificate
   var x509CertificateValidator = {
      name: 'Certificate',
      tagClass: asn1.Class.UNIVERSAL,
      type: asn1.Type.SEQUENCE,
      constructed: true,
      value: [{
         name: 'Certificate.TBSCertificate',
         tagClass: asn1.Class.UNIVERSAL,
         type: asn1.Type.SEQUENCE,
         constructed: true,
         value: [{
            name: 'Certificate.TBSCertificate.version',
            tagClass: asn1.Class.CONTEXT_SPECIFIC,
            type: 0,
            constructed: true,
            optional: true,
            value: [{
               name: 'Certificate.TBSCertificate.version.integer',
               tagClass: asn1.Class.UNIVERSAL,
               type: asn1.Type.BIT_STRING,
               constructed: false,
               capture: 'certVersion'
            }]
         }, {
            name: 'Certificate.TBSCertificate.serialNumber',
            tagClass: asn1.Class.UNIVERSAL,
            type: asn1.Type.BIT_STRING,
            constructed: false,
            capture: 'certSerialNumber'
         }, {
            name: 'Certificate.TBSCertificate.signature',
            tagClass: asn1.Class.UNIVERSAL,
            type: asn1.Type.SEQUENCE,
            constructed: true,
            value: [{
               name: 'Certificate.TBSCertificate.signature.algorithm',
               tagClass: asn1.Class.UNIVERSAL,
               type: asn1.Type.OID,
               constructed: false,
               capture: 'certSignatureOid'
            }]
         }, {
            name: 'Certificate.TBSCertificate.issuer',
            tagClass: asn1.Class.UNIVERSAL,
            type: asn1.Type.SEQUENCE,
            constructed: true,
            captureAsn1: 'certIssuer'
         }, {
            name: 'Certificate.TBSCertificate.validity',
            tagClass: asn1.Class.UNIVERSAL,
            type: asn1.Type.SEQUENCE,
            constructed: true,
            value: [{
               // notBefore (Time) (only UTC time is supported)
               name: 'Certificate.TBSCertificate.validity.notBefore',
               tagClass: asn1.Class.UNIVERSAL,
               type: asn1.Type.UTCTIME,
               constructed: false,
               capture: 'certNotBefore'
            }, {
               // notAfter (Time) (only UTC time is supported)
               name: 'Certificate.TBSCertificate.validity.notAfter',
               tagClass: asn1.Class.UNIVERSAL,
               type: asn1.Type.UTCTIME,
               constructed: false,
               capture: 'certNotAfter'
            }]
         }, {
            // Name (subject) (RDNSequence)
            name: 'Certificate.TBSCertificate.subject',
            tagClass: asn1.Class.UNIVERSAL,
            type: asn1.Type.SEQUENCE,
            constructed: true,
            captureAsn1: 'certSubject'
         }, 
            // SubjectPublicKeyInfo
            publicKeyValidator
         , {
            // issuerUniqueID (optional)
            name: 'Certificate.TBSCertificate.issuerUniqueID',
            tagClass: asn1.Class.CONTEXT_SPECIFIC,
            type: 1,
            constructed: false,
            capture: 'certIssuerUniqueId',
            optional: true
         }, {
            // subjectUniqueID (optional)
            name: 'Certificate.TBSCertificate.subjectUniqueID',
            tagClass: asn1.Class.CONTEXT_SPECIFIC,
            type: 2,
            constructed: false,
            capture: 'certSubjectUniqueId',
            optional: true
         }, {
            // Extensions (optional)
            name: 'Certificate.TBSCertificate.extensions',
            tagClass: asn1.Class.CONTEXT_SPECIFIC,
            type: 3,
            constructed: true,
            captureAsn1: 'certExtensions',
            optional: true
         }]
      }, {
         // AlgorithmIdentifier (signature algorithm)
         name: 'Certificate.signatureAlgorithm',
         tagClass: asn1.Class.UNIVERSAL,
         type: asn1.Type.SEQUENCE,
         constructed: true,
         value: [{
            // algorithm
            name: 'Certificate.signatureAlgorithm.algorithm',
            tagClass: asn1.Class.UNIVERSAL,
            type: asn1.Type.OID,
            constructed: false,
            capture: 'certSignatureOid'
         }],
      }, {
         // SignatureValue
         name: 'Certificate.signatureValue',
         tagClass: asn1.Class.UNIVERSAL,
         type: asn1.Type.BITSTRING,
         constructed: false,
         capture: 'certSignature'
      }]
   };
   
   /**
    * Converts an RDNSequence of ASN.1 DER-encoded RelativeDistinguishedName
    * sets into an array with objects that have type and value properties.
    * 
    * @param rdn the RDNSequence to convert.
    */
   var _getAttributesAsArray = function(rdn)
   {
      var rval = [];
      
      // each value in 'rdn' in is a SET of RelativeDistinguishedName
      var set, attr, obj;
      for(var si = 0; si < rdn.value.length; ++si)
      {
         // get the RelativeDistinguishedName set
         set = rdn.value[si];
         
         // each value in the SET is an AttributeTypeAndValue sequence
         // containing first a type (an OID) and second a value (defined by
         // the OID)
         for(var i = 0; i < set.value.length; ++i)
         {
            obj = {};
            attr = set.value[i];
            obj.type = asn1.derToOid(attr.value[0].value);
            obj.value = attr.value[1].value;
            // if the OID is known, get its name and short name
            if(obj.type in oids)
            {
               obj.name = oids[obj.type];
               if(obj.name in _shortNames)
               {
                  obj.shortName = _shortNames[obj.name];
               }
            }
            rval.push(obj);
         }
      }
      
      return rval;
   };
   
   // regex for stripping PEM header and footer
   var _pemRegex = new RegExp(
      '-----BEGIN [^-]+-----([A-Za-z0-9+\/=\s/\r/\n]+)-----END [^-]+-----');
   
   /**
    * Converts PEM-formatted data into an certificate or key.
    * 
    * @param pem the PEM-formatted data.
    * @param func the certificate or key function to convert from ASN.1.
    * 
    * @return the certificate or key.
    */
   var _fromPem = function(pem, func)
   {
      var rval = null;
      
      // get matching base64
      var m = _pemRegex.exec(pem);
      if(m)
      {
         // base64 decode to get DER
         var der = krypto.util.createBuffer(krypto.util.decode64(m[1]));
         
         // parse DER into asn.1 object
         var obj = asn1.fromDer(der);
         
         // convert from asn.1
         rval = func(obj);
      }
      else
      {
         throw 'Invalid PEM format';
      }
      
      return rval;
   };
   
   
   /**
    * Converts an X.509 certificate from PEM format.
    * 
    * @param pem the PEM-formatted certificate.
    * 
    * @return the certificate.
    */
   pki.certificateFromPem = function(pem)
   {
      return _fromPem(pem, pki.certificateFromAsn1);
   };
   
   /**
    * Converts an RSA public key from PEM format.
    * 
    * @param pem the PEM-formatted public key.
    * 
    * @return the public key.
    */
   pki.publicKeyFromPem = function(pem)
   {
      return _fromPem(pem, pki.publicKeyFromAsn1);
   };
   
   /**
    * Converts an X.509v3 RSA certificate from an ASN.1 object.
    * 
    * @param obj the asn1 representation of an X.509v3 RSA certificate.
    * 
    * @return the certificate.
    */
   pki.certificateFromAsn1 = function(obj)
   {
      // validate certificate and capture data
      var capture = {};
      var errors = [];
      if(!asn1.validate(obj, x509CertificateValidator, capture, errors))
      {
         throw {
            message: 'Cannot read X.509 certificate. ' +
               'ASN.1 object is not an X509v3 Certificate.',
            errors: errors
         };
      }
      
      // get oid
      var oid = asn1.derToOid(capture.publicKeyOid);
      if(oid !== krypto.oids['rsaEncryption'])
      {
         throw {
            message: 'Cannot read public key. OID is not RSA.'
         };
      }
      
      // start building cert
      var cert = {};
      cert.version = capture.certVersion ?
         capture.certVersion.charCodeAt(0) : 0;
      var serial = krypto.util.createBuffer(capture.certSerialNumber);
      cert.serialNumber = serial.toHex();
      cert.signatureOid = krypto.asn1.derToOid(capture.certSignatureOid);
      // skip "unused bits" in signature value BITSTRING
      var signature = krypto.util.createBuffer(capture.certSignature);
      ++signature.read;
      cert.signature = signature.getBytes();
      cert.validity = {};
      cert.validity.notBefore = asn1.utcTimeToDate(capture.certNotBefore);
      cert.validity.notAfter = asn1.utcTimeToDate(capture.certNotAfter);
      
      // handle issuer
      cert.issuer = {};
      cert.issuer.getField = function(sn)
      {
         var rval = null;
         var attr;
         for(var i = 0; i < cert.issuer.attributes.length; ++i)
         {
            attr = cert.issuer.attributes[i];
            if(attr.shortName && attr.shortName === sn)
            {
               rval = attr.value;
               break;
            }
         }
         return rval;
      };
      cert.issuer.attributes = _getAttributesAsArray(capture.certIssuer);
      if(capture.certIssuerUniqueId)
      {
         cert.issuer.uniqueId = capture.certIssuerUniqueId;
      }
      
      // handle subject
      cert.subject = {};
      cert.subject.getField = function(sn)
      {
         var rval = null;
         var attr;
         for(var i = 0; i < cert.subject.attributes.length; ++i)
         {
            attr = cert.subject.attributes[i];
            if(attr.shortName && attr.shortName === sn)
            {
               rval = attr.value;
               break;
            }
         }
         return rval;
      };
      cert.subject.attributes = _getAttributesAsArray(capture.certSubject);
      if(capture.certSubjectUniqueId)
      {
         cert.subject.uniqueId = capture.certSubjectUniqueId;
      }
      
      // handle extensions
      cert.extensions = [];
      if(capture.certExtensions)
      {
         var e, ext, extseq;
         for(var i = 0; i < capture.certExtensions.value.length; ++i)
         {
            // get extension sequence
            extseq = capture.certExtensions.value[i];
            for(var ei = 0; ei < extseq.value.length; ++ei)
            {
               // an extension has:
               // [0] extnID      OBJECT IDENTIFIER
               // [1] critical    BOOLEAN DEFAULT FALSE
               // [2] extnValue   OCTET STRING
               ext = extseq.value[ei];
               e = {};
               e.id = asn1.derToOid(ext.value[0].value);
               e.critical = false;
               if(ext.value[1].type === asn1.Type.BOOLEAN)
               {
                  e.critical = (ext.value[1].value == 1);
                  e.value = ext.value[2].value;
               }
               else
               {
                  e.value = ext.value[1].value;
               }
               // if the oid is known, get its name
               if(e.id in oids)
               {
                  e.name = oids[e.id];
               }
               cert.extensions.push(e);
            }
         }
      }
      
      // convert RSA public key from ASN.1
      cert.publicKey = pki.publicKeyFromAsn1(capture.subjectPublicKeyInfo);
      
      /**
       * Attempts verify the signature on the passed certificate using this
       * certificate's public key.
       * 
       * @param child the certificate to verify.
       * 
       * @return true if verified, false if not.
       */
      cert.verify = function(child)
      {
         var rval = false;
         
         // check OID for supported signature types
         var md = null;
         if(child.signatureOid in oids)
         {
            var oid = oids[child.signatureOid];
            if(oid === 'sha1withRSAEncryption')
            {
               md = krypto.md.sha1.create();
            }
            else if(oid === 'md5withRSAEncryption')
            {
               md = krypto.md.md5.create();
            }
         }
         
         if(md !== null)
         {
            // FIXME: update digest with child cert data
            
            // verify signature on cert using public key
            rval = cert.publicKey.verify(
               md.digest().getBytes(), cert.signature);
         }
         
         return rval;
      };
      
      return cert;
   };
   
   /**
    * Converts a public key from an ASN.1 object.
    * 
    * @param obj the asn1 representation of of a SubjectPublicKeyInfo.
    * 
    * @return the public key.
    */
   pki.publicKeyFromAsn1 = function(obj)
   {
      // validate subject public key info and capture data
      var capture = {};
      var errors = [];
      if(!asn1.validate(obj, publicKeyValidator, capture, errors))
      {
         throw {
            message: 'Cannot read public key. ' +
               'ASN.1 object is not a SubjectPublicKeyInfo.',
            errors: errors
         };
      }
      
      // get oid
      var oid = asn1.derToOid(capture.publicKeyOid);
      if(oid !== krypto.oids['rsaEncryption'])
      {
         throw {
            message: 'Cannot read public key. Unknown OID.',
            oid: oid
         };
      }
      
      // get RSA params
      errors = [];
      if(!asn1.validate(
         capture.rsaPublicKey, rsaPublicKeyValidator, capture, errors))
      {
         throw {
            message: 'Cannot read public key. ' +
               'ASN.1 object is not an RSAPublicKey.',
            errors: errors
         };
      }
      
      // create public key
      return pki.createRsaPublicKey(
         new BigInteger(capture.publicKeyModulus, 256),
         new BigInteger(capture.publicKeyExponent, 256));
   };
   
   /**
    * Creates an RSA public key from BigIntegers modulus and exponent.
    * 
    * @param modulus the modulus.
    * @param exponent the exponent.
    * 
    * @return the public key.
    */
   pki.createRsaPublicKey = function(modulus, exponent)
   {
      var key = 
      {
         modulus: modulus,
         exponent: exponent
      };
      
      /**
       * Encrypts the given data with this public key.
       * 
       * @param data the byte string to encrypt.
       * 
       * @return the encrypted byte string.
       */
      key.encrypt = function(data)
      {
         return pki.rsa.encrypt(data, key.modulus, key.exponent, 0x02);
      };
      
      /**
       * Verifies the given signature against the given digest.
       * 
       * Once RSA-decrypted, the signature is an OCTET STRING that holds
       * a DigestInfo.
       * 
       * DigestInfo ::= SEQUENCE {
       *    digestAlgorithm DigestAlgorithmIdentifier,
       *    digest Digest
       * }
       * DigestAlgorithmIdentifier ::= AlgorithmIdentifier
       * Digest ::= OCTET STRING
       * 
       * @param digest the message digest hash to compare against the signature.
       * @param signature the signature to verify.
       * 
       * @return true if the signature was verified, false if not.
       */
      key.verify = function(digest, signature)
      {
         // do rsa decryption
         var d = pki.rsa.decrypt(signature, key.modulus, key.exponent, 0x02);
         
         // d is ASN.1 BER-encoded DigestInfo
         var obj = asn1.fromDer(d);
         console.log('DigestInfo', asn1.prettyPrint(obj));
         
         // compare the given digest to the decrypted one
         return digest === obj.value[1].value;
      };
      
      return key;
   };
   
   /**
    * RSA encryption and decryption, see RFC 2313.
    */ 
   pki.rsa = {};
   
   /**
    * Performs RSA encryption.
    * 
    * @param m the message to encrypt as a byte string.
    * @param n the modulus to use.
    * @param c the exponent to use.
    * @param bt the block type to use (0x01 for private key, 0x02 for public).
    * 
    * @return the encrypted bytes as a string.
    */
   pki.rsa.encrypt = function(m, n, c, bt)
   {
      // get the length of the modulus in bytes
      var k = n.bitLength() << 3;
      
      if(m.length > k - 11)
      {
         throw {
            message: 'Message is too long to encrypt.',
            length: m.length,
            max: (k - 11)
         };
      }
      
      /* A block type BT, a padding string PS, and the data D shall be
         formatted into an octet string EB, the encryption block:
         
         EB = 00 || BT || PS || 00 || D
         
         The block type BT shall be a single octet indicating the structure of
         the encryption block. For this version of the document it shall have
         value 00, 01, or 02. For a private-key operation, the block type
         shall be 00 or 01. For a public-key operation, it shall be 02.
         
         The padding string PS shall consist of k-3-||D|| octets. For block
         type 00, the octets shall have value 00; for block type 01, they
         shall have value FF; and for block type 02, they shall be
         pseudorandomly generated and nonzero. This makes the length of the
         encryption block EB equal to k.
       */
      
      // build the encryption block
      var eb = krypto.util.createBuffer();
      eb.putByte(0x00);
      eb.putByte(bt);
      
      // create the padding
      var padNum = k - 3 - m.length();
      var padByte;
      if(bt === 0x00 || bt === 0x01)
      {
         padByte = (bt === 0x00) ? 0x00 : 0xFF;
         for(var i = 0; i < padNum; ++i)
         {
            eb.putByte(padByte);
         }
      }
      else
      {
         for(var i = 0; i < padNum; ++i)
         {
            padByte = Math.floor(Math.random() * 255) + 1;
            eb.putByte(padByte);
         }
      }
      
      eb.putByte(0x00);
      eb.putBytes(m);
      
      // load encryption block as big integer 'x'
      var x = new BigInteger(eb.getBytes(), 256);
      
      // do RSA encryption
      var y = x.modPowInt(c, n);
      
      // convert y into byte string
      return y.toString(256);
   };
   
   /**
    * Performs RSA decryption.
    * 
    * @param ed the encrypted data to decrypt in as a byte string.
    * @param n the modulus to use.
    * @param c the exponent to use.
    * @param pub true for a public key operation, false for private.
    * @param ml the message length, if known.
    * 
    * @return the decrypted message in as a byte string.
    */
   pki.rsa.decrypt = function(ed, n, c, pub, ml)
   {
      var m = krypto.util.createBuffer();
      
      // get the length of the modulus in bytes
      var k = n.bitLength() << 3;
      
      // error if the length of the encrypted data ED is not k
      if(ed.length != k)
      {
         throw {
            message: 'Encrypted message length is invalid.',
            length: m.length,
            expected: k
         };
      }
      
      // convert encrypted data into a big integer
      var y = new BigInteger(ed, 256);
      
      // do RSA decryption
      var x = y.modPowInt(c, n);
      
      // create the encryption block
      var eb = krypto.util.createBuffer(x.toString(256));
      
      /* It is an error if any of the following conditions occurs:
      
         1. The encryption block EB cannot be parsed unambiguously.
         2. The padding string PS consists of fewer than eight octets
            or is inconsisent with the block type BT.
         3. The decryption process is a public-key operation and the block
            type BT is not 00 or 01, or the decryption process is a
            private-key operation and the block type is not 02.
       */
      
      // parse the encryption block
      var first = eb.getByte();
      var bt = eb.getByte();
      if(first !== 0x00 ||
         (pub && bt !== 0x00 && bt !== 0x01) ||
         (!pub && bt != 0x02) ||
         (pub && bt === 0x00 && typeof(ml) === 'undefined'))
      {
         throw {
            message: 'Encryption block is invalid.'
         };
      }
      
      var padNum = 0;
      if(bt === 0x00)
      {
         // check all padding bytes for 0x00
         var padNum = k - 3 - ml;
         for(var i = 0; i < padNum; ++i)
         {
            if(eb.getByte() !== 0x00)
            {
               throw {
                  message: 'Encryption block is invalid.'
               };
            }
         }
      }
      else if(bt === 0x01)
      {
         // check all padding bytes for 0xFF up to 0x00
         var padNum = 0;
         while(eb.length() > 1)
         {
            if(eb.getByte() !== 0xFF)
            {
               throw {
                  message: 'Encryption block is invalid.'
               };
            }
            ++padNum;
         }
      }
      else if(bt === 0x02)
      {
         // look for 0x00 byte
         var padNum = 0;
         while(eb.length() > 1)
         {
            if(eb.getByte() === 0x00)
            {
               --eb.read;
               break;
            }
            ++padNum;
         }
      }
      
      // zero must be 0x00 and padNum must be (k - 3 - message length)
      var zero = eb.getByte();
      if(zero !== 0x00 || padNum !== (k - 3 - eb.length()))
      {
         throw {
            message: 'Encryption block is invalid.'
         };
      }
      
      // return message
      return eb.getBytes();
   };
   
   /**
    * The pki API.
    */
   window.krypto.pki = pki;
})();

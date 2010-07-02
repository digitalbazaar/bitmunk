/**
 * Javascript implementation of a basic Public Key Infrastructure, including
 * support for RSA public and private keys.
 *
 * @author Dave Longley
 *
 * Copyright (c) 2010 Digital Bazaar, Inc. All rights reserved.
 * 
 * The ASN.1 representation of an X.509v3 certificate is as follows
 * (see RFC 2459):
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
 * An RSA private key as the following structure:
 * 
 * RSAPrivateKey ::= SEQUENCE {
 *    version Version,
 *    modulus INTEGER, -- n
 *    publicExponent INTEGER, -- e
 *    privateExponent INTEGER, -- d
 *    prime1 INTEGER, -- p
 *    prime2 INTEGER, -- q
 *    exponent1 INTEGER, -- d mod (p-1)
 *    exponent2 INTEGER, -- d mod (q-1)
 *    coefficient INTEGER -- (inverse of q) mod p
 * }
 *
 * Version ::= INTEGER
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
   //oids['1.2.840.113549.1.1.2'] = 'md2withRSAEncryption';
   //oids['md2withRSAEncryption'] = '1.2.840.113549.1.1.2';
   //oids['1.2.840.113549.1.1.3'] = 'md4withRSAEncryption';
   //oids['md4withRSAEncryption'] = '1.2.840.113549.1.1.3';
   oids['1.2.840.113549.1.1.4'] = 'md5withRSAEncryption';
   oids['md5withRSAEncryption'] = '1.2.840.113549.1.1.4';
   oids['1.2.840.113549.1.1.5'] = 'sha1withRSAEncryption';
   oids['sha1withRSAEncryption'] = '1.2.840.113549.1.1.5';
   
   oids['1.3.14.3.2.26'] = 'sha1';
   oids['sha1'] = '1.3.14.3.2.26';
   oids['1.2.840.113549.2.5'] = 'md5';
   oids['md5'] = '1.2.840.113549.2.5';
   
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
   oids['subjectKeyIdentifier'] = '2.5.29.14';
   oids['2.5.29.15'] = 'keyUsage';
   oids['keyUsage'] = '2.5.29.15';
   oids['2.5.29.16'] = 'privateKeyUsagePeriod';
   oids['2.5.29.17'] = 'subjectAltName';
   oids['subjectAltName'] = '2.5.29.17';
   oids['2.5.29.18'] = 'issuerAltName';
   oids['issuerAltName'] = '2.5.29.18';
   oids['2.5.29.19'] = 'basicConstraints';
   oids['basicConstraints'] = '2.5.29.19';
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
   oids['extKeyUsage'] = '2.5.29.37';
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
         captureAsn1: 'certTbs',
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
         }]
      }, {
         // SignatureValue
         name: 'Certificate.signatureValue',
         tagClass: asn1.Class.UNIVERSAL,
         type: asn1.Type.BITSTRING,
         constructed: false,
         capture: 'certSignature'
      }]
   };
   
   // validator for an RSA private key
   var rsaPrivateKeyValidator = {
      // RSAPrivateKey
      name: 'RSAPrivateKey',
      tagClass: asn1.Class.UNIVERSAL,
      type: asn1.Type.SEQUENCE,
      constructed: true,
      value: [{
         // Version (INTEGER)
         name: 'RSAPrivateKey.version',
         tagClass: asn1.Class.UNIVERSAL,
         type: asn1.Type.INTEGER,
         constructed: false,
         capture: 'privateKeyVersion'
      }, {
         // modulus (n)
         name: 'RSAPrivateKey.modulus',
         tagClass: asn1.Class.UNIVERSAL,
         type: asn1.Type.INTEGER,
         constructed: false,
         capture: 'privateKeyModulus'
      }, {
         // publicExponent (e)
         name: 'RSAPrivateKey.publicExponent',
         tagClass: asn1.Class.UNIVERSAL,
         type: asn1.Type.INTEGER,
         constructed: false,
         capture: 'privateKeyPublicExponent'
      }, {
         // privateExponent (d)
         name: 'RSAPrivateKey.privateExponent',
         tagClass: asn1.Class.UNIVERSAL,
         type: asn1.Type.INTEGER,
         constructed: false,
         capture: 'privateKeyPrivateExponent'
      }, {
         // prime1 (p)
         name: 'RSAPrivateKey.prime1',
         tagClass: asn1.Class.UNIVERSAL,
         type: asn1.Type.INTEGER,
         constructed: false,
         capture: 'privateKeyPrime1'
      }, {
         // prime2 (q)
         name: 'RSAPrivateKey.prime2',
         tagClass: asn1.Class.UNIVERSAL,
         type: asn1.Type.INTEGER,
         constructed: false,
         capture: 'privateKeyPrime2'
      }, {
         // exponent1 (d mod (p-1))
         name: 'RSAPrivateKey.exponent1',
         tagClass: asn1.Class.UNIVERSAL,
         type: asn1.Type.INTEGER,
         constructed: false,
         capture: 'privateKeyExponent1'
      }, {
         // exponent2 (d mod (q-1))
         name: 'RSAPrivateKey.exponent2',
         tagClass: asn1.Class.UNIVERSAL,
         type: asn1.Type.INTEGER,
         constructed: false,
         capture: 'privateKeyExponent2'
      }, {
         // coefficient ((inverse of q) mod p)
         name: 'RSAPrivateKey.coefficient',
         tagClass: asn1.Class.UNIVERSAL,
         type: asn1.Type.INTEGER,
         constructed: false,
         capture: 'privateKeyCoefficient'
      }]
   };
   
   /**
    * Converts an RDNSequence of ASN.1 DER-encoded RelativeDistinguishedName
    * sets into an array with objects that have type and value properties.
    * 
    * @param rdn the RDNSequence to convert.
    * @param md a message digest to append type and value to if provided.
    */
   var _getAttributesAsArray = function(rdn, md)
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
            if(md)
            {
               md.update(obj.type);
               md.update(obj.value);
            }
            rval.push(obj);
         }
      }
      
      return rval;
   };
   
   /**
    * Gets an issuer or subject attribute from its name, type, or short name.
    * 
    * @param obj the issuer or subject object.
    * @param options a short name string or an object with:
    *           shortName the short name for the attribute.
    *           name the name for the attribute.
    *           type the type for the attribute.
    * 
    * @return the attribute.
    */
   var _getAttribute = function(obj, options)
   {
      if(options.constructor == String)
      {
         options = {
            shortName: options
         };
      }
      
      var rval = null;
      var attr;
      for(var i = 0; rval === null && i < obj.attributes.length; ++i)
      {
         attr = obj.attributes[i];
         if(options.type && options.type === attr.type)
         {
            rval = attr;
         }
         else if(options.name && options.name === attr.name)
         {
            rval = attr;
         }
         else if(options.shortName && options.shortName === attr.shortName)
         {
            rval = attr;
         }
      }
      return rval;
   };
   
   /**
    * Converts an ASN.1 extensions object (with extension sequences as its
    * values) into an array of extension objects with types and values.
    * 
    * Supported extensions:
    * 
    * id-ce-keyUsage OBJECT IDENTIFIER ::=  { id-ce 15 }
    * KeyUsage ::= BIT STRING {
    *    digitalSignature        (0),
    *    nonRepudiation          (1),
    *    keyEncipherment         (2),
    *    dataEncipherment        (3),
    *    keyAgreement            (4),
    *    keyCertSign             (5),
    *    cRLSign                 (6),
    *    encipherOnly            (7),
    *    decipherOnly            (8)
    * }
    * 
    * id-ce-basicConstraints OBJECT IDENTIFIER ::=  { id-ce 19 }
    * BasicConstraints ::= SEQUENCE {
    *    cA                      BOOLEAN DEFAULT FALSE,
    *    pathLenConstraint       INTEGER (0..MAX) OPTIONAL
    * }
    * 
    * @param exts the extensions ASN.1 with extension sequences to parse.
    * 
    * @return the array.
    */
   var _parseExtensions = function(exts)
   {
      var rval = [];
      
      var e, ext, extseq;
      for(var i = 0; i < exts.value.length; ++i)
      {
         // get extension sequence
         extseq = exts.value[i];
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
               e.critical = (ext.value[1].value.charCodeAt(0) != 0x00);
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
               
               // handle key usage
               if(e.name === 'keyUsage')
               {
                  // get value as BIT STRING
                  var ev = asn1.fromDer(e.value);
                  var b2 = 0x00;
                  var b3 = 0x00;
                  if(ev.value.length > 1)
                  {
                     // skip first byte, just indicates unused bits which
                     // will be padded with 0s anyway
                     // get bytes with flag bits
                     b2 = ev.value.charCodeAt(1);
                     b3 = ev.value.length > 2 ? ev.value.charCodeAt(2) : 0;
                  }
                  // set flags
                  e.digitalSignature = (b2 & 0x80) == 0x80;
                  e.nonRepudiation = (b2 & 0x40) == 0x40;
                  e.keyEncipherment = (b2 & 0x20) == 0x20;
                  e.dataEncipherment = (b2 & 0x10) == 0x10;
                  e.keyAgreement = (b2 & 0x08) == 0x08;
                  e.keyCertSign = (b2 & 0x04) == 0x04;
                  e.cRLSign = (b2 & 0x02) == 0x02;
                  e.encipherOnly = (b2 & 0x01) == 0x01;
                  e.decipherOnly = (b3 & 0x80) == 0x80;
               }
               // handle basic constraints
               if(e.name === 'basicConstraints')
               {
                  // get value as SEQUENCE
                  var ev = asn1.fromDer(e.value);
                  // get cA BOOLEAN flag (defaults to false)
                  if(ev.value.length > 0)
                  {
                     e.cA = (ev.value[0].value.charCodeAt(0) != 0x00);
                  }
                  else
                  {
                     e.cA = false;
                  }
                  // get path length constraint
                  if(ev.value.length > 1)
                  {
                     var tmp = krypto.util.createBuffer(ev.value[1].value);
                     e.pathLenConstraint = tmp.getInt(tmp.length() << 3);
                  }
               }
            }
            rval.push(e);
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
    * Note: If the certificate is to be verified then compute hash should
    * be set to true. There is currently no implementation for converting
    * a certificate back to ASN.1 so the TBSCertificate part of the ASN.1
    * object needs to be scanned before the cert object is created.
    * 
    * @param pem the PEM-formatted certificate.
    * @param computeHash true to compute the hash for verification.
    * 
    * @return the certificate.
    */
   pki.certificateFromPem = function(pem, computeHash)
   {
      return _fromPem(pem, function(obj)
      {
         return pki.certificateFromAsn1(obj, computeHash);
      });
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
    * Converts an RSA private key from PEM format.
    * 
    * @param pem the PEM-formatted private key.
    * 
    * @return the private key.
    */
   pki.privateKeyFromPem = function(pem)
   {
      return _fromPem(pem, pki.privateKeyFromAsn1);
   };
   
   /**
    * Converts an X.509v3 RSA certificate from an ASN.1 object.
    * 
    * Note: If the certificate is to be verified then compute hash should
    * be set to true. There is currently no implementation for converting
    * a certificate back to ASN.1 so the TBSCertificate part of the ASN.1
    * object needs to be scanned before the cert object is created.
    * 
    * @param obj the asn1 representation of an X.509v3 RSA certificate.
    * @param computeHash true to compute the hash for verification.
    * 
    * @return the certificate.
    */
   pki.certificateFromAsn1 = function(obj, computeHash)
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
      
      if(computeHash)
      {
         // check signature OID for supported signature types
         cert.md = null;
         if(cert.signatureOid in oids)
         {
            var oid = oids[cert.signatureOid];
            if(oid === 'sha1withRSAEncryption')
            {
               cert.md = krypto.md.sha1.create();
            }
            else if(oid === 'md5withRSAEncryption')
            {
               cert.md = krypto.md.md5.create();
            }
         }
         if(cert.md === null)
         {
            throw {
               message: 'Could not compute certificate digest. ' +
                  'Unknown signature OID.',
               signatureOid: cert.signatureOid
            };
         }
         
         // produce DER formatted TBSCertificate and digest it
         var bytes = asn1.toDer(capture.certTbs);
         cert.md.update(bytes.getBytes());
      }
      
      // handle issuer, build issuer message digest
      var imd = krypto.md.sha1.create();
      cert.issuer = {};
      cert.issuer.getField = function(sn)
      {
         return _getAttribute(cert.issuer, sn);
      };
      cert.issuer.attributes = _getAttributesAsArray(capture.certIssuer, imd);
      if(capture.certIssuerUniqueId)
      {
         cert.issuer.uniqueId = capture.certIssuerUniqueId;
      }
      cert.issuer.hash = imd.digest().toHex();
      
      // handle subject, build subject message digest
      var smd = krypto.md.sha1.create();
      cert.subject = {};
      cert.subject.getField = function(sn)
      {
         return _getAttribute(cert.subject, sn);
      };
      cert.subject.attributes = _getAttributesAsArray(capture.certSubject, smd);
      if(capture.certSubjectUniqueId)
      {
         cert.subject.uniqueId = capture.certSubjectUniqueId;
      }
      cert.subject.hash = smd.digest().toHex();
      
      // handle extensions
      if(capture.certExtensions)
      {
         cert.extensions = _parseExtensions(capture.certExtensions);
      }
      else
      {
         cert.extensions = [];
      }
      
      /**
       * Gets an extension by its name or id.
       * 
       * @param options the name to use or an object with:
       *           name the name to use.
       *           id the id to use.
       * 
       * @return the extension or null if not found.
       */
      cert.getExtension = function(options)
      {
         if(options.constructor == String)
         {
            options = {
               name: options
            };
         }
         
         var rval = null;
         var ext;
         for(var i = 0; rval === null && i < cert.extensions; ++i)
         {
            ext = cert.extensions[i];
            if(options.id && ext.id === options.id)
            {
               rval = ext;
            }
            else if(options.name && ext.name === options.name)
            {
               rval = ext;
            }
         }
         return rval;
      };
      
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
         
         if(child.md !== null)
         {
            // verify signature on cert using public key
            rval = cert.publicKey.verify(
               child.md.digest().getBytes(), cert.signature);
         }
         
         return rval;
      };
      
      /**
       * Returns true if the passed certificate's subject is the issuer of
       * this certificate.
       * 
       * @param parent the certificate to check.
       * 
       * @return true if the passed certificate's subject is the issuer of
       *         this certificate.
       */
      cert.isIssuer = function(parent)
      {
         var rval = false;
         
         var i = cert.issuer;
         var s = parent.subject;
         
         // compare hashes if present
         if(i.hash && s.hash)
         {
            rval = (i.hash === s.hash);
         }
         // if all attributes are the same then issuer matches subject
         else if(i.attributes.length === s.attributes.length)
         {
            rval = true;
            var iattr, sattr;
            for(var n; rval && n < i.attributes.length; ++i)
            {
               iattr = i.attributes[n];
               sattr = s.attributes[n];
               if(iattr.type !== sattr.type || iattr.value !== sattr.value)
               {
                  // attribute mismatch
                  rval = false;
               }
            }
         }
         
         return rval;
      };
      
      return cert;
   };
   
   /**
    * Creates a CA store.
    * 
    * @param certs an optional array of certificate objects or PEM-formatted
    *           certificate strings to add to the CA store.
    * 
    * @return the CA store.
    */
   pki.createCaStore = function(certs)
   {
      // stored certificates
      var _certs = {};
      
      // create CA store
      var caStore = {};
      
      /**
       * Gets the certificate that issued the passed certificate or its
       * 'parent'.
       * 
       * @param cert the certificate to get the parent for.
       * 
       * @return the parent certificate or null if none was found.
       */
      caStore.getIssuer = function(cert)
      {
         var rval = null;
         
         // TODO: produce issuer hash if it doesn't exist
         
         // get the entry using the cert's issuer hash
         if(cert.issuer.hash in _certs)
         {
            rval = _certs[cert.issuer.hash];
            
            // see if there are multiple matches
            if(rval.constructor == Array)
            {
               // TODO: resolve multiple matches by checking
               // authorityKey/subjectKey/issuerUniqueID/other identifiers, etc.
               // FIXME: or alternatively do authority key mapping
               // if possible (X.509v1 certs can't work?)
               throw {
                  message:
                     'Resolving multiple issuer matches not implemented yet.'
               };
            }
         }
         
         return rval;
      };
      
      /**
       * Adds a trusted certificate to the store.
       * 
       * @param cert the certificate to add as a trusted certificate.
       */
      caStore.addCertificate = function(cert)
      {
         // TODO: produce subject hash if it doesn't exist
         if(cert.subject.hash in _certs)
         {
            // subject hash already exists, append to array
            var tmp = _certs[cert.subject.hash];
            if(tmp.constructor != Array)
            {
               tmp = [tmp];
            }
            tmp.push(cert);
         }
         else
         {
            _certs[cert.subject.hash] = cert;
         }
      };
      
      // auto-add passed in certs
      if(certs)
      {
         // parse PEM-formatted certificates as necessary
         for(var i = 0; i < certs.length; ++i)
         {
            var cert = certs[i];
            if(cert.constructor == String)
            {
               // convert from pem
               caStore.addCertificate(krypto.pki.certificateFromPem(cert));
            }
            else
            {
               // assume krypto.pki cert just add it
               caStore.addCertificate(cert);
            }
         }
      }
      
      return caStore;
   };
   
   /**
    * Converts a public key from an ASN.1 object.
    * 
    * @param obj the asn1 representation of a SubjectPublicKeyInfo.
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
      
      // FIXME: inefficient, get a BigInteger that uses byte strings
      var n = krypto.util.createBuffer(capture.publicKeyModulus).toHex();
      var e = krypto.util.createBuffer(capture.publicKeyExponent).toHex();
      
      // create public key
      return pki.createRsaPublicKey(
         new BigInteger(n, 16),
         new BigInteger(e, 16));
   };
   
   /**
    * Converts a private key from an ASN.1 object.
    * 
    * @param obj the asn1 representation of an RSAPrivateKey.
    * 
    * @return the private key.
    */
   pki.privateKeyFromAsn1 = function(obj)
   {
      // get RSAPrivateKey
      var capture = {};
      var errors = [];
      if(!asn1.validate(obj, rsaPrivateKeyValidator, capture, errors))
      {
         throw {
            message: 'Cannot read private key. ' +
               'ASN.1 object is not an RSAPrivateKey.',
            errors: errors
         };
      }
      
      // Note: Version is currently ignored.
      // capture.privateKeyVersion
      // FIXME: inefficient, get a BigInteger that uses byte strings
      var n, e, d, p, q, dP, dQ, qInv;
      n = krypto.util.createBuffer(capture.privateKeyModulus).toHex();
      e = krypto.util.createBuffer(capture.privateKeyPublicExponent).toHex();
      d = krypto.util.createBuffer(capture.privateKeyPrivateExponent).toHex();
      p = krypto.util.createBuffer(capture.privateKeyPrime1).toHex();
      q = krypto.util.createBuffer(capture.privateKeyPrime2).toHex();
      dP = krypto.util.createBuffer(capture.privateKeyExponent1).toHex();
      dQ = krypto.util.createBuffer(capture.privateKeyExponent2).toHex();
      qInv = krypto.util.createBuffer(capture.privateKeyCoefficient).toHex();
      
      // create private key
      return pki.createRsaPrivateKey(
         new BigInteger(n, 16),
         new BigInteger(e, 16),
         new BigInteger(d, 16),
         new BigInteger(p, 16),
         new BigInteger(q, 16),
         new BigInteger(dP, 16),
         new BigInteger(dQ, 16),
         new BigInteger(qInv, 16));
   };
   
   /**
    * Creates an RSA public key from BigIntegers modulus and exponent.
    * 
    * @param n the modulus.
    * @param e the exponent.
    * 
    * @return the public key.
    */
   pki.createRsaPublicKey = function(n, e)
   {
      var key =
      {
         n: n,
         e: e
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
         return pki.rsa.encrypt(data, key, 0x02);
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
         var d = pki.rsa.decrypt(signature, key, true);
         
         // d is ASN.1 BER-encoded DigestInfo
         var obj = asn1.fromDer(d);
         
         // compare the given digest to the decrypted one
         return digest === obj.value[1].value;
      };
      
      return key;
   };
   
   /**
    * Creates an RSA private key from BigIntegers modulus, exponent, primes,
    * prime exponents, and modular multiplicative inverse.
    * 
    * @param n the modulus.
    * @param e the public exponent.
    * @param d the private exponent ((inverse of e) mod n).
    * @param p the first prime.
    * @param q the second prime.
    * @param dP exponent1 (d mod (p-1)).
    * @param dQ exponent2 (d mod (q-1)).
    * @param qInv ((inverse of q) mod p)
    * 
    * @return the private key.
    */
   pki.createRsaPrivateKey = function(n, e, d, p, q, dP, dQ, qInv)
   {
      var key =
      {
         n: n,
         e: e,
         d: d,
         p: p,
         q: q,
         dP: dP,
         dQ: dQ,
         qInv: qInv
      };
      
      /**
       * Decrypts the given data with this private key.
       * 
       * @param data the byte string to decrypt.
       * 
       * @return the decrypted byte string.
       */
      key.decrypt = function(data)
      {
         return pki.rsa.decrypt(data, key, false);
      };
      
      /**
       * Signs the given digest, producing a signature.
       * 
       * First the digest is stored in a DigestInfo object. Then that object
       * is RSA-encrypted to produce the signature.
       * 
       * DigestInfo ::= SEQUENCE {
       *    digestAlgorithm DigestAlgorithmIdentifier,
       *    digest Digest
       * }
       * DigestAlgorithmIdentifier ::= AlgorithmIdentifier
       * Digest ::= OCTET STRING
       * 
       * @param md the message digest object with the hash to sign.
       * 
       * @return the signature as a byte string.
       */
      key.sign = function(md)
      {
         // get the oid for the algorithm
         var oid;
         if(md.algorithm in oids)
         {
            oid = oids[md.algorithm];
         }
         else
         {
            throw {
               message: 'Unknown message digest algorithm',
               algorithm: md.algorithm
            };
         }
         oidBytes = asn1.oidToDer(oid).getBytes();
         
         // create the digest info
         var digestInfo = asn1.create(
            asn1.Class.UNIVERSAL, asn1.Type.SEQUENCE, true, []);
         var digestAlgorithm = asn1.create(
            asn1.Class.UNIVERSAL, asn1.Type.SEQUENCE, true, []);
         digestAlgorithm.value.push(asn1.create(
            asn1.Class.UNIVERSAL, asn1.Type.OID, false, oidBytes));
         digestAlgorithm.value.push(asn1.create(
            asn1.Class.UNIVERSAL, asn1.Type.NULL, false, ''));
         var digest = asn1.create(
            asn1.Class.UNIVERSAL, asn1.Type.OCTETSTRING,
            false, md.digest().getBytes());
         digestInfo.value.push(digestAlgorithm);
         digestInfo.value.push(digest);
         
         // encode digest info
         var d = asn1.toDer(digestInfo).getBytes();
         
         // do rsa encryption
         return pki.rsa.encrypt(d, key, 0x01);
      };
      
      return key;
   };
   
   /**
    * RSA encryption and decryption, see RFC 2313.
    */ 
   pki.rsa = {};
   
   /**
    * Performs x^c mod n (RSA encryption or decryption operation).
    * 
    * @param x the number to raise and mod.
    * @param key the key to use.
    * @param pub true if the key is public, false if private.
    * 
    * @return the result of x^c mod n.
    */
   var _modPow = function(x, key, pub)
   {
      var y;
      
      if(pub)
      {
         y = x.modPow(key.e, key.n);
      }
      else
      {
         // pre-compute dP, dQ, and qInv if necessary
         if(!key.dP)
         {
            key.dP = key.d.mod(p.subtract(BigInteger.ONE));
         }
         if(!key.dQ)
         {
            key.dQ = key.d.mod(q.substract(BigInteger.ONE));
         }
         if(!key.qInv)
         {
            key.qInv = key.q.modInverse(key.p);
         }
         
         /* Chinese remainder theorem (CRT) states:
            
            Suppose n1, n2, ..., nk are positive integers which are pairwise
            coprime (n1 and n2 have no common factors other than 1). For any
            integers x1, x2, ..., xk there exists an integer x solving the
            system of simultaneous congruences (where ~= means modularly
            congruent so a ~= b mod n means a mod n = b mod n):
            
            x ~= x1 mod n1
            x ~= x2 mod n2
            ...
            x ~= xk mod nk
            
            This system of congruences has a single simultaneous solution x
            between 0 and n - 1. Furthermore, each xk solution and x itself
            is congruent modulo the product n = n1*n2*...*nk.
            So x1 mod n = x2 mod n = xk mod n = x mod n.
            
            The single simultaneous solution x can be solved with the following
            equation:
            
            x = sum(xi*ri*si) mod n where ri = n/ni and si = ri^-1 mod ni.
            
            Where x is less than n, xi = x mod ni.
            
            For RSA we are only concerned with k = 2. The modulus n = pq, where
            p and q are coprime. The RSA decryption algorithm is:
            
            y = x^d mod n
            
            Given the above:
            
            x1 = x^d mod p
            r1 = n/p = q
            s1 = q^-1 mod p
            x2 = x^d mod q
            r2 = n/q = p
            s2 = p^-1 mod q
            
            So y = (x1r1s1 + x2r2s2) mod n
                 = ((x^d mod p)q(q^-1 mod p) + (x^d mod q)p(p^-1 mod q)) mod n
            
            According to Fermat's Little Theorem, if the modulus P is prime,
            for any integer A not evenly divisible by P, A^(P-1) ~= 1 mod P.
            Since A is not divisible by P it follows that if:
            N ~= M mod (P - 1), then A^N mod P = A^M mod P. Therefore:
            
            A^N mod P = A^(M mod (P - 1)) mod P. (The latter takes less effort
            to calculate). In order to calculate x^d mod p more quickly the
            exponent d mod (p - 1) is stored in the RSA private key (the same
            is done for x^d mod q). These values are referred to as dP and dQ
            respectively. Therefore we now have:
            
            y = ((x^dP mod p)q(q^-1 mod p) + (x^dQ mod q)p(p^-1 mod q)) mod n
            
            Since we'll be reducing x^dP by modulo p (same for q) we can also
            reduce x by p (and q respectively) before hand. Therefore, let
            
            xp = ((x mod p)^dP mod p), and
            xq = ((x mod q)^dQ mod q), yielding:
            
            y = (xp*q*(q^-1 mod p) + xq*p*(p^-1 mod q)) mod n
            
            This can be further reduced to a simple algorithm that only
            requires 1 inverse (the q inverse is used) to be used and stored.
            The algorithm is called Garner's algorithm. If qInv is the
            inverse of q, we simply calculate:
            
            y = (qInv*(xp - xq) mod p) * q + xq
            
            However, there are two further complications. First, we need to
            ensure that xp > xq to prevent signed BigIntegers from being used
            so we add p until this is true (since we will be mod'ing with
            p anyway). Then, there is a known timing attack on algorithms
            using the CRT. To mitigate this risk, "cryptographic blinding"
            should be used (*Not yet implemented*). This requires simply
            generating a random number r between 0 and n-1 and its inverse
            and multiplying x by r^e before calculating y and then multiplying
            y by r^-1 afterwards.
          */
         
         // TODO: do cryptographic blinding
         
         // calculate xp and xq
         var xp = x.mod(key.p).modPow(key.dP, key.p);
         var xq = x.mod(key.q).modPow(key.dQ, key.q);
         
         // xp must be larger than xq to avoid signed bit usage
         while(xp.compareTo(xq) < 0)
         {
            xp = xp.add(key.p);
         }
         
         // do last step
         y = xp.subtract(xq)
            .multiply(key.qInv).mod(key.p)
            .multiply(key.q).add(xq);
      }
      
      return y;
   };
   
   /**
    * Performs RSA encryption.
    * 
    * @param m the message to encrypt as a byte string.
    * @param key the RSA key to use.
    * @param bt the block type to use (0x01 for private key, 0x02 for public).
    * 
    * @return the encrypted bytes as a string.
    */
   pki.rsa.encrypt = function(m, key, bt)
   {
      // get the length of the modulus in bytes
      var k = key.n.bitLength() >>> 3;
      
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
      
      // create the padding, get key type
      var pub;
      var padNum = k - 3 - m.length;
      var padByte;
      if(bt === 0x00 || bt === 0x01)
      {
         pub = false;
         padByte = (bt === 0x00) ? 0x00 : 0xFF;
         for(var i = 0; i < padNum; ++i)
         {
            eb.putByte(padByte);
         }
      }
      else
      {
         pub = true;
         for(var i = 0; i < padNum; ++i)
         {
            padByte = Math.floor(Math.random() * 255) + 1;
            eb.putByte(padByte);
         }
      }
      
      // zero followed by message
      eb.putByte(0x00);
      eb.putBytes(m);
      
      // load encryption block as big integer 'x'
      // FIXME: hex conversion inefficient, get BigInteger w/byte strings
      var x = new BigInteger(eb.toHex(), 16);
      
      // do RSA encryption
      var y = _modPow(x, key, pub);
      
      // convert y into the encrypted data byte string, if y is shorter in
      // bytes than k, then prepend zero bytes to fill up ed
      // FIXME: hex conversion inefficient, get BigInteger w/byte strings
      var yhex = y.toString(16);
      var ed = krypto.util.createBuffer();
      var zeros = k - Math.ceil(yhex.length / 2);
      while(zeros > 0)
      {
         ed.putByte(0x00);
         --zeros;
      }
      ed.putBytes(krypto.util.hexToBytes(yhex));
      return ed.getBytes();
   };
   
   /**
    * Performs RSA decryption.
    * 
    * @param ed the encrypted data to decrypt in as a byte string.
    * @param key the RSA key to use.
    * @param pub true for a public key operation, false for private.
    * @param ml the message length, if known.
    * 
    * @return the decrypted message in as a byte string.
    */
   pki.rsa.decrypt = function(ed, key, pub, ml)
   {
      var m = krypto.util.createBuffer();
      
      // get the length of the modulus in bytes
      var k = Math.ceil(key.n.bitLength() / 8);
      
      // error if the length of the encrypted data ED is not k
      if(ed.length != k)
      {
         throw {
            message: 'Encrypted message length is invalid.',
            length: ed.length,
            expected: k
         };
      }
      
      // convert encrypted data into a big integer
      // FIXME: hex conversion inefficient, get BigInteger w/byte strings
      var y = new BigInteger(krypto.util.createBuffer(ed).toHex(), 16);
      
      // do RSA decryption
      var x = _modPow(y, key, pub);
      
      // create the encryption block, if x is shorter in bytes than k, then
      // prepend zero bytes to fill up eb
      // FIXME: hex conversion inefficient, get BigInteger w/byte strings
      var xhex = x.toString(16);
      var eb = krypto.util.createBuffer();
      var zeros = k - Math.ceil(xhex.length / 2);
      while(zeros > 0)
      {
         eb.putByte(0x00);
         --zeros;
      }
      eb.putBytes(krypto.util.hexToBytes(xhex));
      
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
         // find the first byte that isn't 0xFF, should be after all padding
         var padNum = 0;
         while(eb.length() > 1)
         {
            if(eb.getByte() !== 0xFF)
            {
               --eb.read;
               break;
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

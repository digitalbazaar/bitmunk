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
   var pki = krypto.pki || {};
   var oids = krypto.oids || {};
   
   /**
    * Known OIDs.
    */
   oids['RSA'] = '1.2.840.113549.1.1.1';
   oids['1.2.840.113549.1.1.1'] = 'RSA';
   
   // validator for an RSA SubjectPublicKeyInfo structure
   var rsaPublicKeyValidator = {
      // SubjectPublicKeyInfo
      tagClass: asn1.Class.UNIVERSAL,
      type: asn1.Type.SEQUENCE,
      constructed: true,
      value: [{
         // AlgorithmIdentifier
         tagClass: asn1.Class.UNIVERSAL,
         type: asn1.Type.SEQUENCE,
         constructed: true,
         value: [{
            // algorithm
            tagClass: asn1.Class.UNIVERSAL,
            type: asn1.Type.OID,
            constructed: false,
            capture: 'publicKeyOid'
         }]
      }, {
         // subjectPublicKey
         tagClass: asn1.Class.UNIVERSAL,
         type: asn1.Type.SEQUENCE,
         constructed: true,
         value: [{
            // modulus (n)
            tagClass: asn1.Class.UNIVERSAL,
            type: asn1.Type.INTEGER,
            constructed: false,
            capture: 'publicKeyModulus'
         }, {
            // publicExponent (e)
            tagClass: asn1.Class.UNIVERSAL,
            type: asn1.Type.INTEGER,
            constructed: false,
            capture: 'publicKeyExponent'
         }]
      }]
   };
   
   // validator for an X.509v3 certificate with an RSA public key
   var x509CertificateValidator = {
      // Certificate
      tagClass: asn1.Class.UNIVERSAL,
      type: asn1.Type.SEQUENCE,
      constructed: true,
      value: [{
         // TBSCertificate
         tagClass: asn1.Class.UNIVERSAL,
         type: asn1.Type.SEQUENCE,
         constructed: true,
         value: [{
            // Version
            tagClass: asn1.Class.UNIVERSAL,
            type: asn1.Type.INTEGER,
            constructed: false,
            capture: 'certVersion'
         }, {
            // CertificateSerialNumber
            tagClass: asn1.Class.UNIVERSAL,
            type: asn1.Type.INTEGER,
            constructed: false,
            capture: 'certSerialNumber'
         }, {
            // AlgorithmIdentifier (signature)
            tagClass: asn1.Class.UNIVERSAL,
            type: asn1.Type.SEQUENCE,
            constructed: true,
            value: [{
               // algorithm
               tagClass: asn1.Class.UNIVERSAL,
               type: asn1.Type.OID,
               constructed: false,
               capture: 'certSignatureOid'
            }]
         }, {
            // Name (issuer) (RDNSequence)
            tagClass: asn1.Class.UNIVERSAL,
            type: asn1.Type.SEQUENCE,
            constructed: true,
            value: [{
               // RelativeDistinguishedName (SET OF AttributeTypeAndValue)
               tagClass: asn1.Class.UNIVERSAL,
               type: asn1.Type.SET,
               constructed: true,
               capture: 'certIssuer'
            }]
         }, {
            // Validity
            tagClass: asn1.Class.UNIVERSAL,
            type: asn1.Type.SEQUENCE,
            constructed: true,
            value: [{
               // notBefore (Time) (only UTC time is supported)
               tagClass: asn1.Class.UNIVERSAL,
               type: asn1.Type.UTCTIME,
               constructed: false,
               capture: 'certNotBefore'
            }, {
               // notAfter (Time) (only UTC time is supported)
               tagClass: asn1.Class.UNIVERSAL,
               type: asn1.Type.UTCTIME,
               constructed: false,
               capture: 'certNotAfter'
            }]
         }, {
            // Name (subject) (RDNSequence)
            tagClass: asn1.Class.UNIVERSAL,
            type: asn1.Type.SEQUENCE,
            constructed: true,
            value: [{
               // RelativeDistinguishedName (SET OF AttributeTypeAndValue)
               tagClass: asn1.Class.UNIVERSAL,
               type: asn1.Type.SET,
               constructed: true,
               capture: 'certSubject'
            }]
         }, rsaPublicKeyValidator /* SubjectPublicKeyInfo */, {
            // issuerUniqueID (optional)
            tagClass: asn1.Class.UNIVERSAL,
            type: asn1.Type.BITSTRING,
            constructed: false,
            capture: 'certIssuerUniqueId',
            optional: true
         }, {
            // subjectUniqueID (optional)
            tagClass: asn1.Class.UNIVERSAL,
            type: asn1.Type.BITSTRING,
            constructed: false,
            capture: 'certSubjectUniqueId',
            optional: true
         }, {
            // Extensions (optional)
            tagClass: asn1.Class.UNIVERSAL,
            type: asn1.Type.SEQUENCE,
            constructed: true,
            capture: 'certExtensions',
            optional: true
         }]
      }, {
         // AlgorithmIdentifier (signature algorithm)
         tagClass: asn1.Class.UNIVERSAL,
         type: asn1.Type.SEQUENCE,
         constructed: true,
         value: [{
            // algorithm
            tagClass: asn1.Class.UNIVERSAL,
            type: asn1.Type.OID,
            constructed: false,
            capture: 'certSignatureOid'
         }],
      }, {
         // SignatureValue
         tagClass: asn1.Class.UNIVERSAL,
         type: asn1.Type.BITSTRING,
         constructed: false,
         capture: 'certSignature'
      }]
   };
   
   /**
    * Converts a set of ASN.1 DER-encoded AttributeTypeAndValues into an
    * array with objects that have type and value properties.
    * 
    * @param set the ASN.1 set to convert.
    */
   var _getAttributesAsArray = function(set)
   {
      var rval = [];
      
      // each value is an AttributeTypeAndValue sequence containing
      // first a type (an OID) and second a value (defined by the OID)
      var attr, obj;
      for(var i = 0; i < set.value.length; ++i)
      {
         obj = {};
         attr = set.value[i];
         obj.type = asn1.derToOid(attr.value[0].value);
         obj.value = asn1.attr.value[1].value;
         rval.push(obj);
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
      console.log('obj', asn1.prettyPrint(obj));
      
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
      if(oid !== krypto.oids['RSA'])
      {
         throw {
            message: 'Cannot read public key. OID is not RSA.'
         };
      }
      
      // start building cert
      var cert = {};
      cert.version = capture.certVersion;
      var tmp = krypto.util.createBuffer(capture.certSerialNumber);
      cert.serialNumber = tmp.getInt(tmp.length() << 3);
      cert.signatureOid = krypto.asn1.derToOid(capture.certSignatureOid);
      cert.signature = capture.certSignature;
      cert.validity = {};
      cert.notBefore = new Date();
      cert.notBefore.setTime(parseInt(capture.certNotBefore));
      cert.notAfter = new Date();
      cert.notAfter.setTime(parseInt(capture.certNotAfter));
      
      // handle issuer
      cert.issuer = {};
      cert.issuer.attributes = _getAttributesAsArray(capture.certIssuer);
      if(capture.certIssuerUniqueId)
      {
         cert.issuer.uniqueId = capture.certIssuerUniqueId;
      }
      
      // handle subject
      cert.subject = {};
      cert.subject.attributes = _getAttributesAsArray(capture.certSubject);
      if(capture.certSubjectUniqueId)
      {
         cert.subject.uniqueId = capture.certSubjectUniqueId;
      }
      
      // handle extensions
      cert.extensions = [];
      if(capture.certExtensions)
      {
         // an extension has:
         // [0] extnID      OBJECT IDENTIFIER
         // [1] critical    BOOLEAN DEFAULT FALSE
         // [2] extnValue   OCTET STRING
         var e;
         var ext;
         for(var i = 0; i < capture.certExtensions.value.length; ++i)
         {
            e = {};
            ext = capture.certExtensions.value[i];
            e.id = asn1.derToOid(ext.value[0].value);
            e.critical = (ext.value[1].value == 1);
            e.value = ext.value[2].value;
            cert.extensions.push(e);
         }
      }
      
      // create public key
      cert.publicKey = createRsaPublicKey(
         new BigInteger(capture.publicKeyModulus, 256),
         new BigInteger(capture.publicKeyExponent, 256));
      
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
      if(!asn1.validate(obj, rsaPublicKeyValidator, capture, errors))
      {
         throw {
            message: 'Cannot read public key. ' +
               'ASN.1 object is not a SubjectPublicKeyInfo.',
            errors: errors
         };
      }
      
      // get oid
      var oid = asn1.derToOid(capture.publicKeyOid);
      if(oid !== krypto.oids['RSA'])
      {
         throw {
            message: 'Cannot read public key. Unknown OID.',
            oid: oid
         };
      }
      
      // create public key
      return createRsaPublicKey(
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
      var pk = 
      {
         modulus: modulus,
         exponent: exponent
      };
      
      /**
       * Encrypts the given data with this public key.
       * 
       * @param b the byte buffer of data to encrypt.
       */
      pk.encrypt = function(b)
      {
         // FIXME: implement me
      };
   };
   
   /**
    * The pki API.
    */
   window.krypto.pki = pki;
})();

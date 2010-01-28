/**
 * API for X.509 Certificates and Public Keys
 *
 * @author Dave Longley
 *
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 * 
 * An X.509 public key is stored in ASN.1 format. The SubjectPublicKeyInfo
 * ASN.1 structure is composed of an algorithm of type AlgorithmIdentifier
 * and a subjectPublicKey of type bit string.
 * 
 * The AlgorithmIdentifier contains an Object Identifier (OID) and parameters
 * for the algorithm, if any.
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
 * This implementation only supports the RSA algorithm. For an RSA public key:
 * 
 * The subjectPublicKey bit string contains the RSA public key as a sequence:
 * 
 * RSAPublicKey ::= SEQUENCE
 * {
 *    modulus INTEGER, -- n
 *    publicExponent INTEGER -- e
 * }
 * 
 * The object identifier for an RSA key is: "1.2.840.113549.1.1.1"
 * The parameters for the AlgorithmIdentifier has a value of NULL.
 */
(function()
{
   crypto = crypto || {};
   crypto.x509 =
   {
      /**
       * Object Identifiers (OID).
       */
      OID_RSA_KEY = '1.2.840.113549.1.1.1',
      
      /**
       * Creates a new asn1 object.
       * 
       * @param type the UNIVERSAL data type (tag number) for the object.
       * @param constructed true if the asn1 object is constructed from other
       *           asn1 objects, false if not.
       * @param value the value for the object, if it is not constructed.
       * 
       * @return the asn1 object.
       */
      create: function(type, constructed, value)
      {
         /**
          * An asn1 object has a type, a constructed flag, and a value. The
          * value's type depends on the constructed flag. If constructed, it
          * will contain a list of other asn1 objects. If not, it will contain
          * the ASN.1 value as an array of bytes formatted according to the
          * ASN.1 data type.
          */
         var asn1 =
         {
            type: type,
            constructed: constructed,
            value: value
         };
         
         return asn1;
      },
      
      /**
       * Parses an asn1 object from an array of bytes in DER format.
       * 
       * @param bytes the bytes to parse from.
       * @param remainder an object to set any remainder 'bytes' in.
       * 
       * @return the asn1 object or a map with 'asn1' and 'bytes' for remaining
       *         bytes.
       */
      fromDer: function(bytes, remainder)
      {
         // minimum length for ASN.1 DER structure is 2
         if(bytes.length < 2)
         {
            throw 'Invalid DER length: ' + bytes.length;
         }
         
         // classes other than UNIVERSAL not supported, check bits 7-8 for 0
         if(bytes[0] & 0xC0)
         {
            throw 'Unsupported ASN.1 class: ' + (bytes[0] & 0xC0);
         }
         
         // get the type (bits 1-5)
         var type = bytes[0] & 0x1F;
         
         // see if the length is "short form" or "long form" (bit 8 set)
         var longForm = bytes[1] & 0x80;
         var valueStart;
         var length;
         if(!longForm)
         {
            // length is just the first byte
            length = bytes[1];
            valueStart = 2;
         }
         else
         {
            // length is stored in # bytes (bits 7-1) in base 256
            // value starts at 2 + # bytes
            length = 0;
            var lenBytes = bytes[1] & 0x7F;
            var valueStart = lenBytes + 2;
            for(var i = 2; i < valueStart; i++)
            {
               length += bytes[i] << (8 * lenBytes);
               lenBytes--;
            }
         }
         
         // get the value
         var value = bytes.splice(valueStart, length);
         if(value.length != length)
         {
            throw 'ASN.1 value length does not match: ' +
               value.length + ' != ' + length; 
         }
         
         // get the remaining bytes
         if(remainder)
         {
            if(bytes.length > (valueStart + length))
            {
               remainder.bytes = bytes.splice(valueStart + length);
            }
            else
            {
               remainder.bytes = [];
            }
         }
         
         // constructed is bit 6 (32 = 0x20)
         var constructed = bytes[0] & 0x20;
         if(constructed)
         {
            // parse child asn1 objects
            var tmp = value;
            value = [];
            var remainder = {};
            do
            {
               value.push(crypto.asn1.fromDer(tmp, remainder));
               tmp = remainder.bytes;
            }
            while(tmp.length > 0);
         }
         
         // create and return asn1 object
         return crypto.asn1.create(type, constructed, value);
      },
      
      /**
       * Converts the given asn1 object to an array of bytes in DER format.
       * 
       * @param asn1 the asn1 object to convert to bytes.
       * 
       * @return the array of bytes.
       */
      toDer: function(asn1)
      {
         var bytes = [];
         
         // build the tag
         bytes[0] = asn1.type & 0xFF;
         
         // for storing the ASN.1 value
         var value;
         
         // if constructed, use each child asn1 object's DER bytes as value
         if(asn1.constructed)
         {
            // turn on 6th bit (0x20 = 32) to indicate asn1 is constructed
            // from other asn1 objects
            bytes[0] |= 0x20;
            
            // concatenate all child DER bytes together
            value = [];
            for(var i = 0; i < asn1.value.length; i++)
            {
               value = value.concat(crypto.asn1.toDer(asn1.value[i]));
            }
         }
         // use asn1.value directly
         else
         {
            value = asn1.value;
         }
         
         // use "short form" encoding
         if(value.length <= 127)
         {
            // one byte describes the length
            // bit 8 = 0 and bits 7-1 = length
            bytes[1] = value.length & 0x7F;
         }
         // use "long form" encoding
         else
         {
            // 2 to 127 bytes describe the length
            // first byte: bit 8 = 1 and bits 7-1 = # of additional bytes
            // other bytes: length in base 256, big-endian
            var len = value.length;
            var lenBytes = [];
            do
            {
               lenBytes.push(value & 0xFF);
               len = len >> 8;
            }
            while(len > 0);
            lenBytes.reverse();
            
            // set first byte to # of additional bytes and turn on bit 8
            // concatenate length bytes
            bytes[1] = (len.length & 0xFF) | 0x80;
            bytes = bytes.concat(lenBytes);
         }
         
         // concatenate value bytes
         bytes = bytes.concat(value);
         
         return bytes;
      }
   };
})();

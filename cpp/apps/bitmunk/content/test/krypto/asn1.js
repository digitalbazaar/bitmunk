/**
 * API for Abstract Syntax Notation Number One
 *
 * @author Dave Longley
 *
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 * 
 * An API for storing data using the Abstract Syntax Notation Number One
 * format using DER (Distinguished Encoding Rules) encoding. This encoding is
 * commonly used to store data for PKI, i.e. X.509 Certificates, and this
 * implementation exists for that purpose.
 *
 * Abstract Syntax Notation Number One (ASN.1) is used to define the abstract
 * syntax of information without restricting the way the information is encoded
 * for transmission. It provides a standard that allows for open systems
 * communication. ASN.1 defines the syntax of information data and a number of
 * simple data types as well as a notation for describing them and specifying
 * values for them.
 *
 * The RSA algorithm creates public and private keys that are often stored in
 * X.509 or PKCS#X formats -- which use ASN.1 (encoded in DER format). This
 * class provides the most basic functionality required to store and load DSA
 * keys that are encoded according to ASN.1. 
 *
 * The most common binary encodings for ASN.1 are BER (Basic Encoding Rules)
 * and DER (Distinguished Encoding Rules). DER is just a subset of BER that
 * has stricter requirements for how data must be encoded.
 *
 * Each ASN.1 structure has a tag (a byte identifying the ASN.1 structure type)
 * and a byte array for the value of this ASN1 structure which may be data or a
 * list of ASN.1 structures.
 *
 * Each ASN.1 structure using BER is (Tag-Length-Value):
 * 
 * | byte 0 | bytes X | bytes Y |
 * |--------|---------|----------
 * |  tag   | length  |  value  |
 *
 * ASN.1 allows for tags to be of "High-tag-number form" which allows a tag to
 * be two or more octets, but that is not supported by this class. A tag is
 * only 1 byte. Bits 1-5 give the tag number (ie the data type within a
 * particular 'class'), 6 indicates whether or not the ASN.1 value is
 * constructed from other ASN.1 values, and bits 7 and 8 give the 'class'. This
 * this particular implementation only works with a 'class' of UNIVERSAL. The
 * UNIVERSAL class has bits 7 and 8 set to 0. The tag numbers for the data
 * types for the class UNIVERSAL are listed below:
 *
 * UNIVERSAL 0 Reserved for use by the encoding rules
 * UNIVERSAL 1 Boolean type
 * UNIVERSAL 2 Integer type
 * UNIVERSAL 3 Bitstring type
 * UNIVERSAL 4 Octetstring type
 * UNIVERSAL 5 Null type
 * UNIVERSAL 6 Object identifier type
 * UNIVERSAL 7 Object descriptor type
 * UNIVERSAL 8 External type and Instance-of type
 * UNIVERSAL 9 Real type
 * UNIVERSAL 10 Enumerated type
 * UNIVERSAL 11 Embedded-pdv type
 * UNIVERSAL 12 UTF8String type
 * UNIVERSAL 13 Relative object identifier type
 * UNIVERSAL 14-15 Reserved for future editions
 * UNIVERSAL 16 Sequence and Sequence-of types
 * UNIVERSAL 17 Set and Set-of types
 * UNIVERSAL 18-22, 25-30 Character string types
 * UNIVERSAL 23-24 Time types
 *
 * The length of an ASN.1 structure is specified after the tag identifier. The
 * length may take up 1 or more bytes, it depends on the length of the value of
 * the ASN.1 structure. DER encoding requires that if the ASN.1 structure has a
 * value that has a length greater than 127, more than 1 byte will be used to
 * store its length, otherwise just one byte will be used. This is strict.
 *
 * In the case that the length of the ASN.1 value is less than 127, 1 octet
 * (byte) is used to store the "short form" length. The 8th bit has a value of
 * 0 indicating the length is "short form" and not "long form" and bits 7-1
 * give the length of the data. (The 8th bit is the left-most, most significant
 * bit: also known as big endian or network format).
 *
 * In the case that the length of the ASN.1 value is greater than 127, 2 to
 * 127 octets (bytes) are used to store the "long form" length. The first
 * byte's 8th bit is set to 1 to indicate the length is "long form." Bits 7-1
 * give the number of additional octets. All following octets are in base 256
 * with the most significant digit first. So, for instance, if the length of a
 * value was 257, the first byte would be set to:
 *
 * 10000010 = 130 = 0x82.
 *
 * This indicates there are 2 octets (base 256) for the length. The second and
 * third bytes (the octets just mentioned) would store the length in base 256:
 *
 * octet 2: 00000001 = 1 * 256^1 = 256
 * octet 3: 00000001 = 1 * 256^0 = 1
 * total = 257
 *
 * The algorithm for converting a js integer value of 257 to base-256 is:
 *
 * var value = 257;
 * var bytes = [];
 * bytes[0] = (value >> 8) & 0xFF; // most significant byte first
 * bytes[1] = value & 0xFF;        // least significant byte last
 *
 * or in the general case (naive implementation):
 * 
 * var value = x;  // some integer x
 * var bytes = [];
 * do
 * {
 *    bytes.push(value & 0xFF);
 *    value = value >> 8;
 * }
 * while(value > 0);
 * bytes.reverse();  // swap from little-endian to big-endian
 * 
 * On the ASN.1 UNIVERSAL Object Identifier (OID) type:
 * 
 * An OID can be written like: "value1.value2.value3...valuen"
 * 
 * The DER encoding rules:
 * 
 * The first byte has the value 40 * value1 + value2. 
 * The following bytes, if any, encode the remaining values. Each value is
 * encoded in base 128, most significant digit first (big endian), with as
 * few digits as possible, and the most significant bit of each byte set
 * to 1 except the last in each value's encoding. For example: Given the
 * OID "1.2.840.113549", its DER encoding is (remember each byte except the
 * last one in each encoding is OR'd with 0x80):
 * 
 * byte 1: 40 * 1 + 2 = 42 = 0x2A.
 * bytes 2-3: 128 * 6 + 72 = 840 = 6 72 = 6 72 = 0x0648 = 0x8648
 * bytes 4-6: 16384 * 6 + 128 * 119 + 13 = 6 119 13 = 0x06770D = 0x86F70D
 * 
 * The final value is: 0x2A864886F70D.
 * The full OID (including ASN.1 tag and length of 6 bytes) is:
 * 0x06062A864886F70D
 */
(function()
{
   crypto = crypto || {};
   crypto.asn1 =
   {
      /**
       * ASN.1 types. Not all types are supported by this implementation, only
       * those necessary to implement a simple PKI are implemented.
       */
      TYPE_NONE:        0,
      TYPE_BOOLEAN:     1,
      TYPE_INTEGER:     2,
      TYPE_BITSTRING:   3,
      TYPE_OCTETSTRING: 4,
      TYPE_NULL:        5,
      TYPE_OID:         6,
      TYPE_SEQUENCE:    16,
      TYPE_SET:         17,
  
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
      },
      
      /**
       * Converts an OID dot-separated string to a byte array. The byte array
       * contains only the DER-encoded value, not any tag or length bytes.
       * 
       * @param oid the OID dot-separated string.
       * 
       * @return the byte array.
       */
      oidToDer: function(oid)
      {
         // split OID into individual values
         var values = oid.split('.');
         var bytes = [];
         
         // first byte is 40 * value1 + value2
         bytes[0] = 40 * oarseInt(values[0]) + parseInt(values[1]);
         // other bytes are each value in base 128 with 8th bit set except for
         // the last byte for each value
         for(var i = 2; i < values.length; i++)
         {
            // we reverse the value bytes so last is true to begin with
            var last = true;
            var valueBytes = [];
            var value = parseInt(values[i]);
            do
            {
               var byte = value & 0x7F;
               value = value >> 7;
               if(!last)
               {
                  byte |= 0x80;
               }
               valueBytes.push(byte);
               last = false;
            }
            while(value > 0);
            valueBytes.reverse();
            bytes = bytes.concat(valueBytes);
         }
         
         return bytes;
      }
   };
})();

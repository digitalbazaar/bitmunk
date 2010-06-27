/**
 * Javscript implementation of Abstract Syntax Notation Number One.
 *
 * @author Dave Longley
 *
 * Copyright (c) 2010 Digital Bazaar, Inc. All rights reserved.
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
 * constructed from other ASN.1 values, and bits 7 and 8 give the 'class'. If
 * bits 7 and 8 are both zero, the class is UNIVERSAL. If only bit 7 is set,
 * then the class is APPLICATION. If only bit 8 is set, then the class is
 * CONTEXT_SPECIFIC. If both bits 7 and 8 are set, then the class is PRIVATE.
 * The tag numbers for the data types for the class UNIVERSAL are listed below:
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
 * with the most significant digit first (typical big-endian binary unsigned
 * integer storage). So, for instance, if the length of a value was 257, the
 * first byte would be set to:
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
 * On the ASN.1 UNIVERSAL Object Identifier (OID) type:
 * 
 * An OID can be written like: "value1.value2.value3...valueN"
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
   // local alias for krypto stuff
   var krypto = window.krypto;
   
   /**
    * ASN.1 implementation.
    */
   var asn1 = {};
   
   /**
    * ASN.1 classes.
    */
   asn1.Class =
   {
      UNIVERSAL:        0,
      APPLICATION:      1,
      CONTEXT_SPECIFIC: 2,
      PRIVATE:          3
   };
   
   /**
    * ASN.1 types. Not all types are supported by this implementation, only
    * those necessary to implement a simple PKI are implemented.
    */
   asn1.Type =
   {
      NONE:         0,
      BOOLEAN:      1,
      BITSTRING:    2,
      INTEGER:      3,
      OCTETSTRING:  4,
      NULL:         5,
      OID:          6,
      SEQUENCE:    16,
      SET:         17
   };
   
   /**
    * Creates a new asn1 object.
    * 
    * @param tagClass the tag class for the object.
    * @param type the data type (tag number) for the object.
    * @param constructed true if the asn1 object is constructed from other
    *           asn1 objects, false if not.
    * @param value the value for the object, if it is not constructed.
    * 
    * @return the asn1 object.
    */
   asn1.create = function(tagClass, type, constructed, value)
   {
      /* An asn1 object has a tagClass, a type, a constructed flag, and a
         value. The value's type depends on the constructed flag. If
         constructed, it will contain a list of other asn1 objects. If not,
         it will contain the ASN.1 value as an array of bytes formatted
         according to the ASN.1 data type.
       */
      return {
         tagClass: tagClass,
         type: type,
         constructed: constructed,
         value: value
      };
   };
   
   /**
    * Parses an asn1 object from a byte buffer in DER format.
    * 
    * @param bytes the byte buffer to parse from.
    * 
    * @return the parsed asn1 object.
    */
   asn1.fromDer = function(bytes)
   {
      // minimum length for ASN.1 DER structure is 2
      if(bytes.length() < 2)
      {
         throw 'Invalid DER length: ' + bytes.length();
      }
      
      // get the first byte
      var b1 = bytes.getByte();
      
      // get the tag class
      var tagClass;
      switch(b1 & 0xC0)
      {
         case 0x00:
            tagClass = asn1.Class.UNIVERSAL;
            break;
         case 0x40:
            tagClass = asn1.Class.APPLICATION;
            break;
         case 0x80:
            tagClass = asn1.Class.CONTEXT_SPECIFIC;
            break;
         case 0xC0:
            tagClass = asn1.Class.PRIVATE;
            break;
      }
      
      // get the type (bits 1-5)
      var type = b1 & 0x1F;
      
      // see if the length is "short form" or "long form" (bit 8 set)
      var b2 = bytes.getByte();
      var longForm = b2 & 0x80;
      var length;
      if(!longForm)
      {
         // length is just the first byte
         length = b2;
      }
      else
      {
         // the number of bytes the length is stored in is specified in bits
         // 7 through 1 and each length byte is in big-endian base-256
         length = bytes.getInt((b2 & 0x7F) << 3);
      }
      
      // ensure there are enough bytes to get the value
      if(bytes.length() < length)
      {
         throw 'Not enough bytes to read ASN.1 value: ' +
            bytes.length() + ' < ' + length; 
      }
      
      // prepare to get value
      var value;
      
      // constructed flag is bit 6 (32 = 0x20) of the first byte
      var constructed = ((b1 & 0x20) == 0x20);
      if(constructed)
      {
         // parse child asn1 objects from the value
         value = [];
         var start = bytes.length();
         while(length > 0)
         {
            value.push(asn1.fromDer(bytes));
            length -= start - bytes.length();
            start = bytes.length();
         }
      }
      else
      {
         // asn1 not constructed, get raw value
         value = bytes.getBytes(length);
      }
      
      // create and return asn1 object
      return asn1.create(tagClass, type, constructed, value);
   };
   
   /**
    * Converts the given asn1 object to a buffer of bytes in DER format.
    * 
    * @param asn1 the asn1 object to convert to bytes.
    * 
    * @return the buffer of bytes.
    */
   asn1.toDer = function(asn1)
   {
      var bytes = krypto.util.createBuffer();
      
      // build the first byte
      var b1 = asn1.type;
      
      // for storing the ASN.1 value
      var value = krypto.util.createBuffer();
      
      // if constructed, use each child asn1 object's DER bytes as value
      if(asn1.constructed)
      {
         // turn on 6th bit (0x20 = 32) to indicate asn1 is constructed
         // from other asn1 objects
         b1 |= 0x20;
         
         // add all of the child DER bytes together
         for(var i = 0; i < asn1.value.length; ++i)
         {
            value.putBuffer(asn1.toDer(asn1.value[i]));
         }
      }
      // use asn1.value directly
      else
      {
         value.putBytes(asn1.value);
      }
      
      // add tag byte
      bytes.putByte(b1);
      
      // use "short form" encoding
      if(value.length <= 127)
      {
         // one byte describes the length
         // bit 8 = 0 and bits 7-1 = length
         bytes.putByte(value.length & 0x7F);
      }
      // use "long form" encoding
      else
      {
         // 2 to 127 bytes describe the length
         // first byte: bit 8 = 1 and bits 7-1 = # of additional bytes
         // other bytes: length in base 256, big-endian
         var len = value.length;
         var lenBytes = '';
         do
         {
            lenBytes += String.fromCharCode(len & 0xFF);
            len = len >> 8;
         }
         while(len > 0);
         
         // set first byte to # of additional bytes and turn on bit 8
         // concatenate length bytes in reverse since they were generated
         // little endian and we need big endian
         bytes.putByte(lenBytes.length | 0x80);
         for(var i = lenBytes.length - 1; i >= 0; --i)
         {
            bytes.putByte(lenBytes[i]);
         }
      }
      
      // concatenate value bytes
      bytes.putBuffer(value);
      return bytes;
   };
   
   /**
    * Converts an OID dot-separated string to a byte buffer. The byte buffer
    * contains only the DER-encoded value, not any tag or length bytes.
    * 
    * @param oid the OID dot-separated string.
    * 
    * @return the byte buffer.
    */
   asn1.oidToDer = function(oid)
   {
      // split OID into individual values
      var values = oid.split('.');
      var bytes = krypto.util.createBuffer();
      
      // first byte is 40 * value1 + value2
      bytes.putByte(40 * parseInt(values[0]) + parseInt(values[1]));
      // other bytes are each value in base 128 with 8th bit set except for
      // the last byte for each value
      var last, valueBytes, value, byte;
      for(var i = 2; i < values.length; ++i)
      {
         // produce value bytes in reverse because we don't know how many
         // bytes it will take to store the value
         last = true;
         valueBytes = [];
         value = parseInt(values[i]);
         do
         {
            byte = value & 0x7F;
            value = value >> 7;
            // if value is not last, then turn on 8th bit
            if(!last)
            {
               byte |= 0x80;
            }
            valueBytes.push(byte);
            last = false;
         }
         while(value > 0);
         
         // add value bytes in reverse (needs to be in big endian)
         for(var n = valueBytes.length - 1; n >= 0; --n)
         {
            bytes.putByte(valueBytes[n]);
         }
      }
      
      return bytes;
   };
   
   /**
    * Converts a DER-encoded byte buffer to an OID dot-separated string. The
    * byte buffer should contain only the DER-encoded value, not any tag or
    * length bytes.
    * 
    * @param bytes the byte buffer.
    * 
    * @return the OID dot-separated string.
    */
   asn1.derToOid = function(bytes)
   {
      var oid;
      
      // first byte is 40 * value1 + value2
      var b = bytes.getByte();
      oid = Math.floor(b / 40) + '.' + (b % 40);
      
      // other bytes are each value in base 128 with 8th bit set except for
      // the last byte for each value
      var value = 0;
      while(bytes.length() > 0)
      {
         b = bytes.getByte();
         value = value << 7;
         // not the last byte for the value
         if(b & 0x80)
         {
            value += b & 0x7F;
         }
         // last byte
         else
         {
            oid += '.' + (value + b);
            value = 0;
         }
      }
      
      return oid;
   };
   
   /**
    * The asn.1 API.
    */
   window.krypto.asn1 = asn1;
})();

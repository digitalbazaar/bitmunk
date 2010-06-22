/**
 * Crypto utilities.
 * 
 * @author Dave Longley
 *
 * Copyright (c) 2010 Digital Bazaar, Inc. All rights reserved.
 */
(function()
{
   /**
    * The util namespace.
    */
   var util = {};
   
   /**
    * Creates a buffer that stores bytes.
    * 
    * @param b the bytes to wrap (as a string) (optional).
    */
   util.createBuffer = function(b)
   {
      var buf =
      {
         // the data in this buffer.
         data: b || new String(),
         // the pointer for reading from this buffer.
         read: 0
      };
      
      /**
       * Gets the number of bytes in this buffer.
       * 
       * @return the number of bytes in this buffer.
       */
      buf.length = function()
      {
         return buf.data.length - buf.read;
      };
      
      /**
       * Puts a byte in this buffer.
       * 
       * @param b the byte to put.
       */
      buf.putByte = function(b)
      {
         buf.data += String.fromCharCode(b);
      };
      
      /**
       * Puts bytes in this buffer.
       * 
       * @param bytes the bytes (as a string) to put.
       */
      buf.putBytes = function(bytes)
      {
         buf.data += bytes;
      };
      
      /**
       * Puts a 16-bit integer in this buffer in big-endian order.
       * 
       * @param i the 16-bit integer.
       */
      buf.putInt16 = function(i)
      {
         buf.data +=
            String.fromCharCode(i >> 8 & 0xFF) +
            String.fromCharCode(i & 0xFF);
      };
      
      /**
       * Puts a 24-bit integer in this buffer in big-endian order.
       * 
       * @param i the 24-bit integer.
       */
      buf.putInt24 = function(i)
      {
         buf.data +=
            String.fromCharCode(i >> 16 & 0xFF) +
            String.fromCharCode(i >> 8 & 0xFF) +
            String.fromCharCode(i & 0xFF);
      };
      
      /**
       * Puts a 32-bit integer in this buffer in big-endian order.
       * 
       * @param i the 32-bit integer.
       */
      buf.putInt32 = function(i)
      {
         buf.data +=
            String.fromCharCode(i >> 24 & 0xFF) +
            String.fromCharCode(i >> 16 & 0xFF) +
            String.fromCharCode(i >> 8 & 0xFF) +
            String.fromCharCode(i & 0xFF);
      };
      
      /**
       * Puts a 16-bit integer in this buffer in little-endian order.
       * 
       * @param i the 16-bit integer.
       */
      buf.putInt16Le = function(i)
      {
         buf.data +=
            String.fromCharCode(i & 0xFF) +
            String.fromCharCode(i >> 8 & 0xFF);
      };
      
      /**
       * Puts a 24-bit integer in this buffer in little-endian order.
       * 
       * @param i the 24-bit integer.
       */
      buf.putInt24Le = function(i)
      {
         buf.data +=
            String.fromCharCode(i & 0xFF) +
            String.fromCharCode(i >> 8 & 0xFF) +
            String.fromCharCode(i >> 16 & 0xFF);
      };
      
      /**
       * Puts a 32-bit integer in this buffer in little-endian order.
       * 
       * @param i the 32-bit integer.
       */
      buf.putInt32Le = function(i)
      {
         buf.data +=
            String.fromCharCode(i & 0xFF) +
            String.fromCharCode(i >> 8 & 0xFF) +
            String.fromCharCode(i >> 16 & 0xFF) +
            String.fromCharCode(i >> 24 & 0xFF);
      };
      
      /**
       * Puts an n-bit integer in this buffer in big-endian order.
       * 
       * @param i the n-bit integer.
       * @param n the number of bits in the integer.
       */
      buf.putInt = function(i, n)
      {
         do
         {
            n -= 8;
            buf.data += String.fromCharCode((i >> n) & 0xFF);
         }
         while(n > 0);
      };
      
      /**
       * Puts the given buffer into this buffer.
       * 
       * @param buffer the buffer to put into this one.
       */
      buf.putBuffer = function(buffer)
      {
         buf.data += buffer.getBytes();
      };
      
      /**
       * Gets a byte from this buffer and advances the read pointer by 1.
       * 
       * @return the byte.
       */
      buf.getByte = function()
      {
         return buf.data.charCodeAt(buf.read++);
      };
      
      /**
       * Gets a uint16 from this buffer in big-endian order and advances the
       * read pointer by 2.
       * 
       * @return the uint16.
       */
      buf.getInt16 = function()
      {
         return (
            buf.data.charCodeAt(buf.read++) << 8 ^
            buf.data.charCodeAt(buf.read++));
      };
      
      /**
       * Gets a uint24 from this buffer in big-endian order and advances the
       * read pointer by 3.
       * 
       * @return the uint24.
       */
      buf.getInt24 = function()
      {
         return (
            buf.data.charCodeAt(buf.read++) << 16 ^
            buf.data.charCodeAt(buf.read++) << 8 ^
            buf.data.charCodeAt(buf.read++));
      };
      
      /**
       * Gets a uint32 from this buffer in big-endian order and advances the
       * read pointer by 4.
       * 
       * @return the word.
       */
      buf.getInt32 = function()
      {
         return (
            buf.data.charCodeAt(buf.read++) << 24 ^
            buf.data.charCodeAt(buf.read++) << 16 ^
            buf.data.charCodeAt(buf.read++) << 8 ^
            buf.data.charCodeAt(buf.read++));
      };
      
      /**
       * Gets a uint16 from this buffer in little-endian order and advances the
       * read pointer by 2.
       * 
       * @return the uint16.
       */
      buf.getInt16Le = function()
      {
         return (
            buf.data.charCodeAt(buf.read++) ^
            buf.data.charCodeAt(buf.read++) << 8);
      };
      
      /**
       * Gets a uint24 from this buffer in little-endian order and advances the
       * read pointer by 3.
       * 
       * @return the uint24.
       */
      buf.getInt24Le = function()
      {
         return (
            buf.data.charCodeAt(buf.read++) ^
            buf.data.charCodeAt(buf.read++) << 8 ^
            buf.data.charCodeAt(buf.read++) << 16);
      };
      
      /**
       * Gets a uint32 from this buffer in little-endian order and advances the
       * read pointer by 4.
       * 
       * @return the word.
       */
      buf.getInt32Le = function()
      {
         return (
            buf.data.charCodeAt(buf.read++) ^
            buf.data.charCodeAt(buf.read++) << 8 ^
            buf.data.charCodeAt(buf.read++) << 16 ^
            buf.data.charCodeAt(buf.read++) << 24);
      };
      
      /**
       * Reads bytes out into a string and clears them from the buffer.
       * 
       * @param count the number of bytes to read, undefined, null or 0
       *           for all.
       * 
       * @return a string of bytes.
       */
      buf.getBytes = function(count)
      {
         var rval;
         if(count)
         {
            // read count bytes
            count = Math.min(buf.length(), count);
            rval = buf.data.slice(buf.read, buf.read + count);
            buf.read += count;
         }
         else if(count === 0)
         {
            rval = '';
         }
         else
         {
            // read all bytes, optimize to only copy when needed
            rval = (buf.read === 0) ? buf.data : buf.data.slice(buf.read);
            buf.clear();
         }
         return rval;
      };
      
      /**
       * Gets a string of all the bytes without modifying the read
       * pointer.
       * 
       * @return an array of bytes.
       */
      buf.bytes = function()
      {
         return buf.data.slice(buf.read);
      };
      
      /**
       * Gets a byte at the given index without modifying the read
       * pointer.
       * 
       * @param i the byte index.
       * 
       * @return the byte.
       */
      buf.at = function(i)
      {
         return buf.data.charCodeAt(buf.read + i);
      };
      
      /**
       * Gets the last byte without modifying the read pointer.
       * 
       * @return the last byte.
       */
      buf.last = function()
      {
         return buf.data.charCodeAt(buf.data.length - 1);
      };
      
      /**
       * Creates a copy of this buffer.
       * 
       * @return the copy.
       */
      buf.copy = function()
      {
         var c = window.krypto.util.createBuffer(buf.data);
         c.read = buf.read;
         return c;
      };
      
      /**
       * Compacts this buffer.
       */
      buf.compact = function()
      {
         if(buf.read > 0)
         {
            buf.data = buf.data.slice(buf.read);
            buf.read = 0;
         }
      };
      
      /**
       * Clears this buffer.
       */
      buf.clear = function()
      {
         buf.data = new String();
         buf.read = 0;
      };
      
      /**
       * Shortens this buffer by triming bytes off of the end of this
       * buffer.
       * 
       * @param count the number of bytes to trim off.
       */
      buf.truncate = function(count)
      {
         var len = Math.max(0, buf.length() - count);
         buf.data = buf.data.substr(buf.read, len);
         buf.read = 0;
      };
      
      /**
       * Converts this buffer to a hexadecimal string.
       * 
       * @return a hexadecimal string.
       */
      buf.toHex = function()
      {
         var rval = '';
         var len = buf.length();
         for(var i = buf.read; i < len; i++)
         {
            var byte = buf.data.charCodeAt(i);
            if(byte < 16)
            {
               rval += '0';
            }
            rval += byte.toString(16);
         }
         return rval;
      };
      
      return buf;
   };
   
   // base64 characters, reverse mapping
   var _base64 =
      'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=';
   var _base64Idx = [
   /* 43 -43 = 0*/
   /* '+',  1,  2,  3,'/' */
       62, -1, -1, -1, 63,

   /* '0','1','2','3','4','5','6','7','8','9' */
       52, 53, 54, 55, 56, 57, 58, 59, 60, 61,

   /* 15, 16, 17,'=', 19, 20, 21 */
      -1, -1, -1, 64, -1, -1, -1,

   /* 65 - 43 = 22*/
   /*'A','B','C','D','E','F','G','H','I','J','K','L','M', */
       0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12,

   /* 'N','O','P','Q','R','S','T','U','V','W','X','Y','Z' */
       13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,

   /* 91 - 43 = 48 */
   /* 48, 49, 50, 51, 52, 53 */
      -1, -1, -1, -1, -1, -1,

   /* 97 - 43 = 54*/
   /* 'a','b','c','d','e','f','g','h','i','j','k','l','m' */
       26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38,

   /* 'n','o','p','q','r','s','t','u','v','w','x','y','z' */
       39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51
   ];
   
   /**
    * Base64 encodes a string of bytes.
    * 
    * @param input the string of bytes to encode.
    * 
    * @return the base64-encoded output.
    */
   util.encode64 = function(input)
   {
      var output = new String();
      var chr1, chr2, chr3;
      var i = 0;
      while(i < input.length)
      {
         chr1 = input.charCodeAt(i++);
         chr2 = input.charCodeAt(i++);
         chr3 = input.charCodeAt(i++);
         
         // encode 4 character group
         output += _base64.charAt(chr1 >> 2);
         output += _base64.charAt(((chr1 & 3) << 4) | (chr2 >> 4));
         if(isNaN(chr2))
         {
            output += '==';
         }
         else
         {
            output += _base64.charAt(((chr2 & 15) << 2) | (chr3 >> 6));
            output += isNaN(chr3) ? '=' : _base64.charAt(chr3 & 63);
         }
      }
      
      return output;      
   };
   
   /**
    * Base64 decodes a string into a string of bytes.
    * 
    * @param input the base64-encoded input.
    * 
    * @return the raw bytes.
    */
   util.decode64 = function(input)
   {
      // remove all non-base64 characters
      input = input.replace(/[^A-Za-z0-9\+\/\=]/g, '');
      
      var output = new String();
      var enc1, enc2, enc3, enc4;
      var i = 0;
      
      while(i < input.length)
      {
         enc1 = _base64Idx[input.charCodeAt(i++) - 43];
         enc2 = _base64Idx[input.charCodeAt(i++) - 43];
         enc3 = _base64Idx[input.charCodeAt(i++) - 43];
         enc4 = _base64Idx[input.charCodeAt(i++) - 43];
         
         output += String.fromCharCode((enc1 << 2) | (enc2 >> 4));
         if(enc3 !== 64)
         {
            // decoded at least 2 bytes
            output += String.fromCharCode(((enc2 & 15) << 4) | (enc3 >> 2));
            if(enc4 !== 64)
            {
               // decoded 3 bytes
               output += String.fromCharCode(((enc3 & 3) << 6) | enc4);
            }
         }
      }
      
      return output;
   };
   
   /**
    * Deflates the given data using a flash interface.
    * 
    * @param api the flash interface.
    * @param bytes the data.
    * 
    * @return the deflated data as a string.
    */
   util.deflate = function(api, bytes)
   {
      return util.decode64(api.deflate(util.encode64(bytes)).rval);
   };
   
   /**
    * Inflates the given data using a flash interface.
    * 
    * @param api the flash interface.
    * @param bytes the data.
    * 
    * @return the inflated data as a string, null on error.
    */
   util.inflate = function(api, bytes)
   {
      var rval = api.inflate(util.encode64(bytes)).rval;
      return (rval === null) ? null : util.decode64(rval);
   };
   
   /**
    * The crypto namespace and util API.
    */
   window.krypto = window.krypto || {};
   window.krypto.util = util;
})();

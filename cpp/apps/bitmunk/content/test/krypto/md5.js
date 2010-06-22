/**
 * Message Digest Algorithm 5 with 128-bit digest (MD5) implementation.
 * 
 * This implementation is currently limited to message lengths (in bytes) that
 * are up to 32-bits in size.
 * 
 * @author Dave Longley
 *
 * Copyright (c) 2010 Digital Bazaar, Inc. All rights reserved.
 */
(function()
{
   // local alias for krypto stuff
   var krypto = window.krypto;
   
   // padding, constant tables for calculating md5
   var _padding = null;
   var _r = null;
   var _k = null;
   var _initialized = false;
   
   /**
    * Initializes the constant tables.
    */
   var _init = function()
   {
      // create padding
      _padding = String.fromCharCode(128);
      for(var i = 0; i < 64; i++)
      {
         _padding += String.fromCharCode(0);
      }
      
      // rounds table
      _r = [
         7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,
         5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,
         4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,
         6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21
      ];
      
      // get the result of abs(sin(i + 1)) as a 32-bit integer
      _k = new Array(64);
      for(var i = 0; i < 64; ++i)
      {
         _k[i] = Math.floor(Math.abs(Math.sin(i + 1)) * 0x100000000);
      }
      
      // now initialized
      _initialized = true;
   };
   
   /**
    * Updates an MD5 state with the given byte buffer.
    * 
    * @param s the MD5 state to update.
    * @param w the array to use to store words.
    * @param bytes the byte buffer to update with.
    */
   var _update = function(s, w, bytes)
   {
      // consume 512 bit (64 byte) chunks
      var t1, t2, a, b, c, d, f, g, r;
      while(bytes.length() >= 64)
      {
         // get sixteen 32-bit little-endian words
         for(var i = 0; i < 16; ++i)
         {
            w[i] = bytes.getInt32Le();
         }
         
         // initialize hash value for this chunk
         a = s.h0;
         b = s.h1;
         c = s.h2;
         d = s.h3;
         
         // do rounds loops
         var i = 0;
         for(; i < 16; ++i)
         {
            f = d ^ (b & (c ^ d));
            g = i;
            t1 = d;
            d = c;
            c = b;
            t2 = (a + f + _k[i] + w[g]);
            r = _r[i];
            b += (t2 << r) | (t2 >>> (32 - r));
            a = t1;
         }
         for(; i < 32; ++i)
         {
            f = c ^ (d & (b ^ c));
            g = ((i << 2) + i + 1) % 16;
            t1 = d;
            d = c;
            c = b;
            t2 = (a + f + _k[i] + w[g]);
            r = _r[i];
            b += (t2 << r) | (t2 >>> (32 - r));
            a = t1;
         }
         for(; i < 48; ++i)
         {
            f = b ^ c ^ d;
            g = ((i << 1) + i + 5) % 16;
            t1 = d;
            d = c;
            c = b;
            t2 = (a + f + _k[i] + w[g]);
            r = _r[i];
            b += (t2 << r) | (t2 >>> (32 - r));
            a = t1;
         }
         for(; i < 64; ++i)
         {
            f = c ^ (b | ~d);
            g = ((i << 2) + (i << 1) + i) % 16;
            t1 = d;
            d = c;
            c = b;
            t2 = (a + f + _k[i] + w[g]);
            r = _r[i];
            b += (t2 << r) | (t2 >>> (32 - r));
            a = t1;
         }
         
         // update hash state
         s.h0 += a;
         s.h1 += b;
         s.h2 += c;
         s.h3 += d;
      }
   };
   
   // the md5 interface
   var md5 = {};
   
   /**
    * Creates an MD5 message digest object.
    * 
    * @return a message digest object.
    */
   md5.create = function()
   {
      // do initialization as necessary
      if(!_initialized)
      {
         _init();
      }
      
      // MD5 state contains four 32-bit integers
      var _state = null;
      
      // input buffer
      var _input = krypto.util.createBuffer();
      
      // length of message so far (does not including padding)
      var _length = 0;
      
      // used for word storage
      var _w = new Array(16);
      
      // message digest object
      var md = {};
      
      /**
       * Resets the digest.
       */
      md.reset = function()
      {
         _length = 0;
         _input = krypto.util.createBuffer();
         _state =
         {
            h0: 0x67452301,
            h1: 0xEFCDAB89,
            h2: 0x98BADCFE,
            h3: 0x10325476
         };
      };
      md.reset();
      
      /**
       * Updates the digest with the given bytes.
       * 
       * @param bytes the bytes to update with.
       */
      md.update = function(bytes)
      {
         // update message length
         _length += bytes.length;
         
         // add bytes to input buffer
         _input.putBytes(bytes);
         
         // process bytes
         _update(_state, _w, _input);
         
         // compact input buffer every 2K or if empty
         if(_input.read > 2048 || _input.length() === 0)
         {
            _input.compact();
         }
      };
      
      /**
       * Produces the digest.
       * 
       * @return a byte buffer containing the digest value.
       */
      md.digest = function()
      {
         /* Note: Here we copy the remaining bytes in the input buffer and
            add the appropriate MD5 padding. Then we do the final update
            on a copy of the state so that if the user wants to get
            intermediate digests they can do so.
          */
         
         /* Determine the number of bytes that must be added to the message
            to ensure its length is congruent to 448 mod 512. In other words,
            a 64-bit integer that gives the length of the message will be
            appended to the message and whatever the length of the message is
            plus 64 bits must be a multiple of 512. So the length of the
            message must be congruent to 448 mod 512 because 512 - 64 = 448.
            
            In order to fill up the message length it must be filled with
            padding that begins with 1 bit followed by all 0 bits. Padding
            must *always* be present, so if the message length is already
            congruent to 448 mod 512, then 512 padding bits must be added.
          */
         
         // 512 bits == 64 bytes, 448 bits == 56 bytes, 64 bits = 8 bytes
         // _padding starts with 1 byte with first bit is set in it which
         // is byte value 128, then there may be up to 63 other pad bytes
         var padBytes = krypto.util.createBuffer();
         padBytes.putBytes(_input.bytes());
         padBytes.putBytes(_padding.substr(0, 64 - ((_length + 8) % 64)));
         
         /* Now append length of the message. The length is appended in bits
            as a 64-bit number in little-endian format. Since we store the
            length in bytes, we must multiply it by 8 (or left shift by 3). So
            here store the high 3 bits in the high end of the second 32-bits of
            the 64-bit number and the lower 5 bits in the low end of the
            second 32-bits.
          */
         padBytes.putInt32Le((_length << 3) & 0xFFFFFFFF);
         padBytes.putInt32Le((_length >>> 29) & 0xFF);
         var s2 =
         {
            h0: _state.h0,
            h1: _state.h1,
            h2: _state.h2,
            h3: _state.h3
         };
         _update(s2, _w, padBytes);
         var rval = krypto.util.createBuffer();
         rval.putInt32Le(s2.h0);
         rval.putInt32Le(s2.h1);
         rval.putInt32Le(s2.h2);
         rval.putInt32Le(s2.h3);
         return rval;
      };
      
      return md;
   };
   
   // expose md5 interface in krypto library
   krypto.md = krypto.md || {};
   krypto.md.md5 = md5;
})();

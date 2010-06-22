/**
 * Secure Hash Algorithm with 160-bit digest (SHA-1) implementation.
 * 
 * This implementation is currently limited to message lengths that are
 * up to 32-bits in size.
 * 
 * @author Dave Longley
 *
 * Copyright (c) 2010 Digital Bazaar, Inc. All rights reserved.
 */
(function()
{
   // local alias for krypto stuff
   var krypto = window.krypto;
   
   /**
    * Updates a SHA-1 state with the given byte buffer.
    * 
    * @param state the SHA-1 state to update.
    * @param bytes the byte buffer to update with.
    */
   var _update = function(state, bytes)
   {
      // consume 512 bit (64 byte) chunks
      var w = new Array(16);
      var tmp, a, b, c, d, e, f;
      while(bytes.length() >= 64)
      {
         // get sixteen 32-bit big-endian words
         for(var i = 0; i < 16; ++i)
         {
            w[i] = bytes.getInt32();
         }
         
         // extend 16 words into 80 32-bit words according to SHA-1 algorithm
         // and for 32-79 use Max Locktyukhin's optimization
         for(var i = 16; i < 32; ++i)
         {
            tmp = (w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16]);
            w[i] = (tmp << 1) | (tmp >>> 31);
         }
         for(var i = 32; i < 80; ++i)
         {
            tmp = (w[i - 6] ^ w[i - 16] ^ w[i - 28] ^ w[i - 32]);
            w[i] = (tmp << 2) | (tmp >>> 30);
         }
         
         // initialize hash value for this chunk
         a = state.h0;
         b = state.h1;
         c = state.h2;
         d = state.h3;
         e = state.h4;
         
         // do rounds loops
         var i = 0;
         for(; i < 20; ++i)
         {
            f = d ^ (b & (c ^ d));
            tmp = ((a << 5) | (a >>> 27)) + f + e + 0x5A827999 + w[i];
            e = d;
            d = c;
            c = (b << 30) | (b >>> 2);
            b = a;
            a = tmp & 0xFFFFFFFF;
         }
         for(; i < 40; ++i)
         {
            f = b ^ c ^ d;
            tmp = ((a << 5) | (a >>> 27)) + f + e + 0x6ED9EBA1 + w[i];
            e = d;
            d = c;
            c = (b << 30) | (b >>> 2);
            b = a;
            a = tmp & 0xFFFFFFFF;
         }
         for(; i < 60; ++i)
         {
            f = (b & c) | (d & (b ^ c));
            tmp = ((a << 5) | (a >>> 27)) + f + e + 0x8F1BBCDC + w[i];
            e = d;
            d = c;
            c = (b << 30) | (b >>> 2);
            b = a;
            a = tmp & 0xFFFFFFFF;
         }
         for(; i < 80; ++i)
         {
            f = b ^ c ^ d;
            tmp = ((a << 5) | (a >>> 27)) + f + e + 0xCA62C1D6 + w[i];
            e = d;
            d = c;
            c = (b << 30) | (b >>> 2);
            b = a;
            a = tmp & 0xFFFFFFFF;
         }
         
         // update hash state
         state.h0 = (state.h0 + a) & 0xFFFFFFFF;
         state.h1 = (state.h1 + b) & 0xFFFFFFFF;
         state.h2 = (state.h2 + c) & 0xFFFFFFFF;
         state.h3 = (state.h3 + d) & 0xFFFFFFFF;
         state.h4 = (state.h4 + e) & 0xFFFFFFFF;
      }
   };
   
   // the sha1 interface
   var sha1 = {};
   
   /**
    * Creates a SHA-1 message digest object.
    * 
    * @return a message digest object.
    */
   sha1.create = function()
   {
      // SHA-1 state contains five 32-bit integers
      var _state = null;
      
      // input buffer
      var _input = krypto.util.createBuffer();
      
      // length of message so far (does not including padding)
      var _length = 0;
      
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
            h3: 0x10325476,
            h4: 0xC3D2E1F0
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
         _update(_state, _input);
         
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
            add the appropriate SHA-1 padding. Then we do the final update
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
         // Always at least 1 byte of padding and first bit is set in it,
         // so that's a byte value of 128. Then there may be up to 63 other
         // pad bytes.
         var padBytes = krypto.util.createBuffer();
         padBytes.putBytes(_input.bytes());
         padBytes.putByte(128);
         var padding = 63 - ((_length + 8) % 64);
         for(; padding > 0; --padding)
         {
            padBytes.putByte(0x00);
         }
         
         /* Now append length of the message. The length is appended in bits
            as a 64-bit number. Since we store the length in bytes, we must
            multiply it by 8 (or left shift by 3). So here store the high 3
            bits in the low end of the first 32-bits of the 64-bit number and
            the lower 5 bits in the high end of the second 32-bits.
          */
         padBytes.putInt32((_length >>> 29) & 0xFF);
         padBytes.putInt32((_length << 3) & 0xFFFFFFFF);
         var s2 =
         {
            h0: _state.h0,
            h1: _state.h1,
            h2: _state.h2,
            h3: _state.h3,
            h4: _state.h4
         };
         _update(s2, padBytes);
         var rval = krypto.util.createBuffer();
         rval.putInt32(s2.h0);
         rval.putInt32(s2.h1);
         rval.putInt32(s2.h2);
         rval.putInt32(s2.h3);
         rval.putInt32(s2.h4);
         return rval;
      };
      
      return md;
   };
   
   // expose sha1 interface in krypto library
   krypto.sha1 = sha1;
})();

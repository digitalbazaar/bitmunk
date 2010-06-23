/**
 * Hash-based Message Authentication Code implementation. Requires a message
 * digest object that can be obtained, for example, from krypto.md.sha1 or
 * krypto.md.md5.
 * 
 * @author Dave Longley
 *
 * Copyright (c) 2010 Digital Bazaar, Inc. All rights reserved.
 */
(function()
{
   // local alias for krypto stuff
   var krypto = window.krypto;
   
   // the hmac interface
   var hmac = {};
   
   /**
    * Creates an HMAC object that uses the given message digest object.
    * 
    * @return an HMAC object.
    */
   hmac.create = function()
   {
      // the hmac key to use
      var _key = null;
      
      // the message digest to use
      var _md = null;
      
      // the inner padding
      var _ipadding = null;
      
      // the outer padding
      var _opadding = null;
      
      // hmac context
      var ctx = {};
      
      /**
       * Starts or restarts the HMAC with the given key and message digest.
       * 
       * @param key the key to use as a string, array of bytes, byte buffer,
       *           or null to reuse the previous key.
       * @param md the message digest to use, null to reuse the previous one.
       */
      ctx.start = function(key, md)
      {
         if(md === null)
         {
            // reuse previous message digest
            md = _md;
         }
         else
         {
            // store message digest
            _md = md;
         }
         
         if(key === null)
         {
            // reuse previous key
            key = _key;
         }
         else
         {
            // convert string into byte buffer
            if(key.constructor == String)
            {
               key = krypto.util.createBuffer(key);
            }
            // convert byte array into byte buffer
            else if(key.constructor == Array)
            {
               var tmp = key;
               key = krypto.util.createBuffer();
               for(var i = 0; i < tmp.length; ++i)
               {
                  key.putByte(tmp[i]);
               }
            }
            
            // if key is longer than blocksize, hash it
            var keylen = key.length();
            if(keylen > _md.blockLength)
            {
               console.log('key too long');
               _md.reset();
               _md.update(key.bytes());
               key = _md.digest();
            }
            // if key is shorter than blocksize, pad it with zeros
            else if(keylen < _md.blockLength)
            {
               console.log('key too short');
               var tmp = _md.blockLength - keylen; 
               for(var i = 0; i < tmp; ++i)
               {
                  key.putByte(0x00);
               }
            }
            _key = key;
            console.log('key', _key.toHex());
            
            // mix key into inner and outer padding
            // ipadding = [0x36 * blocksize] ^ key
            // opadding = [0x5C * blocksize] ^ key
            _ipadding = krypto.util.createBuffer();
            _opadding = krypto.util.createBuffer();
            for(var i = 0; i < _md.blockLength; ++i)
            {
               var tmp = key.at(i);
               _ipadding.putByte(0x36 ^ tmp);
               _opadding.putByte(0x5C ^ tmp);
            }
            console.log('_ipadding', _ipadding.toHex());
            console.log('_opadding', _opadding.toHex());
         }
         
         // digest is done like so: hash(opadding | hash(ipadding | message))
         
         // reset message digest and prepare to do inner hash
         // hash(ipadding | message)
         _md.reset();
         _md.update(_ipadding.bytes());
      };
      
      /**
       * Updates the HMAC with the given message bytes.
       * 
       * @param bytes the bytes to update with.
       */
      ctx.update = function(bytes)
      {
         _md.update(bytes);
      };
      
      /**
       * Produces the Message Authentication Code (MAC).
       * 
       * @return a byte buffer containing the digest value.
       */
      ctx.getMac = function()
      {
         // digest is done like so: hash(opadding | hash(ipadding | message))
         // here we do the outer hashing
         var inner = _md.digest();
         _md.reset();
         _md.update(_opadding);
         _md.update(inner.bytes());
         return _md.digest();
      };
      // alias for getMac
      ctx.digest = ctx.getMac;
      
      return ctx;
   };
   
   // expose hmac interface in krypto library
   krypto.hmac = hmac;
})();

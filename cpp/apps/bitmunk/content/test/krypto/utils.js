/**
 * Crypto utilities.
 * 
 * @author Dave Longley
 *
 * Copyright (c) 2010 Digital Bazaar, Inc. All rights reserved.
 */
(function($)
{
   /**
    * The crypto namespace and utils API.
    */
   window.krypto = window.krypto || {};
   window.krypto.utils =
   {
      /**
       * Creates a ByteArray that stores its bytes as big-endian 32-bit
       * integers.
       */
      createByteArray: function()
      {
         var ba =
         {
            /**
             * Stores the 32-bit integers that make up this array.
             */
            words: [],
            
            /**
             * The number of bytes in this array.
             */
            length: 0,
            
            /**
             * Pushes a byte.
             * 
             * @param b the byte to push.
             */
            pushByte: function(b)
            {
               ba.set(ba.length, b);
               ba.length++;
            },
            
            /**
             * Pushes a 32-bit integer.
             * 
             * @param w the 32-bit integer (word).
             */
            pushWord: function(w)
            {
               // simple case, length is on a word barrier
               if(ba.length % 4 == 0)
               {
                  ba.words.push(w);
                  ba.length += 4;
               }
               // complex case, must break the word up into bytes
               else
               {
                  for(var i = 3; i >= 0; i--)
                  {
                     ba.pushByte((w >> (i << 3)) & 0xFF);
                  }
               }
            },
            
            /**
             * Gets a byte from the array.
             * 
             * @param i the index to the byte.
             * 
             * @return the byte.
             */
            get: function(i)
            {
               return (ba.words[i >> 2] >> ((3 - (i % 4)) << 3)) & 0xFF;
            },
            
            /**
             * Sets a byte in the array.
             * 
             * @param i the index to the byte.
             * @param b the new byte to set. 
             */
            set: function(i, b)
            {
               ba.words[i >> 2] |= b << ((3 - (ba.length % 4)) << 3);
            },
            
            /**
             * Gets a javascript array with each element as a byte (0-255).
             * 
             * @return an array of bytes.
             */
            bytes: function()
            {
               var b = [];
               for(var i = 0; i < ba.length; i++)
               {
                  b.push(ba.get(i));
               }
               return b;
            },
            
            /**
             * Creates a copy of this array.
             * 
             * @return the copy.
             */
            copy: function()
            {
               return {
                  words: ba.words.slice(),
                  length: ba.length
               };
            }
         };
         return ba;
      }      
   };
})(jQuery);

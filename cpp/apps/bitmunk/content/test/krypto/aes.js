/**
 * Advanced Encryption Standard (AES) Cipher-Block Chaining implementation.
 * 
 * This implementation is based on the public domain library 'jscrypto' which
 * was written by:
 * 
 * Emily Stark (estark@stanford.edu)
 * Mike Hamburg (mhamburg@stanford.edu)
 * Dan Boneh (dabo@cs.stanford.edu)
 * 
 * Parts of this code are based on the OpenSSL implementation of AES:
 * http://www.openssl.org
 * 
 * @author Dave Longley
 *
 * Copyright (c) 2010 Digital Bazaar, Inc. All rights reserved.
 */
(function($)
{
   // FIXME: stuff here
   
   /**
    * The crypto namespace and aes API.
    */
   window.krypto = window.krypto || {};
   window.krypto.aes =
   {
      /**
       * Creates an AES cipher object that can encrypt or decrypt using
       * the given symmetric key.
       * 
       * @param key the symmetric key to use, as a byte array.
       * 
       * @return the cipher.
       */
      createCipher: function()
      {
         var cipher =
         {
            // FIXME:
         };
         return cipher;
      }
   };
})(jQuery);

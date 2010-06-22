/**
 * Pseudo-random number generator.
 * 
 * Getting strong random bytes is not yet easy to do in javascript. The only
 * truish random data that can be collected is from the mouse or from timing
 * with respect to page loads, etc. This generator makes a poor attempt at
 * providing slightly more random bytes (in a timely fashion and without
 * blocking to wait for user input) than would be provided from an purely
 * arithmetic-PRNG.
 * 
 * FIXME: Use Blum Blum Shub? What about new algorithm by Ribeiro Alvo?
 *
 * @author Dave Longley
 *
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 */
(function($)
{
   // state information for random byte generation
   var lt = +new Date();
   var state =
   {
      loadTime: lt,
      lastTime: lt,
      seed: lt + Math.random() * lt,
      pool: [(lt >> 8) & 0xFF, lt & 0xFF],
      poolSize: 1024
   };
   
   // set up mouse data capture
   $().mousemove(function(e)
   {
      if(state.pool.length < state.poolSize)
      {
         // add mouse coords
         state.pool.push(e.clientX & 0xFF);
         state.pool.push(e.clientY & 0xFF);
      }
   });
   
   /**
    * The crypto namespace and random API.
    */
   window.krypto = window.krypto || {};
   window.krypto.random = {};
   
   /**
    * Gets random bytes. This method tries to make the bytes more
    * unpredictable by drawing from data that can be collected from
    * the user of the browser, ie mouse movement.
    * 
    * @param count the number of random bytes to get.
    * 
    * @return the random bytes in a string.
    */
   window.krypto.random.getBytes = function(count)
   {
      var b = '';
      
      // consume random bytes from pool
      var pb = state.pool.splice(0, count);
      
      /* Draws from Park-Miller "minimal standard" 31 bit PRNG, implemented
      with David G. Carta's optimization: with 32 bit math and without
      division (Public Domain). */
      var hi, lo;
      while(b.length < count)
      {
         lo = 16807 * (state.seed & 0xFFFF);
         hi = 16807 * (state.seed >> 16);
         lo += (hi & 0x7FFF) << 16;
         lo += hi >> 15;
         lo = (lo & 0x7FFFFFFF) + (lo >> 31);
         state.seed = lo & 0xFFFFFFFF;
         
         // consume lower 3 bytes of seed
         for(var i = 0; i < 3 && b.length < count; i++)
         {
            // throw in random or pseudo-random byte (WEAK)
            var byte = state.seed >> (i << 3) & 0xFF;
            byte ^= (b.length < pb.length) ?
               pb[b.length] : Math.random() * 0xFF;
            b += String.fromCharCode(byte & 0xFF);
         }
      }
      
      return b;
   };
})(jQuery);

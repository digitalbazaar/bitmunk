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
       * Creates a buffer that stores bytes.
       * 
       * @param b the bytes to wrap (as a string) (optional).
       */
      createBuffer: function(b)
      {
         var buf =
         {
            /**
             * The data in this buffer.
             */
            data: b || new String(),
            
            /**
             * The pointer for reading from this buffer.
             */
            read: 0,
            
            /**
             * Gets the number of bytes in this buffer.
             * 
             * @return the number of bytes in this buffer.
             */
            length: function()
            {
               return buf.data.length - buf.read;
            },
            
            /**
             * Puts a byte in this buffer.
             * 
             * @param b the byte to put.
             */
            putByte: function(b)
            {
               buf.data += String.fromCharCode(b);
            },
            
            /**
             * Puts a 32-bit integer in this buffer.
             * 
             * @param w the 32-bit integer (word).
             */
            putInt32: function(w)
            {
               var str =
                  String.fromCharCode(w >> 24 & 0xFF) +
                  String.fromCharCode(w >> 16 & 0xFF) +
                  String.fromCharCode(w >> 8 & 0xFF) +
                  String.fromCharCode(w & 0xFF);
               buf.data += str;
            },
            
            /**
             * Puts the given buffer into this buffer.
             * 
             * @param buffer the buffer to put into this one.
             */
            putBuffer: function(buffer)
            {
               buf.data += buffer.getBytes();
            },
            
            /**
             * Gets a byte from this buffer and advances the read pointer by 1.
             * 
             * @return the byte.
             */
            getByte: function()
            {
               return buf.data.charCodeAt(buf.read++);
            },
            
            /**
             * Gets a word from this buffer and advances the read pointer by 4.
             * 
             * @return the word.
             */
            getInt32: function()
            {
               return (
                  buf.data.charCodeAt(buf.read++) << 24 ^
                  buf.data.charCodeAt(buf.read++) << 16 ^
                  buf.data.charCodeAt(buf.read++) << 8 ^
                  buf.data.charCodeAt(buf.read++));
            },
            
            /**
             * Reads all bytes out into a string and clears the buffer.
             * 
             * @return a string of bytes.
             */
            getBytes: function()
            {
               var rval = buf.data.slice(buf.read);
               buf.clear();
               return rval;
            },
            
            /**
             * Gets a string of all the bytes without modifying the read
             * pointer.
             * 
             * @return an array of bytes.
             */
            bytes: function()
            {
               return buf.data.slice(buf.read);
            },
            
            /**
             * Gets a byte at the given index without modifying the read
             * pointer.
             * 
             * @param i the byte index.
             * 
             * @return the byte.
             */
            at: function(i)
            {
               return buf.data.charCodeAt(buf.read + i);
            },
            
            /**
             * Creates a copy of this buffer.
             * 
             * @return the copy.
             */
            copy: function()
            {
               var c = window.krypto.utils.createBuffer(buf.data);
               c.read = buf.read;
               return c;
            },
            
            /**
             * Compacts this buffer.
             */
            compact: function()
            {
               if(buf.read > 0)
               {
                  buf.data = buf.data.slice(buf.read);
                  buf.read = 0;
               }
            },
            
            /**
             * Clears this buffer.
             */
            clear: function()
            {
               buf.data = new String();
               buf.read = 0;
            },
            
            /**
             * Shortens this buffer by triming bytes off of the end of this
             * buffer.
             * 
             * @param count the number of bytes to trim off.
             */
            truncate: function(count)
            {
               var len = Math.max(0, buf.length() - count);
               buf.data = buf.data.substr(buf.read, len);
               buf.read = 0;
            }
         };
         return buf;
      }
   };
})(jQuery);

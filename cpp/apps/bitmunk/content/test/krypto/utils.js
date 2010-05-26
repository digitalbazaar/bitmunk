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
             * Puts bytes in this buffer.
             * 
             * @param bytes the bytes (as a string) to put.
             */
            putBytes: function(bytes)
            {
               buf.data += bytes;
            },
            
            /**
             * Puts a 16-bit integer in this buffer.
             * 
             * @param i the 16-bit integer.
             */
            putInt16: function(i)
            {
               buf.data +=
                  String.fromCharCode(s >> 8 & 0xFF) +
                  String.fromCharCode(s & 0xFF);
            },
            
            /**
             * Puts a 24-bit integer in this buffer.
             * 
             * @param i the 24-bit integer.
             */
            putInt24: function(s)
            {
               buf.data +=
                  String.fromCharCode(s >> 16 & 0xFF) +
                  String.fromCharCode(s >> 8 & 0xFF) +
                  String.fromCharCode(s & 0xFF);
            },
            
            /**
             * Puts a 32-bit integer in this buffer.
             * 
             * @param i the 32-bit integer.
             */
            putInt32: function(i)
            {
               buf.data +=
                  String.fromCharCode(i >> 24 & 0xFF) +
                  String.fromCharCode(i >> 16 & 0xFF) +
                  String.fromCharCode(i >> 8 & 0xFF) +
                  String.fromCharCode(i & 0xFF);
            },
            
            /**
             * Puts an n-bit integer in this buffer.
             * 
             * @param i the n-bit integer.
             * @param n the number of bits in the integer.
             */
            putInt: function(i, n)
            {
               do
               {
                  n -= 8;
                  buf.data += String.fromCharCode((i >> n) & 0xFF);
               }
               while(bits > 0);
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
             * Gets a uint16 from this buffer and advances the read pointer
             * by 2.
             * 
             * @return the uint16.
             */
            getInt16: function()
            {
               return (
                  buf.data.charCodeAt(buf.read++) << 8 ^
                  buf.data.charCodeAt(buf.read++));
            },
            
            /**
             * Gets a uint24 from this buffer and advances the read pointer
             * by 3.
             * 
             * @return the uint24.
             */
            getInt24: function()
            {
               return (
                  buf.data.charCodeAt(buf.read++) << 16 ^
                  buf.data.charCodeAt(buf.read++) << 8 ^
                  buf.data.charCodeAt(buf.read++));
            },
            
            /**
             * Gets a uint32 from this buffer and advances the read pointer
             * by 4.
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
             * Reads bytes out into a string and clears them from the buffer.
             * 
             * @param count the number of bytes to read, undefined, null or 0
             *           for all.
             * 
             * @return a string of bytes.
             */
            getBytes: function(count)
            {
               var rval;
               if(count)
               {
                  // read count bytes
                  rval = buf.data.slice(buf.read, buf.read + count);
                  buf.read += count;
               }
               else
               {
                  // read all bytes
                  rval = buf.data.slice(buf.read);
                  buf.clear();
               }
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
             * Gets the last byte without modifying the read pointer.
             * 
             * @return the last byte.
             */
            last: function()
            {
               return buf.data.charCodeAt(buf.data.length - 1);
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

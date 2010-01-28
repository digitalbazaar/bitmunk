/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 * 
 * @author Dave Longley
 */
package db.net
{
   /**
    * An HttpRequest is used to store and serialize the method, version, and
    * headers for making an HTTP request.
    * 
    * Originally the URLRequest class was used for this, but it was discovered
    * that it does not support all HTTP methods (i.e. DELETE).
    */
   public class HttpRequest
   {
      import flash.net.URLRequestHeader;
      import flash.net.URLRequestMethod;
      import flash.net.URLVariables;
      import flash.utils.ByteArray;
      
      // http request path, method, version, and headers (URLRequestHeaders)
      public var url:String;
      public var method:String;
      public var version:String;
      public var headers:Array;
      
      // the http content-type
      public var contentType:String;
      
      // data containing either:
      // 1. A ByteArray (binary data)
      // 2. A URLVariables object (x-www-form-urlencoded if non-GET, appended
      //    to the url if GET)
      // 3. Object is converted to a string and sent if non-GET and non-blank.
      private var mData:Object;
      
      // stores bytes to be sent out after toString() has been called
      private var mBody:ByteArray;
      
      // CRLF (carriage return + line feed delimiter)
      private static const CRLF:String = "\r\n";
      
      /**
       * Creates a new HttpRequest.
       */
      public function HttpRequest()
      {
         // set defaults
         url = "/";
         method = "GET";
         version = "HTTP/1.1";
         headers = new Array();
         mData = null;
         mBody = null;
      }
      
      /**
       * Sets the entity body data to send.
       *
       * @param data the entity body data to send.
       */
      public function set data(value:Object):void
      {
         // set data and reset body bytes
         mData = value;
         mBody = null;
      }
      
      /**
       * Gets the entity body data to be sent.
       * 
       * @return the entity body data to be sent.
       */
      public function get data():Object
      {
         return mData;
      }
      
      /**
       * Gets body bytes to be sent out after calling toString(). Returns null
       * for no data.
       * 
       * @return the body as an array of bytes to be sent, null for none.
       */
      public function get body():ByteArray
      {
         return mBody;
      }
      
      /**
       * Converts this http request into a string that can be sent as a
       * HTTP request. Does not include any data.
       * 
       * @return the string representation of this request.
       */
      public function toString():String
      {
         /* Sample request header:
          GET /some/path/?query HTTP/1.1
          Host: www.someurl.com
          Connection: close
          Accept-Encoding: deflate
          Accept: image/gif, text/html
          User-Agent: Mozilla 4.0
          */
         
         // build extra headers
         var hdrs:Array = headers.slice();
         hdrs.push(new URLRequestHeader("Accept-Encoding", "deflate"));
         hdrs.push(new URLRequestHeader("User-Agent", "monarch.net Flash 1.0"));
         
         // add accept header if not already added
         var acceptAdded:Boolean = false;
         for each(var h:URLRequestHeader in headers)
         {
            if(h.name.toLowerCase() == "accept")
            {
               acceptAdded = true;
               break;
            }
         }
         if(!acceptAdded)
         {
            hdrs.push(new URLRequestHeader("Accept", "*/*"));
         }
         
         // start building path
         var path:String = url;
         
         // create body bytes if they are null but data is to be sent
         if(mBody == null && mData != null)
         {
            if(mData is ByteArray)
            {
               // just use data bytes directly
               mBody = mData as ByteArray;
            }
            else if(mData is URLVariables && method == URLRequestMethod.GET)
            {
               // append variables to the end of the url
               path += "?" + mData.toString();
            }
            else
            {
               // create new byte array and write data to it as a string
               mBody = new ByteArray();
               mBody.writeUTFBytes(mData.toString());
            }
            
            if(mBody != null)
            {
               // deflate data, if any exists
               mBody.compress();
            }
         }
         
         // if there is an entity-body, add appropriate headers
         if(mBody != null)
         {
            if(contentType)
            {
               // add content-type header
               hdrs.push(new URLRequestHeader("Content-Type", contentType));
            }
            
            // add content-encoding header
            hdrs.push(new URLRequestHeader("Content-Encoding", "deflate"));
            
            // add content length header
            var length:String = "" + mBody.length;
            hdrs.push(new URLRequestHeader("Content-Length", length));
         }
         
         // build http request string
         var str:String =
            method.toUpperCase() + " " + path + " " + version + CRLF;
         for each(var header:URLRequestHeader in hdrs)
         {
            str += header.name + ": " + header.value + CRLF;
         }
         // final terminating CRLF
         str += CRLF;
         
         return str;
      }
   }
}

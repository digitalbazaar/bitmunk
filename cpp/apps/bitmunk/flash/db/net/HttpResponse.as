/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 * 
 * @author Dave Longley
 */
package db.net
{
   /**
    * An HttpResponse is used to represent a response from an http server.
    */
   public class HttpResponse
   {
      import db.net.HttpRequest;
      import flash.net.URLRequestHeader;
      
      // the http response version, i.e. "HTTP/1.1"
      public var version:String;
      
      // the http response status code, i.e. 404
      public var statusCode:uint;
      
      // the http response status message, i.e. "Not Found"
      public var statusMessage:String;
      
      // the content-type used (i.e "text/json")
      public var contentType:String;
      
      // the content-encoding used (i.e. "deflate")
      public var contentEncoding:String;
      
      // the transfer-encoding used (i.e. "chunked")
      public var transferEncoding:String;
      
      // the content length, if present
      public var contentLength:int;
      
      // the connection header, if present
      public var connection:String;
      
      // an array of the http headers in the response, this
      // array contains URLRequestHeader objects, any trailers
      // will be appended to this when received
      public var headers:Array;
      
      // stores the response data: either a URLVariables object
      // if the content-type was application/x-www-form-urlencode,
      // otherwise a ByteArray...
      // if there was no content, then it is null
      public var data:Object;
      
      // set to true if the header has been received
      public var headerReceived:Boolean;
      
      // set to true if the body has been received
      public var bodyReceived:Boolean;
      
      // set to true if the request is being retried
      public var retried:Boolean;
      
      // the ID associated with the request/response
      public var id:uint;
      
      // the request associated with this response
      public var request:HttpRequest;
      
      // the start/total time (in ms) for this response
      public var time:Number;
      
      // some user-data to associate with this response
      public var userData:Object;
      
      // CRLF (carriage return + line feed)
      private static const CRLF:String = "\r\n";
      
      /**
       * Creates a new empty HttpResponse.
       */
      public function HttpResponse()
      {
         reset();
      }
      
      /**
       * Resets all members in this http response to their initial values.
       */
      public function reset():void
      {
         version = null;
         statusCode = 0;
         statusMessage = null;
         contentType = null;
         contentEncoding = null;
         transferEncoding = null;
         connection = null;
         contentLength = -1;
         headers = new Array();
         data = null;
         headerReceived = bodyReceived = false;
         id = 0;
         request = null;
         time = 0;
         userData = null;
      }
      
      /**
       * Returns true if this response has content, false if not.
       * 
       * @return true if this response has content, false if not.
       */
      public function hasContent():Boolean
      {
         return (contentType != "");
      }
      
      /**
       * Converts this HttpResponse to a string.
       *
       * @return the string.
       */
      public function toString():String
      {
         // format: VERSION CODE MSG
         var rval:String =
            version + " " + statusCode + " " + statusMessage + CRLF;
         
         // add headers
         for each(var header:URLRequestHeader in headers)
         {
            rval += header.name + ": " + header.value + CRLF;
         }
         
         // add content headers
         if(contentType != null)
         {
            rval += "Content-Type: " + contentType + CRLF;
         }
         if(contentEncoding != null)
         {
            rval += "Content-Encoding: " + contentEncoding + CRLF;
         }
         
         // only use EITHER transfer-encoding or content-length, not both
         if(transferEncoding != null)
         {
            rval += "Transfer-Encoding: " + transferEncoding + CRLF;
         }
         else if(contentLength != -1)
         {
            rval += "Content-Length: " + contentLength + CRLF;
         }
         
         // connection header
         if(connection != null)
         {
            rval += "Connection: " + connection + CRLF;
         }
         
         // terminating CRLF
         rval += CRLF;
         
         return rval;
      }
   }
}


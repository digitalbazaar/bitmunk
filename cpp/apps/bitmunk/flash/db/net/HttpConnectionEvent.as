/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 * 
 * @author Dave Longley
 */
package db.net
{
   import flash.events.Event;
   
   /**
    * HttpConnectionEvents are dispatched by an HttpConnection during
    * its course of processing requests and responses.
    */
   public class HttpConnectionEvent extends Event
   {
      import db.net.HttpResponse;
      
      import flash.events.TextEvent;
      
      // the types of http connection events
      public static const OPEN:String = "open";        // connection ready
      public static const REQUEST:String = "request";  // request sent
      public static const HEADER:String = "header";    // header received
      public static const LOADING:String = "loading";  // loading body
      public static const BODY:String = "body";        // body received
      public static const TIMEOUT:String = "timeout";  // receive timeout
      public static const IO_ERROR:String = "ioError";
      public static const SECURITY_ERROR:String = "securityError";
      public static const IDLE:String = "idle";
      
      // the associated HttpResponse
      public var response:HttpResponse;
      
      // the associated connection
      public var connection:HttpConnection;
      
      // an associated event, for io and security errors
      public var cause:TextEvent;
      
      /**
       * Creates a new HttpConnectionEvent.
       * 
       * @param type the type of event.
       * @param response the associated HttpResponse.
       * @param cause the related event that caused this one.
       */
      public function HttpConnectionEvent(
         type:String, response:HttpResponse, cause:TextEvent = null)
      {
         this.response = response;
         if(cause != null)
         {
            this.cause = cause.clone() as TextEvent;
         }
         super(type, false, false);
      }
   }
}

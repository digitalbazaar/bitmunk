/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 * 
 * @author Dave Longley
 */
package db.net
{
   /**
    * A Cookie is a small piece of state information sent by a server to a
    * client that will be resent by the client in its next communication with
    * the server.
    */
   public class Cookie
   {
      // the name of the cookie
      public var name:String;
      
      // value the value of the cookie
      public var value:String;
      
      // comment an optional comment string
      public var comment:String;
      
      // max age of the cookie in seconds relative to the created time
      public var maxAge:int;
      
      // secure true if the cookie must be sent over a secure protocol
      public var secure:Boolean;
      
      // true to restrict access to the cookie from javascript
      public var httpOnly:Boolean;
      
      // the path for the cookie
      public var path:String;
      
      // the optional domain the cookie belongs to (must start with a dot)
      public var domain:String;
      
      // the optional version of the cookie
      public var version:String;
      
      // stores the creation time, in UTC seconds, of the cookie
      public var created:Number;
      
      /**
       * Creates a new Cookie.
       * 
       * @param name the name of the cookie.
       * @param value the value of the cookie.
       * @param maxAge the maximum age of the cookie in seconds, 0 to expire
       *               and -1 for infinite.
       * @param secure true if the cookie must be sent over a secure protocol.
       * @param httpOnly true to restrict access to the cookie from javascript.
	    * @param path the path to the cookie, defaults to "/".
	    * @param domain the domain the cookie belongs to (must start with a dot),
	    *               defaults to none.
	    * @param version the cookie version, defaults to null to send no version.
       */
      public function Cookie(
         name:String = "", value:String = "", maxAge:int = 0,
         secure:Boolean = true, httpOnly:Boolean = true,
         path:String = "/", domain:String = null, version:String = null)
      {
         this.name = name;
         this.value = value;
         this.maxAge = maxAge;
         this.secure = secure;
         this.httpOnly = httpOnly;
         this.path = path;
         this.domain = domain;
         this.version = version;
         this.created = 0;
      }
      
      /**
       * Checks to see if this cookie has expired. If this cookie's max-age
       * plus its created time is less than the time now, it has expired,
       * unless its max-age is set to -1, which indicates it will never expire.
       * 
       * @return true if this cookie has expired, false if not.
       */
      public function get expired():Boolean
      {
         var rval:Boolean = false;
         
         if(maxAge != -1)
         {
	         // get time now
	         var date:Date = new Date();
	         var now:Number = new Date().valueOf() / 1000.;
	         
	         // get expiration date, compare to now
	         var expires:Number = this.created + this.maxAge;
            if(expires <= now)
            {
               // expiration date is before now, so cookie is expired
               rval = true;
            }
         }
         
         return rval;
      }
   }
}

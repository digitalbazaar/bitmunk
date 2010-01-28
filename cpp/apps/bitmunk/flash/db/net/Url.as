/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 * 
 * @author Dave Longley
 */
package db.net
{
   /**
    * A Url provides easy access to the protocol, host, and port of a URL.
    */
   public class Url
   {
      import mx.utils.URLUtil;
      
      private var mUrl:String;
      public var protocol:String;
      public var host:String;
      public var port:uint;
      
      /**
       * Creates a new Url.
       * 
       * @param url the full or relative url.
       */
      public function Url(url:String)
      {
         this.url = url;
      }
      
      /**
       * Sets the url.
       * 
       * @param url the new url to use.
       */
      public function set url(value:String):void
      {
         mUrl = value;
         this.host = URLUtil.getServerName(value);
         this.port = URLUtil.getPort(value);
         this.protocol = URLUtil.getProtocol(value);
      }
      
      /**
       * Gets the url as string.
       * 
       * @return the url as a string.
       */
      public function get url():String
      {
         return mUrl;
      }
      
      /**
       * Gets the protocol, host, and port of this url.
       * 
       * For example:
       *
       * If:
       * protocol = "http"
       * host = "www.myserver.com"
       * port = "8080"
       * 
       * Then this function returns:
       * "http://www.myserver.com:8080"
       * 
       * @returns the protocol, host, and port of this url.
       */
      public function get root():String
      {
         return protocol + "://" + host + ":" + port;
      }
      
      /**
       * Gets the host and port of this url.
       * 
       * For example:
       *
       * If:
       * host = "www.myserver.com"
       * port = "8080"
       * 
       * Then this function returns:
       * "www.myserver.com:8080"
       * 
       * @returns the protocol, host, and port of this url.
       */
      public function get hostWithPort():String
      {
         return host + ":" + port;
      }
      
      /**
       * Returns true if this url has a protocol of http, https, or rtmp.
       * 
       * @return true if this url has a protocol of http.
       */
      public function isHttp():Boolean
      {
         return URLUtil.isHttpURL(url);
      }
      
      /**
       * Returns true if this url has a protocol of https.
       * 
       * @return true if this url has a protocol of https.
       */
      public function isHttps():Boolean
      {
         return URLUtil.isHttpsURL(url);
      }
      
      /**
       * Gets a string representation of this url. This will
       * return the url property.
       * 
       * @return the string representation of this url.
       */
      public function toString():String
      {
         return this.url;
      }
   }
}


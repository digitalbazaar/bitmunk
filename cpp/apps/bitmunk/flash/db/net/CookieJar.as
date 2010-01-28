/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 * 
 * @author Dave Longley
 */
package db.net
{
   /**
    * A CookieJar stores cookies and is capable of being serialized to a
    * HttpRequest or from an HttpResponse.
    */
   public class CookieJar
   {
      import db.net.Cookie;
      import db.net.HttpRequest;
      import db.net.HttpResponse;
      import db.net.Url;
      
      import flash.net.SharedObject;
      import flash.net.URLRequestHeader;
      import flash.net.URLRequestMethod;
      import mx.utils.StringUtil;
      
      // the url the cookies are for
      private var mUrl:Url;
      // a map of cookie name to cookie object
      private var mCookies:Object;
      // the number of cookies in the jar
      private var mCookieCount:uint;
      
      /**
       * Creates a new, empty CookieJar.
       * 
       * @param url the url with protocol, host, and port.
       */
      public function CookieJar(url:Url)
      {
         mUrl = url;
         mCookies = new Object();
         mCookieCount = 0;
         
         try
         {
            // load any locally stored cookies
            var store:SharedObject = SharedObject.getLocal("monarch.net.CookieJar");
            if("jars" in store.data && store.data.jars is Array)
            {
               // load old cookies (find by url)
               for each(var jar:Object in store.data.jars)
               {
                  if(jar.url == mUrl.root)
                  {
                     // jar found
                     mCookieCount = jar.cookieCount;
                     
                     // must load in manually because store doesn't
                     // understand the "Cookie" class
                     for each(var cookie:Object in jar.cookies)
                     {
                        var c:Cookie = new Cookie(
                           cookie.name, cookie.value, cookie.maxAge,
                           cookie.secure, cookie.httpOnly,
                           cookie.path, cookie.domain, cookie.version);
                        c.created = cookie.created;
                        mCookies[cookie.name] = c;
                     }
                     break;
                  }
               }
            }
         }
         catch(e:Error)
         {
            // could not load local cookies... will cause cookies to be
            // deleted whenever the user refreshes/changes top-level page
            trace(e.getStackTrace());
         }
      }
      
      /**
       * Writes this cookie jar to local storage.
       */
      private function store():void
      {
         try
         {
            // flush cookies to local store
            var store:SharedObject = SharedObject.getLocal("monarch.net.CookieJar");
            var existingJar:Object = null;
            if(!("jars" in store.data))
            {
               // create new jars collection
               store.data.jars = new Array();
            }
            else
            {
               // find existing cookie jar
               for each(var jar:Object in store.data.jars)
               {
                  if(jar.url == mUrl.root)
                  {
                     // jar found
                     existingJar = jar;
                     break;
                  }
               }
            }
            
            // create jar if one doesn't exist
            if(existingJar == null)
            {
               existingJar = new Object();
               store.data.jars.push(existingJar);
            }
            
            // set jar properties and flush storage
            existingJar.url = mUrl.root;
            existingJar.cookies = mCookies;
            existingJar.cookieCount = mCookieCount;
            store.flush();
         }
         catch(e:Error)
         {
            // could not load local cookies... will cause cookies to be
            // deleted whenever the user refreshes/changes top-level page
            trace(e.getStackTrace());
         }
      }
      
      /**
       * Gets the number of cookies in this jar.
       * 
       * @return the number of cookies in this jar.
       */
      public function get count():uint
      {
         return mCookieCount;
      }
      
      /**
       * Reads cookies into this jar from the passed HttpResponse. Any
       * duplicate cookies set in this jar will be overwritten.
       * 
       * The cookies will be read from any "Set-Cookie" header fields.
       * 
       * @param response the HttpResponse to read the cookies from.
       */
      public function readCookies(response:HttpResponse):void
      {
         // get the current date in seconds
         var date:Date = new Date();
         var now:Number = new Date().valueOf() / 1000.;
         
         // iterate over headers, looking for "Set-Cookie" fields:
         // Set-Cookie: cookie1_name=cookie1_value; max-age=0; path=/
         // Set-Cookie: c2=v2; expires=Thu, 21-Aug-2008 23:47:25 GMT; path=/
         for each(var field:URLRequestHeader in response.headers)
         {
            if(field.name.toLowerCase() == "set-cookie")
            {
               // parse cookies by semi-colons (cannot parse by commas because
               // the "expires" value may contain a comma and no one follows
               // the standard)
               var cookie:Cookie = null;
               var params:Array = field.value.split(";");
               for(var i:uint = 0; i < params.length; i++)
               {
                  // split name=value
                  var nv:Array = params[i].split("=");
                  nv[0] = StringUtil.trim(nv[0]);
                  nv[1] = StringUtil.trim(nv.slice(1).join("="));
                  
                  // cookie_name=value
                  if(i == 0)
                  {
                     // cookie has a name, so create it
                     cookie = new Cookie(nv[0], nv[1]);
                     cookie.secure = false;
                     cookie.httpOnly = false;
                  }
                  // property_name=value
                  else
                  {
                     nv[0] = nv[0].toLowerCase();
                     switch(nv[0])
                     {
                        case "expires":
                           // parse expiration time
                           var secs:Number = Date.parse(nv[1]) / 1000.;
                           cookie.maxAge = int(Math.max(0, secs - now));
                           break;
                        case "max-age":
                           cookie.maxAge = int(nv[1]);
                        case "secure":
                           cookie.secure = true;
                           break;
                        case "httponly":
                           cookie.httpOnly = true;
                           break;
                        default:
                           if(nv[0] in cookie)
                           {
                              cookie[nv[0]] = nv[1];
                           }
                           // ignore invalid cookie params
                           break;
                     }
                  }
               }
               
               // add cookie if successfully parsed, do not flush until
               // finished
               if(cookie != null)
               {
                  setCookie(cookie, false);
               }
            }
         }
         
         // store cookies
         store();
      }
      
      /**
       * Writes the cookies in this jar to the passed HttpRequest.
       * 
       * The cookies will be written to the "Cookie" header field. If that
       * field already has a value, it will be appended to. Any cookies that
       * have expired will not be sent and will be removed from this jar
       * automatically.
       * 
       * @param request the request to write the cookies to.
       */
      public function writeCookies(request:HttpRequest):void
      {
         if(this.count > 0)
         {
	         // find an existing "Cookie" header field or create a new one
            var created:Boolean = false;
	         var field:URLRequestHeader = null;
	         for each(var header:URLRequestHeader in request.headers)
	         {
	            if(header.name.toLowerCase() == "cookie")
	            {
	               field = header;
	               break;
	            }
	         }
	         if(field == null)
	         {
	            field = new URLRequestHeader("Cookie", "");
               created = true;
	         }
            
            // get the current date in seconds
            var date:Date = new Date();
            var now:Number = new Date().valueOf() / 1000.;
            
            // append each cookie to the header field value
            for each(var cookie:Cookie in mCookies)
            {
               // check the cookie for expiration
               if(cookie.expired)
               {
                  removeCookie(cookie.name, false);
               }
               else if(mUrl.isHttps() == cookie.secure)
               {
	               // append semi-colon and space if not first cookie
	               if(field.value.length > 0)
	               {
	                  field.value += "; ";
	               }
	               
	               // output cookie name and value
	               field.value += cookie.name + "=" + cookie.value;
               }
            }
            
            // add field to request if it was created
            if(created && field.value.length > 0)
            {
               request.headers.push(field);
            }
            
            // store cookies
            store();
         }
      }
      
      /**
       * Sets a cookie in this jar. It must be written out to an http header
       * to send it somewhere.
       * 
       * @param cookie the cookie to set in this jar.
       * @param write true to flush cookie storage, false not to.
       */
      public function setCookie(cookie:Cookie, write:Boolean = true):void
      {
         // update cookie count if cookie is brand new
         if(!(cookie.name in mCookies))
         {
            mCookieCount++;
         }
         
         // set cookie created date
         var date:Date = new Date();
         cookie.created = new Date().valueOf() / 1000.;
         
         // add cookie
         mCookies[cookie.name] = cookie;
         
         if(write)
         {
            // store cookies
            store();
         }
      }
      
      /**
       * Gets a cookie from this jar.
       * 
       * @param name the name of the cookie to get.
       * 
       * @return the cookie (set to null if no such cookie).
       */
      public function getCookie(name:String):Cookie
      {
         var rval:Cookie = null;
         
         if(name in mCookies)
         {
            rval = mCookies[name];
         }
         
         return rval;
      }
      
      /**
       * Removes a cookie from this jar.
       * 
       * @param name the name of the cookie to remove.
       * @param write true to flush cookie storage, false not to.
       * 
       * @return true if a cookie was remove, false if not.
       */
      public function removeCookie(name:String, write:Boolean = true):Boolean
      {
         var rval:Boolean = false;
         
         if(name in mCookies)
         {
            delete mCookies[name];
            mCookieCount--;
            rval = true;
            
            if(write)
            {
               // flush storage
               store();
            }
         }
         
         return rval;
      }
      
      /**
       * Clears all cookies from this jar.
       */
      public function clear():void
      {
         mCookies = new Object();
         mCookieCount = 0;
      }
   }
}

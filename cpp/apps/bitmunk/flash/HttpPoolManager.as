/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 * 
 * @author Dave Longley
 */
package
{
   import flash.display.Sprite;
   
   /**
    * An HttpPoolManager is a flash object that can be embedded in a webpage to
    * allow javascript access to pools of http connections.
    * 
    * The javascript must create a pool and specify the root url to connect to
    * (including http/https, host, and port) and the number of concurrent
    * connections permitted.
    * 
    * If the server supports keep-alive connections, then persistent
    * connections will be used by this class. This is done internally with
    * flash's Socket class, so it will not use up the limited number of
    * persistent connections per-site via the browser container.
    * 
    * Similarly, if the connections are to an SSL url, a configuration that
    * specifies the certificates to trust must by specified. Without doing
    * so, a default certificate authority store will be used that may not
    * trust your server's certificate. Self-signed certificates are permitted,
    * which is particularly useful for applications that wish to take
    * advantage of SSL-encryption, but do not require SSL-authentication.
    */
   public class HttpPoolManager extends Sprite
   {
      import com.hurlant.crypto.cert.X509Certificate;
      import com.hurlant.crypto.cert.X509CertificateCollection;
      import com.hurlant.crypto.tls.TLSConfig;
      import com.hurlant.crypto.tls.TLSEngine;
      import com.hurlant.util.der.PEM;
      
      import db.net.Cookie;
      import db.net.CookieJar;
      import db.net.HttpConnection;
      import db.net.HttpConnectionEvent;
      import db.net.HttpRequest;
      import db.net.HttpResponse;
      import db.net.Url;
      
      import flash.external.ExternalInterface;
      import flash.events.EventDispatcher;
      import flash.net.URLRequestHeader;
      import flash.net.URLRequestMethod;
      import flash.net.URLVariables;
      import flash.system.Security;
      import flash.utils.ByteArray;
      import flash.utils.Dictionary;
      
      // a map of pool ID to url and pool
      private var mPoolMap:Object;
      
      // a counter for pool IDs (Note: assumes there will be no overflow)
      private var mPoolIdCounter:uint;
      
      // an event dispatcher for sending events to javascript
      private var mEventDispatcher:EventDispatcher;
      
      // true if values must be escaped for javascript, false if not
      private var mEscapeForJavascript:Boolean;
      
      /**
       * Creates a new, unitialized HttpPoolManager.
       * 
       * @throws Error - if no external interface is available to provide
       *                 javascript access.
       */
      public function HttpPoolManager()
      {
         if(!ExternalInterface.available)
         {
            trace("external interface is not available");
            throw new Error(
               "Flash's ExternalInterface is not available. This is a " +
               "requirement of HttpPoolManager and therefore, " +
               "HttpPoolManager will be unavailable.");
         }
         else
         {
            try
            {
               // set up javascript access:
               
               // initializes/cleans up the HttpPoolManager
               ExternalInterface.addCallback("init", init);
               ExternalInterface.addCallback("cleanup", cleanup);
               
               // creates a pool of connections to proxy data to a url
               ExternalInterface.addCallback("create", create);
               ExternalInterface.addCallback("destroy", destroy);
               
               // add a callback for subscribing to http response events
               ExternalInterface.addCallback("subscribe", subscribe);
               
               // add methods for setting and getting cookies
               ExternalInterface.addCallback("setCookie", setCookie);
               ExternalInterface.addCallback("getCookie", getCookie);
               
               // add method for queuing up requests
               ExternalInterface.addCallback("queue", queue);
               
               // http pool manager is now ready
               ExternalInterface.call("hpm.ready");
            }
            catch(e:Error)
            {
               trace(e.getStackTrace());
               // FIXME: debug only
               //ExternalInterface.call("console.log",
               //   escapeForJavascript(
               //      "XXX: error=" +
               //      e.errorID + "," + e.name + "," + e.message));
               throw e;
            }
         }
      }
      
      /**
       * ExternalInterface is broken. When sending a strings to javascript. It
       * will simply wrap strings with '\"' (escaped quotes) and then dump
       * them into a javascript string variable. This causes all backslashes
       * in the strings to be screwed up because they were not likewise escaped.
       * 
       * To fix this, we must recursively escape all strings in arrays and
       * objects (anything being returned to javascript). We must also hope
       * that we don't have circular references ... or DISASTER.
       * 
       * FIXME: Please remove this horrible efficiency sink once the bug is
       * fixed. 
       */
      private function escapeForJavascript(value:*):*
      {
         var rval:*;
         
         if(mEscapeForJavascript)
         {
            if(value is String)
            {
               // replace all slashes with 2 slashes
               rval = value.replace(/\\/g, "\\\\");
            }
            else if(value is Array)
            {
               var array:Array = value as Array;
               for(var i:uint = 0; i < array.length; i++)
               {
                  array[i] = escapeForJavascript(array[i]);
               }
               rval = array;
            }
            else if(value is Dictionary)
            {
               var dict:Dictionary = value as Dictionary;
               for(var key:Object in dict)
               {
                  dict[key] = escapeForJavascript(dict[key]);
               }
               rval = dict;
            }
            else if(value is Object)
            {
               var object:Object = value as Object;
               for(var name:String in object)
               {
                  object[name] = escapeForJavascript(object[name]);
               }
               rval = object;
            }
         }
         else
         {
            rval = value;
         }
         
         return rval;
      }
      
      /**
       * Called by javascript to initialize this HttpPoolManager.
       * 
       * @param options:
       *        escapeForJavascript: true if values must be escaped for
       *           javascript, false if not.
       *        marshallExceptions: true to pass exceptions to and from
       *           javascript.
       */
      private function init(options:Object):void
      {
         trace("HttpPoolManager.init()");
         
         // define options if necessary
         if(!options)
         {
            options = new Object();
         }
         
         // create pool map, ID counter, and event dispatcher
         mPoolMap = new Object();
         mPoolIdCounter = 0;
         mEventDispatcher = new EventDispatcher();
         mEscapeForJavascript = options.escapeForJavascript || true;
         
         // enable marshalling exceptions if appropriate
         if(options.marshallExceptions === true)
         {
            try
            {
               // Note: setting marshallExceptions in IE, even inside of
               // a try-catch block will terminate flash. Don't set this
               // on IE.
               ExternalInterface.marshallExceptions = true;
            }
            catch(e:Error)
            {
               trace(e.getStackTrace());
            }
         }
      }
      
      /**
       * Called by javascript to clean up an HttpPoolManager.
       */
      private function cleanup():void
      {
         trace("HttpPoolManager.cleanup()");
         
         mPoolMap = null;
         mPoolIdCounter = 0;
         mEventDispatcher = null;
         
         //ExternalInterface.call(
         //   "console.log", "HttpPoolManager.cleanup() done");
      }
      
      /**
       * Called by javascript to create a pool of connections to a url.
       * 
       * @param url the url to connect to.
       * @param poolSize the maximum number of concurrent connections.
       * @param socketPolicyPort the port to obtain a socket policy file from.
       * @param certs an array of PEM-formatted certificates to trust.
       * @param verifyCommonName an alternative X.509 subject common name to
       *                         trust rather than the URL host name.
       * 
       * @return the pool ID.
       */
      private function create(
         url:String, poolSize:uint, socketPolicyPort:uint,
         certs:Array = null, verifyCommonName:String = null):String
      {
         trace("HttpPoolManager.create(" +
            url + ", " + poolSize + ", " + socketPolicyPort +
            ", certs, " + verifyCommonName + ")");
         
         // create the pool map entry
         var entry:Object = new Object();
         entry.id = String(++mPoolIdCounter);
         entry.pool = new Array();
         entry.idleConnections = new Array();
         entry.requestQueue = new Array();
         entry.url = new Url(url);
         entry.cookieJar = new CookieJar(entry.url);
         
         // build TLSConfig if certs are provided
         var tlsconfig:TLSConfig = null;
         if(certs != null)
         {
            // create X509CertificateCollection
            var caStore:X509CertificateCollection =
               new X509CertificateCollection();
            for each(var pem:String in certs)
            {
               // create certificate for each PEM in certs
               var x509:X509Certificate = new X509Certificate(
                  PEM.readCertIntoArray(pem));
               caStore.addCertificate(x509);
            }
            
            // create TLS config
            // entity, cipher suites, compressions, cert, key, CA store
            tlsconfig = new TLSConfig(
               TLSEngine.CLIENT, // entity
               null,             // cipher suites
               null,             // compressions
               null,             // certificate
               null,             // private key
               caStore);         // certificate authority storage
         }
         
         // load socket policy file
         // (permits socket access to backend)
         Security.loadPolicyFile(
        	   "xmlsocket://" + entry.url.host + ":" + socketPolicyPort);
         
         // create http connections for pool
         for(var i:uint = 0; i < poolSize; i++)
         {
            var conn:HttpConnection = new HttpConnection();
            conn.url = entry.url;
            conn.tlsconfig = tlsconfig;
            conn.verifyCommonName = verifyCommonName;
            conn.poolId = entry.id;
            conn.addEventListener(HttpConnectionEvent.OPEN, dispatch);
            conn.addEventListener(HttpConnectionEvent.REQUEST, dispatch);
            conn.addEventListener(HttpConnectionEvent.HEADER, dispatch);
            conn.addEventListener(HttpConnectionEvent.HEADER, readCookies);
            conn.addEventListener(HttpConnectionEvent.LOADING, dispatch);
            conn.addEventListener(HttpConnectionEvent.BODY, dispatch);
            conn.addEventListener(HttpConnectionEvent.TIMEOUT, dispatch);
            conn.addEventListener(HttpConnectionEvent.IO_ERROR, dispatch);
            conn.addEventListener(HttpConnectionEvent.SECURITY_ERROR, dispatch);
            conn.addEventListener(HttpConnectionEvent.IDLE, dequeue);
            entry.pool.push(conn);
            entry.idleConnections.push(conn);
         }
         
         // add entry to pool map
         mPoolMap[entry.id] = entry;
         
         //ExternalInterface.call(
         //   "console.log", "HttpPoolManager.create() done");
         
         return entry.id;
      }
      
      /**
       * Called by javascript to clean up a connection pool.
       * 
       * @param id the ID of the pool to clean up.
       */
      private function destroy(id:String):void
      {
         trace("HttpPoolManager.destroy(" + id + ")");
         
         if(id in mPoolMap)
         {
            // remove pool entry
            delete mPoolMap[id];
         }
         
         //ExternalInterface.call(
         //   "console.log", "HttpPoolManager.destroy() done");
      }
      
      /**
       * Registers a javascript function as a callback for an event.
       * 
       * @param eventType the type of event (HttpConnectionEvent types).
       * @param callback the name of the callback function.
       */
      private function subscribe(eventType:String, callback:String):void
      {
         switch(eventType)
         {
            case HttpConnectionEvent.OPEN:
            case HttpConnectionEvent.REQUEST:
            case HttpConnectionEvent.HEADER:
            case HttpConnectionEvent.LOADING:
            case HttpConnectionEvent.BODY:
            case HttpConnectionEvent.TIMEOUT:
            case HttpConnectionEvent.IO_ERROR:
            case HttpConnectionEvent.SECURITY_ERROR:
               mEventDispatcher.addEventListener(
                  eventType, function(event:HttpConnectionEvent):void
                  {
                     //ExternalInterface.call(
                     //   "console.log", "HttpPoolManager eventDispatch");
                     
                     // build event for javascript
                     var e:Object = new Object();
                     e.id = event.response.id;
                     e.type = event.type;
                     e.response = event.response;
                     
                     if(event.response.data != null)
                     {
                        // convert ByteArray to string for javascript
                        e.body = (event.response.data is ByteArray) ?
                           event.response.data.toString() :
                           event.response.data;
                     }
                     else
                     {
                        // no body
                        e.body = null;
                     }
                     
                     // send event to javascript
                     ExternalInterface.call(callback, escapeForJavascript(e));
                  });
               break;
            default:
               throw new ArgumentError(
                  "Could not subscribe to http response event. " +
                  "Invalid event type specified.\neventType: " + eventType);
         }
         
         //ExternalInterface.call(
         //   "console.log", "HttpPoolManager.subscribe() done");
      }
      
      /**
       * Sets a cookie for a particular pool. The cookie object must have a
       * name and value or be null to clear a cookie.
       * 
       * @param id the pool ID.
       * @param name the name of the cookie.
       * @param value the value of the cookie or null to remove the cookie.
       * @param maxAge how long before the cookie should expire, in seconds.
       */
      private function setCookie(
         poolId:String, name:String, value:String, maxAge:int = -1):void
      {
         if(!(poolId in mPoolMap))
         {
            throw new ArgumentError("Cannot set cookie. Invalid pool ID.");
         }
         
         // get the pool map entry
         var entry:Object = mPoolMap[poolId];
         if(value != null)
         {
            // create and set cookie
            var cookie:Cookie = new Cookie(name, value);
            cookie.secure = entry.url.isHttps();
            cookie.httpOnly = false;
            cookie.maxAge = maxAge;
            entry.cookieJar.setCookie(cookie);
         }
         else
         {
            entry.cookieJar.removeCookie(name);
         }
         
         //ExternalInterface.call(
         //   "console.log", "HttpPoolManager.setCookie() done");
      }
      
      /**
       * Gets the value of a cookie for a particular pool.
       * 
       * @param id the pool ID.
       * @param name the name of the cookie.
       * 
       * @return the cookie value or null if it does not exist.
       */
      private function getCookie(poolId:String, name:String):String
      {
         var rval:String = null;
         
         if(!(poolId in mPoolMap))
         {
            throw new ArgumentError("Cannot get cookie. Invalid pool ID.");
         }
         
         // get the pool map entry and cookie
         var entry:Object = mPoolMap[poolId];
         var cookie:Cookie = entry.cookieJar.getCookie(name);
         if(cookie != null && !cookie.httpOnly)
         {
            rval = cookie.value;
         }
         
         //ExternalInterface.call(
         //   "console.log", "HttpPoolManager.getCookie() done");
         
         return escapeForJavascript(rval);
      }
      
      /**
       * Queues a request to be sent out. The passed request
       * must have the following format:
       * 
       * {
       *    method: string,
       *    url: string,
       *    contentType: string,
       *    data: string or an object with properties that are url variables,
       *    requestHeaders: array of {name,value} objects
       * }
       * 
       * @param poolId the ID of the pool to use to send the request.
       * @param request the request from javascript.
       * @param id a unique ID for the request.
       */
      private function queue(
         poolId:String, request:Object, id:uint = 0):void
      {
         //ExternalInterface.call("console.log", "queueing request", request);
         
         if(!(poolId in mPoolMap))
         {
            //ExternalInterface.call("console.log", "invalid pool ID", request);
            throw new ArgumentError("Cannot queue request. Invalid pool ID.");
         }
         
         // get map entry
         var entry:Object = mPoolMap[poolId];
         
         // save pool ID and unique ID along with request object
         request.poolId = poolId;
         request.id = id;
         
         if(entry.idleConnections.length > 0)
         {
         	// add request to an already idle connection
         	var hc:HttpConnection =
         	   entry.idleConnections.shift() as HttpConnection;
         	addRequest(request, hc);
         }
         else
         {
            // add the request to the request queue
            entry.requestQueue.push(request);
            //ExternalInterface.call("console.log", "queued request", request);
         }
      }
      
      /**
       * Called when an http connection goes idle so that another request can
       * be queued with it.
       * 
       * @param event the event.
       */
      private function dequeue(event:HttpConnectionEvent):void
      {
         // get pool entry
         var entry:Object = mPoolMap[event.connection.poolId];
      	
         if(entry.requestQueue.length > 0)
         {
            // dequeue request and add it to the idle connection
      	   var request:Object = entry.requestQueue.shift();
      	  	addRequest(request, event.connection);
      	}
      	else
      	{
      	   // add connection to queue of idle connections
      	   entry.idleConnections.push(event.connection);
         }
      }
      
      /**
       * Adds a request to the given connection.
       * 
       * @param request the request to add.
       * @param conn the connection to add the request to.conn teh
       */
      private function addRequest(request:Object, conn:HttpConnection):void
      {
         // build HttpRequest
         var req:HttpRequest = new HttpRequest();
         req.method = request.method || URLRequestMethod.GET;
         req.url = request.url || "/";
         req.contentType = request.contentType || null;
         req.data = null;
         
         if(request.data)
         {
            if(request.data is String)
            {
               req.data = request.data;
            }
            else if(request.data)
            {
               // iterate over properties of data,
               // parsing each as a url variable
               var vars:URLVariables = new URLVariables;
               for(var key:String in request.data)
               {
                  vars[key] = request.data[key];
               }
               req.data = vars;
            }
         }
         
         if(request.requestHeaders && request.requestHeaders is Array)
         {
            // add custom request headers
            var headers:Array = request.requestHeaders as Array;
            for(var j:uint = 0; j < headers.length; j++)
            {
               var header:Object = headers[j];
               req.headers.push(
                  new URLRequestHeader(header.name, header.value));
            }
         }
         
         // get the pool map entry
         var entry:Object = mPoolMap[request.poolId];
         
         // add cookies for pool to request
         entry.cookieJar.writeCookies(req);
         
         // queue the request
         //ExternalInterface.call("console.log", "adding request", request);
         conn.addRequest(req, request.id, request.poolId);
      }
      
      /**
       * Called when an http response's header is read so that its cookies
       * can be parsed.
       * 
       * @param event the event.
       */
      private function readCookies(event:HttpConnectionEvent):void
      {
         // get the pool entry
         if(event.response.userData in mPoolMap)
         {
            // read cookies from response
            var entry:Object = mPoolMap[event.response.userData];
            entry.cookieJar.readCookies(event.response);
         }
      }
      
      /**
       * Called when an http response event occurs to dispatch the event
       * to any javascript callbacks.
       * 
       * @param event the event.
       */
      private function dispatch(event:HttpConnectionEvent):void
      {
        if(event.type == HttpConnectionEvent.TIMEOUT)
        {
           // close connection
           event.target.close();
        }
        
        mEventDispatcher.dispatchEvent(
           new HttpConnectionEvent(event.type, event.response));
      }
   }
}

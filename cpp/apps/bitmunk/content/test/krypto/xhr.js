/**
 * XmlHttpRequest implementation that uses TLS and flash SocketPool.
 *
 * @author Dave Longley
 *
 * Copyright (c) 2010 Digital Bazaar, Inc. All rights reserved.
 */
(function($)
{
   // logging category
   var cat = 'krypto.xhr';
   
   /*
   XMLHttpRequest interface definition from:
   http://www.w3.org/TR/XMLHttpRequest
   
   interface XMLHttpRequest {
     // event handler
     attribute EventListener onreadystatechange;
   
     // state
     const unsigned short UNSENT = 0;
     const unsigned short OPENED = 1;
     const unsigned short HEADERS_RECEIVED = 2;
     const unsigned short LOADING = 3;
     const unsigned short DONE = 4;
     readonly attribute unsigned short readyState;
   
     // request
     void open(in DOMString method, in DOMString url);
     void open(in DOMString method, in DOMString url, in boolean async);
     void open(in DOMString method, in DOMString url,
               in boolean async, in DOMString user);
     void open(in DOMString method, in DOMString url,
               in boolean async, in DOMString user, in DOMString password);
     void setRequestHeader(in DOMString header, in DOMString value);
     void send();
     void send(in DOMString data);
     void send(in Document data);
     void abort();
   
     // response
     DOMString getAllResponseHeaders();
     DOMString getResponseHeader(in DOMString header);
     readonly attribute DOMString responseText;
     readonly attribute Document responseXML;
     readonly attribute unsigned short status;
     readonly attribute DOMString statusText;
   };
   */
   
   // readyStates
   var UNSENT = 0;
   var OPENED = 1;
   var HEADERS_RECEIVED = 2;
   var LOADING = 3;
   var DONE = 4;
   
   // exceptions
   var INVALID_STATE_ERR = 11;
   var SYNTAX_ERR = 12;
   var SECURITY_ERR = 18;
   var NETWORK_ERR = 19;
   var ABORT_ERR = 20;
   
   // private flash socket pool vars
   var _sp = null;
   var _policyPort = 19845;
   
   // TODO: allow multiple http clients (this xhr can do cross-domain
   // so there is 1 http client per domain but each client can have N
   // connections
   var _client = null;
   
   // the maximum number of concurrents connections per client
   var _maxConnections = 10;
   
   // local aliases
   var krypto = window.krypto;
   var net = krypto.net;
   var http = krypto.http;
   
   // define the xhr interface
   // TODO: change to monarch or js lib name
   var xhrApi = {};
   
   /**
    * Initializes flash XHR support.
    * 
    * @param options:
    *           url: the base URL to connect to, ie: https://myserver.com
    *              (current implementation only supports one host).
    *           flashId: the dom ID of the flash SocketPool.
    *           policyPort: the port that provides the server's flash policy.
    *           msie: true if browser is internet explorer, false if not.
    *           connections: the maximum number of concurrent connections.
    *           caCerts: a list of PEM-formatted certificates to trust.
    *           verify: optional TLS certificate verify callback to use (see
    *              krypto.tls for details).
    *           persistCookies: true to use persistent cookies via flash local
    *              storage, false to only keep cookies in javascript.
    *           primeTlsSockets: true to immediately connect TLS sockets on
    *              their creation so that they will cache TLS sessions for
    *              reuse.
    */
   xhrApi.init = function(options)
   {
      // TODO: create this only once when multiple hosts are supported
      // create the flash socket pool
      _sp = net.createSocketPool({
         flashId: options.flashId,
         policyPort: options.policyPort || _policyPort,
         msie: options.msie || false
      });
      
      // TODO: allow 1 client per domain
      // create http client
      _client = http.createClient({
         url: options.url,
         socketPool: _sp,
         connections: options.connections || _maxConnections,
         caCerts: options.caCerts,
         persistCookies: options.persistCookies || true,
         primeTlsSockets: options.primeTlsSockets || false,
         verify: options.verify
      });
   };
   
   /**
    * Called to clean up the flash interface.
    */
   xhrApi.cleanup = function()
   {
      _client.destroy();
      _sp.destroy();
   };
   
   /**
    * Sets a cookie.
    * 
    * @param cookie the cookie with parameters:
    *    name: the name of the cookie.
    *    value: the value of the cookie.
    *    comment: an optional comment string.
    *    maxAge: the age of the cookie in seconds relative to created time.
    *    secure: true if the cookie must be sent over a secure protocol.
    *    httpOnly: true to restrict access to the cookie from javascript
    *       (inaffective since the cookies are stored in javascript).
    *    path: the path for the cookie.
    *    domain: optional domain the cookie belongs to (must start with dot).
    *    version: optional version of the cookie.
    *    created: creation time, in UTC seconds, of the cookie.
    */
   xhrApi.setCookie = function(cookie)
   {
      // TODO: when multiple http clients are supported, set the cookie on
      // the one with the correct domain (or on all of them if domain is null)
      // also handle 'secure' flag in a similar way
      
      // default cookie expiration to never
      cookie.maxAge = cookie.maxAge || -1;
      _client.setCookie(cookie);
   };
   
   /**
    * Gets a cookie.
    * 
    * @param name the name of the cookie.
    * 
    * @return the cookie or null if not found.
    */
   xhrApi.getCookie = function(name)
   {
      return _client.getCookie(name);
   };
   
   /**
    * Removes a cookie.
    * 
    * @param name the name of the cookie.
    * 
    * @return true if a cookie was removed, false if not.
    */
   xhrApi.removeCookie = function(name)
   {
      return _client.removeCookie(name);
   };
   
   /**
    * Creates a new XmlHttpRequest.
    * 
    * @param options:
    *        logWarningOnError: If true and an HTTP error status code is
    *           received then log a warning, otherwise log a verbose
    *           message.
    *        verbose: If true be very verbose in the output including
    *           the response event and response body, otherwise only include
    *           status, timing, and data size.
    *        logError: a multi-var log function for warnings that takes
    *           the log category as the first var.
    *        logWarning: a multi-var log function for warnings that takes
    *           the log category as the first var.
    *        logDebug: a multi-var log function for warnings that takes
    *           the log category as the first var.
    *        logVerbose: a multi-var log function for warnings that takes
    *           the log category as the first var.
    * 
    * @return the XmlHttpRequest.
    */
   xhrApi.create = function(options)
   {
      // set option defaults
      options = $.extend(
      {
         logWarningOnError: true,
         verbose: false,
         logError: function(){},
         logWarning: function(){},
         logDebug: function(){},
         logVerbose: function(){}
      }, options || {});
      
      // private xhr state
      var _state =
      {
         // request storage
         request: null,
         // response storage
         response: null,
         // asynchronous, true if doing asynchronous communication
         asynchronous: true,
         // sendFlag, true if send has been called
         sendFlag: false,
         // errorFlag, true if a network error occurred
         errorFlag: false
      };
      
      // private log functions
      var _log =
      {
         error: options.logError,
         warning: options.logWarning,
         debug: options.logDebug,
         verbose: options.logVerbose
      };
      
      // create public xhr interface
      var xhr =
      {
         // an EventListener
         onreadystatechange: null,
         // readonly, the current readyState
         readyState: UNSENT,
         // a string with the response entity-body
         responseText: '',
         // a Document for response entity-bodies that are XML
         responseXML: null,
         // readonly, returns the HTTP status code (i.e. 404)
         status: 0,
         // readonly, returns the HTTP status message (i.e. 'Not Found')
         statusText: ''
      };
      
      /**
       * Opens the request. This method will create the HTTP request to send.
       * 
       * @param method the HTTP method (i.e. 'GET').
       * @param url the relative url (the HTTP request path).
       * @param async always true, ignored.
       * @param user always null, ignored.
       * @param password always null, ignored.
       */
      xhr.open = function(method, url, async, user, password)
      {
         // 1. validate Document if one is associated
         // TODO: not implemented (not used yet)
         
         // 2. validate method token
         // 3. change method to uppercase if it matches a known
         // method (here we just require it to be uppercase, and
         // we do not allow the standard methods)
         // 4. disallow CONNECT, TRACE, or TRACK with a security error
         switch(method)
         {
            case 'DELETE':
            case 'GET':
            case 'HEAD':
            case 'OPTIONS':
            case 'POST':
            case 'PUT':
               // valid method
               break;
            case 'CONNECT':
            case 'TRACE':
            case 'TRACK':
               // FIXME: handle exceptions
               throw SECURITY_ERR;
               break;
            default:
               // FIXME: handle exceptions
               //throw new SyntaxError('Invalid method: ' + method);
               throw SYNTAX_ERR;
         }
         
         // TODO: other validation steps in algorithm are not implemented
         
         // 19. set send flag to false
         // set response body to null
         // empty list of request headers
         // set request method to given method
         // set request URL
         // set username, password
         // set asychronous flag
         _state.sendFlag = false;
         xhr.responseText = '';
         xhr.responseXML = null;
         
         // custom: reset status and statusText
         xhr.status = 0;
         xhr.statusText = '';
         
         // create the HTTP request
         _state.request = http.createRequest({
            method: method,
            path: url
         });
         
         // 20. set state to OPENED
         xhr.readyState = OPENED;
         
         // 21. dispatch onreadystatechange
         if(xhr.onreadystatechange)
         {
            xhr.onreadystatechange();
         }
      };
      
      /**
       * Adds an HTTP header field to the request.
       * 
       * @param header the name of the header field.
       * @param value the value of the header field.
       */
      xhr.setRequestHeader = function(header, value)
      {
         // 1. if state is not OPENED or send flag is true, raise exception
         if(xhr.readyState != OPENED || _state.sendFlag)
         {
            // FIXME: handle exceptions properly
            throw INVALID_STATE_ERR;
         }
         
         // TODO: other validation steps in spec aren't implemented
         
         // set header
         _state.request.setField(header, value);
      };
      
      /**
       * Sends the request and any associated data.
       * 
       * @param data a string or Document object to send, null to send no data.
       */
      xhr.send = function(data)
      {
         // 1. if state is not OPENED or 2. send flag is true, raise
         // an invalid state exception
         if(xhr.readyState != OPENED || _state.sendFlag)
         {
            // FIXME: handle exceptions properly
            throw INVALID_STATE_ERR;
         }
         
         // 3. ignore data if method is GET or HEAD
         if(data &&
            _state.request.method !== 'GET' &&
            _state.request.method !== 'HEAD')
         {
            // handle non-IE case
            if(typeof(XMLSerializer) !== 'undefined')
            {
               if(data instanceof Document)
               {
                  var xs = new XMLSerializer();
                  _state.request.body = xs.serializeToString(data);
               }
               else
               {
                  _state.request.body = data;
               }
            }
            // poorly implemented IE case
            else
            {
               if(typeof(data.xml) !== 'undefined')
               {
                  _state.request.body = xml;
               }
               else
               {
                  _state.request.body = data;
               }
            }
         }
         
         // 4. release storage mutex (not used)
         
         // 5. set error flag to false
         _state.errorFlag = false;
         
         // 6. if asynchronous is true (must be in this implementation)
         
         // 6.1 set send flag to true
         _state.sendFlag = true;
         
         // 6.2 dispatch onreadystatechange
         if(xhr.onreadystatechange)
         {
            xhr.onreadystatechange();
         }
         
         // create send options
         var options = {};
         options.request = _state.request;
         options.headerReady = function(e)
         {
            // headers received
            xhr.readyState = HEADERS_RECEIVED;
            xhr.status = e.response.code;
            xhr.statusText = e.response.message;
            _state.response = e.response;
            if(xhr.onreadystatechange)
            {
               xhr.onreadystatechange();
            }
            if(!_state.response.aborted)
            {
               // now loading body
               xhr.readyState = LOADING;
               if(xhr.onreadystatechange)
               {
                  xhr.onreadystatechange();
               }
            }
         };
         options.bodyReady = function(e)
         {
            xhr.readyState = DONE;
            var ct = e.response.getField('Content-Type');
            // Note: this null/undefined check is done outside because IE
            // dies otherwise on a "'null' is null" error
            if(ct)
            {
               if(ct.indexOf('text/xml') === 0 ||
                  ct.indexOf('application/xml') === 0 ||
                  ct.indexOf('+xml') !== -1)
               {
                  try
                  {
                     var doc = new ActiveXObject('MicrosoftXMLDOM');
                     doc.async = false;
                     doc.loadXML(e.response.body);
                     xhr.responseXML = doc;
                  }
                  catch(ex)
                  {
                     var parser = new DOMParser();
                     xhr.responseXML =
                        parser.parseFromString(ex.body, 'text/xml');
                  }
               }
            }
            var length = 0;
            if(e.response.body !== null)
            {
               xhr.responseText = e.response.body;
               length = e.response.body.length;
            }
            // build logging output
            var req = _state.request;
            var output =
               req.method + ' ' + req.path + ' ' +
               xhr.status + ' ' + xhr.statusText + ' ' +
               length + 'B ' +
               (e.request.connectTime + e.request.time + e.response.time) +
               'ms';
            // TODO: provide log functions via options
            if(options.verbose)
            {
               var lFunc = (xhr.status >= 400 && options.logWarningOnError) ?
                  _log.warning : _log.verbose;
               lFunc(cat, output,
                  e, e.response.body ? '\n' + e.response.body : '\nNo content');
            }
            else
            {
               var lFunc = (xhr.status >= 400 && options.logWarningOnError) ?
                  _log.warning : _log.debug;
               lFunc(cat, output);
            }
            if(xhr.onreadystatechange)
            {
               xhr.onreadystatechange();
            }
         };
         options.error = function(e)
         {
            var req = _state.request;
            _log.error(cat, req.method + ' ' + req.path, e);
            
            // 1. set response body to null
            xhr.responseText = '';
            xhr.responseXML = null;
            
            // 2. set error flag to true (and reset status)
            _state.errorFlag = true;
            xhr.status = 0;
            xhr.statusText = '';
            
            // 3. set state to done
            xhr.readyState = DONE;
            
            // 4. asyc flag is always true, so dispatch onreadystatechange
            if(xhr.onreadystatechange)
            {
               xhr.onreadystatechange();
            }
         };
         
         // 7. send request
         _client.send(options);
      };
      
      /**
       * Aborts the request.
       */
      xhr.abort = function()
      {
         // 1. abort send
         // 2. stop network activity
         _state.request.abort();
         
         // 3. set response to null
         xhr.responseText = '';
         xhr.responseXML = null;
         
         // 4. set error flag to true (and reset status)
         _state.errorFlag = true;
         xhr.status = 0;
         xhr.statusText = '';
         
         // 5. clear user headers
         _state.request = null;
         _state.response = null;
         
         // 6. if state is DONE or UNSENT, or if OPENED and send flag is false
         if(xhr.readyState == DONE || xhr.readyState == UNSENT ||
            (xhr.readyState == OPENED && !_state.sendFlag))
         {
            // 7. set ready state to unsent
            xhr.readyState = UNSENT;
         }
         else
         {
            // 6.1 set state to DONE
            xhr.readyState = DONE;
            
            // 6.2 set send flag to false
            _state.sendFlag = false;
            
            // 6.3 dispatch onreadystatechange
            if(xhr.onreadystatechange)
            {
               xhr.onreadystatechange();
            }
            
            // 7. set state to UNSENT
            xhr.readyState = UNSENT;
         }
      };
      
      /**
       * Gets all response headers as a string.
       * 
       * @return the HTTP-encoded response header fields.
       */
      xhr.getAllResponseHeaders = function()
      {
         var rval = '';
         var fields = _state.response.fields;
         $.each(fields, function(name, array)
         {
            $.each(array, function(i, value)
            {
               rval += name + ': ' + value + '\r\n';
            });
         });
         return rval;
      };
      
      /**
       * Gets a single header field value or, if there are multiple
       * fields with the same name, a comma-separated list of header
       * values.
       * 
       * @return the header field value(s) or null.
       */
      xhr.getResponseHeader = function(header)
      {
         var rval = null;
         if(header in _state.response.fields)
         {
            rval = _state.response.fields[header];
            if(rval.constructor == Array)
            {
               rval = rval.join();
            }
         }
         return rval;
      };
      
      return xhr;
   };
   
   // expose public api
   krypto.xhr = xhrApi;
})(jQuery);

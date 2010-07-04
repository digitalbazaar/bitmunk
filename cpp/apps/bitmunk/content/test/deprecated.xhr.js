/**
 * Bitmunk Web UI --- Task Management Functions
 *
 * @author Dave Longley
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 *
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 */
(function($)
{
   // logging category
   var cat = 'bitmunk.webui.core.Xhr.deprecated';
   
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
   
   // the flash http pool manager
   var sHpm = null;
   
   // the maximum number of concurrent connections to the backend
   var sConnections = 10;
   
   // the pool ID for all https connections
   var sPoolId = 0;
   
   // the next unique request ID
   var sNextRequestId = 1;
   
   // define the xhr interface
   bitmunk.deprecated = {};
   bitmunk.deprecated.xhr =
   {
      // the map of current requests (request ID => request)
      requests: {},
      
      /**
       * Initializes flash XHR support.
       * 
       * @param task the task to use to synchronize initialization.
       */
      init: function(task)
      {
         // get flash config
         var cfg;
         task.block();
         $.ajax(
         {
            type: 'GET',
            url: bitmunk.api.localRoot + 'webui/config/flash',
            contentType: 'application/json',
            success: function(data)
            {
               // get config
               cfg = JSON.parse(data);
               task.unblock();
            },
            error: function()
            {
               task.fail();
            }
         });
         
         task.next(function(task)
         {
            // get the flash interface
            sHpm = document.getElementById('httpPoolManager');
            
            // returns the javascript to pass to flash to handle a request event
            // (when an event occurs in flash, it will execute the javascript)
            var createCallHandler = function(f)
            {
               var rval;
               if($.browser.msie === true)
               {
                  // IE calls javascript on the thread of the external object
                  // (in this case flash) ... which will either run concurrently
                  // with other javascript or pre-empt any running javascript in
                  // the middle of its execution (BAD!) ... calling setTimeout()
                  // will schedule the javascript to run on the javascript thread
                  // and solve this EVIL problem
                  rval =
                     'function(e){setTimeout(' +
                     'function(){bitmunk.deprecated.xhr.requests[e.id].' + f + '(e);},' +
                     '0);}';
               }
               else
               {
                  rval =
                     'function(e){bitmunk.deprecated.xhr.requests[e.id].' + f + '(e);}';
               }
               
               return rval;
            };
            
            // initialize the flash interface
            sHpm.init({
               escapeForJavascript: true,
               marshallExceptions: !$.browser.msie
            });
            
            // create the https pool to window location 
            sPoolId = sHpm.create(
               'https://' + window.location.host, sConnections,
               cfg.policyServer.port, cfg.ssl.certificates, 'localhost');
            
            // subscribe to flash events
            sHpm.subscribe('header',
               createCallHandler('headerEventReceived'));
            sHpm.subscribe('loading',
               createCallHandler('loadingEventReceived'));
            sHpm.subscribe('body',
               createCallHandler('bodyEventReceived'));
            sHpm.subscribe('timeout',
               createCallHandler('timeoutEventReceived'));
            sHpm.subscribe('ioError',
               createCallHandler('ioErrorEventReceived'));
            sHpm.subscribe('securityError',
               createCallHandler('securityErrorEventReceived'));
         });
      },
      
      /**
       * Called to clean up the flash interface.
       */
      cleanup: function()
      {
         sHpm.cleanup();
      },
      
      /**
       * Sets a cookie.
       * 
       * @param name the name of the cookie.
       * @param value the value of the cookie, null to remove the cookie.
       */
      setCookie: function(name, value)
      {
         sHpm.setCookie(sPoolId, name, value);
      },
      
      /**
       * Gets a cookie.
       * 
       * @param name the name of the cookie.
       * 
       * @return the cookie value or null if not found.
       */
      getCookie: function(name)
      {
         return sHpm.getCookie(sPoolId, name);
      },
      
      /**
       * Creates a new bitmunk XmlHttpRequest.
       * 
       * @param options:
       *        logWarningOnError: If true and an HTTP error status code is
       *           received then log a warning, otherwise log a verbose
       *           message.
       * 
       * @return the bitmunk XmlHttpRequest.
       */
      create: function(options)
      {
         // set option defaults
         options = $.extend(
         {
            logWarningOnError: true
         }, options || {});
         
         // stores the request to send to flash
         var hpmRequest = null;
         
         // create storage for all request data
         var request =
         {
            // request header storage
            headers: [],
            // asynchronous, true if doing asynchronous communication
            asynchronous: true,
            // sendFlag, true if send has been called
            sendFlag: false,
            // errorFlag, true if a network error occurred
            errorFlag: false,
            
            // xhr wrapper object
            xhr:
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
               statusText: '',
               
               /**
                * Opens the request. This method will create the request to
                * send to flash.
                * 
                * @param method the HTTP method (i.e. 'GET').
                * @param url the relative url (the HTTP request path).
                * @param async always true, ignored.
                * @param user always null, ignored.
                * @param password always null, ignored.
                */
               open: function(method, url, async, user, password)
               {
                  // 1. validate Document if one is associated
                  // FIXME: not implemented (not used yet)
                  
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
                  
                  // FIXME: other validation steps in algorithm are not
                  // implemented
                  
                  // 19. set send flag to false
                  // set response body to null
                  // empty list of request headers
                  // set request method to given method
                  // set request URL
                  // set username, password
                  // set asychronous flag
                  request.sendFlag = false;
                  request.xhr.responseText = '';
                  request.xhr.responseXML = null;
                  
                  // custom: reset status and statusText
                  request.xhr.status = 0;
                  request.xhr.statusText = '';
                  
                  // create the flash request, update readyState
                  var id = sNextRequestId++;
                  hpmRequest =
                  {
                     method: method,
                     url: url,
                     requestHeaders: request.headers,
                     data: null,
                     id: id
                  };
                  bitmunk.deprecated.xhr.requests[id] = request;
                  
                  // 20. set state to OPENED
                  request.xhr.readyState = OPENED;
                  
                  // 21. dispatch onreadystatechange
                  if(request.xhr.onreadystatechange)
                  {
                     request.xhr.onreadystatechange();
                  }
               },
               
               /**
                * Adds an HTTP header field to the request.
                * 
                * @param header the name of the header field.
                * @param value the value of the header field.
                */
               setRequestHeader: function(header, value)
               {
                  // 1. if state is not OPENED or send flag is true, raise
                  // an exception
                  if(request.xhr.readyState != OPENED || request.sendFlag)
                  {
                     // FIXME: handle exceptions properly
                     throw INVALID_STATE_ERR;
                  }
                  
                  // FIXME: other validation steps in spec aren't implemented
                  
                  // add header
                  request.headers.push({name: header, value: value});
               },
               
               /**
                * Sends the request and any associated data.
                * 
                * @param data a string or Document object to send, or null to
                *             send no data.
                */
               send: function(data)
               {
                  // 1. if state is not OPENED or 2. send flag is true, raise
                  // an invalid state exception
                  if(request.xhr.readyState != OPENED || request.sendFlag)
                  {
                     // FIXME: handle exceptions properly
                     throw INVALID_STATE_ERR;
                  }
                  
                  // 3. ignore data if method is GET or HEAD
                  if(data &&
                     hpmRequest.method != 'GET' &&
                     hpmRequest.method != 'HEAD')
                  {
                     // handle non-IE case
                     if(typeof XMLSerializer != 'undefined')
                     {
                        if(data instanceof Document)
                        {
                           var xs = new XMLSerializer();
                           hpmRequest.data = xs.serializeToString(data);
                        }
                        else
                        {
                           hpmRequest.data = data;
                        }
                     }
                     // poorly implemented IE case
                     else
                     {
                        if(data.xml !== undefined)
                        {
                           hpmRequest.data = xml;
                        }
                        else
                        {
                           hpmRequest.data = data;
                        }
                     }
                  }
                  
                  // 4. release storage mutex (not used)
                  
                  // 5. set error flag to false
                  request.errorFlag = false;
                  
                  // 6. if asynchronous is true (must be in this implementation)
                  
                  // 6.1 set send flag to true
                  request.sendFlag = true;
                  
                  // 6.2 dispatch onreadystatechange
                  if(request.xhr.onreadystatechange)
                  {
                     request.xhr.onreadystatechange();
                  }
                  
                  // 7. queue request with flash
                  sHpm.queue(sPoolId, hpmRequest, hpmRequest.id);
               },
               
               /**
                * Aborts the request.
                */
               abort: function()
               {
                  // 1/2. abort send by ignoring events from flash
                  delete bitmunk.deprecated.xhr.requests[hpmRequest.id];
                  
                  // 3. set response to null
                  request.xhr.responseText = '';
                  request.xhr.responseXML = null;
                  
                  // 4. set error flag to true (and reset status)
                  request.errorFlag = true;
                  request.xhr.status = 0;
                  request.xhr.statusText = '';
                  
                  // 5. clear error flag and user headers
                  request.xhr.headers = [];                  
                  
                  // 6. if state is DONE, or if send flag is false
                  if(request.xhr.readyState == DONE || request.sendFlag)
                  {
                     // 7. set ready state to unsent
                     request.xhr.readyState = UNSENT;
                  }
                  else
                  {
                     // 6.1 set state to DONE
                     request.xhr.readyState = DONE;
                     
                     // 6.2 set send flag to false
                     request.sendFlag = false;
                     
                     // 6.3 dispatch onreadystatechange
                     if(request.xhr.onreadystatechange)
                     {
                        request.xhr.onreadystatechange();
                     }
                     
                     // 7. set state to UNSENT
                     request.xhr.readyState = UNSENT;
                  }
                  
                  // clean up flash request
                  hpmRequest = null;
               },
               
               /**
                * Gets all response headers as a string.
                * 
                * @return the HTTP-encoded response header fields.
                */
               getAllResponseHeaders: function()
               {
                  var rval = '';
                  $.each(request.headers, function(i, header)
                  {
                     rval += header.name + ': ' + header.value + '\r\n';
                  });
                  return rval;
               },
               
               /**
                * Gets a single header field value or, if there are multiple
                * fields with the same name, a comma-separated list of header
                * values.
                * 
                * @return the header field value(s) or null.
                */
               getResponseHeader: function(header)
               {
                  var rval = null;
                  $.each(request.headers, function(i, aHeader)
                  {
                     if(aHeader.name.toLowerCase() == header.toLowerCase())
                     {
                        if(rval)
                        {
                           rval += ', ' + aHeader.value;
                        }
                        else
                        {
                           rval = aHeader.value;
                        }
                     }
                  });
                  return rval;
               }
            },
            
            /**
             * Called when a header event is received from flash.
             * 
             * @param e the event.
             */
            headerEventReceived: function(e)
            {
               request.xhr.readyState = HEADERS_RECEIVED;
               request.xhr.status = e.response.statusCode;
               request.xhr.statusText = e.response.statusMessage;
               request.headers = e.response.headers;
               if(request.xhr.onreadystatechange)
               {
                  request.xhr.onreadystatechange();
               }
            },
            
            /**
             * Called when a loading event is received from flash.
             * 
             * @param e the event.
             */
            loadingEventReceived: function(e)
            {
               request.xhr.readyState = LOADING;
               if(request.xhr.onreadystatechange)
               {
                  request.xhr.onreadystatechange();
               }
            },
            
            /**
             * Called when a body event is received from flash.
             * 
             * @param e the event.
             */
            bodyEventReceived: function(e)
            {
               request.xhr.readyState = DONE;
               var ct = e.response.contentType;
               // Note: this null/undefined check is done outside because IE
               // dies otherwise on a "'null' is null" error
               if(ct)
               {
                  if(ct.indexOf('text/xml') === 0 ||
                     ct.indexOf('application/xml') === 0 ||
                     ct.indexOf('+xml') != -1)
                  {
                     try
                     {
                        var doc = new ActiveXObject('MicrosoftXMLDOM');
                        doc.async = false;
                        doc.loadXML(e.body);
                        request.xhr.responseXML = doc;
                     }
                     catch(ex)
                     {
                        var parser = new DOMParser();
                        request.xhr.responseXML =
                           parser.parseFromString(ex.body, 'text/xml');
                     }
                  }
               }
               var length = 0;
               if(e.body !== null)
               {
                  request.xhr.responseText = e.body;
                  length = e.body.length;
               }
               delete bitmunk.deprecated.xhr.requests[e.id];
               var logFunction =
                  (request.xhr.status >= 400 && options.logWarningOnError) ?
                  bitmunk.log.warning : bitmunk.log.verbose;
               logFunction(cat,
                  hpmRequest.method + ' ' + hpmRequest.url + ' ' +
                  request.xhr.status + ' ' + request.xhr.statusText + ' ' +
                  length + 'B ' +
                  e.response.time + 'ms',
                  hpmRequest, e, e.body ? '\n' + e.body : '\nNo content');
               if(request.xhr.onreadystatechange)
               {
                  request.xhr.onreadystatechange();
               }
            },
            
            /**
             * Handles a network error.
             * 
             * @param e the related event.
             */
            handleNetworkError: function(e)
            {
               // network error, stop other events
               delete bitmunk.deprecated.xhr.requests[e.id];
               
               // 1. set response body to null
               request.xhr.responseText = '';
               request.xhr.responseXML = null;
               
               // 2. set error flag to true (and reset status)
               request.errorFlag = true;
               request.xhr.status = 0;
               request.xhr.statusText = '';
               
               // 3. set state to done
               request.xhr.readyState = DONE;
               
               // 4. asyc flag is always true, so dispatch onreadystatechange
               if(request.xhr.onreadystatechange)
               {
                  request.xhr.onreadystatechange();
               }
            },
            
            /**
             * Called when a timeout event is received from flash.
             * 
             * @param e the event.
             */
            timeoutEventReceived: function(e)
            {
               bitmunk.log.error(cat,
                  hpmRequest.method + ' ' + hpmRequest.url, hpmRequest, e);
               
               // handle network error
               bitmunk.deprecated.xhr.requests[e.id].handleNetworkError(e);
            },
            
            /**
             * Called when an IO error event is received from flash.
             * 
             * @param e the event.
             */
            ioErrorEventReceived: function(e)
            {
               bitmunk.log.error(cat,
                  hpmRequest.method + ' ' + hpmRequest.url, hpmRequest, e);
               
               // handle network error
               bitmunk.deprecated.xhr.requests[e.id].handleNetworkError(e);
            },
            
            /**
             * Called when a security error event is received from flash.
             * 
             * @param e the event.
             */
            securityErrorEventReceived: function(e)
            {
               bitmunk.log.error(cat,
                  hpmRequest.method + ' ' + hpmRequest.url, hpmRequest, e);
               
               // handle network error
               bitmunk.deprecated.xhr.requests[e.id].handleNetworkError(e);
            }
         };
         
         return request.xhr;
      }
   };
   
   // NOTE: xhr support is implicit and not a required dependency
   // plugin registered after registration code is initialized
   bitmunk._implicitPluginInfo.task =
   {
      pluginId: 'bitmunk.webui.core.Xhr.deprecated',
      name: 'Bitmunk Deprecated Xhr'
   };
})(jQuery);

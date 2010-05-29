/**
 * Socket implementation that uses flash SocketPool class as a backend.
 *
 * @author Dave Longley
 *
 * Copyright (c) 2010 Digital Bazaar, Inc. All rights reserved.
 */
(function()
{
   // Note: Current implementation supports 1 flash socket pool per page. This
   // can be changed by storing net information in a map using flash IDs. It
   // also requires specifying the flash ID when creating sockets, etc.
   
   // private flash socket pool vars
   var _sp = null;
   var _defaultPolicyPort;
   
   // define net namespace
   var net =
   {
      // map of socket ID to sockets
      sockets: {}
   };
   
   /**
    * Initializes flash socket support.
    * 
    * @param options:
    *           flashId: the dom ID for the flash object element.
    *           policyPort: the default policy port for sockets.
    *           msie: true if the browser is msie, false if not.
    */
   net.init = function(options)
   {
      // defaults
      _defaultPolicyPort = options.policyPort || 19845;
      options.msie = options.msie || false;
      
      // initialize the flash interface
      _sp = document.getElementById(options.flashId);
      _sp.init({marshallExceptions: !options.msie});
      
      // create event handler, subscribe to flash events
      if(options.msie === true)
      {
         net.handler = function(e)
         {
            if(e.id in net.sockets)
            {
               // get handler function
               var f;
               switch(e.type)
               {
                  case 'connect':
                     f = 'connected';
                     break;
                  case 'close':
                     f = 'closed';
                     break;
                  case 'socketData':
                     f = 'data';
                     break;
                  default:
                     f = 'error';
                     break;
               }
               /* IE calls javascript on the thread of the external object
                  that triggered the event (in this case flash) ... which will
                  either run concurrently with other javascript or pre-empt any
                  running javascript in the middle of its execution (BAD!) ...
                  calling setTimeout() will schedule the javascript to run on
                  the javascript thread and solve this EVIL problem. */
               setTimeout(function(){net.sockets[e.id][f](e)}, 0);
            }
         };
      }
      else
      {
         net.handler = function(e)
         {
            if(e.id in net.sockets)
            {
               console.log('event type', e.type);
               // get handler function
               var f;
               switch(e.type)
               {
                  case 'connect':
                     f = 'connected';
                     break;
                  case 'close':
                     f = 'closed';
                     break;
                  case 'socketData':
                     f = 'data';
                     break;
                  default:
                     f = 'error';
                     break;
               }
               net.sockets[e.id][f](e);
            }
         };
      }
      var handler = 'window.krypto.net.handler';
      _sp.subscribe('connect', handler);
      _sp.subscribe('close', handler);
      _sp.subscribe('socketData', handler);
      _sp.subscribe('ioError', handler);
      _sp.subscribe('securityError', handler);
   };
   
   /**
    * Cleans up flash socket support.
    */
   net.cleanup = function()
   {
      for(var id in net.sockets)
      {
         net.sockets[id].destroy();
      }
      _sp.cleanup();
   };

   /**
    * Creates a new socket.
    * 
    * @param options:
    *           connected: function(event) called when the socket connects.
    *           closed: function(event) called when the socket closes.
    *           data: function(event) called when socket data arrives.
    *           error: function(event) called when a socket error occurs.
    */
   net.createSocket = function(options)
   {
      // create flash socket 
      var id = _sp.create();
      
      // create javascript socket wrapper
      var socket =
      {
         // set handlers
         connected: options.connected || function(e){},
         closed: options.closed || function(e){},
         data: options.data || function(e){},
         error: options.error || function(e){}
      };
      
      /**
       * Connects this socket.
       * 
       * @param options:
       *           host: the host to connect to.
       *           port: the port to connec to.
       *           policyPort: the policy port to use (if non-default). 
       */
      socket.connect = function(options)
      {
         _sp.connect(
            id,
            options.host,
            options.port,
            options.policyPort || _defaultPolicyPort);
      };
      
      /**
       * Closes this socket.
       */
      socket.close = function()
      {
         _sp.close(id);
      };
      
      /**
       * Writes bytes to this socket.
       * 
       * @param bytes the bytes (as a string) to write.
       */
      socket.send = function(bytes)
      {
         _sp.send(id, bytes);
      };
      
      /**
       * Destroys this socket.
       */
      socket.destroy = function()
      {
         _sp.destroy(id);
         delete net.sockets[id];
      };
      
      // store and return socket
      net.sockets[id] = socket;
      return socket;
   };
   
   // public access to net namespace
   window.krypto = window.krypto || {};
   window.krypto.net = net;
})();

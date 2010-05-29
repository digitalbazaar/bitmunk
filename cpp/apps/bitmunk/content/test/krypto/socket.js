/**
 * Socket implementation that uses flash SocketPool class as a backend.
 *
 * @author Dave Longley
 *
 * Copyright (c) 2010 Digital Bazaar, Inc. All rights reserved.
 */
(function()
{
   // define net namespace
   var net =
   {
      // map of flash ID to socket pool
      socketPools: {}
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
      // set default
      options.msie = options.msie || false;
      
      // initialize the flash interface
      var id = options.flashId;
      var api = document.getElementById(id);
      api.init({marshallExceptions: !options.msie});
      
      // create socket pool entry
      var sp =
      {
         // flash interface
         flashApi: api,
         // map of socket ID to sockets
         sockets: {},
         // default policy port
         policyPort: options.policyPort || 19845
      };
      net.socketPools[id] = sp;
      
      // create event handler, subscribe to flash events
      if(options.msie === true)
      {
         sp.handler = function(e)
         {
            if(e.id in sp.sockets)
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
               setTimeout(function(){sp.sockets[e.id][f](e)}, 0);
            }
         };
      }
      else
      {
         sp.handler = function(e)
         {
            if(e.id in sp.sockets)
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
               sp.sockets[e.id][f](e);
            }
         };
      }
      var handler = 'window.krypto.net.socketPools[\'' + id + '\'].handler';
      api.subscribe('connect', handler);
      api.subscribe('close', handler);
      api.subscribe('socketData', handler);
      api.subscribe('ioError', handler);
      api.subscribe('securityError', handler);
   };
   
   /**
    * Cleans up flash socket support.
    * 
    * @param options:
    *           flashId: the dom ID for the flash object element.
    */
   net.cleanup = function(options)
   {
      var sp = net.socketPools[options.flashId];
      delete net.socketPools[options.flashId];
      for(var id in sp.sockets)
      {
         sp.sockets[id].destroy();
      }
      sp.flashApi.cleanup();
   };

   /**
    * Creates a new socket.
    * 
    * @param options:
    *           flashId: the dom ID for the flash object element.
    *           connected: function(event) called when the socket connects.
    *           closed: function(event) called when the socket closes.
    *           data: function(event) called when socket data has arrived, it
    *              can be read from the socket using receive().
    *           error: function(event) called when a socket error occurs.
    */
   net.createSocket = function(options)
   {
      // get related socket pool and flash API
      var sp = net.socketPools[options.flashId];
      var api = sp.flashApi;
      
      // create flash socket 
      var id = api.create();
      
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
         api.connect(
            id, options.host, options.port,
            options.policyPort || sp.policyPort);
      };
      
      /**
       * Closes this socket.
       */
      socket.close = function()
      {
         api.close(id);
      };
      
      /**
       * Determines if the socket is connected or not.
       * 
       * @return true if connected, false if not.
       */
      socket.isConnected = function()
      {
         return api.isConnected(id);
      };
      
      /**
       * Writes bytes to this socket.
       * 
       * @param bytes the bytes (as a string) to write.
       */
      socket.send = function(bytes)
      {
         api.send(id, bytes);
      };
      
      /**
       * Reads bytes from this socket (non-blocking). Fewer than the number
       * of bytes requested may be read if enough bytes are not available.
       * 
       * This method should be called from the data handler if there are
       * enough bytes available. To see how many bytes are available, check
       * the 'bytesAvailable' property of the event passed to the data handler.
       * 
       * @param count the maximum number of bytes to read.
       * 
       * @return the bytes read (as a string) or null on error.
       */
      socket.receive = function(count)
      {
         return api.receive(id, count);
      };
      
      /**
       * Destroys this socket.
       */
      socket.destroy = function()
      {
         api.destroy(id);
         delete sp.sockets[id];
      };
      
      // store and return socket
      sp.sockets[id] = socket;
      return socket;
   };
   
   // public access to net namespace
   window.krypto = window.krypto || {};
   window.krypto.net = net;
})();

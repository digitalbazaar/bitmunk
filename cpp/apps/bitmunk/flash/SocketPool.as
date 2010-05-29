/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 * 
 * @author Dave Longley
 */
package
{
   import flash.display.Sprite;
   
   /**
    * A SocketPool is a flash object that can be embedded in a webpage to
    * allow javascript access to pools of Sockets.
    * 
    * Javascript can create a pool and then as many Sockets as it desires. Each
    * Socket will be assigned a unique ID that allows continued javascript
    * access to it. There is no limit on the number of persistent socket
    * connections.
    */
   public class SocketPool extends Sprite
   {
      import flash.errors.IOError;
      import flash.events.Event;
      import flash.events.IOErrorEvent;
      import flash.events.ProgressEvent;
      import flash.events.SecurityErrorEvent;
      import flash.events.EventDispatcher;
      import flash.external.ExternalInterface;
      import flash.system.Security;
      import flash.utils.ByteArray;
      
      // a map of ID to Socket
      private var mSocketMap:Object;
      
      // a counter for Socket IDs (Note: assumes there will be no overflow)
      private var mNextId:uint;
      
      // an event dispatcher for sending events to javascript
      private var mEventDispatcher:EventDispatcher;
      
      /**
       * Creates a new, unitialized SocketPool.
       * 
       * @throws Error - if no external interface is available to provide
       *                 javascript access.
       */
      public function SocketPool()
      {
         if(!ExternalInterface.available)
         {
            trace("ExternalInterface is not available");
            throw new Error(
               "Flash's ExternalInterface is not available. This is a " +
               "requirement of SocketPool and therefore, it will be " +
               "unavailable.");
         }
         else
         {
            try
            {
               // set up javascript access:
               
               // initializes/cleans up the SocketPool
               ExternalInterface.addCallback("init", init);
               ExternalInterface.addCallback("cleanup", cleanup);
               
               // creates/destroys a socket
               ExternalInterface.addCallback("create", create);
               ExternalInterface.addCallback("destroy", destroy);
               
               // connects/closes a socket
               ExternalInterface.addCallback("connect", connect);
               ExternalInterface.addCallback("close", close);
               
               // sends data over the socket
               ExternalInterface.addCallback("send", send);
               
               // add a callback for subscribing to socket events
               ExternalInterface.addCallback("subscribe", subscribe);
               
               // socket pool is now ready
               ExternalInterface.call("sp.ready");
            }
            catch(e:Error)
            {
               log("error=" + e.errorID + "," + e.name + "," + e.message);
               throw e;
            }
            
            log("ready");
         }
      }
      
      /**
       * A debug logging function.
       * 
       * @param obj the string or error to log.
       */
      CONFIG::debugging
      private function log(obj:Object):void
      {
         if(obj is String)
         {
            var str:String = obj as String;
            ExternalInterface.call("console.log", "SocketPool", str);
         }
         else if(obj is Error)
         {
            var e:Error = obj as Error;
            log("error=" + e.errorID + "," + e.name + "," + e.message);
         }
      }
      
      CONFIG::release
      private function log(obj:Object):void
      {
         // log nothing in release mode
      }
      
      /**
       * Called by javascript to initialize this SocketPool.
       * 
       * @param options:
       *        marshallExceptions: true to pass exceptions to and from
       *           javascript.
       */
      private function init(... args):void
      {
         log("init()");
         
         // get options from first argument
         var options:Object = args.length > 0 ? args[0] : null;
         
         // create socket map, set next ID, and create event dispatcher
         mSocketMap = new Object();
         mNextId = 1;
         mEventDispatcher = new EventDispatcher();
         
         // enable marshalling exceptions if appropriate
         if(options != null &&
            "marshallExceptions" in options &&
            options.marshallExceptions === true)
         {
            try
            {
               // Note: setting marshallExceptions in IE, even inside of a
               // try-catch block will terminate flash. Don't set this on IE.
               ExternalInterface.marshallExceptions = true;
            }
            catch(e:Error)
            {
               log(e);
            }
         }
         
         log("init() done");
      }
      
      /**
       * Called by javascript to clean up a SocketPool.
       */
      private function cleanup():void
      {
         log("cleanup()");
         
         mSocketMap = null;
         mNextId = 1;
         mEventDispatcher = null;
         
         log("cleanup() done");
      }
      
      /**
       * Handles events.
       * 
       * @param e the event to handle.
       */
      private function handleEvent(e:Event):void
      {
         // dispatch socket event
         mEventDispatcher.dispatchEvent(
            new SocketEvent(e.type, e.target as PooledSocket));
      }
      
      /**
       * Called by javascript to create a Socket.
       * 
       * @return the Socket ID.
       */
      private function create():String
      {
         log("create()");
         
         // create a Socket
         var id:String = "" + mNextId++;
         var s:PooledSocket = new PooledSocket();
         s.id = id;
         s.addEventListener(Event.CONNECT, handleEvent);
         s.addEventListener(Event.CLOSE, handleEvent);
         s.addEventListener(ProgressEvent.SOCKET_DATA, handleEvent);
         s.addEventListener(IOErrorEvent.IO_ERROR, handleEvent);
         s.addEventListener(SecurityErrorEvent.SECURITY_ERROR, handleEvent);
         mSocketMap[id] = s;
         
         log("socket " + id + " created");
         log("create() done");
         
         return id;
      }
      
      /**
       * Called by javascript to clean up a Socket.
       * 
       * @param id the ID of the Socket to clean up.
       */
      private function destroy(id:String):void
      {
         log("destroy(" + id + ")");
         
         if(id in mSocketMap)
         {
            // remove Socket
            delete mSocketMap[id];
            log("socket " + id + " destroyed");
         }
         
         log("destroy(" + id + ") done");
      }
      
      /**
       * Connects the Socket with the given ID to the given host and port,
       * using the given socket policy port.
       *
       * @param id the ID of the Socket.
       * @param host the host to connect to.
       * @param port the port to connect to.
       * @param spPort the security policy port to use.
       */
      private function connect(
         id:String, host:String, port:uint, spPort:uint):void
      {
         log("connect(" + id + "," + host + "," + port + "," + spPort + ")");
         
         if(id in mSocketMap)
         {
            // get the Socket
            var s:PooledSocket = mSocketMap[id];
            
            // load socket policy file
            // (permits socket access to backend)
            Security.loadPolicyFile("xmlsocket://" + host + ":" + spPort);
            
            // connect
            s.connect(host, port);
         }
         else
         {
            // no such socket
            log("socket " + id + " does not exist");
         }
         
         log("connect(" + id + ") done");
      }
      
      /**
       * Closes the Socket with the given ID.
       *
       * @param id the ID of the Socket.
       */
      private function close(id:String):void
      {
         log("close(" + id + ")");
         
         if(id in mSocketMap)
         {
            // close the Socket
            var s:PooledSocket = mSocketMap[id];
            if(s.connected)
            {
               s.close();
            }
         }
         else
         {
            // no such socket
            log("socket " + id + " does not exist");
         }
         
         log("close(" + id + ") done");
      }
      
      /**
       * Writes bytes to a Socket.
       *
       * @param id the ID of the Socket.
       * @param bytes the string of bytes to write.
       */
      private function send(id:String, bytes:String):void
      {
         log("send(" + id + ")");
         
         if(id in mSocketMap)
         {
         	// write bytes to socket
            var s:PooledSocket = mSocketMap[id];
            s.writeUTFBytes(bytes);
            s.flush();
         }
         else
         {
            // no such socket
            log("socket " + id + " does not exist");
         }
         
         log("send(" + id + ") done");
      }
      
      /**
       * Registers a javascript function as a callback for an event.
       * 
       * @param eventType the type of event (HttpConnectionEvent types).
       * @param callback the name of the callback function.
       */
      private function subscribe(eventType:String, callback:String):void
      {
         log("subscribe(" + eventType + "," + callback + ")");
         
         switch(eventType)
         {
            case Event.CONNECT:
            case Event.CLOSE:
            case IOErrorEvent.IO_ERROR:
            case SecurityErrorEvent.SECURITY_ERROR:
            {
               log(eventType + " => " + callback);
               mEventDispatcher.addEventListener(
                  eventType, function(event:SocketEvent):void
               {
                  log("event dispatched: " + eventType);
                  
                  // build event for javascript
                  var e:Object = new Object();
                  e.id = event.socket.id;
                  e.type = eventType;
                  
                  // send event to javascript
                  ExternalInterface.call(callback, e);
               });
               break;
            }
            case ProgressEvent.SOCKET_DATA:
            {
               log(eventType + " => " + callback);
               mEventDispatcher.addEventListener(
                  eventType, function(event:SocketEvent):void
               {
                  log("event dispatched: " + eventType);
                  
                  try
                  {
                  	// FIXME: need a system where socket owner can tell
                  	// a socket to buffer X number of bytes before reporting
                  	// that the data has been received (else do timeout)
                  	// FIXME: this will require a buffer to be stored along
                  	// with the socket ... or maybe use the internal buffer
                  	// and wait for bytesAvailable to be enough?
                     
                     // read all available bytes
                     var e:Object = new Object();
                     e.id = event.socket.id;
                     e.type = eventType;
                     e.bytes = event.socket.readUTFBytes(
                        event.socket.bytesAvailable);
                     
                     // send to javascript
                     ExternalInterface.call(callback, e);
                  }
                  catch(e:IOError)
                  {
                     log(e);
                     
                     // dispatch IO error event
                     mEventDispatcher.dispatchEvent(new IOErrorEvent(
                        IOErrorEvent.IO_ERROR,
                        false, false, e.message));
                     if(event.socket.connected)
                     {
                        event.socket.close();
                     }
                  }
               });
               break;
            }
            default:
               throw new ArgumentError(
                  "Could not subscribe to event. " +
                  "Invalid event type specified: " + eventType);
         }
         
         log("subscribe(" + eventType + "," + callback + ") done");
      }
   }
}

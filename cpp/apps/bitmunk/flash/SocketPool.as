/*
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 * 
 * @author Dave Longley
 */
package
{
   import flash.display.Sprite;
   
   /**
    * A SocketPool is a flash object that can be embedded in a web page to
    * allow javascript access to pools of Sockets.
    * 
    * Javascript can create a pool and then as many Sockets as it desires. Each
    * Socket will be assigned a unique ID that allows continued javascript
    * access to it. There is no limit on the number of persistent socket
    * connections.
    */
   public class SocketPool extends Sprite
   {
      import flash.events.Event;
      import flash.events.EventDispatcher;
	  import flash.errors.IOError;
      import flash.events.IOErrorEvent;
      import flash.events.ProgressEvent;
      import flash.events.SecurityErrorEvent;
      import flash.events.TextEvent;
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
               
               // checks for a connection
               ExternalInterface.addCallback("isConnected", isConnected);
               
               // sends/receives data over the socket
               ExternalInterface.addCallback("send", send);
               ExternalInterface.addCallback("receive", receive);
               
               // gets the number of bytes available on a socket
               ExternalInterface.addCallback(
                  "getBytesAvailable", getBytesAvailable);
               
               // add a callback for subscribing to socket events
               ExternalInterface.addCallback("subscribe", subscribe);
               
               // add callbacks for deflate/inflate
               ExternalInterface.addCallback("deflate", deflate);
               ExternalInterface.addCallback("inflate", inflate);
               
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
         var message:String = (e is TextEvent) ? (e as TextEvent).text : null;
         mEventDispatcher.dispatchEvent(
            new SocketEvent(e.type, e.target as PooledSocket, message));
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
       * Determines if the Socket with the given ID is connected or not.
       *
       * @param id the ID of the Socket.
       *
       * @return true if the socket is connected, false if not.
       */
      private function isConnected(id:String):Boolean
      {
         var rval:Boolean = false;
         log("isConnected(" + id + ")");
         
         if(id in mSocketMap)
         {
            // check the Socket
            var s:PooledSocket = mSocketMap[id];
            rval = s.connected;
         }
         else
         {
            // no such socket
            log("socket " + id + " does not exist");
         }
         
         log("isConnected(" + id + ") done");
         return rval;
      }
      
      /**
       * Writes bytes to a Socket.
       *
       * @param id the ID of the Socket.
       * @param bytes the string of bytes to write.
       *
       * @return true on success, false on failure. 
       */
      private function send(id:String, bytes:String):Boolean
      {
         var rval:Boolean = false;
         log("send(" + id + ")");
         
         if(id in mSocketMap)
         {
         	// write bytes to socket
            var s:PooledSocket = mSocketMap[id];
            try
            {
               s.writeUTFBytes(bytes);
               s.flush();
               rval = true;
            }
            catch(e:IOError)
            {
               log(e);
               
               // dispatch IO error event
               mEventDispatcher.dispatchEvent(new SocketEvent(
                  IOErrorEvent.IO_ERROR, s, e.message));
               if(s.connected)
               {
                  s.close();
               }
            }
         }
         else
         {
            // no such socket
            log("socket " + id + " does not exist");
         }
         
         log("send(" + id + ") done");
         return rval;
      }
      
      /**
       * Receives bytes from a Socket.
       *
       * @param id the ID of the Socket.
       * @param count the maximum number of bytes to receive.
       *
       * @return the received bytes, null on error.
       */
      private function receive(id:String, count:uint):String
      {
      	 var rval:String = null;
         log("receive(" + id + "," + count + ")");
         
         if(id in mSocketMap)
         {
         	// only read what is available
            var s:PooledSocket = mSocketMap[id];
            if(count > s.bytesAvailable)
            {
               count = s.bytesAvailable;
            }
            
            try
            {
               // read bytes from socket
               rval = s.readUTFBytes(count);
            }
            catch(e:IOError)
            {
               log(e);
               
               // dispatch IO error event
               mEventDispatcher.dispatchEvent(new SocketEvent(
                  IOErrorEvent.IO_ERROR, s, e.message));
               if(s.connected)
               {
                  s.close();
               }
            }
         }
         else
         {
            // no such socket
            log("socket " + id + " does not exist");
         }
         
         log("receive(" + id + "," + count + ") done");
         return rval;
      }
      
      /**
       * Gets the number of bytes available from a Socket.
       *
       * @param id the ID of the Socket.
       *
       * @return the number of available bytes.
       */
      private function getBytesAvailable(id:String):uint
      {
         var rval:uint = 0;
         log("getBytesAvailable(" + id + ")");
         
         if(id in mSocketMap)
         {
            var s:PooledSocket = mSocketMap[id];
            rval = s.bytesAvailable;
         }
         else
         {
            // no such socket
            log("socket " + id + " does not exist");
         }
         
         log("getBytesAvailable(" + id +") done");
         return rval;
      }      
      
      /**
       * Registers a javascript function as a callback for an event.
       * 
       * @param eventType the type of event (socket event types).
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
            case ProgressEvent.SOCKET_DATA:
            {
               log(eventType + " => " + callback);
               mEventDispatcher.addEventListener(
                  eventType, function(event:SocketEvent):void
               {
                  log("event dispatched: " + eventType);
                  
                  // build event for javascript
                  var e:Object = new Object();
                  e.id = event.socket ? event.socket.id : 0;
                  e.type = eventType;
                  if(event.socket && event.socket.connected)
                  {
                     e.bytesAvailable = event.socket.bytesAvailable;
                  }
                  else
                  {
                     e.bytesAvailable = 0;
                  }
                  if(event.message)
                  {
                     e.message = event.message;
                  }
                  
                  // send event to javascript
                  ExternalInterface.call(callback, e);
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
      
      /**
       * Deflates the given data.
       * 
       * @param data the data to deflate.
       * 
       * @return the deflated data.
       */
      private function deflate(data:String):String
      {
         var b:ByteArray = new ByteArray();
         b.writeUTFBytes(data);
         b.compress();
         b.position = 0;
         return b.readUTFBytes(b.length);
      }
      
      /**
       * Inflates the given data.
       * 
       * @param data the data to inflate.
       * 
       * @return the inflated data.
       */
      private function inflate(data:String):String
      {
         var b:ByteArray = new ByteArray();
         b.writeUTFBytes(data);
         b.uncompress();
         b.position = 0;
         return b.readUTFBytes(b.length);
      }
   }
}

/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 * 
 * @author Dave Longley
 */
package db.net
{
   import flash.events.EventDispatcher;
   
   /**
    * An HttpConnection is uses a socket to communicate with an http server.
    * 
    * The following events are dispatched:
    * 
    * HttpConnectionEvent.OPEN    : connection is open and request will be sent.
    * HttpConnectionEvent.REQUEST : http request has been sent.
    * HttpConnectionEvent.HEADERS : http response headers are received.
    * HttpConnectionEvent.LOADING : http entity-body is being received.
    * HttpConnectionEvent.BODY    : http entity-body has been received.
    * HttpConnectionEvent.TIMEOUT : a receive timeout occurred.
    * 
    * IOErrorEvent.IO_ERROR             : an IO error occurred.
    * SecurityErrorEvent.SECURITY_ERROR : a security error occurred.
    */
   public class HttpConnection extends EventDispatcher
   {
      import com.hurlant.crypto.tls.TLSConfig;
      import com.hurlant.crypto.tls.TLSEngine;
      import com.hurlant.crypto.tls.TLSSocket;
      
      import db.net.HttpConnectionEvent;
      import db.net.HttpRequest;
      import db.net.HttpResponse;
      import db.net.Url;
      
      import flash.errors.IOError;
      import flash.events.Event;
      import flash.events.IEventDispatcher;
      import flash.events.IOErrorEvent;
      import flash.events.ProgressEvent;
      import flash.events.SecurityErrorEvent;
      import flash.events.TimerEvent;
      import flash.net.Socket;
      import flash.net.URLRequestHeader;
      import flash.net.URLRequestMethod;
      import flash.net.URLVariables;
      import flash.utils.ByteArray;
      import flash.utils.IDataInput;
      import flash.utils.IDataOutput;
      import flash.utils.Timer;
      import mx.utils.StringUtil;
      import mx.utils.URLUtil;
      
      // Note: debug-only, see ExternalInterface.call("console.log", ...)
      //import flash.external.ExternalInterface;
      
      // a reference to the non-TLS socket, if in use
      private var mSocket:Socket;
      
      // a reference to the TLS socket, if in use
      private var mTlsSocket:TLSSocket;
      // a temporary socket to use with the TLS socket
      // since their current implementation requires a
      // socket to be connected manually before starting
      // its TLS engine
      private var _socket:Socket;
      
      // the url in use
      private var mUrl:Url;
      
      // set to true once this connection is considered "open",
      // meaning it is either connected or about to connect
      private var mOpen:Boolean;
      
      // a queue of HttpResponses with associated requests to send
      // it was more convenient to just store the unique ID with the
      // HttpResponse object when requests were queued, rather than
      // create another object just to wrap an HttpRequest and an ID,
      // therefore this array stores HttpResponses with their request
      // property set to the request that is to be sent
      private var mRequestQueue:Array;
      
      // a buffer of received, but unparsed data
      private var mBuffer:ByteArray;
      // current read position for the receive buffer
      private var mReadPosition:uint;
      
      // the current HttpResponse
      private var mResponse:HttpResponse;
      
      // response entity-body receive modes
      private const RECV_CONTENT_LENGTH:int = 1;
      private const RECV_CHUNKED:int = 2;
      private var mReceiveBodyMode:int;
      
      // the remaining bytes in the current body chunk
      private var mChunkSize:int;
      // true once last chunk has been read
      private var mChunksFinished:Boolean;
      
      // a timer used to implement the receive timeout
      private var mReceiveTimeout:Timer;
      
      // character codes for "\r" and "\n"
      private static const CR:uint = String("\r").charCodeAt(0);
      private static const LF:uint = String("\n").charCodeAt(0);
      
      // the TLSConfig to use when connecting
      public var tlsconfig:TLSConfig;
      
      // an alternative X.509 subject common name to use when
      // checking SSL certificates
      public var verifyCommonName:String;
      
      // a pool ID if this connection is part of a pool
      public var poolId:String;
      
      /**
       * Creates a new http connection.
       */
      public function HttpConnection()
      {
         mOpen = false;
         mResponse = null;
         mRequestQueue = new Array();
         
         // default to a 30 second receive timeout
         mReceiveTimeout = new Timer(30 * 1000);
         
         // add receive timeout handler
         addEventListener(TimerEvent.TIMER, receiveTimedOut);
      }
      
      /**
       * Sets the url to connect to.
       * 
       * @param the url to connection to, null to establish no connection.
       */
      public function set url(u:Url):void
      {
         mUrl = u;
      }
      
      /**
       * Gets the url to connect to.
       * 
       * @return the url to connection to, null if the connection's
       *         remote end point has not been set yet.
       */
      public function get url():Url
      {
         return mUrl;
      }
      
      /**
       * Gets the number of requests in this connection's queue.
       * 
       * @return the number of requests in this connection's queue.
       */
      public function get requestCount():uint
      {
         return mRequestQueue.length;
      }
      
      /**
       * Gets whether or not this connection is currently open. An open
       * connection is either connected or in the process of connecting.
       * 
       * @return true if this connection is currently open, false if not.
       */
      public function get open():Boolean
      {
         return mOpen;
      }
      
      /**
       * Gets whether or not this connection is currently connected.
       * 
       * @return true if this connection is currently open, false if not.
       */
      public function get connected():Boolean
      {
         return (mSocket != null && mSocket.connected) ||
                (_socket != null && _socket.connected) ||
                (mTlsSocket != null && mTlsSocket.connected);
      }
      
      /**
       * Called when the receive timeout is reached.
       * 
       * @param event the event that occurred.
       */
      private function receiveTimedOut(event:TimerEvent):void
      {
         // dispatch http connection timeout event
         dispatchEvent(new HttpConnectionEvent(
            HttpConnectionEvent.TIMEOUT, mResponse));
      }
      
      /**
       * Creates a formatted string using StringUtil.substitute.
       * 
       * @param format the string format with "{n}"s to replace.
       * @param args the rest of the arguments.
       * 
       * @return the formatted string.
       */
      private function printf(format:String, ... args):String
      {
         return StringUtil.substitute(format, args);
      }
      
      /**
       * Adds event listeners to the passed socket.
       * 
       * @param s the socket.
       */
      private function addEventListeners(s:IEventDispatcher):void
      {
         s.addEventListener(Event.CONNECT, socketConnected);
         s.addEventListener(Event.CLOSE, socketClosed);
         s.addEventListener(ProgressEvent.SOCKET_DATA, dataReceived);
         s.addEventListener(IOErrorEvent.IO_ERROR, ioError);
         s.addEventListener(SecurityErrorEvent.SECURITY_ERROR, securityError);
      }
      
      /**
       * Removes event listeners from the passed socket.
       * 
       * @param s the socket.
       */
      private function removeEventListeners(s:IEventDispatcher):void
      {
         s.removeEventListener(Event.CONNECT, socketConnected);
         s.removeEventListener(Event.CLOSE, socketClosed);
         s.removeEventListener(ProgressEvent.SOCKET_DATA, dataReceived);
         s.removeEventListener(IOErrorEvent.IO_ERROR, ioError);
         s.removeEventListener(
            SecurityErrorEvent.SECURITY_ERROR, securityError);
      }
      
      /**
       * Connects to the passed host and port.
       * 
       * @param url the url with http/https scheme, host, and port.
       * 
       * @throws ArgumentError - invalid url.
       * @throws IOError - connection failed.
       * @throws SecurityError - port too high, some security limitation.
       */
      public function connect(url:String):void
      {
         if(mOpen)
         {
            // already open, cannot connect again
            throw new IOError(printf(
               "Already connected/connecting to: '{0}'", mUrl.root));
         }
         
         // get url
         mUrl = new Url(url);
         
         // now open
         mOpen = true;
         
         if(mUrl.isHttps())
         {
            // create tls socket using as3crypto library
            //mTlsSocket = new TLSSocket();
            // create temporary hack socket, replace with
            // mTlsSocket.connect() once that library works out
            // issues with sending a handshake as soon as the
            // connection is established
            _socket = new Socket();
            _socket.addEventListener(Event.CONNECT, _socketConnected);
            _socket.addEventListener(Event.CLOSE, socketClosed);
            _socket.addEventListener(IOErrorEvent.IO_ERROR, ioError);
            _socket.addEventListener(
               SecurityErrorEvent.SECURITY_ERROR, securityError);
            _socket.connect(mUrl.host, mUrl.port);
            // FIXME: do any other config setup that is necessary here
            //mTlsSocket.connect(mUrl.host, mUrl.port);
         }
         else if(mUrl.isHttp())
         {
            // create non-tls socket
            mSocket = new Socket();
            addEventListeners(mSocket);
            mSocket.connect(mUrl.host, mUrl.port);
         }
         else
         {
            // no longer processing request/response
            mResponse = null;
            
            // invalid url schema
            throw new ArgumentError(
               printf("Invalid protocol: '{0}'", mUrl.protocol));
         }
         
         //ExternalInterface.call("console.log", "connecting to " + mUrl.root);
         trace(printf("connecting to '{0}'...", mUrl.root));
      }
      
      /**
       * Called when the hack _socket connects so that the TLS
       * engine can be started.
       * 
       * @param event the event that occurred.
       */
      private function _socketConnected(event:Event):void
      {
         trace(printf("TLS socket connected to '{0}'", mUrl.root));
         //ExternalInterface.call("console.log", "connected to " + mUrl.root);
         
         // remove listeners
         _socket.removeEventListener(Event.CONNECT, _socketConnected);
         _socket.removeEventListener(Event.CLOSE, socketClosed);
         _socket.removeEventListener(IOErrorEvent.IO_ERROR, ioError);
         _socket.removeEventListener(
            SecurityErrorEvent.SECURITY_ERROR, securityError);
         
         // start TLS engine
         //trace("starting TLS engine");
         mTlsSocket = new TLSSocket();
         addEventListeners(mTlsSocket);
         var cn:String = (verifyCommonName != null ?
            verifyCommonName : mUrl.host);
         mTlsSocket.startTLS(_socket, cn, tlsconfig);
         mTlsSocket.flush();
         _socket = null;
         //trace("TLS engine started");
         
         // do current request
         if(mResponse != null)
         {
            sendRequest(mResponse.request);
         }
         // schedule next request
         else
         {
            scheduleNextRequest();
         }
      }
      
      /**
       * Called when a socket connects.
       * 
       * @param event the event that occurred.
       */
      private function socketConnected(event:Event):void
      {
         trace(printf("connected to '{0}'", mUrl.root));
         //ExternalInterface.call("console.log", "connected to " + mUrl.root);
         
         // do current request
         if(mResponse != null)
         {
            sendRequest(mResponse.request);
         }
         // schedule next request
         else
         {
            scheduleNextRequest();
         }
      }
      
      /**
       * Closes the current connection.
       * 
       * @throws IOError - if connection could not be closed.
       */
      public function close():void
      {
         if(mOpen)
         {
            try
            {
               // no longer open or processing
               mOpen = false;
               mResponse = null;
               //ExternalInterface.call("console.log", "closed");
               
               // stop timer
               mReceiveTimeout.stop();
               
               if(mSocket != null)
               {
                  removeEventListeners(mSocket);
                  if(mSocket.connected)
                  {
                     mSocket.close();
                  }
               }
               else if(_socket != null)
               {
                  _socket.removeEventListener(Event.CONNECT, _socketConnected);
                  _socket.removeEventListener(Event.CLOSE, socketClosed);
                  _socket.removeEventListener(IOErrorEvent.IO_ERROR, ioError);
                  _socket.removeEventListener(
                     SecurityErrorEvent.SECURITY_ERROR, securityError);
                  if(_socket.connected)
                  {
                     _socket.close();
                  }
               }
               else
               {
                  removeEventListeners(mTlsSocket);
                  if(mTlsSocket.connected)
                  {
                     // FIXME: must flush TLS socket to avoid exception
                     mTlsSocket.flush();
                     mTlsSocket.close();
                  }
               }
            }
            catch(e:Error)
            {
               // ignore irrecoverable crazy errors from as3crypto library
               //trace(e.getStackTrace());
            }
            
            mSocket = null;
            _socket = null;
            mTlsSocket = null;
            
            trace(printf("disconnected from '{0}'", mUrl.root));
            //ExternalInterface.call(
            //   "console.log", "disconnected from " + mUrl.root);
            
            // schedule next request
            scheduleNextRequest();
         }
      }
      
      /**
       * Called when the server closes the socket connection.
       * 
       * @param event the event that occurred.
       */
      private function socketClosed(event:Event):void
      {
         // FIXME: if TLSSocket/TLSEngine throws an error, then we
         // have no way of catching it and telling things to stop
         // trying to re-connect!
         
         // FIXME: how do we tell if the request has been sent yet?
         // It doesn't seem like flash exposes this to us... we can't safely
         // assume that just because we haven't received a response header
         // that we didn't send our request
         
         // put request back on queue if it hasn't been sent yet and this
         // isn't our second retry
         if(mResponse != null && !mResponse.headerReceived &&
            !mResponse.retried)
         {
            // create a response object to wrap the request, set retried flag
            var response:HttpResponse = new HttpResponse();
            response.id = mResponse.id;
            response.request = mResponse.request;
            response.userData = mResponse.userData;
            response.retried = true;
            mRequestQueue.unshift(response);
         }
         else
         {
            // dispatch IO error
            dispatchEvent(new HttpConnectionEvent(
               HttpConnectionEvent.IO_ERROR, mResponse,
               new IOErrorEvent(
                  IOErrorEvent.IO_ERROR, false, false,
                  'Connection closed by SSL-error or server.')));
         }
         
         // close socket
         close();
      }
      
      /**
       * Adds a HttpRequest to be sent. The request will be sent once the
       * http connection is connected and has sent any other requests that
       * were added prior to this one.
       * 
       * @param request the request to send.
       * @param id a unique ID to associate with the request.
       * @param userData user-data to associate with the response.
       */
      public function addRequest(
         request:HttpRequest, id:uint = 0, userData:Object = null):void
      {
         // create a response object to wrap the request
         var response:HttpResponse = new HttpResponse();
         response.id = id;
         response.request = request;
         response.userData = userData;
         
         // queue the request
         mRequestQueue.push(response);
         
         //ExternalInterface.call("console.log", "request queued", response);
         
         // schedule the next request
         scheduleNextRequest();
      }
      
      /**
       * Clears all requests in the request queue.
       */
      public function clearRequestQueue():void
      {
         mRequestQueue.clear();
      }
      
      /**
       * Schedules the next request in the queue to be sent.
       */
      private function scheduleNextRequest():void
      {
         //ExternalInterface.call("console.log", "scheduling next request");
         
         // schedule processing of request asynchronously
         var timer:Timer = new Timer(0, 1);
         timer.addEventListener(TimerEvent.TIMER_COMPLETE, sendNextRequest);
         timer.start();
      }
      
      /**
       * Sends the next request once a timer interval has been reached.
       * 
       * @param e the timer event.
       */
      private function sendNextRequest(e:TimerEvent):void
      {
         if(mRequestQueue.length > 0)
         {
            // reconnect if necessary
            if(!mOpen && mUrl != null)
            {
               // initialize objects to handle response
               mBuffer = new ByteArray();
               mResponse = mRequestQueue.shift() as HttpResponse;
               mReadPosition = 0;
               mChunkSize = 0;
               mChunksFinished = false;
               
               // connect
               //ExternalInterface.call("console.log", "connecting");
               connect(mUrl.root);
            }
            // process next request if connected and not processing
            else if(this.connected && mResponse == null)
            {
               //ExternalInterface.call("console.log", "was idle")
               
               // initialize objects to handle response
               mBuffer = new ByteArray();
               mResponse = mRequestQueue.shift() as HttpResponse;
               mReadPosition = 0;
               mChunkSize = 0;
               mChunksFinished = false;
               
               // send the next request
               sendRequest(mResponse.request);
            }
         }
         // send idle event if not processing
         else if(mResponse == null)
         {
         	//ExternalInterface.call("console.log", "going idle")
            
            // dispatch http connection idle event
            var event:HttpConnectionEvent = new HttpConnectionEvent(
               HttpConnectionEvent.IDLE, null);
            event.connection = this;
            dispatchEvent(event);
         }
      }
      
      /**
       * Sends an http request to the connected http server.
       * 
       * @param request to request to send.
       * 
       * @throws IOError - send failed.
       */
      private function sendRequest(request:HttpRequest):void
      {
         trace(printf("sending request to '{0}'", mUrl.root));
         //ExternalInterface.call(
         //   "console.log", "sending request to " + mUrl.root);
         
         try
         {
            // add host header if not already added
            var hostAdded:Boolean = false;
            for each(var h:URLRequestHeader in request.headers)
            {
               if(h.name.toLowerCase() == "host")
               {
                  hostAdded = true;
                  break;
               }
            }
            if(!hostAdded)
            {
               request.headers.push(
                  new URLRequestHeader("Host", mUrl.hostWithPort));
            }
            
            // build request
            var str:String = request.toString();
            
            // debug only
            //trace("request:\n" + str);
            
            // get socket output
            var output:IDataOutput = (mSocket != null ? mSocket : mTlsSocket);
            
            // dispatch open event
            dispatchEvent(new HttpConnectionEvent(
               HttpConnectionEvent.OPEN, mResponse));
            
            // send request
            output.writeUTFBytes(str);
            
            // send body, if any
            if(request.body != null)
            {
               output.writeBytes(request.body, 0, request.body.length);
            }
            
            // update response time
            mResponse.time = new Date().valueOf();
            
            // flush socket (sadly, flush() is not available on IDataOutput)
            if(mSocket != null)
            {
               mSocket.flush();
            }
            else
            {
               mTlsSocket.flush();
            }
            
            // dispatch request event
            dispatchEvent(new HttpConnectionEvent(
               HttpConnectionEvent.REQUEST, mResponse));
            
            // start timer with 30 second timeout
            mReceiveTimeout.delay = 1000 * 30;
            mReceiveTimeout.start();
         }
         catch(e:Error)
         {
            trace(e.getStackTrace());
            close();
            throw e;
         }
      }
      
      /**
       * Called when data is received on a socket.
       * 
       * @param event the event that occurred.
       */
      private function dataReceived(event:ProgressEvent):void
      {
         try
         {
            // reset timer
            mReceiveTimeout.reset();
            
            // get socket input
            var input:IDataInput = IDataInput(event.target);
            
            if(mResponse.bodyReceived)
            {
               // read and discard socket bytes, body already received
               var bytes:ByteArray = new ByteArray();
               input.readBytes(bytes, 0, input.bytesAvailable);
               bytes = null;
            }
            else
            {
               // read socket data into response buffer
               input.readBytes(
                  mBuffer, mBuffer.position, input.bytesAvailable);
               mBuffer.position = mBuffer.length;
               
               /*
               // debug only
               trace("buffer length after: " + mBuffer.length);
               trace("read position: " + mReadPosition);
               trace("bytes in buffer: " + (mBuffer.length - mReadPosition));
               var tmp:ByteArray = new ByteArray();
               mBuffer.position = mReadPosition;
               mBuffer.readBytes(tmp, 0, mBuffer.length - mReadPosition);
               mBuffer.position = mBuffer.length;
               trace("start buffer[\n" +
                  tmp.toString() + "]\nend buffer\n");
               ExternalInterface.call("console.log",
                  "start buffer[\n" + tmp.toString() + "]\nend buffer\n");
               */
               
               // parse more of the response
               parseResponse();
            }
         }
         catch(e:IOError)
         {
            trace(e.getStackTrace());
            
            // dispatch http connection IO error event
            dispatchEvent(new HttpConnectionEvent(
               HttpConnectionEvent.IO_ERROR, mResponse,
               new IOErrorEvent(
                  IOErrorEvent.IO_ERROR, false, false, e.message)));
            close();
         }
         catch(e:SecurityError)
         {
            trace(e.getStackTrace());
            
            // dispatch http connection security error event
            dispatchEvent(new HttpConnectionEvent(
               HttpConnectionEvent.SECURITY_ERROR, mResponse,
               new SecurityErrorEvent(
                  SecurityErrorEvent.SECURITY_ERROR, false, false, e.message)));
            close();
         }
         catch(e:Error)
         {
            trace(e.getStackTrace());
            close();
         }
      }
      
      /**
       * Parses some more of the receive buffer's bytes into an HttpResponse.
       * 
       * @throws IOError - if an IO error occurs.
       */
      private function parseResponse():void
      {
         // parse more of the response header
         if(!mResponse.headerReceived)
         {
            parseResponseHeader();
         }
         
         // parse more of the response body
         if(mResponse.headerReceived && !mResponse.bodyReceived)
         {
            switch(mReceiveBodyMode)
            {
               case RECV_CONTENT_LENGTH:
                  if((mBuffer.length - mReadPosition) >=
                     mResponse.contentLength)
                  {
                     // all content received
                     consumeBytes(
                        mResponse.contentLength, mResponse.data as ByteArray);
                     mResponse.bodyReceived = true;
                  }
                  break;
               case RECV_CHUNKED:
                  parseChunkedBody();
                  break;
               default:
                  throw new IOError("Invalid transfer encoding.");
            }
         }
         
         if(mResponse.bodyReceived)
         {
            // reclaim receive buffer
            mBuffer = null;
            
            if(mResponse.data != null)
            {
               // set buffer position to beginning for reading
               ByteArray(mResponse.data).position = 0;
            }
            
            // handle content encoding
            handleContentEncoding();
            
            // update response time
            mResponse.time = new Date().valueOf() - mResponse.time;
            
            // dispatch body received event
            dispatchEvent(new HttpConnectionEvent(
               HttpConnectionEvent.BODY, mResponse));
            
            // log connection header
            trace("Connection: " + (mResponse.connection != null ?
               mResponse.connection : "keep-alive"));
            
            // close connection if requested by server
            if(mResponse.connection != null &&
               mResponse.connection.indexOf("close") != -1)
            {
               close();
            }
            else
            {
               // done with request, doing keep-alive, set timeout to 5 minutes
               mResponse = null;
               mReceiveTimeout.delay = 1000 * 60 * 5;
               mReceiveTimeout.reset();
               
               // schedule next request
               scheduleNextRequest();
            }
         }
      }
      
      /**
       * Parses some more of the receive buffer's bytes as the http
       * response header.
       * 
       * @throws IOError - if an IO error occurs.
       */
      private function parseResponseHeader():void
      {
         /* Sample response header:
         HTTP/1.1 200 OK
         Server: Some Server
         Date: Sat, 21 Jan 2006 19:15:46 GMT
         Content-Encoding: gzip
         Content-Length: 400
         Content-Type: text/html
         Connection: close
         */
         
         try
         {
            // try to read a header line
            var line:String = readCrlf();
            
            // if found a header line
            while(line != null && !mResponse.headerReceived)
            {
               // last line in header is length == 0
               if(line.length == 0)
               {
                  if(mResponse.version == null)
                  {
                     // status line not found, invalid response
                     throw new IOError(
                        "Invalid http response header. No status line.");
                  }
                  
                  // header now received, determine body information
                  mResponse.headerReceived = true;
                  
                  // Note: EITHER transfer-encoding or content-length is
                  // used, not BOTH
                  
                  // debug only
                  //trace("transfer-encoding: " + mResponse.transferEncoding);
                  //trace("content-length: " + mResponse.contentLength);
                  
                  // check response for "chunked" transfer-encoding
                  if(mResponse.transferEncoding != null &&
                     mResponse.transferEncoding.indexOf("chunked") != -1)
                  {
                     mReceiveBodyMode = RECV_CHUNKED;
                     mResponse.data = new ByteArray();
                  }
                  // check response for content-length
                  else if(mResponse.contentLength != -1)
                  {
                     mReceiveBodyMode = RECV_CONTENT_LENGTH;
                     mResponse.data = new ByteArray();
                  }
                  else
                  {
                     // no body to receive
                     mResponse.bodyReceived = true;
                     
                     // FIXME: handle unspecified content-length, but
                     // content-type is present w/Connection: close?
                     // (read until close?)
                  }
                  
                  // dispatch received header event
                  dispatchEvent(new HttpConnectionEvent(
                     HttpConnectionEvent.HEADER, mResponse));
                  
                  // dispatch body loading event
                  dispatchEvent(new HttpConnectionEvent(
                     HttpConnectionEvent.LOADING, mResponse));
               }
               // if first line
               else if(mResponse.version == null)
               {
                  // parse response status line
                  // looking for VERSION CODE MSG
                  var params:Array = line.split(" ");
                  mResponse.version = params[0];
                  mResponse.statusCode = uint(params[1]);
                  mResponse.statusMessage = params.slice(2).join(" ");
                  
                  // debug only
                  //trace("response status line: '" + line + "'");
                  
                  // read next line
                  line = readCrlf();
               }
               else
               {
                  // parse header line
                  var header:URLRequestHeader = parseHeader(line);
                  
                  // handle special headers and others
                  switch(header.name)
                  {
                     case "Content-Type":
                        mResponse.contentType = header.value;
                        break;
                     case "Content-Encoding":
                        mResponse.contentEncoding = header.value;
                        break;
                     case "Transfer-Encoding":
                        mResponse.transferEncoding = header.value;
                        break;
                     case "Content-Length":
                        mResponse.contentLength = int(header.value);
                        break;
                     case "Connection":
                        mResponse.connection = header.value;
                        break;
                     default:
                        // add other header
                        mResponse.headers.push(header);
                  }
                  
                  // read next line
                  line = readCrlf();
               }
            }
         }
         catch(e:Error)
         {
            // invalid http format
            throw new IOError(
               "Invalid http response header, cause:\n" +
               e.getStackTrace());
         }
      }
      
      /**
       * Parses some more of the receive buffer's bytes as a chunked http
       * response entity-body.
       * 
       * @throws IOError - if an IO error occurs.
       */
      private function parseChunkedBody():void
      {
         /*
         Chunked transfer-encoding sends data in a series of chunks,
         followed by a set of 0-N http trailers.
         The format is as follows:
         
         chunk-size (in hex) CRLF
         chunk data (with "chunk-size" many bytes) CRLF
         ... (N many chunks)
         chunk-size (of 0 indicating the last chunk) CRLF
         N many http trailers followed by CRLF
         blank line + CRLF (terminates the trailers)
         
         If there are no http trailers, then after the chunk-size of 0,
         there is still a single CRLF (indicating the blank line + CRLF
         that terminates the trailers). In other words, you always terminate
         the trailers with blank line + CRLF, regardless of 0-N trailers.
         */
         
         /* From RFC-2616, section 3.6.1, here is the pseudo-code for
         implementing chunked transfer-encoding:
         
         length := 0
         read chunk-size, chunk-extension (if any) and CRLF
         while (chunk-size > 0) {
            read chunk-data and CRLF
            append chunk-data to entity-body
            length := length + chunk-size
             read chunk-size and CRLF
         }
         read entity-header
         while (entity-header not empty) {
            append entity-header to existing header fields
            read entity-header
         }
         Content-Length := length
         Remove "chunked" from Transfer-Encoding
         */
         
         var line:String = "";
         while(line != null && (mBuffer.length - mReadPosition) > 0)
         {
            // if in the process of reading a chunk
            if(mChunkSize > 0)
            {
               // if there are not enough bytes to read chunk and its
               // trailing CRLF,  we must wait for more data to be received
               if((mChunkSize + 2) > (mBuffer.length - mReadPosition))
               {
                  break;
               }
               
               // read chunk data, skip CRLF
               consumeBytes(mChunkSize, mResponse.data as ByteArray);
               consumeBytes(2, null);
               mChunkSize = 0;
            }
            // if chunks not finished, read the next chunk size
            else if(!mChunksFinished)
            {
               // read chunk-size line
               line = readCrlf();
               if(line != null)
               {
                  // parse chunk-size (ignore any chunk extension)
                  mChunkSize = int("0x" + line.split(";", 1)[0]);
                  mChunksFinished = (mChunkSize == 0);
                  
                  // debug only
                  //trace("chunk-size: " + mChunkSize);
               }
            }
            // chunks finished, read trailers
            else
            {
               // read next trailer
               line = readCrlf();
               while(line != null)
               {
                  if(line.length > 0)
                  {
                     // add trailer
                     mResponse.headers.push(parseHeader(line));
                     // read next trailer
                     line = readCrlf();
                  }
                  else
                  {
                     // body received
                     mResponse.bodyReceived = true;
                     line = null;
                  }
               }
            }
         }
      }
      
      /**
       * Handles content-encoding once the response body is received
       * (i.e. deflate).
       * 
       * @throws IOError - if an IO error occurs.
       */
      private function handleContentEncoding():void
      {
         // check content-encoding for deflate
         if(mResponse.contentEncoding != null &&
            mResponse.contentEncoding.indexOf("deflate") != -1)
         {
            // inflate data
            mResponse.data.uncompress();
         }
         
         // debug only
         /*
         if(mResponse.data)
         {
            trace("read response data:\n" + mResponse.data.toString());
         }
         */
         
         // check content-type for post-form data
         if(mResponse.contentType == "application/x-www-form-urlencode")
         {
            // convert response string to URLVariables
            var vars:URLVariables = new URLVariables();
            vars.decode(mResponse.data.toString());
            mResponse.data = vars;
         }
      }
      
      /**
       * Called when an IO error occurs on a socket.
       * 
       * @param event the event that occurred.
       */
      private function ioError(event:IOErrorEvent):void
      {
         trace(event.text);
         //ExternalInterface.call("console.log", "ioError: " + event.text);
         dispatchEvent(new HttpConnectionEvent(
            HttpConnectionEvent.IO_ERROR, mResponse, event));
         close();
      }
      
      /**
       * Called when a security error occurs on socket.
       * 
       * @param event the event that occurred.
       */
      private function securityError(event:SecurityErrorEvent):void
      {
         trace(event.text);
         //ExternalInterface.call("console.log", "securityError: " + event.text);
         dispatchEvent(new HttpConnectionEvent(
            HttpConnectionEvent.SECURITY_ERROR, mResponse, event));
         close();
      }
      
      /**
       * Reads data out of the receive buffer and into the passed ByteArray or
       * into the returned string, consuming the bytes and shifting the data in
       * the receive buffer if necessary.
       * 
       * The position of the buffer for writing will stay in the correct
       * place.
       * 
       * @param count the number of bytes to read and consume.
       * @param buffer the buffer to populate, null to return the bytes as
       *               a UTF string.
       * 
       * @return the UTF-bytes String or null.
       */
      private function consumeBytes(count:uint, buffer:ByteArray):String
      {
         var rval:String;
         
         // set position to the read position
         mBuffer.position = mReadPosition;
         
         // debug only
         //trace("reading " + count + " bytes from " + mReadPosition);
         
         // read the bytes
         if(buffer == null)
         {
            rval = mBuffer.readUTFBytes(count);
         }
         else
         {
            mBuffer.readBytes(buffer, buffer.length, count);
         }
         
         // update read position
         mReadPosition += count;
         
         // if the consumed bytes take up at least half of the length
         // of the buffer, shift the bytes down and reclaim the memory
         if(count >= (mBuffer.length / 2))
         {
            var newBuffer:ByteArray = new ByteArray();
            mBuffer.readBytes(newBuffer, 0, mBuffer.length - mReadPosition);
            mBuffer = newBuffer;
            mReadPosition = 0;
         }
         
         // restore position to write position
         mBuffer.position = mBuffer.length;
         
         return rval;
      }
      
      /**
       * Reads a single CRLF-terminated line from the receive buffer.
       * 
       * @return the read line or null if no CRLF was found yet.
       * 
       * @throws IOError - read failed.
       */
      private function readCrlf():String
      {
         var line:String = null;
         
         // each line is complete when CRLF is found
         for(var index:uint = mReadPosition;
             line == null && (index + 1) < mBuffer.length; index++)
         {
            // find the next CRLF in the buffer
            if(mBuffer[index] == CR && mBuffer[index + 1] == LF)
            {
               // CRLF found, read a line, skip CRLF bytes
               line = consumeBytes(index - mReadPosition, null);
               consumeBytes(2, null);
               
               // debug only
               //trace("read response line: '" + line + "'");
            }
         }
         
         return line;
      }
      
      /**
       * Creates a URLRequestHeader out of the passed line.
       * 
       * @param line the line to parse.
       * 
       * @return the URLRequestHeader.
       */
      private function parseHeader(line:String):URLRequestHeader
      {
         // looking for NAME:VALUE
         var params:Array = line.split(":");
         var name:String = StringUtil.trim(params[0]);
         var value:String = StringUtil.trim(params.slice(1).join(":"));
         
         // normalize header name to capitalized first letter and after hyphens
         var firstLetter:String = name.substr(0, 1).toUpperCase();
         var otherLetters:String = name.substring(1).toLowerCase();
         var normalized:String = firstLetter;
         for(var index:uint = 1; index < name.length; index++)
         {
            normalized += name.charAt(index);
            if(name.charAt(index) == "-" && index + 1 < name.length)
            {
               normalized += name.charAt(index + 1).toUpperCase();
               index++;
            }
         }
         
         return new URLRequestHeader(normalized, value);
      }
   }
}

/**
 * HTTP client-side implementation that uses krypto.net sockets.
 *
 * @author Dave Longley
 *
 * Copyright (c) 2010 Digital Bazaar, Inc. All rights reserved.
 */
(function()
{
   // define http namespace
   var http = {};
   
   // normalizes an http header field name
   var _normalize = function(name)
   {
      return name.toLowerCase().replace(/(^.)|(-.)/g,
         function(a){return a.toUpperCase();});
   };
   
   // parses the scheme, host, and port from a url
   var _parseUrl = function(str)
   {
      var url = {};
      var regex = /^(https?):\/\/([^:&^\/]*):?(\d*)(.*)$/g;
      str.replace(regex, function(full, scheme, host, port) {
         url.scheme = scheme;
         url.host = host;
         url.port = port || (scheme === 'https' ? 443 : 80);
      });
      return url;
   };
   
   /**
    * Connects and sends a request.
    *
    * @param socket the socket to use.
    */
   var _doRequest = function(socket)
   {
      // socket no longer idle
      socket.idle = false;
      
      if(socket.isConnected())
      {
         // already connected
         socket.connected({
            type: 'connect',
            id: socket.id
         });
      }
      else
      {
         // connect
         socket.connect(client.url);
      }
   };
   
   /**
    * Handles the next request or marks a socket as idle.
    * 
    * @param client the http client.
    * @param socket the socket.
    */
   var _handleNextRequest = function(client, socket)
   {
      if(!socket.idle)
      {
         // clear buffer
         socket.buffer.clear();
         
         // mark socket idle if no pending requests
         if(client.requests.length == 0)
         {
            socket.idle = true;
            client.idle.push(socket);
         }
         // handle pending request
         else
         {
            socket.options = client.requests.shift();
            _doRequest(socket);
         }
      }
   };
   
   /**
    * Creates an http client that uses krypto.net sockets as a backend and
    * krypto.tls for security.
    * 
    * @param options:
    *           url: the url to connect to (scheme://host:port).
    *           socketPool: the flash socket pool to use.
    *           connections: number of connections to use to handle requests.
    *           caCerts: an array of certificates to trust for TLS, certs may
    *              be PEM-formatted or cert objects produced via krypto.pki.
    * 
    * @return the client.
    */
   http.createClient = function(options)
   {
      // create CA store to share with all TLS connections
      var caStore = null;
      if(options.caCerts)
      {
         caStore = krypto.pki.createCaStore(options.caCerts);
      }
      
      // get scheme, host, and port from url
      var url = _parseUrl(options.url);
      if(!url.scheme || !url.host || !url.port)
      {
         throw {
            message: 'Invalid url.',
            details: {
               url: options.url
            }
         };
      }
      
      // default to 1 connection
      options.connections = options.connections || 1;
      
      // local aliases
      var net = window.krypto.net;
      var util = window.krypto.util;
      
      // create client
      var sp = options.socketPool;
      var client =
      {
         // url
         url: url,
         // socket pool
         socketPool: sp,
         // queue of requests to service
         requests: [],
         // all sockets
         sockets: [],
         // idle sockets
         idle: []
      };
      
      // determine if TLS is used
      var useTls = (url.scheme === 'https');
      
      // create sockets
      for(var i = 0; i < options.connections; i++)
      {
         var socket = sp.createSocket();
         // wrap socket for TLS
         if(useTls)
         {
            // FIXME: add TLS session cache
            socket = window.krypto.tls.wrapSocket({
               sessionId: null,
               caStore: caStore,
               socket: socket,
               verify: function(c, verified, depth, certs)
               {
                  // FIXME: if depth === 0, check certs[0] common name
                  return verified;
               }
            });
         }
         // set up handlers
         socket.connected = function(e)
         {
            socket.options.connected(e);
            var request = socket.options.request;
            var out = request.toString();
            if(request.body)
            {
               out += request.body;
            }
            socket.send(out);
         };
         socket.closed = function(e)
         {
            // handle unspecified content-length transfer
            var response = socket.options.response;
            if(response.readBodyUntilClose)
            {
               response.bodyReceived = true;
               socket.options.bodyReady({
                  request: socket.options.request,
                  response: response
               });
            }
            socket.options.closed(e);
            _handleNextRequest(client, socket);
         };
         socket.data = function(e)
         {
            // receive all bytes available
            var response = socket.options.response;
            var bytes = socket.receive(e.bytesAvailable);
            if(bytes !== null)
            {
               // receive header and then body
               socket.buffer.putBytes(bytes);
               if(!response.headerReceived)
               {
                  response.readHeader(socket.buffer);
                  if(response.headerReceived)
                  {
                     socket.options.headerReady({
                        request: socket.options.request,
                        response: response
                     });
                  }
               }
               if(response.headerReceived && !response.bodyReceived)
               {
                  response.readBody(socket.buffer);
               }
               if(response.bodyReceived)
               {
                  socket.options.bodyReady({
                     request: socket.options.request,
                     response: response
                  });
                  var value = response.getField('Connection') || '';
                  if(value.indexOf('close') != -1)
                  {
                     // close socket
                     socket.close();
                  }
                  _handleNextRequest(client, socket);
               }
            }
         };
         socket.error = function(e)
         {
            // do error callback, include request
            socket.options.error({
               type: e.type,
               message: e.message,
               request: socket.options.request,
               response: socket.options.response
            });
            socket.close();
            _handleNextRequest(client, socket);
         };
         socket.idle = true;
         socket.buffer = util.createBuffer();
         client.sockets.push(socket);
         client.idle.push(socket);
      }
      
      /**
       * Sends a request.
       * 
       * @param options:
       *           request: the request to send.
       *           connected: a callback for when the connection is open.
       *           closed: a callback for when the connection is closed.
       *           headerReady: a callback for when the response header arrives.
       *           bodyReady: a callback for when the response body arrives.
       *           error: a callback for if an error occurs.
       */
      client.send = function(options)
      {
         // set default dummy handlers
         options.connected = options.connected || function(){};
         options.closed = options.close || function(){};
         options.headerReady = options.headerReady || function(){};
         options.bodyReady = options.bodyReady || function(){};
         options.error = options.error || function(){};
         
         // create response
         options.response = http.createResponse();
         options.response.flashApi = client.socketPool.flashApi;
         options.request.flashApi = client.socketPool.flashApi;
         
         // queue request options if there are no idle sockets
         if(client.idle.length == 0)
         {
            client.requests.push(options);
         }
         // use an idle socket
         else
         {
            var socket = client.idle.pop();
            socket.options = options;
            _doRequest(socket);
         }
      };
      
      /**
       * Destroys this client.
       */
      client.destroy = function()
      {
         // clear pending requests, close and destroy sockets
         client.requests = [];
         for(var i = 0; i < client.sockets.length; i++)
         {
            client.sockets[i].close();
            client.sockets[i].destroy();
         }
         client.socketPool = null;
         client.sockets = [];
         client.idle = [];
      };
      
      return client;
   };
   
   /**
    * Creates an http header object.
    * 
    * @return the http header object.
    */
   var _createHeader = function()
   {
      var header =
      {
         fields: {},
         setField: function(name, value)
         {
            // normalize field name, trim value
            header.fields[_normalize(name)] =
               [('' + value).replace(/^\s*/, '').replace(/\s*$/, '')];
         },
         appendField: function(name, value)
         {
            name = _normalize(name);
            if(!(name in header.fields))
            {
               header.fields[name] = [];
            }
            header.fields[name].push(
               ('' + value).replace(/^\s*/, '').replace(/\s*$/, ''));
         },
         getField: function(name, index)
         {
            var rval = null;
            name = _normalize(name);
            if(name in header.fields)
            {
               index = index || 0;
               rval = header.fields[name][index];
            }
            return rval;
         }
      };
      return header;
   };
   
   /**
    * Creates an http request.
    * 
    * @param options:
    *           version: the version.
    *           method: the method.
    *           path: the path.
    *           body: the body.
    *           headers: custom header fields to add,
    *              ie: [{'Content-Length': 0}].
    * 
    * @return the http request.
    */
   http.createRequest = function(options)
   {
      options = options || {};
      var request = _createHeader();
      request.version = options.version || 'HTTP/1.1';
      request.method = options.method || null;
      request.path = options.path || null;
      request.body = options.body || null;
      request.bodyDeflated = false;
      request.flashApi = null;
      
      // add custom headers
      var headers = options.headers || [];
      for(var i = 0; i < headers.length; i++)
      {
         for(var name in headers[i])
         {
            request.appendField(name, headers[i][name]);
         }
      }
      
      // set default headers
      if(request.getField('User-Agent') === null)
      {
         request.setField('User-Agent', 'javascript krypto.http 1.0');
      }
      if(request.getField('Accept') === null)
      {
         request.setField('Accept', '*/*');
      }
      if(request.getField('Connection') === null)
      {
         request.setField('Connection', 'close');
      }
      
      /**
       * Converts an http request into a string that can be sent as a
       * HTTP request. Does not include any data.
       * 
       * @return the string representation of the request.
       */
      request.toString = function()
      {
         /* Sample request header:
          GET /some/path/?query HTTP/1.1
          Host: www.someurl.com
          Connection: close
          Accept-Encoding: deflate
          Accept: image/gif, text/html
          User-Agent: Mozilla 4.0
          */
         
         // add Accept-Encoding if not specified
         if(request.flashApi !== null &&
            request.getField('Accept-Encoding') === null)
         {
            request.setField('Accept-Encoding', 'deflate');
         }
         
         // if the body isn't null, deflate it if its larger than 100 bytes
         if(request.flashApi !== null && request.body !== null &&
            request.getField('Content-Encoding') === null &&
            !request.bodyDeflated && request.body.length > 100)
         {
            // use flash to compress data
            request.body = util.deflate(request.flashApi, request.body);
            request.bodyDeflated = true;
            request.setField('Content-Encoding', 'deflate');
            request.setField('Content-Length', request.body.length);
         }
         else if(request.body !== null)
         {
            // set content length for body
            request.setField('Content-Length', request.body.length);
         }
         
         // build start line
         var rval =
            request.method.toUpperCase() + ' ' + request.path + ' ' +
            request.version + '\r\n';
         
         // add each header
         for(var name in request.fields)
         {
            var fields = request.fields[name];
            for(var i = 0; i < fields.length; i++)
            {
               rval += name + ': ' + fields[i] + '\r\n';
            }
         }
         // final terminating CRLF
         rval += '\r\n';
         
         return rval;
      };
      
      return request;
   };
   
   /**
    * Creates an empty http response header.
    * 
    * @return the empty http response header.
    */
   http.createResponse = function()
   {
      // private vars
      var _first = true;
      var _chunkSize = 0;
      var _chunksFinished = false;
      
      // create response
      var response = _createHeader();
      response.version = null;
      response.code = 0;
      response.message = null;
      response.body = null;
      response.headerReceived = false;
      response.bodyReceived = false;
      response.flashApi = null;
      
      /**
       * Reads a line that ends in CRLF from a byte buffer.
       * 
       * @param b the byte buffer.
       * 
       * @return the line or null if none was found.
       */
      var _readCrlf = function(b)
      {
         var line = null;
         var i = b.data.indexOf('\r\n', b.read);
         if(i != -1)
         {
            // read line, skip CRLF
            line = b.getBytes(i - b.read);
            b.getBytes(2);
         }
         return line;
      };
      
      /**
       * Parses a header field and appends it to the response.
       * 
       * @param line the header field line.
       */
      var _parseHeader = function(line)
      {
         var tmp = line.indexOf(':');
         var name = line.substring(0, tmp++);
         response.appendField(
            name, (tmp < line.length) ? line.substring(tmp) : '');
      };
      
      /**
       * Reads an http response header from a buffer of bytes.
       * 
       * @param b the byte buffer to parse the header from.
       * 
       * @return true if the whole header was read, false if not.
       */
      response.readHeader = function(b)
      {
         // read header lines (each ends in CRLF)
         var line = '';
         while(!response.headerReceived && line !== null)
         {
            line = _readCrlf(b);
            if(line !== null)
            {
               // parse first line
               if(_first)
               {
                  _first = false;
                  var tmp = line.split(' ');
                  if(tmp.length >= 3)
                  {
                     response.version = tmp[0];
                     response.code = parseInt(tmp[1], 10);
                     response.message = tmp.slice(2).join(' ');
                  }
                  else
                  {
                     // invalid header
                     throw {
                        message: 'Invalid http response header.',
                        details: {
                           'line': line
                        }
                     };
                  }
               }
               // handle final line
               else if(line.length == 0)
               {
                  // end of header
                  response.headerReceived = true;
               }
               // parse header
               else
               {
                  _parseHeader(line);
               }
            }
         }
         
         return response.headerReceived;
      };
      
      /**
       * Reads some chunked http response entity-body from the given buffer of
       * bytes.
       * 
       * @param b the byte buffer to read from.
       * 
       * @return true if the whole body was read, false if not.
       */
      var _readChunkedBody = function(b)
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
         
         var line = '';
         while(line !== null && b.length() > 0)
         {
            // if in the process of reading a chunk
            if(_chunkSize > 0)
            {
               // if there are not enough bytes to read chunk and its
               // trailing CRLF,  we must wait for more data to be received
               if(_chunkSize + 2 > b.length())
               {
                  break;
               }
               
               // read chunk data, skip CRLF
               response.body += b.getBytes(_chunkSize);
               b.getBytes(2);
               _chunkSize = 0;
            }
            // if chunks not finished, read the next chunk size
            else if(!_chunksFinished)
            {
               // read chunk-size line
               line = _readCrlf(b);
               if(line !== null)
               {
                  // parse chunk-size (ignore any chunk extension)
                  _chunkSize = parseInt(line.split(';', 1)[0], 16);
                  _chunksFinished = (_chunkSize == 0);
               }
            }
            // chunks finished, read trailers
            else
            {
               // read next trailer
               line = _readCrlf(b);
               while(line !== null)
               {
                  if(line.length > 0)
                  {
                     // parse trailer
                     _parseHeader(line);
                     // read next trailer
                     line = _readCrlf(b);
                  }
                  else
                  {
                     // body received
                     response.bodyReceived = true;
                     line = null;
                  }
               }
            }
         }
         
         return response.bodyReceived;
      };
      
      /**
       * Reads an http response body from a buffer of bytes.
       * 
       * @param b the byte buffer to read from.
       * 
       * @return true if the whole body was read, false if not.
       */
      response.readBody = function(b)
      {
         var contentLength = response.getField('Content-Length');
         var transferEncoding = response.getField('Transfer-Encoding');
         
         // read specified length
         if(contentLength !== null && contentLength >= 0)
         {
            response.body = response.body || '';
            response.body += b.getBytes(contentLength);
            response.bodyReceived = (response.body.length == contentLength);
         }
         // read chunked encoding
         else if(transferEncoding !== null)
         {
            if(transferEncoding.indexOf('chunked') != -1)
            {
               response.body = response.body || '';
               _readChunkedBody(b);
            }
            else
            {
               throw {
                  message: 'Unknown Transfer-Encoding.',
                  details: {
                     'transferEncoding' : transferEncoding
                  }
               };
            }
         }
         // read all data in the buffer
         else if((contentLength !== null && contentLength < 0) ||
                 (contentLength === null &&
                   response.getField('Content-Type') !== null))
         {
            response.body = response.body || '';
            response.body += b.getBytes();
            response.readBodyUntilClose = true;
         }
         else
         {
            // no body
            response.body = null;
            response.bodyReceived = true;
         }
         
         if(response.flashApi !== null &&
            response.bodyReceived && response.body !== null &&
            response.getField('Content-Encoding') === 'deflate')
         {
            // inflate using flash api
            response.body = util.inflate(response.flashApi, response.body);
         }
         
         return response.bodyReceived;
      };
           
      return response;
   };
   
   // public access to http namespace
   window.krypto = window.krypto || {};
   window.krypto.http = http;
})();

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
   
   // FIXME: move to member of http client
   // private var for flash with inflate/deflate calls
   var _api = null;
   
   /**
    * Creates an http client that uses krypto.net sockets as a backend and
    * krypto.tls for security.
    * 
    * @param options:
    *           flashId: the dom ID of the flash socket pool element to use.
    *           connections: number of connections to use to handle requests.
    *           init: true to initialize the flash, false not to (default).
    *           policyPort: the default flash policy port to use.
    *           msie: true if the browser is IE, false if not.
    */
   http.createClient = function(options)
   {
      // local alias for net
      var net = window.krypto.net;
      
      // initialize socket pool if appropriate
      if(options.init === true)
      {
         net.init(options);
      }
      
      // create client
      var client =
      {
         flashId: options.flashId,
         api: net.socketPools[options.flashId].api
      };
      
      // create sockets for connections
   };
   
   /**
    * Initializes flash support for inflate and deflate.
    * 
    * @param flashId the dom ID of the flash element with an api with inflate
    *           and deflate calls.
    */
   http.initFlashDeflateSupport = function(flashId)
   {
      _api = document.getElementById(flashId);
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
               [value.replace(/^\s*/, '').replace(/\s*$/, '')];
         },
         appendField: function(name, value)
         {
            name = _normalize(name);
            if(!(name in header.fields))
            {
               header.fields[name] = [];
            }
            header.fields[name].push(
               value.replace(/^\s*/, '').replace(/\s*$/, ''));
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
      if(_api !== null && request.getField('Accept-Encoding' === null))
      {
         request.setField('Accept-Encoding', 'deflate');
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
         
         // if the body isn't null, deflate it by default
         if(_api !== null && request.body !== null &&
            request.getField('Content-Encoding') === null &&
            !request.bodyDeflated)
         {
            // use flash to compress data
            request.setField('Content-Encoding', 'deflate');
            request.body = api.deflate(request.body);
            request.bodyDeflated = true;
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
                  _chunkSize = parseInt('0x' + line.split(';', 1)[0], 16);
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
         else if(contentLength !== null && contentLength < 0)
         {
            response.body = response.body || '';
            response.body += b.getBytes();
         }
         else
         {
            // no body
            response.body = null;
            response.bodyReceived = true;
         }
         
         if(_api !== null &&
            response.bodyReceived && response.body !== null &&
            response.getField('Content-Encoding') === 'deflate')
         {
            // inflate using flash api
            response.body = _api.inflate(response.body);
         }
         
         return response.bodyReceived;
      };
           
      return response;
   };
   
   // public access to http namespace
   window.krypto = window.krypto || {};
   window.krypto.http = http;
})();

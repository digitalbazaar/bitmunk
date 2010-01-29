/**
 * The webservices module is used to call remote webservices using JSON
 * serialization over HTTP.
 */

/**
 * Possible xhr states.
 */
const XHR_UNSENT           = 0;
const XHR_OPENED           = 1;
const XHR_HEADERS_RECEIVED = 2;
const XHR_LOADING          = 3;
const XHR_DONE             = 4;

/**
 * Sends an HTTP request, with optional JSON-formatted data, to the given URL.
 * 
 * If data is given, the method used will be POST. If it is not given, the
 * method used will be GET.
 * 
 * @param options:
 *           url: the URL to send the request to.
 *           data: the optional data to send (will be converted to JSON).
 *           success: the callback to call on success.
 *           error: the callback to call on error.
 *           complete: the callback to call on completion.
 */
function sendRequest(options)
{
   _bitmunkLog('sendRequest()');
   
   // set data to null if not specified
   if(options.data === undefined)
   {
      options.data = null;
   }
   
   var xhr = new XMLHttpRequest();
   if(!xhr)
   {
      _bitmunkLog('sendRequest(): could not create XMLHttpRequest');
      
      var obj =
      {
         code: 0,
         message: 'Could not create XMLHttpRequest.',
         type: 'webservices.NoXMLHttpRequest'
      };
      if(options.error)
      {
         options.error(null, obj);
      }
      if(options.complete)
      {
         options.complete(null, obj);
      }
   }
   else
   {
      _bitmunkLog('sendRequest(): setting up onreadystate');
      
      // called whenever the xhr state changes:
      xhr.onreadystatechange = function()
      {
         switch(xhr.readyState)
         {
            case XHR_HEADERS_RECEIVED:
               /* If the status is 204 (No Content) or if the content-length
                  is zero, then XMLHttpRequest will fail -- we preempt that
                  from happening here and simply return null as our response
                  data. */
               _bitmunkLog('sendRequest(): headers received');
               
               if(xhr.status == 204 ||
                  xhr.getResponseHeader('Content-Length') == 0)
               {
                  _bitmunkLog('sendRequest(): header 204 or 0 content-length');
                  
                  // call success
                  if(options.success)
                  {
                     _bitmunkLog('sendRequest(): calling success');
                     options.success(xhr, null);
                  }
                  // call complete
                  if(options.complete(xhr, null))
                  {
                     _bitmunkLog('sendRequest(): calling complete');
                     options.complete(xhr, null);
                  }
                  
                  // abort request
                  xhr.onreadystatechange = function(){};
                  xhr.abort();
               }
               break;
            case XHR_DONE:
               // an error occurred if the HTTP response status is 400+
               var error = (xhr.status >= 400);
               
               _bitmunkLog('sendRequest(): XHR done, error: ' + error);
               
               // convert content from JSON
               var obj;
               try
               {
                  // convert content from JSON
                  obj = JSON.parse(xhr.responseText);
                  _bitmunkLog('sendRequest(): JSON parsed');
               }
               catch(e)
               {
                  _bitmunkLog('sendRequest(): JSON parse error: ' + e);
                  
                  // exception
                  obj =
                  {
                     code: 0,
                     message:
                        'There was a problem converting the webservice ' +
                        'response from JSON: ' + e,
                     type: 'webservices.JsonParseError'
                  };
                  error = true;
               }
               
               if(error)
               {
                  if(options.error)
                  {
                     options.error(xhr, obj);
                  }
               }
               else if(options.success)
               {
                  options.success(xhr, obj);
               }
               if(options.complete)
               {
                  options.complete(xhr, obj);
               }
               break;
            default:
               // nothing to do
               break;
         }
      };
      
      // called when a connection error occurs
      xhr.onerror = function()
      {
         var obj =
         {
            code: 0,
            message: 'Could not connect to the webservice.',
            type: 'webservices.NoXMLHttpRequest'
         };
         if(options.error)
         {
            options.error(xhr, obj);
         }
         if(options.complete)
         {
            options.complete(xhr, obj);
         }
      };
      
      // open the URL
      var method = options.data ? 'POST' : 'GET';
      xhr.open(method, options.url, true);
      
      // set request headers
      xhr.setRequestHeader('Accept', 'application/json');
      if(options.data)
      {
         xhr.setRequestHeader('Content-Type', 'application/json');
      }
      
      // send any data asynchronously
      xhr.send(options.data);
   }
}

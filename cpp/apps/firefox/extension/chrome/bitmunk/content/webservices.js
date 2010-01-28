/**
 * The webservices module is used to call remote webservices using the
 * JSON serialization protocol and HTTP post. It is a much simpler
 * form of RPC than SOAP or XMLRPC, thus it is easier to understand,
 * implement and debug. 
 */

/**
 * Sends an XML HTTP request to a given URL and calls a given callback
 * with the data.
 *
 * @param url the URL to call
 * @param callback the callback to call with the returned data.
 * @param postData the data to use when posting to the given URL.
 */
function sendJsonRequest(url, callback, callbackData, postData)
{
   var method = (postData) ? "POST" : "GET";
   var xhr = new XMLHttpRequest();

   if(!xhr)
   {
      return;
   }
   
   // The on ready request state change function is called whenever
   // the state of the connection is updated. It is used to detect
   // when the data is ready.
   xhr.onreadystatechange = function()
   {
      // Check the content length when the ready-state is 2, because if we
      // don't abort when the content length is 0, XMLHttpRequest will fail.
      if(this.readyState == 2)
      {
         // check the content length if it is set
         var length = this.getResponseHeader('Content-Length');
         
         // if the content length is 0, abort the request and do the
         // callback
         if((length == 0) || this.status == 204)
         {
            // perform the callback with an empty JSON object.
            if(callbackData)
               convertJsonObject("{}", callback, callbackData);
            else
               convertJsonObject("{}", callback);
         
            // abort the request
            this.onreadystatechange = function(){};
            this.abort();
         }
      }
      else if(this.readyState == 4)
      {
         if(this.status >= 400)
         {
            // if the request failed for some reason, state the reason
            var message = "The HTTP status code was " + this.status +
                          ", which means there was a non-fatal error."
            var errortype = "webservices.sendJsonRequest";
            convertJsonObject(
               '{"code":0,"message":"' + message + '","type":"' +
               errortype + '"}', callback);
         }
         else
         {
            // if the request succeeded and we got a 200 OK, then
            // continue processing
            if(callbackData)
               convertJsonObject(this.responseText, callback, callbackData);
            else
               convertJsonObject(this.responseText, callback);
         }
      }
   };
   
   // The on error function is called whenever there is a connection
   // error. We still want to execute the callback function if there
   // is an error.
   xhr.onerror = function()
   {
      // if the request failed for some reason, state the reason
      message = "The HTTP request failed because the server could not " +
                "be contacted.";
      errortype = "webservices.sendJsonRequest";
      convertJsonObject(
         '{"code":0,"message":"' + message + '","type":"' + errortype + '"}',
         callback);
   };
   
   // open the URL and set the request header and send the data
   xhr.open(method, url, true);
   
   // if we're posting data, set the content type to JSON
   if(postData)
   {
      xhr.setRequestHeader('Content-Type', 'application/json');
   }
   xhr.setRequestHeader('Accept', 'application/json');
   
   // send the data
   xhr.send(postData);
}

/**
 * Parses JSON text, creates an object, and calls a callback with the
 * given callback data, if provided.
 *
 * @param jsonText the text encoded in JSON.
 * @param callback the function that should be called once the JSON
 *                 data has been turned into Javascript object form (optional).
 * @param callbackData the data that should be sent to the callback
 *                     function (optional).
 */ 
function convertJsonObject(jsonText, callback, callbackData)
{
   var obj = {};

   try
   {
      obj = JSON.parse(jsonText);
   }
   catch(error)
   {
      obj.code = 0;
      obj.message = "JSON parse error. There was a problem decoding the " +
                    "JSON message that was sent from the server." + 
                    error.message;
      obj.type = "webservices.convertJsonObject";
   }

   if(callbackData)
   {
      callback(obj, callbackData);
   }
   else
   {
      callback(obj);
   }
}

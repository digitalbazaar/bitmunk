/**
 * Bitmunk Web UI XHR api (krypto.xhr backend)
 *
 * @author Dave Longley
 *
 * Copyright (c) 2010 Digital Bazaar, Inc. All rights reserved.
 */
(function($)
{
   // define the xhr interface
   bitmunk.xhr = {};
   
   /**
    * Initializes XHR support.
    * 
    * @param task the task to use to synchronize initialization.
    */
   bitmunk.xhr.init = function(task)
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
      
      // initialize krypto.xhr
      task.next(function(task)
      {
         window.krypto.xhr.init({
            url: 'https://' + window.location.host,
            flashId: 'socketPool',
            policyPort: cfg.policyServer.port,
            msie: $.browser.msie,
            connections: 10,
            caCerts: cfg.ssl.certificates,
            verify: function(c, verified, depth, certs)
            {
               // TODO: do bitmunk-peer-specific checks
               return verified;
            },
            persistCookies: true
         });
      });
   };
   
   /**
    * Extend krypto.xhr.
    */
   bitmunk.xhr.cleanup = window.krypto.xhr.cleanup;
   bitmunk.xhr.setCookie = window.krypto.xhr.setCookie;
   bitmunk.xhr.getCookie = window.krypto.xhr.getCookie;
   bitmunk.xhr.removeCookie = window.krypto.xhr.removeCookie;
   
   // create krypto.xhr with bitmunk logging
   bitmunk.xhr.create = function()
   {
      return window.krypto.xhr.create({
         logError: bitmunk.log.error,
         logWarning: bitmunk.log.warning,
         logDebug: bitmunk.log.debug,
         logVerbose: bitmunk.log.verbose
      });
   };
   
   // NOTE: xhr support is implicit and not a required dependency
   // plugin registered after registration code is initialized
   bitmunk._implicitPluginInfo.task =
   {
      pluginId: 'bitmunk.webui.core.Xhr',
      name: 'Bitmunk Xhr'
   };
})(jQuery);

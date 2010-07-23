/**
 * Bitmunk Web UI XHR api (forge.xhr backend)
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
      
      // initialize forge.xhr
      task.next(function(task)
      {
         window.forge.xhr.init({
            url: 'https://' + window.location.host,
            flashId: 'socketPool',
            policyPort: cfg.policyServer.port,
            msie: $.browser.msie,
            // use 3 concurrent TLS connections, 1 for event polling, 2
            // for api calls
            connections: 3,
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
    * Extend forge.xhr.
    */
   bitmunk.xhr.cleanup = window.forge.xhr.cleanup;
   bitmunk.xhr.setCookie = window.forge.xhr.setCookie;
   bitmunk.xhr.getCookie = window.forge.xhr.getCookie;
   bitmunk.xhr.removeCookie = window.forge.xhr.removeCookie;
   
   // create forge.xhr with bitmunk logging
   bitmunk.xhr.create = function(options)
   {
      options = $.extend({
         logError: bitmunk.log.error,
         logWarning: bitmunk.log.warning,
         logDebug: bitmunk.log.debug,
         logVerbose: bitmunk.log.verbose
      }, options);
      return window.forge.xhr.create(options);
   };
   
   // NOTE: xhr support is implicit and not a required dependency
   // plugin registered after registration code is initialized
   bitmunk._implicitPluginInfo.task =
   {
      pluginId: 'bitmunk.webui.core.Xhr',
      name: 'Bitmunk Xhr'
   };
})(jQuery);

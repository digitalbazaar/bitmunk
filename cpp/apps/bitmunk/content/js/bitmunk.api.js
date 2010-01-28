/**
 * Bitmunk Web UI --- Bitmunk API Support
 *
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 *
 * Copyright (c) 2008-2009 Digital Bazaar, Inc.  All rights reserved.
 */
(function($) {
   // logging category
   var cat = 'bitmunk.webui.core.Api';
   
   // event namespace
   var sNS = '.bitmunk-webui-core-Api';
   
   /**
    * bitmunk.api is a namespace for Bitmunk API values and manipulation
    * functions.  The following values should be used rather than directly
    * specifiying the API locations.
    *
    * Base URLs:
    *    localUrl: URL for the local node. (ie, 'http://localhost:1234/')
    *    bitmunkUrl: URL for the non-SSL Bitmunk service
    *       (ie, 'http://bitmunk.com/')
    *    secureBitmunkUrl: URL for the secure SSL Bitmunk service
    *       (ie, 'https://bitmunk.com/')
    * The common API root path:
    *    root: The common API root (ie, '/api/3.0/')
    * Base URLs plus the root:
    *    localRoot:  URL for the local API.
    *    bitmunkRoot: URL for the non-SSL Bitmunk API.
    *    secureBitmunkRoot: URL for the secure SSL Bitmunk API.
    *
    * The APIs can be setup with the setUrls() function if needed.  The
    * proxyUrl() function is used to properly format a URL to be used to access
    * remote services through the node's proxy.
    */
   bitmunk.api =
   {
      // The scheme, host, and port for local server connections.
      // Default to current server.
      localUrl: "",
      
      // The scheme, host, and port for Bitmunk server connections.
      // These values will be fetched from the local server configuration upon
      // login
      bitmunkUrl: "http://bitmunk.com",
      secureBitmunkUrl: "https://bitmunk.com",
      
      // Root path of API calls
      root: '/api/3.0/',
      root32: '/api/3.2/',
      
      /**
       * Configure core bitmunk parameters.  This will also setup related vars:
       * bitmunk.api.{localRoot,bitmunkRoot,secureBitmunkRoot}.
       *
       * @param cfg object with optional configuration keys and values.
       *       apiRoot: Base of all API calls (default: "/api/3.0/"
       *       localUrl: URL for local connections (default: current URL)
       *       bitmunkUrl: URL for non-SSL connections to Bitmunk
       *             (default: "http://bitmunk.com")
       *       secureBitmunkUrl: URL for SSL connections to Bitmunk
       *             (default: "https://bitmunk.com")
       */
      setUrls: function(cfg)
      {
         if(typeof(cfg) == 'object')
         {
            // set new value or default to current value
            bitmunk.api.root = cfg.apiRoot || bitmunk.api.root;
            bitmunk.api.localUrl = cfg.localUrl || bitmunk.api.localUrl;
            bitmunk.api.bitmunkUrl = cfg.bitmunkUrl || bitmunk.api.bitmunkUrl;
            bitmunk.api.secureBitmunkUrl =
               cfg.secureBitmunkUrl || bitmunk.api.secureBitmunkUrl;
         }
         bitmunk.api.localRoot = bitmunk.api.localUrl + bitmunk.api.root;
         bitmunk.api.localRoot32 = bitmunk.api.localUrl + bitmunk.api.root32;       
         bitmunk.api.bitmunkRoot = bitmunk.api.bitmunkUrl + bitmunk.api.root;
         bitmunk.api.secureBitmunkRoot =
            bitmunk.api.secureBitmunkUrl + bitmunk.api.root;
      },
      
      /**
       * Create a proxy URL.
       * @param options:
       *    timeout: the timeout, in milliseconds, for the proxy to wait for
       *             a response.
       *    url: the target URL as a full URL to any host or a partial path to
       *         connect to the local host.
       */
      proxyUrl: function(options)
      {
         options = $.extend({
            url: null,
            timeout: null
         }, options);
         
         var params =
         {
            url: options.url
         };
         if(options.timeout !== null)
         {
            params.timeout = options.timeout;
         }
         
         return bitmunk.api.localRoot + 'webui/proxy?' + $.param(params);
      },
      
      /**
       * Performs an HTTP request via a BTP proxy. This method is used in
       * the same manner that a regular ajax call is used, but it will speak
       * through the BPE's btp proxy.
       * 
       * @param options:
       *    method: the HTTP method, i.e. GET, POST, PUT, DELETE.
       *    url:
       *       the non-proxied URL to post to, for example 
       *       "/api/3.0/system/directives/process/42".
       *    data:
       *       if specified, an object to be converted to json and sent to
       *       the given URL.
       *    beforeSend:
       *       if specified, a function to call before sending post data.
       *    success:
       *       if specified, a function to call if an HTTP success code
       *       is received.
       *    error:
       *       if specified, a function to call if an HTTP error code
       *       is received.
       *    complete:
       *       if specified, a function to call regardless of success or
       *       failure.
       *    global:
       *       false to stop the global error handler.
       *    cache:
       *       true for AJAX call to try and cache resources, false to try and
       *       disable caching. 
       *    timeout:
       *       the proxy timeout to use, in seconds.
       *    ajaxTimeout:
       *       the AJAX timeout to the proxy to use, in milliseconds.
       *       (Using milliseconds rather than seconds to match units of AJAX
       *       call.)
       */
      proxy: function(options)
      {
         options = $.extend({
            data: null,
            beforeSend: function() {},
            success: function() {},
            error: function() {},
            complete: function() {},
            xhrOptions: {}
         }, options);
         
         // include query params if specified
         if(options.params)
         {
            options.url += '?' + $.param(options.params);
         }
         
         // create the proxy URL
         var proxyUrl = bitmunk.api.proxyUrl({
            url: options.url,
            timeout: options.timeout
         });
         
         // create the jQuery AJAX options
         var ajaxOptions =
         { 
            type: options.method,
            url: proxyUrl,
            dataType: 'text',
            beforeSend: options.beforeSend,
            success: options.success,
            error: options.error,
            complete: options.complete,
            xhr: function()
            {
               return bitmunk.xhr.create(options.xhrOptions);
            }
         };
         // optional options
         if('cache' in options)
         {
            ajaxOptions.cache = options.cache;
         }
         if('ajaxTimeout' in options)
         {
            ajaxOptions.timeout = options.ajaxTimeout;
         }
         
         // include data if specified
         if(options.data)
         {
            ajaxOptions.contentType = 'application/json';
            ajaxOptions.data = JSON.stringify(options.data);
         }
         
         // update global if specified
         if(options.global === false)
         {
            ajaxOptions.global = false;
         }
         
         // do ajax 
         $.ajax(ajaxOptions);
      }
   };
   
   // setup default *Root vars via config call
   bitmunk.api.setUrls();
   
   /**
    * Handle configuration changes.
    */
   // FIXME: The config model does not keep track of node config so this is
   // FIXME: currently just updated on initial config load.
   $(bitmunk.api).bind('bitmunk-node-config-changed' + sNS,
      function(e, config)
      {
         var url;
         url = bitmunk.util.getPath(config, ['bitmunkUrl']);
         if(typeof(url) != 'undefined')
         {
            bitmunk.api.setUrls({
               bitmunkUrl: url
            });
         }
         url = bitmunk.util.getPath(config, ['secureBitmunkUrl']);
         if(typeof(url) != 'undefined')
         {
            bitmunk.api.setUrls({
               secureBitmunkUrl: url
            });
         }
      });
   
   // register plugin
   var defaults = $.extend(true, {}, bitmunk.sys.corePluginDefaults);
   bitmunk.sys.register($.extend(defaults, {
      pluginId: cat,
      name: 'Bitmunk Api'
   }));
})(jQuery);

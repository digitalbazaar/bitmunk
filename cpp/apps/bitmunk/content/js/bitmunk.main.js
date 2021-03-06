/**
 * Bitmunk Web UI --- Main Functions
 *
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 *
 * Copyright (c) 2008-2009 Digital Bazaar, Inc.  All rights reserved.
 */
jQuery(function($) {
   // logging category
   var cat = 'bitmunk.webui.Main';
   var sLogCategory = cat;
   
   // event namespace
   var sNS = '.bitmunk-webui-Main';
   
   // prefix for document (ie, web page) titles
   bitmunk.baseTitle = 'Bitmunk Personal Edition - ';
   
   // plugin id for main app
   bitmunk.mainPluginId = 'bitmunk.webui.Main';
   
   // common static view ids and the last view ID
   var sViewIds =
   {
      loading: '__loading__',
      errorLoading: '__errorLoading__',
      unknown: '__unknown__',
      login: 'login',
      offline: 'offline',
      main: 'main',
      last: 'purchases',
      _default: 'purchases'
   };
   // debug access
   bitmunk.debug.set(cat, 'viewIds', sViewIds);
   
   // the config loaded after logging in
   var sConfig = null;
   // debug access
   bitmunk.debug.set(cat, 'config', sConfig);
   
   // This tabbed pane contains the main visual content. This will be
   // either the login page, the main tabbed pane, or an offline notice.
   var sDesktopPane;
   
   // The screen pane with menu and content views.
   var sScreenPane;
   
   // Always bind external links to open in a new window to avoid
   // single-page application from stopping due to page navigation.
   $('a.external').live('click', function()
   {
       window.open($(this).attr('href'));
       return false;
   });
   
   // set jGrowl defaults
   $.jGrowl.defaults.closer = false;
   $.jGrowl.defaults.glue = 'before';
   $.jGrowl.defaults.life = 6000;
   
   /**
    * Offline view info and functions.
    */
   var sOffline = {
      // target for reload. url or null for current location.
      target: null,
      // null or message to display as reason for being on offline page.
      message: null,
      /*
      // FIMXE: polling doesn't work due to cross-domain/jsonp/timeout issues
      // true to poll target ping service until online. will reload target once
      // successful.
      poll: true,
      // if currently polling
      polling: false,
      // do polling
      doPoll: function()
      {
         var url = bitmunk.api.localRoot + 'system/test/echo'; 
         bitmunk.log.info(cat, 'offline poll url:', url);
         $.ajax({
            type: 'GET',
            url: url,
            dataType: 'jsonp',
            timeout: 1000,
            cache: false,
            success: function(obj) {
               bitmunk.log.info(cat, 'offline success', arguments);
            },
            error: function(xhr, textStatus, errorThrown) {
               bitmunk.log.info(cat, 'offline error', arguments);
               setTimeout(sOffline.doPoll,1000);
            },
            complete: function(xhr, textStatus, errorThrown) {
               bitmunk.log.info(cat, 'offline complete', arguments);
            }
         });
      },
      */
      show: function(task)
      {
         // set the offline message if available
         if(sOffline.message !== null)
         {
            $('.offlineMessage').html(sOffline.message).show();
         }
         else
         {
            $('.offlineMessage').hide();
         }
         // set reload link for readability and click handler to force reload
         $('a.offlineReloadLink').attr('href',
            (sOffline.target !== null) ? sOffline.target : '#');
         $('a.offlineReloadLink').click(function()
         {
            if(sOffline.target === null)
            {
               window.location.reload();
               return false;
            }
            return true;
         });
         //if(sOffline.poll)
         //{
         //}
      },
      hide: function(task)
      {
         //sOffline.polling = false;
         $('.offlineMessage').hide();
      }
   };
   
   // show offline view when we detect that the backend disappeared
   $(window)
      .bind('bitmunk-offline' + sNS, function(e, options) {
         bitmunk.log.debug(cat, 'got %s', e.type, e, options);
         // if a port was specified, create a new URL to redirect to
         if(options.port)
         {
            // create URL from window location and using new port
            var url =
               window.location.protocol + "//" +
               window.location.hostname + ":" +
               options.port;
            
            sOffline.target = url;
         }
         else
         {
            sOffline.target = null;
         }

         // set the offline message if available
         sOffline.message =
            (options && 'message' in options) ? options.message : null;
         
         // clean up event system
         bitmunk.events.cleanup();
         
         // stop models
         bitmunk.task.start(
         {
            type: 'stop models',
            name: 'main/offline',
            run: bitmunk.model.stop,
            success: function(task)
            {
               bitmunk.load(sViewIds.offline);
            },
            failure: function(task)
            {
               bitmunk.load(sViewIds.offline);
            }
         });
      })
      .bind('bitmunk-logout' + sNS, function(e) {
         // FIXME: if a different user logs in need to clean up old state
         // (views and more)
         
         // clean up event system
         bitmunk.events.cleanup();
         
         // show login
         bitmunk.load(sViewIds.login);
         /*
         // set the login message if available
         if(obj && 'exception' in obj && 'message' in obj.exception) {
            $('#loginMessage').text(obj.exception.message); } else {
            $('#loginMessage').empty();
         }
         */
      });
   
   /**
    * Global AJAX error handler.  Attempts to determine if the error is a BTP
    * exception.  If it is a known exception then this function will fake a
    * logout.  This can happen for expired sessions or similar issues.
    * Unknown errors (such as failure to connect) are not handled here.
    *
    * @param event the error event.
    * @param xhr the XMLHttpRequest object that caused the error.
    * @param ajaxOptions settings for this request
    * @param thrownError error if one was caught
    */
   $(window).ajaxError(function(event, xhr, ajaxOptions, thrownError)
   {
      var err = function(msg) {
         bitmunk.log.debug(cat,
            'ajaxError: ' + msg, event, xhr, ajaxOptions, thrownError);
      };
      if(xhr.status === 0)
      {
         err('offline');
         // FIXME: for some page we want to go to offline page right away
         // for others we may want to check for N errors before assuming
         // we are really offline
         $.event.trigger('bitmunk-offline', [{
            message: 'Connection error.'
         }]);
      }
      else if(xhr.status >= 400)
      {
         var ex = null;
         try
         {
            ex = JSON.parse(xhr.responseText);
         }
         catch(e) {}
         if(ex !== null && typeof(ex) === 'object' && 'type' in ex)
         {
            err('status >= 400 exception');
            if(ex.type === 'bitmunk.webui.SessionManager.InvalidSession')
            {
               bitmunk.task.start(
               {
                  type: 'invalid session',
                  name: 'logout',
                  run: doLogout,
                  success: function(task)
                  {
                     // FIXME: send logout exception somehow
                     //exception
                  }
               });
            }
            else
            {
               bitmunk.log.warning(cat, 'Unhandled BTP exception.', ex);
            }
         }
         else
         {
            err('status >= 400');
         }
      }
   });
   
   /**
    * Trigger config changed events for parts of config that are needed before
    * models are started.
    */
   var triggerConfigChanged = function(config)
   {
      // FIXME: should have some registration scheme for this
      var parts = [
         ['node', 'bitmunk-node-config-changed'],
         ['bitmunk.webui.WebUi', 'bitmunk-webui-WebUi-config-changed']
      ];
      $.each(parts, function(i, part) {
         if(part[0] in config)
         {
            $.event.trigger(part[1], [config[part[0]]]);
         }
      });
   };
   
   /**
    * On successful login start the login tasks.
    */
   $(window)
      .bind('bitmunk-login-success' + sNS, function(e) {
         // start login task
         bitmunk.task.start(
         {
            type: 'bitmunk.view',
            name: 'login',
            run: function(task)
            {
               bitmunk.log.debug(cat, 'logged in and starting up');
               task.next(doLogin);
            }
         });
      });
   
   /**
    * Performs all of the necessary tasks after a login.
    */
   var doLogin = function(task)
   {
      var timer = +new Date();
      task.next(bitmunk.events.init);
      task.next(startLoading);
      task.next(loadConfiguration);
      task.next(loadPluginInfo);
      task.next(registerPluginLoaders);
      task.next(configurePlugins);
      task.next(callDidLoginHandlers);
      task.next(setupNavigation);
      task.next(pluginsLoaded);
      task.next(function(task)
      {
         timer = +new Date() - timer;
         bitmunk.log.info(cat, 'full login completed in ' + timer + ' ms');
         bitmunk.log.debug('timing',
            'full login completed in ' + timer + ' ms');
      });
   };
   
   // Storage for timeout id for showing loading page so it can be cancelled.
   var sLoadingTimeoutId = null;
   
   /**
    * Start the loading process.  Setup a delay to show loading page if this is
    * taking too long.
    */
   var startLoading = function(task)
   {
      // display the loading page after delay
      // cancel once loading is complete
      sLoadingTimeoutId = setTimeout(function()
      {
         sLoadingTimeoutId = null;
         // show loading page
         var request = bitmunk.util.makeRequest(sViewIds.loading);
         sDesktopPane.showNamedView(task, request);
      }, 300);
   };
   
   /**
    * Load configuration, trigger event, then load plugins.
    */
   var loadConfiguration = function(task)
   {
      var timer = +new Date();
      task.block();
      bitmunk.api.proxy({
         method: 'GET',
         url: bitmunk.api.localRoot + 'system/config',
         params: { nodeuser: bitmunk.user.getUserId() },
         beforeSend: function(xhr)
         {
            bitmunk.log.debug(cat, 'loading configuration');
         },
         success: function(data, textStatus)
         {
            timer = +new Date() - timer;
            bitmunk.log.debug('timing', 'config loaded in ' + timer + ' ms');
            timer = +new Date();
            // store config
            sConfig = JSON.parse(data);
            timer = +new Date() - timer;
            bitmunk.log.debug('timing', 'config parsed in ' + timer + ' ms');
            // notify watchers of new config details
            triggerConfigChanged(sConfig);
            task.unblock();
         },
         error: function(xhr, textStatus, errorThrown)
         {
            var logged = false;
            try
            {
               var exception = JSON.parse(xhr.responseText);
               if(exception.type && (
                  exception.type ==
                  'bitmunk.webui.SessionManager.InvalidSessionId' ||
                  exception.type ==
                  'bitmunk.webui.SessionManager.MissingCookie'))
               {
                  logged = true;
                  bitmunk.log.warning(cat,
                     'Expired or invalid session detected while loading ' +
                     'configuration.');
               }
            }
            catch(e)
            {
               // ignore
            }
            if(!logged)
            {
               bitmunk.log.error(cat,
                  'failure to load configuration', arguments);
            }
            // ajax error also handled via global handler
            task.fail();
         }
      });
   };

   /**
    * Load list of all available plugins.
    */
   var loadPluginInfo = function(task)
   {
      var timer = +new Date();
      task.block();
      $.ajax({
         type: 'GET',
         url: bitmunk.api.localRoot + 'webui/config/plugins?nodeuser=' +
              bitmunk.user.getUserId(),
         dataType: 'json',
         beforeSend: function(xhr) {
            bitmunk.log.debug(cat, 'loading plugins');
         },
         success: function(data, textStatus) {
            timer = +new Date() - timer;
            bitmunk.log.debug('timing',
               'plugin info loaded in ' + timer + ' ms');
            // pass plugin info onto next task
            // FIXME: this seems hackish
            // would be nicer to be able to do something like:
            //   task.sendToNext(args...)
            // and next task of same parent would be run like:
            //   function(task, args...)
            task.userData = {
               pluginInfo: data
            };
            task.unblock();
         },
         error: function(xhr, textStatus, errorThrown) {
            bitmunk.log.error(cat, 'failure to load plugins', arguments);
            // ajax error also handled via global handler
            task.fail();
         },
         xhr: bitmunk.xhr.create
      });
   };
   
   /**
    * Register and load plugin loaders.
    * 
    * @param task the task to use.
    */
   var registerPluginLoaders = function(task)
   {
      // register plugin loaders
      task.next(function(task)
      {
         $.each(task.userData.pluginInfo, function(i, info)
         {
            // FIXME: do not double-register login plugin, fix this so we
            // don't need this hack here (keep in mind that other plugins
            // do double-register in the current design (ie: Main registers
            // again in js/init.js)
            if(info.id !== 'bitmunk.webui.Login')
            {
               // register each resource
               bitmunk.resource.register(
               {
                  pluginId: info.id,
                  type: bitmunk.resource.types.pluginLoader,
                  init: info.init,
                  required: true
               });
            }
         });
      });
      
      // load plugin loaders
      task.next(function(task)
      {
         var timer = +new Date();
         task.next(bitmunk.resource.load);
         task.next(function(task)
         {
            timer = +new Date() - timer;
            bitmunk.log.debug('timing',
               'plugin loaders loaded in ' + timer + ' ms');
         });
      });
   };
   
   /**
    * Fire a second config event so plugins can configure themselves as needed.
    * Note: This may cause re-configuration of non-plugins.
    */
   var configurePlugins = function(task)
   {
      // notify watchers of config details
      triggerConfigChanged(sConfig);
   };
   
   /**
    * Call didLogin() handlers on the plugins.
    */
   var callDidLoginHandlers = function(task)
   {
      var timer = +new Date();
      
      // get all resources
      var resources = bitmunk.resource.get();
      // list of all handlers to run in parallel
      var handlers = [];
      // iterate over plugins to find handlers
      $.each(resources, function(id, pluginResources)
      {
         var options = resources[id][id].options;
         if(options.type == bitmunk.resource.types.plugin &&
            'didLogin' in options)
         {
            handlers.push(options.didLogin);
         }
      });
      task.parallel(handlers);
      
      task.next(function(task)
      {
         timer = +new Date() - timer;
         bitmunk.log.debug('timing',
            'did login handlers completed in ' + timer + ' ms');
      });
   };
   
   /**
    * Setup tabbed pane navigation.
    */
   var setupNavigation = function(task)
   {
      var timer = +new Date();
      
      // create menu array for sorting items according to priority
      var menuItems = [];
      
      // get all resources
      var resources = bitmunk.resource.get();
      
      // iterate over views, collecting menu items
      $.each(resources, function(id, pluginResources)
      {
         $.each(pluginResources, function(id, resource)
         {
            var options = resource.options;
            if(options.type == bitmunk.resource.types.view &&
               'menu' in options)
            {
               menuItems.push(
               {
                  order: options.menu.priority,
                  name: options.name,
                  viewId: options.hash
               });
            }
         });
      });
      
      // sort menu items (high to low)
      menuItems.sort(function(b, a)
      {
         var ap = a.order;
         var bp = b.order;
         return (ap > bp) ? 1 : ((ap < bp) ? -1 : 0);
      });
      
      // clear old screen menu
      sScreenPane.clearMenu();
      
      // add menu items to screen
      $.each(menuItems, function(i, item)
      {
         sScreenPane.addMenuItem(item.name, item.viewId);
      });
      
      timer = +new Date() - timer;
      bitmunk.log.debug('timing', 'navigation setup in ' + timer + ' ms');
   };
   
   /**
    * Plugins are now loaded.
    */
   var pluginsLoaded = function(task)
   {
      var timer = +new Date();
      
      // cancel loading notification
      if(sLoadingTimeoutId !== null)
      {
         clearTimeout(sLoadingTimeoutId);
      }
      
      // show main view with plugins
      var request = bitmunk.util.makeRequest(sViewIds.main);
      sDesktopPane.showNamedView(task, request);
      
      // show last view
      task.next(function(task)
      {
         bitmunk.load(sViewIds.last);
      });
      
      task.next(function(task)
      {
         timer = +new Date() - timer;
         bitmunk.log.debug('timing', 'main view loaded in ' + timer + ' ms');
      });
   };
   
   /**
    * Performs all of the necessary tasks to logout.
    * 
    * @param task the task to synchronize with.
    */
   var doLogout = function(task)
   {
      // clear main screen pane
      task.next(sScreenPane.clear);
      
      // stop all models
      task.next(bitmunk.model.stop);
      
      // clean up event system
      task.next(bitmunk.events.cleanup);
      
      // set last view to default view for next user
      task.next(function(next)
      {
         sViewIds.last = sViewIds._default;
      });
      
      // call willLogout handlers
      task.next(callWillLogoutHandlers);
      
      // log out user
      task.next(function(task)
      {
         bitmunk.user.logout();
      });
   };
   
   /**
    * Call willLogout(task) handlers on the plugins.
    */
   var callWillLogoutHandlers = function(task)
   {
      // get all resources
      var resources = bitmunk.resource.get();
      // list of all handlers to run in parallel
      var handlers = [];
      // iterate over plugins to find handlers
      $.each(resources, function(id, pluginResources)
      {
         var options = resources[id][id].options;
         if(options.type == bitmunk.resource.types.plugin &&
            'willLogout' in options)
         {
            handlers.push(options.willLogout);
         }
      });
      task.parallel(handlers);
   };
   
   /**
    * Performs all of the necessary tasks to shut down the node.
    */
   var doShutdown = function(task)
   {
      bitmunk.log.debug(cat, 'Shutting down node.');
      
      // clean up event system
      task.next(bitmunk.events.cleanup);
      
      // do shutdown
      task.next(function(task)
      {
         // FIXME: call bitmunk.user.logout() first?
         bitmunk.api.proxy({
            method: 'POST',
            url: bitmunk.api.localRoot + 'system/control/state',
            params: { nodeuser: bitmunk.user.getUserId() },
            data: {state: 'shutdown'},
            // Successful shutdown call triggers offline event.
            // Errors handled with global handler.  This may result is the user
            // seeing a login screen due to session timeouts, or connection
            // errors if the node is already offline.
            success: function(data, textStatus) {
               bitmunk.log.debug(cat, 'shutdown success', data, textStatus);
               $.event.trigger('bitmunk-offline', [{
                  message: 'Successful shutdown.'
               }]);
               // send offline event to bitmunk extension
               var ev = document.createEvent("Events");
               ev.initEvent("bpe-offline", true, false);
               document.dispatchEvent(ev);
            }
         });
      });
   };
   
   bitmunk.shutdown = function()
   {
      bitmunk.task.start(
      {
         type: 'bitmunk.view',
         name: 'main/shutdown',
         run: doShutdown
      });
   };
   
   /**
    * Performs all of the necessary tasks to restart the node.
    * 
    * @param task the task to synchronize on.
    * @param port the port to connect to the node when it comes back online.
    */
   var doRestart = function(task, port)
   {
      bitmunk.log.debug(cat, 'Restarting node.');
      
      // clean up event system
      task.next(bitmunk.events.cleanup);
      
      // do shutdown
      task.next(function(task)
      {
         // FIXME: call bitmunk.user.logout() first?
         bitmunk.api.proxy({
            method: 'POST',
            url: bitmunk.api.localRoot + 'system/control/state',
            params: { nodeuser: bitmunk.user.getUserId() },
            data: {state: 'restart'},
            // Successful shutdown call triggers offline event.
            // Errors handled with global handler.  This may result is the user
            // seeing a login screen due to session timeouts, or connection
            // errors if the node is already offline.
            success: function(data, textStatus)
            {
               bitmunk.log.debug(cat, 'shutdown success', data, textStatus);
               $.event.trigger('bitmunk-offline', [{
                  message: 'Restarting...',
                  port: port
               }]);
            }
         });
      });
   };
   
   /**
    * Restarts the backend node.
    * 
    * @param port the port to use to connect to the node when it comes back
    *             online.
    */
   bitmunk.restart = function(port)
   {
      bitmunk.task.start(
      {
         type: 'bitmunk.view',
         name: 'main/restart',
         run: function(task)
         {
            doRestart(task, port);
         }
      });
   };
   
   /**
    * Show the view from the given request.
    * 
    * @param request the request.
    */
   var showView = function(request)
   {
      // cancel any other bitmunk.view tasks
      bitmunk.task.cancel('bitmunk.view');
      
      // FIXME: handle invalid view IDs
      bitmunk.task.start(
      {
         type: 'bitmunk.view',
         name: 'main/history/showView',
         run: function(task)
         {
            task.next('main/history/checklogin', function(task)
            {
               // if the user is not logged in, show the login page
               if(!bitmunk.user.isLoggedIn())
               {
                  var loginReq = bitmunk.util.makeRequest(sViewIds.login);
                  sDesktopPane.showNamedView(task, loginReq);
               }
               // user already logged in
               else
               {
                  var viewId = request.getPath(0);
                  switch(viewId)
                  {
                     case sViewIds.login:
                        // already logged in, show default view
                        bitmunk.load(sViewIds._default);
                        break;
                     case sViewIds.offline:
                        sDesktopPane.showNamedView(task, request);
                        break;
                     default:
                        // handle views that are in the screen pane
                        var desktopReq =
                           bitmunk.util.makeRequest(sViewIds.main);
                        sDesktopPane.showNamedView(task, desktopReq);
                        sScreenPane.showNamedView(task, request);
                        break;
                  }
               }
            });
         },
         failure: function(task)
         {
            bitmunk.log.verbose(cat, 'history showView task: failure');
         }
      });   
   };
   
   /**
    * Initializes the desktop interface.
    */
   var initDesktopInterface = function(task)
   {
      bitmunk.log.debug(cat, 'initializing desktop interface');
      sDesktopPane = bitmunk.tabbedpane.create(
      {
         name: 'desktop'
      });
   };
   
   /**
    * Initializes the screen interface.
    */
   var initScreenInterface = function(task)
   {
      bitmunk.log.debug(cat, 'initializing screen interface');
      sScreenPane = bitmunk.tabbedpane.create(
      {
         name: 'screen',
         show: function(task)
         {
            $('#logoutLink').click(function() {
               bitmunk.task.start(
               {
                  type: 'bitmunk.view',
                  name: 'main/click/logout',
                  run: doLogout
               });
               return false;
            });
            $('#shutdownLink').click(function() {
               bitmunk.shutdown();
               return false;
            });
         },
         hide: function(task)
         {
            $('#logoutLink').unbind('click');
            $('#shutdownLink').unbind('click');
         }
      });
   };
   
   /**
    * Alias for history plugin's load().
    */
   bitmunk.load = function(hash)
   {
      return $.history.load(hash);
   };
   
   var initMain = function(task)
   {
      // Register the main plugin here so that some low-level resources can be
      // pre-loaded.  The plugin will be re-registered later as a generic
      // plugin and overwrite the simple details here.
      task.next('register pre-main plugin', function(task)
      {
         bitmunk.log.debug(cat, 'pre-load pre-main plugin');
         
         var defaults = $.extend(true, {}, bitmunk.sys.corePluginDefaults);
         bitmunk.resource.register($.extend(defaults,
         {
            pluginId: bitmunk.mainPluginId,
            name: 'Bitmunk Main',
            resources: [
               {
                  type: bitmunk.resource.types.view,
                  resourceId: sViewIds.loading,
                  hash: sViewIds.loading,
                  name: 'Loading',
                  html: 'loading.html',
                  required: true
               },
               {
                  type: bitmunk.resource.types.view,
                  resourceId: sViewIds.errorLoading,
                  hash: sViewIds.errorLoading,
                  name: 'Error Loading',
                  html: 'errorLoading.html',
                  required: true
               },
               {
                  type: bitmunk.resource.types.view,
                  resourceId: sViewIds.unknown,
                  hash: sViewIds.unknown,
                  name: 'Unknown',
                  html: 'unknown.html',
                  required: true
               },
               {
                  /**
                   * HTML node and content to use when node is offline.
                   * This may be used when node is shutdown or repeated errors
                   * occur. In either case, the loaded JS can no longer contact
                   * the backend to retrieve any other HTML to display.
                   * Required immediately.
                   */
                  type: bitmunk.resource.types.view,
                  resourceId: sViewIds.offline,
                  hash: sViewIds.offline,
                  name: 'Offline',
                  html: 'offline.html',
                  show: sOffline.show,
                  hide: sOffline.hide,
                  required: true
               }
            ]
         }));
         bitmunk.resource.register({
            pluginId: bitmunk.mainPluginId,
            resourceId: 'content',
            type: bitmunk.resource.types.view,
            name: 'Bitmunk Application',
            data: sDesktopPane,
            required: true
         });
         bitmunk.resource.register({
            pluginId: bitmunk.mainPluginId,
            resourceId: sViewIds.main,
            hash: sViewIds.main,
            type: bitmunk.resource.types.view,
            name: 'Main',
            show: sScreenPane.show,
            hide: sScreenPane.hide,
            html: 'screenPane.html',
            required: true
         });
         // add path to screen pane resource
         bitmunk.resource.extendResource({
            pluginId: bitmunk.mainPluginId,
            resourceId: 'screenPane.html',
            path: '/bitmunk/screenPane.html'
         });
      });
      task.next('register bitmunk.webui.Login', function(task)
      {
         // login plugin must always be available
         bitmunk.resource.register({
            pluginId: 'bitmunk.webui.Login',
            type: bitmunk.resource.types.pluginLoader,
            required: true
         });
      });
      // load all required resources
      task.next(bitmunk.resource.load);
   };
   
   /**
    * Starts Bitmunk.
    */
   bitmunk.main = function()
   {
      bitmunk.log.verbose(cat, 'main: started');
      bitmunk.log.info(cat, 'starting up...');
      
      bitmunk.task.start({
         type: 'bitmunk.main',
         name: 'main',
         run: function(task)
         {
           // startup timer
            var timer = +new Date();
            
            task.next('main/start', function(task)
            {
               bitmunk.log.verbose(cat, 'main task: started');
            });
            task.next('sys/init', bitmunk.sys.init);
            task.next('xhr/init', bitmunk.xhr.init);
            task.next('main/init/screen-interface', initScreenInterface);
            task.next('main/init/desktop-interface', initDesktopInterface);
            task.next('main/init', initMain);
            task.next('main/history', function(task)
            {
               bitmunk.log.debug(cat, 'main triggering start event');
               /*
                Note: The current version of this jquery history plugin will
                automatically call the function we pass to it as soon as we
                call init() on it. We are not set up right here to handle this
                so to make sure our function doesn't do anything until we are
                we set an initialized flag.
                */
               var historyInitialized = false;
               $.history.init(function(fragment)
               {
                  var request = bitmunk.util.makeRequest(fragment);
                  var viewId = request.getPath(0);
                  
                  // if view ID is invalid, make default request
                  if(!viewId || viewId === '')
                  {
                     viewId = sViewIds._default;
                     request = bitmunk.util.makeRequest(viewId);
                  }
                  
                  if(historyInitialized)
                  {
                     showView(request);
                  }
                  else
                  {
                     // store the view ID so we can go here after we log in
                     sViewIds.last = (viewId == sViewIds.offline) ?
                        sViewIds._default : viewId;
                  }
               });
               historyInitialized = true;
            });
            task.next('main/desktop/show', function(task)
            {
               sDesktopPane.show(task);
            });
            task.next('main/logincheck', function(task)
            {
               // if the user is already logged in, do login
               if(bitmunk.user.isLoggedIn()) 
               {
                  task.next(doLogin);
                  task.next(function(task)
                  {
                     // show last view ID
                     bitmunk.load(sViewIds.last);
                  });
               }
               else
               {
                  // show login page
                  bitmunk.load(sViewIds.login);
               }
            });
            task.next('main.done', function(task)
            {
               timer = +new Date() - timer;
               bitmunk.log.verbose(cat, 'main task: done');
               bitmunk.log.info(cat, 'started in ' + timer + ' ms');
               bitmunk.log.debug('timing', 'main started in ' + timer + ' ms');
            });
         },
         failure: function(task)
         {
            bitmunk.log.verbose(cat, 'main task: failure');
         }
      });
      bitmunk.log.verbose(cat, 'main: done');
   };
   
   /**
    * Gets the current view ID.
    * 
    * @return the current view ID.
    */
   bitmunk.getCurrentViewId = function()
   {
      return sScreenPane.currentViewId;
   };
});

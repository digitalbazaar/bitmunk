/**
 * Bitmunk Web UI --- System Support (resource registration, loading, more)
 *
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 *
 * Copyright (c) 2008-2009 Digital Bazaar, Inc.  All rights reserved.
 */
(function($) {
   // logging category
   var cat = 'bitmunk.webui.core.Resource';

   // event namespace
   var sNS = '.bitmunk-webui-core-Resource';

   /**
    * Root site path for resources.  Final form is:
    * <resourceRootPath>/<plugin id>/<resource id>
    * where <resource id> could have path components (ie, object/prop)
    */
   var sResourceRootPath = '/bitmunk/plugins';
   
   /**
    * The resources list is defined as:
    * {
    *    <plugin id>: {
    *       <resource id>: <resource>,
    *       ...
    *    },
    *    ...
    * }
    *
    * A resource object is defined as:
    * {
    *    resourceId: resource id (string),
    *    pluginId: plugin id (string),
    *    type: resource type (string),
    *    data: resource data (any type, optional, default: null),
    *    options: registration options (object),
    *    loadedDate: timestamp when this resource was last loaded or null
    *    state: load state (string, see bitmunk.resource.loadState)
    *    // FIXME: load/unload hooks? events?
    *    // ref/unref, watch/unwatch events, ...
    * }
    *
    * See bitmunk.resource.types for valid type values.
    *
    * Note that plugins are themselves resources and are stored with a
    * resource id of the plugin id.  In this case the plugin field will be a
    * duplicate of the data field.
    *
    * Resource objects trigger the following events:
    * @event bitmunk-resource-loaded
    *        Triggered after the resource is loaded.
    * @event bitmunk-resource-unloaded
    *        Triggered after the resource is unloaded.
    */
   var sResources = {};
   // debug access
   bitmunk.debug.set(cat, 'resources', sResources);
   
   // FIXME change this way of named views to something else?
   /**
    * Map of ids to named views.
    */
   var sNamedViews = {};
   // debug access
   bitmunk.debug.set(cat, 'namedViews', sNamedViews);
   
   /**
    * Script loader tasks.
    *
    * This structure has the following format:
    * sScriptTasks[pluginId][resourceId] = task
    *
    * When code loads javascript it will add the loading task into this
    * structure and block. The javascript being loaded must retreive the task,
    * optionally use it to register or load resources, and then unblock the
    * task.
    */
   var sScriptTasks = {};
   // debug access
   bitmunk.debug.set(cat, 'scriptTasks', sScriptTasks);
   
   /**
    * Resource configuration.
    */
   var sConfig =
   {
      /**
       * Start with global caching off to ensure as least the user cache config
       * option will be loaded properly.
       */
      cache: {
         resources: false,
         lock: false
      }
   };
   // debug access
   bitmunk.debug.set(cat, 'config', sConfig);
   
   /**
    * Low level interface to set a resource.
    */
   var setResource = function(resource)
   {
      bitmunk.util.setPath(sResources,
         [resource.pluginId, resource.resourceId], resource);
   };
   
   /**
    * Low level interface to delete a resource.
    */
   var deleteResource = function(resource)
   {
      bitmunk.util.pathDelete(sResources,
         [resource.pluginId, resource.resourceId]);
   };
   
   /**
    * Map from bitmunk.resource.types type to object with load and unload
    * functions.
    */
   var sTypeHandlers = {};
   // handlers defined below bitmunk.resource

   bitmunk.resource =
   {
      /**
       * Known types.
       */
      types:
      {
         css: 'text/css',
         data: 'application/vnd.bitmunk.webui.data',
         dom: 'application/vnd.bitmunk.webui.dom',
         html: 'text/html',
         javascript: 'application/javascript',
         json: 'application/json',
         // plugin loader info
         pluginLoader: 'application/vnd.bitmunk.webui.plugin.loader',
         plugin: 'application/vnd.bitmunk.webui.plugin',
         text: 'text/plain',
         template: 'text/x-trimpath-template',
         view: 'application/vnd.bitmunk.webui.view'
      },
      
      /**
       * Info for specific types.
       *
       * ajaxType: dataType for jQuery AJAX requets
       */
      typeInfo:
      {
         'text/html': {
            ajaxType: 'html'
         },
         'application/json': {
            ajaxType: 'json'
         },
         'text/x-trimpath-template': {
            ajaxType: 'text'
         },
         'text/plain': {
            ajaxType: 'text'
         }
      },
      
      /**
       * State of a resource.
       */
      loadState:
      {
         unloaded: 'unloaded',
         loading: 'loading',
         loaded: 'loaded',
         unloading: 'unloading'
      },
      
      /**
       * Register a resource.
       *
       * This function registers a named resource of some given type. It has
       * the option to also load the resource or provide the resource data to
       * register. If a resource is not immediately loaded its info is still
       * available and it can be loaded with a load() call at any time. Using
       * the 'data' option for supported resources will set the resource as
       * immediately loaded. Care should be taken to use appropriate data.
       *
       * General options:
       *    pluginId: plugin id of the owner of this resource
       *       has the form: bitmunk.topic.Name
       *    resourceId: resource id
       *       for plugins: defaults to pluginId (ignored)
       *       other resources: a resource id (required)
       *    type: bitmunk.resource.types id (required)
       *    depends: resources this resource depends on (optional)
       *       has the form {pluginId:[resourceId, ...], ...}
       *       
       * Loadable resources (files, plugins):
       *    load: load resource now (optional, default: true)
       *    recursive: load sub-resources (optional, default: false)
       *    cache: caching hint to browser (optional default: true)
       *    save: save resource after loading (optional, default: true)
       *       This can be used if data is just needed in success().
       *    task: task to use for async calls (required)
       *
       * Data-like resources (data, dom, html, json, text, template):
       *    data: reference to data (optional, default: undefined)
       *
       * File-like resources:
       *    rootPath: override root resource path (optional)
       *    pluginPath: override plugin path (optional, default: pluginId)
       *    resourcePath: override resource path (optional, default: resourceId)
       *    path: override the full path
       *       (optional: default rootPath/pluginId/resourceId)
       *
       * Parsable resources (html, json, template):
       *    parse: parse to native form (optional, default: true)
       *       HTML => DOM, JSON => object/array, template => template object
       *
       * View options:
       *    hash: hash id to access the view (optional)
       *    name: name of the view (required)
       *    html: html file name which contains the view contents (optional)
       *    scripts: array of JavaScript file names (optional)
       *    subScripts: if any of the entries in the "scripts" option are
       *        virtual scripts then subScripts should list those scripts and
       *        list the real files that compose each scripts.  This is done so
       *        script loading can be synchronized and the loader knows what
       *        real scripts to wait on. (required if using virtual scripts)
       *        Map of script resource names to an array of real script
       *        resource names.
       *    css: array of CSS file names (optional)
       *    templates: array of template file names to lazy load (optional)
       *    menu: object of menu options (optional, default: no menu)
       *        priority: 0.0 to 1.0 to sort menus in the UI
       *    hide: function to call to hide a view
       *    show: function to call to show a view
       *
       * Plugin Loader options:
       *    loadInit: try and load the init script
       *       (boolean, optional, default: true)
       *    init: init script (string, optional, default: init.js)
       *
       * Plugin options:
       *    name: simple text name (required)
       *    homepage: homepage url (optional)
       *    authors: array of objects: (optional)
       *       name: name of author (required)
       *       email: email of author (optional)
       *    scripts: array of script resource to immediately load (optional)
       *    resources: list of plugin resources (optional)
       *
       * FIXME: remove
       *    // these should be done with login plugin hooks 
       *    didLogin: function to call to initialize this plugin after a login
       *          but before UI is shown.  Called as didLogin(callbacks).
       *          Implementors MUST call one of the callbacks, success() or
       *          error().
       *    willLogout: function to call to cleanup this plugin after UI is
       *          hidden but before logout.  Called as willLogout(callbacks).
       *          Implementors MUST call one of the callbacks, success() or
       *          error().  This call may not happen depending on user behavior.
       *
       * @param options see function description
       */
      register: function(options) {
         // flag if we need to try and load
         var loadCheck = false;
         // General options
         options = $.extend({
            pluginId: null,
            resourceId: null,
            type: null,
            depends: {}
         }, options);
         var type = options.type;
         // Task will be removed from options since options are saved with the
         // resource and we don't want to leak tasks.  Use a local var instead.
         var task = null;
         // Loadable resources options
         if(type == bitmunk.resource.types.css ||
            type == bitmunk.resource.types.html ||
            type == bitmunk.resource.types.javascript ||
            type == bitmunk.resource.types.json ||
            type == bitmunk.resource.types.pluginLoader ||
            type == bitmunk.resource.types.plugin ||
            type == bitmunk.resource.types.text ||
            type == bitmunk.resource.types.template ||
            type == bitmunk.resource.types.view)
         {
            loadCheck = true;
            options = $.extend({
               load: true,
               recursive: false,
               // use global option as default
               cache: sConfig.cache.resources,
               async: true,
               save: true
            }, options);
            // special handling for tasks so they don't leak when options are
            // saved
            if('task' in options)
            {
               task = options.task;
               delete options.task;
            }
         }
         // Data resources
         if(type == bitmunk.resource.types.data ||
            type == bitmunk.resource.types.dom ||
            type == bitmunk.resource.types.html ||
            type == bitmunk.resource.types.json ||
            type == bitmunk.resource.types.text ||
            type == bitmunk.resource.types.template)
         {
            // leave data undefined if not present
         }
         // File-like resources
         if(type == bitmunk.resource.types.css ||
            type == bitmunk.resource.types.html ||
            type == bitmunk.resource.types.javascript ||
            type == bitmunk.resource.types.json ||
            type == bitmunk.resource.types.pluginLoader ||
            type == bitmunk.resource.types.plugin ||
            type == bitmunk.resource.types.text ||
            type == bitmunk.resource.types.template ||
            type == bitmunk.resource.types.view)
         {
            // use path if specified, else create
            if(typeof(options.path) === 'undefined')
            {
               // create path using optional params
               var rootPath = typeof(options.rootPath) === 'undefined' ?
                  sResourceRootPath : options.rootPath;
               var pluginPath = typeof(options.pluginPath) === 'undefined' ?
                  options.pluginId : options.pluginPath;
               var resourcePath = typeof(options.resourcePath) === 'undefined' ?
                  options.resourceId : options.resourcePath;
               options.path = [rootPath, pluginPath, resourcePath].join('/');
            }
         }
         // Parsable resources
         if(type == bitmunk.resource.types.html ||
            type == bitmunk.resource.types.json ||
            type == bitmunk.resource.types.template)
         {
            options = $.extend({
               parse: true
            }, options);
         }
         // Views
         if(type == bitmunk.resource.types.view)
         {
         }
         // Plugin Related
         if(type == bitmunk.resource.types.pluginLoader ||
            type == bitmunk.resource.types.plugin)
         {
            // special case for plugins and pluginLoaders:
            //    resourceId == pluginId
            options.resourceId = options.pluginId;
         }
         // Plugin Loaders
         if(type == bitmunk.resource.types.pluginLoader)
         {
            options = $.extend({
               loadInit: true,
               init: 'init.js'
            }, options);
         }
         // Plugins
         if(type == bitmunk.resource.types.plugin)
         {
            options = $.extend({
               name: null,
               homepage: null,
               authors: [],
               scripts: []
               // ui field verified during load
            }, options);
         }
         
         // check for unset type
         if(options.type === null)
         {
            bitmunk.log.error(cat,
               'register called with unknown type:',
               options.type, options);
         }
         // check for valid pluginId
         else if(typeof(options.pluginId) !== 'string')
         {
            bitmunk.log.error(cat,
               'register called with no plugin id', options);
         }
         // check for valid resourceId
         else if(typeof(options.resourceId) !== 'string')
         {
            bitmunk.log.error(cat,
               'register called with no resource id', options);
         }
         // everyting valid - do registration
         else
         {
            var state = typeof(options.data) === 'undefined' ?
               bitmunk.resource.loadState.unloaded :
               bitmunk.resource.loadState.loaded;
            // register
            var r =
            {
               pluginId: options.pluginId,
               resourceId: options.resourceId,
               type: options.type,
               data: options.data,
               options: options,
               state: state
            };
            setResource(r);
            bitmunk.log.debug(cat, 'register: [%s][%s] (%s)',
               r.pluginId, r.resourceId, r.type);
            bitmunk.log.verbose(cat, 'register details:', options, r);
            // call type specific register handler if available
            if('register' in sTypeHandlers[r.type])
            {
               sTypeHandlers[r.type].register(task, r);
            }
            // fire resource related events
            var fireEvents = function()
            {
               // trigger global events, future events are just on $(r)
               $.event.trigger('bitmunk-resource-registered', [r]);
               if(r.state === bitmunk.resource.loadState.loaded)
               {
                  $(r).trigger('bitmunk-resource-loaded', [r]);
               }
            };
            // fire events in a task if task is set, else just fire now
            if(task)
            {
               task.next('register event', fireEvents);
            }
            else
            {
               fireEvents();
            }
            // 
            if(options.load)
            {
               bitmunk.log.verbose(cat, 'register should load: [%s][%s] (%s)',
                  r.pluginId, r.resourceId, r.type, options, r);
               task.next('register load', function(task)
               {
                  bitmunk.log.verbose(cat, 'register loading: [%s][%s] (%s)',
                     r.pluginId, r.resourceId, r.type, options, r);
                  bitmunk.resource.load({
                     resource: r,
                     task: task
                  });
               });
            }
         }
      },

      /**
       * Unregister a resource.
       *
       * This function unregisters a named resource. It first makes sure the
       * resource is unloaded.
       *
       * General options:
       *    id: resource id (required)
       *    pluginId: plugin id (required)
       *    success: callback on successful unregistration (optional)
       *    error: callback on unregistration error (optional)
       *    complete: callback always called after success or error (optional)
       *
       * @param options see function description
       */
      unregister: function(options) {
         // General options
         options = $.extend({
            id: null,
            pluginId: null,
            sucess: function() {},
            error: function() {},
            complete: function() {}
         }, options);
         /*
         bitmunk.resource.unload({
            success: function() {
               // do unreg
               // call success
            },
            error: function() {
               // log
               // call error
            },
            complete: function() {
               // log
               // call complete
            },
         });
         */
         // unregister
         var r = getResource(options.pluginId, options.id, true);
         deleteResource(r);
         bitmunk.log.debug(cat, 'unregister: [%s][%s] (%s)',
            r.pluginId, r.id, r.type);
         // trigger event and cleanup
         $(r).trigger('bitmunk-resource-unregistered', [r]);
         $(r).unbind();
      },

      /**
       * Load a resource.
       *
       * Most options are specified when registering a resource. Loading one
       * resource will also ensure any resources it depends on are also loaded.
       * Loading will block until the dependency chain is fully loaded.
       *
       * Types specific actions when loading and unloading:
       * (see bitmunk.resources.types for MIME types)
       * css: CSS stylesheet
       *    load: insert element into HEAD
       *    unload: remove element from HEAD
       * data, dom:
       *    load: reference options data
       *    unload: remove 'data' field
       * html: HTML
       *    load: async load, parse, and store jQuery object in 'data' field
       *    unload: delete 'data' field
       * javascript: JavaScript file 
       *    load: insert element into HEAD (will run the script)
       *    unload: nothing
       * json: JSON file 
       *    load: async load, parse, and store in 'data' field
       *    unload: delete 'data' field
       * text: plain text
       *    load: async load and store in 'data' field
       *    unload: delete 'data' field
       * template: plain text
       *    load: async load, create TrimPath template, and store in 'data'
       *       field (use obj.process(data) to render)
       *    unload: delete 'data' field
       * plugin, view:
       *    load: recursively async load sub resources and store info in 'data'
       *    unload: delete 'data' field
       *
       * @param options object with plugin options:
       *    resource: resource object (optional if ids specified)
       *    pluginId: plugin id of form bitmunk.topic.Name
       *       (optional if resource specified)
       *    resourceId: name of this resource
       *       (optional if resource specified or resource is a plugin)
       *    cache: override cache config setting
       *    force: force reload if resource is already loaded (Note: turns off
       *       caching and has no effect if resource is currently loading)
       *    task: task to use while loading
       */
      load: function(options)
      {
         options = $.extend({
            resource: null,
            pluginId: null,
            resourceId: null,
            force: false,
            task: null
            // FIXME: more defaults?
         }, options);
         // get the resource if needed
         if(options.resource === null)
         {
            options.resource = bitmunk.resource.get(
               options.pluginId, options.resourceId, false);
         }
         // options.resource alias
         var r = options.resource;
         if(options.task === null)
         {
            // guess which id info is more valid
            var rInfo = typeof(r) === 'undefined' ? options : r;
            bitmunk.log.error(cat,
               'load: no task specified [%s][%s]',
               rInfo.pluginId, rInfo.resourceId);
            bitmunk.log.verbose(cat, 'load: details:', options);
         }
         else if(typeof(r) === 'undefined')
         {
            bitmunk.log.error(cat,
               'load: unknown resource [%s][%s]',
               options.pluginId, options.resourceId);
            bitmunk.log.verbose(cat, 'load: details:', options);
            options.task.fail();
         }
         else
         {
            // setup cache if needed 
            if(typeof(options.cache) === 'undefined')
            {
               options.cache = r.options.cache;
            }
            bitmunk.log.debug(cat, 'load: [%s][%s] (%s)',
               r.pluginId, r.resourceId, r.type);
            bitmunk.log.verbose(cat, 'load details:', options);
            
            switch(r.state)
            {
               case bitmunk.resource.loadState.unloaded:
                  if(r.type in sTypeHandlers)
                  {
                     r.state = bitmunk.resource.loadState.loading;
                     options.task.next('load handler', function(task)
                     {
                        sTypeHandlers[r.type].load(options);
                     });
                     options.task.next('load done', function(task)
                     {
                        r.state = bitmunk.resource.loadState.loaded;
                        $.event.trigger('bitmunk-resource-loaded', [r]);
                     });
                  }
                  else
                  {
                     bitmunk.log.error(cat,
                        'load: unknown type [%s][%s] (%s)',
                        r.pluginId, r.resourceId, r.type);
                     bitmunk.log.verbose(cat, 'load: details:', options);
                     options.task.fail();
                  }
                  break;
               case bitmunk.resource.loadState.unloading:
                  // FIXME
                  break;
               case bitmunk.resource.loadState.loading:
                  // FIXME
                  break;
               case bitmunk.resource.loadState.loaded:
                  // already loaded, do nothing
                  // FIXME check force flag to reload
                  break;
            }
         }
      },
       
      /**
       */
      unload: function(options)
      {
         bitmunk.log.error(cat, 'FIXME: unload', options);
      },
      
      /**
       * Get a resource.  The value depends on the object type.
       * See load() for details.
       *
       * @param pluginId id of plugin that registered or loaded the resource
       * @param resourceId id of the resource
       * @param data true to get data, false for resource object (default: true)
       */
      get: function(pluginId, resourceId, getData)
      {
         bitmunk.log.verbose(cat,
            'getting resource [%s] [%s]', pluginId, resourceId);
         
         var path = [];
         if(typeof(pluginId) !== 'undefined')
         {
            path.push(pluginId);
         }
         if(typeof(resourceId) !== 'undefined')
         {
            path.push(resourceId);
         }
         if(typeof(getData) === 'boolean' ? getData : false)
         {
            path.push('data');
         }
         return bitmunk.util.getPath(sResources, path);
      },
      
      /**
       * Get a named view.
       *
       * When plugins are registered they store a map from a 'hash' variable
       * to the view resource information.  The view can then be retrieved with
       * this call.
       *
       * @param viewId id of a view registered with a 'hash' value
       */
      getNamedView: function(viewId)
      {
         var rval;
         if(viewId in sNamedViews)
         {
            var info = sNamedViews[viewId];
            rval = bitmunk.resource.get(info.pluginId, info.resourceId);
         }
         else
         {
            bitmunk.log.error(cat, 'getNamedView: unknown view:', viewId);
         }
         return rval;
      },
      
      /**
       * Setup a resource.
       *
       * This method will update the values in a stored resource and unblock
       * waiters on that resource. This should be used from plugin scripts to
       * update resources.
       * 
       * @param options:
       *    pluginId: the plugin id (required)
       *    resourceId: the resource id (required)
       *    *: any other parameters to update
       */
      setupResource: function(options)
      {
         var r = bitmunk.resource.get(options.pluginId, options.resourceId);
         $.extend(r.options, options);
      },
      
      /**
       * Get the list of real scripts for a script resource.
       *
       * @param pluginId a resource plugin id.
       * @param resourceId a resource id.
       *
       * @return a list of script resource names.
       */
      getScriptResources: function(pluginId, resourceId)
      {
         var r = bitmunk.resource.get(pluginId, pluginId);
         // get sub scripts if they exist
         var resources = bitmunk.util.getPath(r.options,
            ['subScripts', resourceId]);
         if(typeof(resources) === 'undefined')
         {
            // just a normal resource
            resources = [resourceId];
         }
         return resources;
      },
      
      /**
       * Initialize access to tasks for dynamically loaded scripts and block
       * the task.
       *
       * @param pluginId a resource plugin id.
       * @param resourceId a resource id.
       * @param task the task used for loading a script.
       */
      initScriptTask: function(pluginId, resourceId, task)
      {
         var resources = bitmunk.resource.getScriptResources(
            pluginId, resourceId);
         // setup the tasks 
         $.each(resources, function(i, resource)
         {
            bitmunk.util.setPath(sScriptTasks,
               [pluginId, resource], task);
         });
         // increase task block count by one for each script
         task.block(resources.length);
      },
      
      /**
       * Get the task used for loading a script.
       *
       * @param pluginId a resource plugin id.
       * @param resourceId a resource id.
       */
      getScriptTask: function(pluginId, resourceId)
      {
         // should exist so just use simple access
         var task = sScriptTasks[pluginId][resourceId];
         if(typeof(task) === 'undefined')
         {
            bitmunk.log.error(
               cat, 'getScriptTask unknown resource:', pluginId, resourceId);
         }
         return task;
      },
      
      /**
       * Delete the task used for loading a script.
       *
       * @param pluginId a resource plugin id.
       * @param resourceId a resource id.
       */
      cleanupScriptTask: function(pluginId, resourceId, task)
      {
         var resources = bitmunk.resource.getScriptResources(
            pluginId, resourceId);
         // setup the tasks 
         $.each(resources, function(i, resource)
         {
            // remove the resourceId entry
            delete sScriptTasks[pluginId][resource];
         });
         if(bitmunk.util.isEmpty(sScriptTasks[pluginId]))
         {
            delete sScriptTasks[pluginId];
         }
      }
   };
   
   sTypeHandlers[bitmunk.resource.types.html] =
   sTypeHandlers[bitmunk.resource.types.json] =
   sTypeHandlers[bitmunk.resource.types.text] =
   sTypeHandlers[bitmunk.resource.types.template] =
   {
      /**
       * Internal support to load a simple data types (html, json, text, template).
       * See bitmunk.resource.register()/load() for documentation.
       */
      load: function(options)
      {
         var r = options.resource;
         bitmunk.log.verbose(cat, 'loadSimple [%s][%s]',
            r.pluginId, r.resourceId, options);
            
         options.task.block();

         $.ajax({
            type: 'GET',
            url: r.options.path,
            dataType: bitmunk.resource.typeInfo[r.type].ajaxType,
            async: true,
            cache: options.cache,
            success: function(data) {
               if(r.options.save)
               {
                  var rdata;
                  switch(r.type)
                  {
                     // convert html to jquery object
                     case bitmunk.resource.types.html:
                        rdata = $(data);
                        break;
                     // convert template to template object
                     case bitmunk.resource.types.template:
                        rdata = TrimPath.parseTemplate(data);
                        break;
                     // otherwise just data from $.ajax
                     default:
                        rdata = data;
                  }
                  bitmunk.util.setPath(sResources,
                     [r.pluginId, r.resourceId, 'data'], rdata);
               }
               options.task.unblock();
            },
            error: function() {
               options.task.fail();
            }
         });
      },
      unload: function(options)
      {
      }
   };

   sTypeHandlers[bitmunk.resource.types.javascript] =
   {
      /**
       * Internal support to load a JavaScript file via DHTML script tag addition.
       * See bitmunk.resource.register()/load() for documentation.
       */
      load: function(options)
      {
         var r = options.resource;
         bitmunk.log.verbose(cat, 'loadScript [%s]', r.options.path, options);
   
         options.task.next(function(task)
         {
            // setup task access and block task
            bitmunk.resource.initScriptTask(
               r.pluginId, r.resourceId, options.task);
            $.ajax({
               type: 'GET',
               url: r.options.path,
               cache: options.cache,
               dataType: 'script',
               success: function() {
                  // save approx loaded date
                  r.at = new Date();
                  // scripts must call getScriptTask(...).unblock()
               },
               error: function() {
                  task.fail();
               }
            });
         });
         options.task.next(function(task)
         {
            // unblocked in script, cleanup task
            bitmunk.resource.cleanupScriptTask(
               r.pluginId, r.resourceId);
         });
      },
      unload: function(options)
      {
         // do nothing for script unloading
      }
   };

   sTypeHandlers[bitmunk.resource.types.css] =
   {
      /**
       * Internal support to load CSS via DHTML script tag addition.
       * See bitmunk.resource.register()/load() for documentation.
       */
      load: function(options)
      {
         var r = options.resource;
         bitmunk.log.verbose(cat, 'loadStyle [%s]', r.options.path, options);
      
         // http://msdn.microsoft.com/en-us/library/ms531194(VS.85).aspx
         // createStylesheet will fail after 31 calls, at which point docs say to
         // use another technique which according to a comment will also fail.
         // Just trying to use a standards-based method for now.
         /*
         // IE shortcut
         if(document.createStyleSheet) {
            document.createStyleSheet(url);
         }
         else
         {
            // standard createElement and add to HEAD
         }
         */
         /*
         var styles = "@import url(' " + url + " ');";
         var newSS = document.createElement('link');
         newSS.rel = 'stylesheet';
         newSS.href = 'data:text/css,'+escape(styles);
         */
         var data = document.createElement('link');
         data.rel = 'stylesheet';
         data.type = 'text/css';
         data.href = r.options.path;
         if(options.cache === false)
         {
            // stolen from jQuery to force a reload
            var ts = +new Date();
            // try replacing _= if it is there
            var ret = data.href.replace(/(\?|&)_=.*?(&|$)/, "$1_=" + ts + "$2");
            // if nothing was replaced, add timestamp to the end
            data.href = ret +
               ((ret == data.href) ?
                  (data.href.match(/\?/) ? "&" : "?") + "_=" + ts :
                  "");
         }
         
         // store the link element
         r.data = data;
         
         // FIXME: load should create el if not there, then add to doc
         // next time can just enable it
         // use tricks to try and wait for it (check attr, delay, etc)
         // unload should just set disable (or remove link if needed for other browsers)
      },
      unload: function(options)
      {
      }
   };

   sTypeHandlers[bitmunk.resource.types.pluginLoader] =
   {
      /**
       * Internal support to load a plugin loader.
       * See bitmunk.resource.register()/load() for documentation.
       */
      load: function(options)
      {
         var r = options.resource;
         var opt = r.options;
         bitmunk.log.verbose(cat, 'loadPluginLoader [%s]', r.pluginId, options);
         
         if(opt.loadInit)
         {
            // block the task here and unblock in the plugin register call
            options.task.next('loadPluginLoader init', function(task)
            {
               bitmunk.log.verbose(
                  cat, 'loadPluginLoader: will register init [%s]',
                  r.pluginId, options);
               // register and load init script immediately
               bitmunk.resource.register({
                  pluginId: r.pluginId,
                  resourceId: opt.init,
                  type: bitmunk.resource.types.javascript,
                  task: task
               });
               // FIXME handle errors
            });
            options.task.next('debug', function(task)
            {
               bitmunk.log.verbose(
                  cat, 'loadPluginLoader: did register init [%s]',
                  r.pluginId, options);
            });
         }
      },
      unload: function(options)
      {
      }
   };

   sTypeHandlers[bitmunk.resource.types.plugin] =
   {
      register: function(task, resource)
      {
         var opts = resource.options;
         bitmunk.log.verbose(
            cat, 'registerPlugin:', resource.pluginId, resource);
         
         // register sub-resources
         $.each(opts.resources || [], function(i, subResource)
         {
            // inherit some defaults from parent plugin
            // block some values to ensure child resource defines their own
            subResource = $.extend({
               pluginId: resource.pluginId,
               resourceId: null,
               type: null,
               homepage: opts.homepage,
               authors: opts.authors,
               name: null,
               // lazy load the resources later by default
               load: false,
               task: task
            }, subResource);
            bitmunk.log.verbose(
               cat, 'registerPlugin subResource:',
               subResource.pluginId, subResource.resourceId,
               subResource);
            bitmunk.resource.register(subResource);
         });
      },
      
      /**
       * Internal support to load a plugin.
       * See bitmunk.resource.register()/load() for documentation.
       */
      load: function(options)
      {
         var r = options.resource;
         bitmunk.log.verbose(cat, 'loadPlugin:', r.pluginId, options);
         
         // plugin options
         var opt = r.options;
         if(opt.scripts.length > 0)
         {
            options.task.next('debug', function(task)
            {
               bitmunk.log.verbose(
                  cat, 'loadPlugin: will load scripts:',
                  r.pluginId, options);
            });
            // load scripts
            $.each(opt.scripts, function(i, script)
            {
               options.task.next('plugin script', function(task)
               {
                  bitmunk.resource.register({
                     pluginId: r.pluginId,
                     resourceId: script,
                     type: bitmunk.resource.types.javascript,
                     task: task
                  });
                  // FIXME handle errors
               });
            });
            options.task.next('debug', function(task)
            {
               bitmunk.log.verbose(
                  cat, 'loadPlugin: did load scripts:',
                  r.pluginId, options);
            });
         }
         // FIXME setup views
         // FIXME make sure setupPages(..) called in plugin load
      },
      unload: function(options)
      {
      }
   };

   sTypeHandlers[bitmunk.resource.types.view] =
   {
      register: function(task, resource)
      {
         // store named view reference to this resource via 'hash' option
         var hash = resource.options.hash;
         if(hash)
         {
            sNamedViews[hash] = {
               pluginId: resource.pluginId,
               resourceId: resource.resourceId
            };
         }
         // array of options var and data type
         // used to build up resources to register
         var loadinfo = [
            ['html', bitmunk.resource.types.html],
            ['scripts', bitmunk.resource.types.javascript],
            ['css', bitmunk.resource.types.css],
            ['templates', bitmunk.resource.types.template]
         ];
         $.each(loadinfo, function(i, info) {
            var typeName = info[0];
            var dataType = info[1];
            if(typeName in resource.options)
            {
               // make an array to handle single resources
               var typeValues = $.makeArray(resource.options[typeName]);
               $.each(typeValues, function(i, value) {
                  task.next(function(task)
                  {
                     bitmunk.resource.register(
                     {
                        pluginId: resource.pluginId,
                        resourceId: value,
                        type: dataType,
                        load: false,
                        task: task
                     });
                     // FIXME handle errors?
                  });
               });
            }
         });
      },
      
      /**
       * Internal support to load a view and it's resources.
       * See bitmunk.resource.register()/load() for documentation.
       */
      load: function(options)
      {
         var r = options.resource;
         bitmunk.log.verbose(
            cat, 'loadView:', r.pluginId, r.resourceId, options);
         
         // plugin options
         var opts = r.options;
         
         // create a view object for this resource
         r.data = bitmunk.view.create(
         {
            id: opts.hash,
            pluginId: r.pluginId,
            resourceId: r.resourceId,
            models: opts.models,
            show: opts.show,
            hide: opts.hide
         });
         
         // load resources recursively 
         if(options.recursive)
         {
            // array of options var and data type
            // used to build up resources to load
            var loadinfo = [
               ['html', bitmunk.resource.types.html],
               ['scripts', bitmunk.resource.types.javascript],
               ['css', bitmunk.resource.types.css],
               ['templates', bitmunk.resource.types.template]
            ];
            $.each(loadinfo, function(i, info) {
               var typeName = info[0];
               var dataType = info[1];
               if(typeName in r.options)
               {
                  // make an array to handle single resources
                  var typeValues = $.makeArray(r.options[typeName]);
                  $.each(typeValues, function(i, value) {
                     options.task.next(function(task)
                     {
                        bitmunk.resource.load(
                        {
                           pluginId: r.pluginId,
                           resourceId: value,
                           type: dataType,
                           task: task
                        });
                        // FIXME handle errors?
                     });
                  });
               }
            });
         }
      },
      unload: function(options)
      {
      }
   };
   
   sTypeHandlers[bitmunk.resource.types.data] =
   {
      /**
       * Internal support for simple generic data type.
       * See bitmunk.resource.register()/load() for documentation.
       */
      load: function(options) {},
      unload: function(options) {}
   };

   /*
    * Check for caching control query vars.
    *
    * cache.resources=<true|false>
    * Enable or disable resource caching. When enabled (true) the browser
    * will do it's default caching. When disabled (false) the resource
    * loading mechanism will append strings to URLs to attempt to disable
    * browser caching logic.
    * 
    * cache.lock=<true|false>
    * When set to true this setting will lock caching regardless of settings
    * that are loaded. This can be used while debugging settings pages.
    */
   if(true)
   {
      var query = bitmunk.util.getQueryVariables();
      if('cache.resources' in query)
      {
         // set with last value
         switch(query['cache.resources'].slice(-1)[0])
         {
            case 'true':
               sConfig.cache.resources = true;
               break;
            case 'false':
               sConfig.cache.resources = false;
               break;
         } 
      }
      if('cache.lock' in query)
      {
         // set with last value
         switch(query['cache.lock'].slice(-1)[0])
         {
            case 'true':
               sConfig.cache.lock = true;
               break;
            case 'false':
               sConfig.cache.lock = false;
               break;
         } 
      }
   }
   
   /**
    * Handle configuration changes.
    * 
    * @event bitmunk-webui-WebUi-config-changed(config)
    *        Triggered when the configuration changes.
    */
   $(bitmunk.resource).bind('bitmunk-webui-WebUi-config-changed' + sNS,
      function(e, config)
      {
         bitmunk.log.debug(cat, 'got event: %s', e.type, config);
         // only try to set if not locked
         if(!sConfig.cache.lock)
         {
            // set if in new config, else use old value
            sConfig.cache.resources =
               bitmunk.util.getPath(config,
                  ['cache', 'resources'],
                  sConfig.cache.resources);
         }
      });
   
   // NOTE: task support is implicit and not a required dependency
   // plugin registered after registration code is initialized
   bitmunk._implicitPluginInfo.resource =
   {
      pluginId: cat,
      name: 'Bitmunk Resource'
   };
})(jQuery);

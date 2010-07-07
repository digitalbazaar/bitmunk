/**
 * Bitmunk Web UI --- Resource Support (registration and loading)
 *
 * @author Dave Longley
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 *
 * Copyright (c) 2008-2010 Digital Bazaar, Inc.  All rights reserved.
 */
(function($) {
   // logging category
   var cat = 'bitmunk.webui.core.Resource';
   
   // event namespace
   var sNS = '.bitmunk-webui-core-Resource';
   
   /**
    * Root site path for resources. Final form is:
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
    *    loadDate: timestamp when this resource was last loaded or null
    *    state: state (string, see bitmunk.resource.state)
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
   
   /**
    * Required resources that are pending load.
    */
   var sLoadQueue = [];
   
   /**
    * Loaded resources that are pending initialization.
    */
   var sInitQueue = [];
   
   // FIXME change this way of named views to something else?
   /**
    * Map of ids to named views.
    */
   var sNamedViews = {};
   // debug access
   bitmunk.debug.set(cat, 'namedViews', sNamedViews);
   
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
    * 
    * @param r the resource to set.
    */
   var _setResource = function(r)
   {
      bitmunk.util.setPath(sResources, [r.pluginId, r.resourceId], r);
   };
   
   /**
    * Low level interface to delete a resource.
    * 
    * @param r the resource to delete.
    */
   var _deleteResource = function(r)
   {
      bitmunk.util.pathDelete(sResources, [r.pluginId, r.resourceId]);
   };
   
   /**
    * Checks to see if the dependencies for the given resource have been met.
    * 
    * @param r the resource to check.
    * 
    * @return true if the dependencies have been met, false if not.
    */
   var _checkDependencies = function(r)
   {
      var rval = true;
      
      $.each(r.depends, function(pluginId, array)
      {
         $.each(array, function(i, resourceId)
         {
            var dep = bitmunk.resource.get(pluginId, resourceId);
            if(dep === null)
            {
               bitmunk.log.error(cat,
                  'resource has unregistered dependency [%s][%s]',
                  pluginId, resourceId);
               throw {
                  message: 'Unregistered dependency.',
                  pluginId: pluginId,
                  resourceId: resourceId,
                  resource: r
               };
            }
            else
            {
               // dependency only met if dep has been initialized
               rval = (dep.state === bitmunk.resource.state.initialized);
            }
            // continue if deps still met, break otherwise
            return rval;
         });
         // continue if deps still met, break otherwise
         return rval;
      });
      
      return rval;
   };
   
   /**
    * Map from bitmunk.resource.types type to object with functions:
    * 
    * register:
    *    perform custom registration, called after general registration
    * load:
    *    load the resource, can be asynchronous
    * unload:
    *    perform custom unloading of the resource, called before resource
    *    data is cleared
    * init:
    *    perform custom initialization, called after load
    */
   var sTypeHandlers = {};
   // handlers defined below bitmunk.resource
   
   // public interface to bitmunk resources
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
       * Possible states of a resource.
       */
      state:
      {
         unloaded: 'unloaded',
         loading: 'loading',
         loaded: 'loaded',
         initialized: 'initialized'
      }
   };
   
   /**
    * Helper function that sets option defaults for resource registration.
    * 
    * @param options the resource registration options.
    * 
    * @return the updated options (with defaults set).
    */
   var _setRegisterDefaults = function(options)
   {
      // setup default general options
      options = $.extend({
         pluginId: null,
         resourceId: null,
         type: null,
         required: false,
         depends: {}
      }, options);
      
      // get resource type
      var type = options.type;
      
      // loadable resource options
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
         options = $.extend({
            // use global option as default
            cache: sConfig.cache.resources,
         }, options);
      }
      // data resource options
      if(type == bitmunk.resource.types.data ||
         type == bitmunk.resource.types.dom ||
         type == bitmunk.resource.types.html ||
         type == bitmunk.resource.types.json ||
         type == bitmunk.resource.types.text ||
         type == bitmunk.resource.types.template)
      {
         // leave data undefined if not present
      }
      // file-like resource options
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
      // parsable resource options
      if(type == bitmunk.resource.types.html ||
         type == bitmunk.resource.types.json ||
         type == bitmunk.resource.types.template)
      {
         options = $.extend({
            parse: true
         }, options);
      }
      // view options
      if(type == bitmunk.resource.types.view)
      {
      }
      // plugin related options
      if(type == bitmunk.resource.types.plugin)
      {
         // special case for plugins:
         //    resourceId == pluginId
         options.resourceId = options.pluginId;
      }
      // plugin loader options
      if(type == bitmunk.resource.types.pluginLoader)
      {
         // special case for pluginLoaders:
         //    resourceId == pluginId.loader
         options.resourceId = options.pluginId + '.loader';
         options = $.extend({
            init: 'init.js'
         }, options);
      }
      // plugin options
      if(type == bitmunk.resource.types.plugin)
      {
         options = $.extend({
            name: null,
            homepage: null,
            authors: [],
            scripts: [],
         }, options);
      }
      
      return options;
   };
   
   /**
    * Registers a resource. This method is not asynchronous.
    *
    * This function registers a named resource of some given type. It has
    * the option to also provide the resource data to register. If the data
    * is not provided for the resource than it can be loaded later. See
    * bitmunk.resource.load().
    * 
    * Using the 'data' option will set the resource as loaded and its data
    * will be set to the value of the 'data' option. Care should be taken to
    * use appropriate data.
    *
    * General options:
    *    pluginId: plugin id of the owner of this resource
    *       has the form: bitmunk.topic.Name
    *    resourceId: resource id
    *       for plugins: defaults to pluginId (ignored)
    *       other resources: a resource id (required)
    *    type: bitmunk.resource.types id (required)
    *    required: true if the resource is immediately required (optional)
    *    depends: resources this resource depends on (optional)
    *       has the form {pluginId:[resourceId, ...], ...}
    *       
    * Loadable resources (files, plugins):
    *    cache: caching hint to browser (optional default: true)
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
    *    // FIXME: still needed? handle w/registerScript()?
    *    subScripts: if any of the entries in the "scripts" option are
    *        virtual scripts then subScripts should list those scripts and
    *        list the real files that compose each scripts. This is done so
    *        subscripts can be specified as dependencies for virtual scripts.
    *        (required if using virtual scripts)
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
    * @param options see function description.
    */
   bitmunk.resource.register = function(options)
   {
      // check for invalid type
      if(!(options.type in sTypeHandlers))
      {
         bitmunk.log.error(cat,
            'register called with invalid type [%s][%s] (%s)',
            options.pluginId, options.resourceId, options.type, options);
         throw {
            message: 'Could not register resource; invalid type specified.'
         };
      }
      
      // set defaults
      options = _setRegisterDefaults(options);
      
      // check for valid pluginId
      if(typeof(options.pluginId) !== 'string')
      {
         bitmunk.log.error(cat, 'register called with no plugin id', options);
         throw {
            message: 'Could not register resource; no plugin id specified.'
         };
      }
      // check for valid resourceId
      if(typeof(options.resourceId) !== 'string')
      {
         bitmunk.log.error(cat, 'register called with no resource id', options);
         throw {
            message: 'Could not register resource; no resource id specified.'
         };
      }
      
      // FIXME: prevent double-registrations... can't enable just yet since
      // Main plugin is currently double-registered intentionally in js/init.js
      // ... that should be fixed
      /*
      if(bitmunk.resource.get(options.pluginId, options.resourceId) !== null)
      {
         return;
      }*/
      
      // set state to loaded if data is provided
      var state = typeof(options.data) === 'undefined' ?
         bitmunk.resource.state.unloaded :
         bitmunk.resource.state.loaded;
      
      // register resource
      var r =
      {
         pluginId: options.pluginId,
         resourceId: options.resourceId,
         type: options.type,
         required: options.required,
         depends: options.depends,
         data: options.data,
         options: options,
         state: state,
         loadDate: null
      };
      _setResource(r);
      
      // require resource immediately if requested
      if(r.required)
      {
         bitmunk.resource.require({resource: r});
      }
      
      // log registration
      bitmunk.log.debug(cat, 'registered: [%s][%s] (%s)',
         r.pluginId, r.resourceId, r.type);
      bitmunk.log.verbose(cat, 'register details:', options, r);
      
      // call type specific register handler if available
      if('register' in sTypeHandlers[r.type])
      {
         sTypeHandlers[r.type].register(r);
      }
   };
   
   /**
    * Unregisters a resource. This method is not asynchronous.
    *
    * This function unregisters a named resource. It first makes sure the
    * resource is unloaded.
    *
    * General options:
    *    pluginId: plugin id (required)
    *    resourceId: resource id (required)
    *
    * @param options see function description.
    */
   bitmunk.resource.unregister = function(options)
   {
      // general options
      options = $.extend({
         pluginId: null,
         resourceId: null
      }, options);
      
      // get resource
      var r = getResource(options.pluginId, options.resourceId);
      if(r === null)
      {
         // already unregistered
      }
      else
      {
         // do custom unload
         if('unload' in sTypeHandlers[r.type])
         {
            sTypeHandlers[r.type].unload(r);
         }
         
         // delete resource
         _deleteResource(r);
         
         // log
         bitmunk.log.debug(cat, 'unregistered: [%s][%s] (%s)',
            r.pluginId, r.id, r.type);
      }
   };
   
   /**
    * Gets resource(s) or resouce data. The value depends on the object type.
    * See load() for details.
    *
    * @param pluginId id of plugin that registered or loaded the resource.
    * @param resourceId id of the resource.
    * @param data true to get data, false for resource object (default: true).
    * 
    * @return resource(s) with the given pluginId/resourceId, the resource
    *         data or null if not found.
    */
   bitmunk.resource.get = function(pluginId, resourceId, getData)
   {
      var rval;
      
      var undefPluginId = (typeof(pluginId) === 'undefined');
      var undefResourceId = (typeof(resourceId) === 'undefined');
      if(undefPluginId && undefResourceId)
      {
         bitmunk.log.verbose(cat, 'getting all resources');
      }
      else if(undefPluginId)
      {
         bitmunk.log.verbose(cat,
            'getting all resources with resource id [%s]', resourceId);
      }
      else if(undefResourceId)
      {
         bitmunk.log.verbose(cat,
            'getting all resources with plugin id [%s]', pluginId);
      }
      else
      {
         bitmunk.log.verbose(cat,
            'getting resource [%s][%s]', pluginId, resourceId);
      }
      
      // build look up path
      var path = [];
      if(!undefPluginId)
      {
         path.push(pluginId);
      }
      if(!undefResourceId)
      {
         path.push(resourceId);
      }
      if(typeof(getData) === 'boolean' ? getData : false)
      {
         path.push('data');
      }
      
      // get resource from path
      rval = bitmunk.util.getPath(sResources, path);
      if(typeof(rval) === 'undefined')
      {
         rval = null;
      }
      
      return rval;
   };
   
   /**
    * Gets a named view.
    *
    * When plugins are registered they store a map from a 'hash' variable
    * to the view resource information. The view can then be retrieved with
    * this call.
    *
    * @param viewId id of a view registered with a 'hash' value.
    */
   bitmunk.resource.getNamedView = function(viewId)
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
   };
   
   /**
    * Marks a previously registered resource as required so that the next
    * time load() is called the resource will be loaded and initialized before
    * the load task completes.
    * 
    * The options must contain either 'resource' or the plugin ID and resource
    * ID to require.
    * 
    * @param options:
    *           resource: the resource.
    *           pluginId: the plugin ID of the required resource.
    *           resourceId the resource ID of the required resource.
    */
   bitmunk.resource.require = function(options)
   {
      // get the resource
      var r = null;
      if(options.resource)
      {
         r = options.resource;
      }
      else
      {
         r = bitmunk.resource.get(options.pluginId, options.resourceId);
      }
      if(r === null)
      {
         bitmunk.log.error(cat,
            'require called with unregistered resource: [%s][%s]',
            options.pluginId, options.resourceId);
         throw {
            message: 'Resource is not registered.',
            pluginId: options.pluginId,
            resourceId: options.resourceId
         };
      }
      
      // mark all dependencies as required
      $.each(r.depends, function(key, array)
      {
         $.each(array, function()
         {
            bitmunk.resource.require({
               pluginId: key,
               resourceId: this
            });
         });
      });
      
      // set required flag
      r.required = true;
      // add to pending queue
      if(r.state === bitmunk.resource.state.unloaded)
      {
         sLoadQueue.push(r);
      }
      // add to init queue
      if(r.state === bitmunk.resource.state.loaded)
      {
         sInitQueue.push(r);
      }
   };
   
   /**
    * Loads all resources that are unloaded and have been marked as required.
    * 
    * To mark a resource as required, call bitmunk.resource.require() and pass
    * the plugin ID and resource ID of the resource to mark. Once all required
    * resources have been marked, a user should call:
    * 
    * task.next(bitmunk.resource.load)
    * 
    * This will load all required resources. The next task will not be run
    * until all marked resources have been successfully loaded.
    * 
    * @param task the task to use to load resources.
    */
   bitmunk.resource.load = function(task)
   {
      // load resources in load queue
      task.next(function(task)
      {
         /* Note: Here we use task blocking as a notification mechanism for
            progress. We only block once to prevent the task from continuing
            until at least one resource has loaded. Once the task resumes, it
            checks to see if there are any more resources that need loading,
            calls load again and then waits to be notified again.
          */
         var blocked = false;
         
         // iterate over pending resources
         $.each(sLoadQueue, function(i, r)
         {
            // if the resource is unloaded, start loading it
            if(r.state === bitmunk.resource.state.unloaded)
            {
               if(!blocked)
               {
                  task.block();
                  blocked = true;
               }
               
               // update state
               r.state = bitmunk.resource.state.loading;
               // include task used to load resource
               r.options.task = task;
               // load resource (may be asynchronous, if so will block task)
               sTypeHandlers[r.type].load(r);
               sInitQueue.push(r);
            }
         });
         // clear pending resources
         sLoadQueue = [];
      });
      
      // initialize all loaded but uninitialized resources
      task.next(function(task)
      {
         // TODO: make this algorithm smarter
         // keep trying init until the queue size remains the same which
         // indicates it won't decrease in size until more resources load
         var length = sInitQueue.length;
         do
         {
            var tmp = sInitQueue;
            length = tmp.length;
            sInitQueue = [];
            $.each(tmp, function(i, r)
            {
               // if resource is loaded now and dependencies are met
               if(r.state === bitmunk.resource.state.loaded &&
                  _checkDependencies(r))
               {
                  // resource ready, initialize
                  if('init' in sTypeHandlers[r.type])
                  {
                     sTypeHandlers[r.type].init(r);
                  }
                  r.state = bitmunk.resource.state.initialized;
               }
               // resource not yet loaded or has unmet dependencies, requeue
               else
               {
                  sInitQueue.push(r);
               }
            });
         }
         while(length > sInitQueue.length);
         
         // if there is more to load, schedule another load
         if(sLoadQueue.length > 0)
         {
            task.next(bitmunk.resource.load);
         }
         // block if there are still resources loading
         else if(sInitQueue.length > 0)
         {
            task.block();
         }
      });
      
      // see if there are any queued resources
      task.next(function(task)
      {
         if(sLoadQueue.length > 0 || sInitQueue.length > 0)
         {
            // run the loader again
            task.next(bitmunk.resource.load);
         }
      });
   };
   
   /**
    * Called from javascript resources to register any dependencies and
    * their initialize function.
    * 
    * @param options registration options:
    *           pluginId: the script's plugin ID.
    *           resourceId: the script's resource ID.
    *           init: an initialize task function (ie function(task)).
    *           depends: resources this resource depends on (optional)
    *              has the form {pluginId:[resourceId, ...], ...}
    */
   bitmunk.resource.registerScript = function(options)
   {
      options = $.extend({
         depends: {},
         init: function(task){}
      }, options);
      
      bitmunk.log.verbose(
         cat, 'registerScript [%s][%s]',
         options.pluginId, options.resourceId, options);
      
      // validate options
      if(typeof(options.pluginId) === 'undefined')
      {
         bitmunk.log.error(cat,
            'registerScript called with no plugin ID', options);
         throw {
            message: 'No plugin ID specified for script registration.'
         }
      }
      if(typeof(options.resourceId) === 'undefined')
      {
         bitmunk.log.error(cat,
            'registerScript called with no resource ID', options);
         throw {
            message: 'No resource ID specified for script registration.'
         }
      }
      if(typeof(options.init) !== 'function') 
      {
         bitmunk.log.error(cat,
            'registerScript called with no init function', options);
         throw {
            message: 'Invalid script initialize function.',
            pluginId: options.pluginId,
            resourceId: options.resourceId
         }
      }
      
      // get the resource
      var r = bitmunk.resource.get(options.pluginId, options.resourceId);
      if(r === null)
      {
         bitmunk.log.error(cat,
            'registerScript called with unregistered resource', options);
         throw {
            message: 'Script is an unregistered resource.',
            pluginId: options.pluginId,
            resourceId: options.resourceId
         }
      }
      
      // merge dependencies
      $.each(options.depends, function(key, array)
      {
         if(key in r.depends)
         {
            r.depends[key] = r.depends[key].concat(array);
         }
         else
         {
            r.depends[key] = array;
         }
      });
      
      // requiring the resource will mark all new dependencies as required
      bitmunk.resource.require({resource: r});
      
      // set init function on resource
      r.init = options.init;
   };
   
   /**
    * Extends an existing resource.
    *
    * This method will update the values in a stored resource. This should be
    * used from plugin scripts to update resources, typically from their init
    * functions.
    * 
    * @param options:
    *    pluginId: the plugin id (required)
    *    resourceId: the resource id (required)
    *    *: any other parameters to update
    */
   bitmunk.resource.extendResource = function(options)
   {
      var r = bitmunk.resource.get(options.pluginId, options.resourceId);
      $.extend(r.options, options);
   };
   
   sTypeHandlers[bitmunk.resource.types.html] =
   sTypeHandlers[bitmunk.resource.types.json] =
   sTypeHandlers[bitmunk.resource.types.text] =
   sTypeHandlers[bitmunk.resource.types.template] =
   {
      /**
       * Internal support to load a simple data types (html, json, text,
       * template).
       * See bitmunk.resource.register()/load() for documentation.
       * 
       * @param r the resource to load.
       */
      load: function(r)
      {
         bitmunk.log.verbose(cat, 'loadSimple [%s][%s]',
            r.pluginId, r.resourceId);
         
         var timer = +new Date();
         $.ajax({
            type: 'GET',
            url: r.options.path,
            dataType: bitmunk.resource.typeInfo[r.type].ajaxType,
            async: true,
            cache: r.options.cache,
            success: function(data)
            {
               timer = +new Date() - timer;
               bitmunk.log.debug('timing',
                  'loaded resource [%s][%s] (%s) in %s ms',
                  r.pluginId, r.resourceId, r.type, timer);
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
               
               // notify load completed
               r.state = bitmunk.resource.state.loaded;
               r.options.task.unblock();
            },
            error: function()
            {
               // notify load failed
               r.options.task.fail();
            }
         });
      }
   };

   sTypeHandlers[bitmunk.resource.types.javascript] =
   {
      /**
       * Internal support to load a JavaScript file.
       * See bitmunk.resource.register()/load() for documentation.
       * 
       * @param r the resource to load.
       */
      load: function(r)
      {
         bitmunk.log.verbose(cat, 'loadScript [%s]', r.options.path, r.options);
         
         var timer = +new Date();
         $.ajax({
            type: 'GET',
            url: r.options.path,
            cache: r.options.cache,
            dataType: 'script',
            success: function()
            {
               // save approx loaded date
               r.loadDate = new Date();
               timer = +(r.loadDate) - timer;
               bitmunk.log.debug('timing',
                  'loaded resource [%s][%s] (%s) in %s ms',
                  r.pluginId, r.resourceId, r.type, timer);
               
               // scripts must call registerScript(...)
               
               // notify load completed
               r.state = bitmunk.resource.state.loaded;
               r.options.task.unblock();
            },
            error: function()
            {
               // notify load failed
               r.options.task.fail();
            }
         });
      },
      
      /**
       * Initialize a JavaScript file by running its custom init script.
       * 
       * @param r the resource to initialize.
       */
      init: function(r)
      {
         if(r.init)
         {
            r.init(r.options.task);
         }
      }
   };

   sTypeHandlers[bitmunk.resource.types.css] =
   {
      /**
       * Internal support to load CSS.
       * See bitmunk.resource.register()/load() for documentation.
       * 
       * @param r the resource to load.
       */
      load: function(r)
      {
         bitmunk.log.verbose(cat, 'loadStyle [%s]', r.options.path, r.options);
         
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
         if(r.options.cache === false)
         {
            // stolen from jQuery to force a reload
            var ts = +new Date();
            // try replacing _= if it is there
            var ret = data.href.replace(/(\?|&)_=.*?(&|$)/, '$1_=' + ts + '$2');
            // if nothing was replaced, add timestamp to the end
            data.href = ret +
               ((ret == data.href) ?
                  (data.href.match(/\?/) ? '&' : '?') + '_=' + ts :
                  '');
         }
         
         // store the link element
         r.data = data;
         
         // FIXME: load should create el if not there, then add to doc
         // next time can just enable it
         // use tricks to try and wait for it (check attr, delay, etc)
         // unload should just set disable (or remove link if needed for other
         // browsers)
         
         // notify load completed
         r.state = bitmunk.resource.state.loaded;
         r.options.task.unblock();
      }
   };

   sTypeHandlers[bitmunk.resource.types.pluginLoader] =
   {
      /**
       * Registers the init script as a dependency.
       * 
       * @param r the resource to custom register.
       */
      register: function(r)
      {
         bitmunk.log.verbose(
            cat, 'loadPluginLoader: will register init [%s]',
            r.pluginId, r.options);
         
         if(!(r.pluginId in r.depends))
         {
            r.depends[r.pluginId] = [];
         }
         r.depends[r.pluginId].push(r.options.init);
         bitmunk.resource.register({
            pluginId: r.pluginId,
            resourceId: r.options.init,
            type: bitmunk.resource.types.javascript,
            required: r.options.required
         });
         
         bitmunk.log.verbose(
            cat, 'loadPluginLoader: did register init [%s]',
            r.pluginId, r.options);
      },
      
      /**
       * Internal support to load a plugin loader.
       * See bitmunk.resource.register()/load() for documentation.
       * 
       * @param r the resource to load.
       */
      load: function(r)
      {
         var opt = r.options;
         bitmunk.log.verbose(
            cat, 'loadPluginLoader [%s] loaded', r.pluginId, opt);
         // notify load completed
         r.state = bitmunk.resource.state.loaded;
         opt.task.unblock();
      }
   };

   sTypeHandlers[bitmunk.resource.types.plugin] =
   {
      /**
       * Custom register for a plugin. Registers subresources and scripts.
       * 
       * @param r the resource to custom register.
       */
      register: function(r)
      {
         var opt = r.options;
         bitmunk.log.verbose(cat, 'registerPlugin:', r.pluginId, r);
         
         // register sub-resources
         $.each(opt.resources || [], function(i, subResource)
         {
            // inherit some defaults from parent plugin
            // block some values to ensure child resource defines their own
            subResource = $.extend({
               pluginId: r.pluginId,
               resourceId: null,
               type: null,
               homepage: opt.homepage,
               authors: opt.authors,
               name: null,
               required: opt.required
            }, subResource);
            bitmunk.log.verbose(
               cat, 'registerPlugin subResource:',
               subResource.pluginId, subResource.resourceId,
               subResource);
            bitmunk.resource.register(subResource);
            
            // add sub-resource as a dependency
            if(!(r.pluginId in r.depends))
            {
               r.depends[r.pluginId] = [];
            }
            r.depends[r.pluginId].push(subResource.resourceId);
         });
         
         // register scripts
         if(opt.scripts.length > 0)
         {
            bitmunk.log.verbose(
               cat, 'loadPlugin: will register scripts:', r.pluginId, opt);
            
            // build a list of real scripts (converting virtual ones as needed)
            var scripts = [];
            $.each(opt.scripts, function(i, script)
            {
               // see if the script is virtual
               if(opt.subScripts && (script in opt.subScripts))
               {
                  scripts = scripts.concat(opt.subScripts[script]);
               }
               else
               {
                  scripts.push(script);
               }
            });
            
            // register each real script
            $.each(scripts, function(i, script)
            {
               bitmunk.log.verbose(
                  cat, 'registerPlugin script:', r.pluginId, script);
               
               bitmunk.resource.register({
                  pluginId: r.pluginId,
                  resourceId: script,
                  type: bitmunk.resource.types.javascript,
                  // require the scripts immediately
                  required: true
               });
            });
            
            // add scripts as dependencies
            if(!(r.pluginId in r.depends))
            {
               r.depends[r.pluginId] = scripts;
            }
            else
            {
               r.depends[r.pluginId].concat(scripts);
            }
            
            bitmunk.log.verbose(
               cat, 'loadPlugin: did register scripts:', r.pluginId, opt);
         }
      },
      
      /**
       * Internal support to load a plugin.
       * See bitmunk.resource.register()/load() for documentation.
       * 
       * @param r the resource to load.
       */
      load: function(r)
      {
         var opt = r.options;
         bitmunk.log.verbose(cat, 'loadPlugin:', r.pluginId, opt);
         // notify load completed
         r.state = bitmunk.resource.state.loaded;
         opt.task.unblock();
      }
   };

   sTypeHandlers[bitmunk.resource.types.view] =
   {
      /**
       * Custom register for a view. Registers subresources.
       * 
       * @param r the resource to custom register.
       */
      register: function(r)
      {
         // store named view reference to this resource via 'hash' option
         var hash = r.options.hash;
         if(hash)
         {
            sNamedViews[hash] = {
               pluginId: r.pluginId,
               resourceId: r.resourceId
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
         
         // register each subresource of the view
         var opt = r.options;
         $.each(loadinfo, function(i, info)
         {
            var typeName = info[0];
            var dataType = info[1];
            if(typeName in opt)
            {
               // get all subresources of the given type
               var subResources = opt[typeName];
               
               // for scripts, convert virtual scripts into real ones
               if(typeName === 'scripts')
               {
                  var scripts = [];
                  $.each(subResources, function(i, script)
                  {
                     // see if the script is virtual
                     if(opt.subScripts && (script in opt.subScripts))
                     {
                        scripts = scripts.concat(opt.subScripts[script]);
                     }
                     else
                     {
                        scripts.push(script);
                     }
                  });
                  subResources = scripts;
               }
               
               // make an array to handle single resources
               var typeValues = $.makeArray(subResources);
               $.each(typeValues, function(i, value)
               {
                  bitmunk.resource.register(
                  {
                     pluginId: r.pluginId,
                     resourceId: value,
                     type: dataType,
                     required: r.options.required
                  });
                  
                  // make subresource a dependency of the view
                  if(!(r.pluginId in r.depends))
                  {
                     r.depends[r.pluginId] = [];
                  }
                  r.depends[r.pluginId].push(value);
               });
            }
         });
      },
      
      /**
       * Internal support to load a view and it's resources.
       * See bitmunk.resource.register()/load() for documentation.
       * 
       * @param r the resource to load.
       */
      load: function(r)
      {
         var opts = r.options;
         bitmunk.log.verbose(cat, 'loadView:', r.pluginId, r.resourceId, opts);
         
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
         
         // notify load completed
         r.state = bitmunk.resource.state.loaded;
         r.options.task.unblock();
      }
   };
   
   sTypeHandlers[bitmunk.resource.types.data] =
   {
      /**
       * Internal support for simple generic data type.
       * See bitmunk.resource.register()/load() for documentation.
       */
      load: function(r)
      {
         // notify load completed
         r.state = bitmunk.resource.state.loaded;
         r.options.task.unblock();
      }
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

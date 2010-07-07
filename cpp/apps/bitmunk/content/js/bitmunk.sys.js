/**
 * Bitmunk Web UI --- System Support
 *
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 *
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
(function($) {
   // logging category
   var cat = 'bitmunk.webui.core.System';

   // initialization flag
   var sIsInitialized = false;
   
   // queue of plugins to register
   var sPluginQueue = [];
   // debug access
   bitmunk.debug.set(cat, 'pluginQueue', sPluginQueue);
   
   bitmunk.sys =
   {
      // store defaults for other core plugins to use
      corePluginDefaults:
      {
         type: bitmunk.resource.types.plugin,
         homepage: 'http://bitmunk.com/',
         authors: [
            {
               name: 'Digital Bazaar, Inc.',
               email: 'support@bitmunk.com'
            }
         ]
      },

      /**
       * Task queue to synchronize system level events sych as plugin loading.
       */
      taskQueueId: 'bitmunk.webui.core.SystemTaskQueue'
   };
   
   /**
    * Use the system task queue to load plugins. This will queue up plugin
    * loading if init() has not yet been called.
    */
   bitmunk.sys.register = function(options)
   {
      if(sIsInitialized)
      {
         bitmunk.task.start({
            type: bitmunk.sys.taskQueueId,
            run: function(task)
            {
               options.task = task;
               bitmunk.resource.register(options);
            }
         });
      }
      else
      {
         sPluginQueue.push(options);
      }
   };
    
   /**
    * Task to initialize the Bitmunk core plugins.
    *
    * @param task the task to use. 
    */
   bitmunk.sys.init = function(task)
   {
      // move implicit plugin object now that sys exists
      bitmunk.sys._implicitPluginInfo = bitmunk._implicitPluginInfo; 
      delete bitmunk._implicitPluginInfo; 
      // register implicit plugins in order
      var plugins =
      [
         bitmunk.sys._implicitPluginInfo.bitmunk,
         bitmunk.sys._implicitPluginInfo.debug,
         bitmunk.sys._implicitPluginInfo.util,
         bitmunk.sys._implicitPluginInfo.logging,
         bitmunk.sys._implicitPluginInfo.task,
         bitmunk.sys._implicitPluginInfo.resource,
         {
            // this system plugin
            pluginId: cat
         }
      ];
      $.each(plugins, function(i, plugin)
      {
         // clone core defaults and extend plugin info
         plugin = $.extend(
            $.extend(true, {}, bitmunk.sys.corePluginDefaults),
            plugin);
         // serialize with system task queue
         bitmunk.task.start({
            type: bitmunk.sys.taskQueueId,
            name: 'sys register implicit',
            run: function(task)
            {
               plugin.task = task;
               bitmunk.resource.register(plugin);
            }
         });
      });
      task.next('sys initialized', function(task)
      {
         // now initialized
         sIsInitialized = true;
      });
      task.next('sys register queue', function(task)
      {
         // process queue until empty
         // register plugins with system queue
         while(sPluginQueue.length > 0)
         {
            bitmunk.sys.register(sPluginQueue.shift());
         }
      });
   };
})(jQuery);

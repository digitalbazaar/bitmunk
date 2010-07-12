/**
 * Bitmunk Web UI --- Logging
 *
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 *
 * Copyright (c) 2008-2009 Digital Bazaar, Inc.  All rights reserved.
 */
(function($)
{
   // event namespace
   var sNS = '.bitmunk-webui-core-Logging';
   
   // extend forge.log
   bitmunk.log = window.forge.log;
   
   /**
    * Handle global WebUI configuration.
    */
   $(bitmunk.log).bind('bitmunk-webui-WebUi-config-changed' + sNS,
      function(e, config)
      {
         var level = bitmunk.util.getPath(config,
            ['loggers', 'console', 'level']);
         if(typeof(level) !== 'undefined')
         {
            bitmunk.log.setLevel(sConsoleLogger, level);
         }
      });
   
   // NOTE: logging support is implicit and not a required dependency
   // plugin registered after registration code is initialized
   bitmunk._implicitPluginInfo.logging =
   {
      pluginId: 'bitmunk.webui.core.Logging',
      name: 'Bitmunk Logging'
   };
})(jQuery);

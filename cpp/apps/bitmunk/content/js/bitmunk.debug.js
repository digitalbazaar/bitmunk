/**
 * Copyright 2008-2010 Digital Bazaar, Inc.
 *
 * Support for Bitmunk web applications.
 *
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 */
(function($)
{
   // extend forge.debug
   bitmunk.debug = window.forge.debug;
   
   // NOTE: debug support is implicit and not a required dependency
   // plugin registered after registration code is initialized
   bitmunk._implicitPluginInfo.debug =
   {
      pluginId: 'bitmunk.webui.core.Debug',
      name: 'Bitmunk Debug'
   };
})(jQuery);

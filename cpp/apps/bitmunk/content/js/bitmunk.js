/**
 * Bitmunk Web UI --- Base support for Bitmunk web applications.
 *
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 *
 * Copyright (c) 2008-2009 Digital Bazaar, Inc.  All rights reserved.
 */
(function($) {
   // create root bitmunk object if needed
   window.bitmunk = window.bitmunk || {};
   
   // local alias
   var bitmunk = window.bitmunk;
   
   // create temporary implicit plugin info for pre-sys plugins
   // these are registered by the sys plugin
   // moved to bitmunk.sys object in the sys plugin
   bitmunk._implicitPluginInfo = {};
   bitmunk._implicitPluginInfo.bitmunk =
   {
      pluginId: 'bitmunk.webui.core.Bitmunk',
      name: 'Bitmunk Core'
   };
})(jQuery);

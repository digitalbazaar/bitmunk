/**
 * Bitmunk Web UI --- Task Management Functions
 *
 * @author Dave Longley
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 *
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 */
(function($)
{
   // extend forge.task
   bitmunk.task = window.forge.task;
   
   // NOTE: task support is implicit and not a required dependency
   // plugin registered after registration code is initialized
   bitmunk._implicitPluginInfo.task =
   {
      pluginId: 'bitmunk.webui.core.Task',
      name: 'Bitmunk Task'
   };
})(jQuery);

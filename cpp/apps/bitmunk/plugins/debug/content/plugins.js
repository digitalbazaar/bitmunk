/**
 * Bitmunk Web UI --- Plugins
 *
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 * 
 * Copyright (c) 2008-2010 Digital Bazaar, Inc.  All rights reserved.
 */
(function($) {

var init = function(task)
{
   // logging category
   var cat = 'bitmunk.webui.Debug';
   
   var show = function(task)
   {
      // get all resources
      var resources = bitmunk.resource.get();
      bitmunk.log.debug(cat, 'plugins show', bitmunk.resources);
      
      // empty and hide old list
      var pc = $('#pluginsContainer').empty();
      pc.hide();
      
      bitmunk.log.debug(cat, 'resources: ', resources);
      
      // iterate over plugins, adding plugin info to list
      var plugins = [];
      $.each(resources, function(i, resource)
      {
         $.each(resource, function(j, subResource)
         {
            if(subResource.type === bitmunk.resource.types.plugin)
            {
               plugins.push(
                  $.extend({
                     pluginId: subResource.pluginId,
                     name: '',
                     homepage: '',
                     authors: []
                  }, subResource.options));
            }
         });
      });
      var values = {
         plugins: plugins
      };
      // process and insert template
      var template = bitmunk.resource.get(
         'bitmunk.webui.Debug', 'plugin.html', true);
      $('#pluginsContainer').html(template.process(values));
      
      // show new list
      pc.show();
   };
   
   var hide = function(task)
   {
   };
   
   bitmunk.resource.extendResource(
   {
      pluginId: 'bitmunk.webui.Debug',
      resourceId: 'plugins',
      show: show,
      hide: hide
   });
}

bitmunk.resource.registerScript({
   pluginId: 'bitmunk.webui.Debug',
   resourceId: 'plugins.js',
   depends: {},
   init: init
});

})(jQuery);

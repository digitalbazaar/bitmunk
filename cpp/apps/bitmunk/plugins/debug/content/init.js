/**
 * Bitmunk Web UI --- Debug
 *
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 * 
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
(function($) {

var init = function(task)
{
   // logging category
   var cat = 'bitmunk.webui.Debug';
   
   bitmunk.log.debug(cat, 'will init');
   
   var load = function() {
      bitmunk.log.debug(cat, 'load', $('#debugSidebar'));
      $('#debugMenuToggle').click(function() {
         $('#debugMenu').toggle();
         return false;
      });
      $('#debugReloadModules').click(function() {
         loadModules();
         return false;
      });
      $('#clearPageCache').click(function() {
         clearPageCache();
         return false;
      });
   };
   
   /*
   $()
      .unbind('bitmunk-page-loaded-modules.bitmunk')
      .bind('bitmunk-page-loaded-modules.bitmunk', function(e) {
         bitmunk.log.debug(cat, 'bplm ev', e);
         var ml = $('#interfacesList').empty();
         ml.hide();
         $.each(bitmunk.modules, function(i, mod) {
            bitmunk.log.debug(cat, i, mod);
            if(mod.details)
            {
               ml.append('<li>' + mod.details.id + ' / ' + mod.details.name + '</li>');
            }
         });
         ml.show();
      });
   */
   
   bitmunk.resource.register({
      type: bitmunk.resource.types.plugin,
      pluginId: 'bitmunk.webui.Debug',
      name: 'Debug',
      homepage: 'http://bitmunk.com/',
      authors: [
         {
            name: 'Digital Bazaar, Inc.',
            email: 'support@bitmunk.com'
         }
      ],
      resources: [
         {
            type: bitmunk.resource.types.view,
            resourceId: 'plugins',
            hash: 'plugins',
            name: 'Plugins',
            html: 'plugins.html',
            scripts: [
               'plugins.js'
            ],
            templates: [
               'plugin.html'
            ],
            menu: {
               priority: 0.1
            }
         }
         /* sidebars
         {
            type: bitmunk.resource.types.sidebar,
            name: 'Debug',
            html: 'debug.html',
            id: 'debugSidebar',
            events: {
               load: load
            }
         }
         */
      ]
   });
   
   bitmunk.log.debug(cat, 'did init');
};

bitmunk.resource.registerScript({
   pluginId: 'bitmunk.webui.Debug',
   resourceId: 'init.js',
   depends: {},
   init: init
});

})(jQuery);

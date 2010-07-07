/**
 * Bitmunk Web UI --- Settings
 *
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 * 
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
(function($) {

var init = function(task)
{
   // logging category
   var cat = 'bitmunk.webui.Settings';
   
   bitmunk.log.debug(cat, 'will init');
   bitmunk.resource.register({
      type: bitmunk.resource.types.plugin,
      pluginId: 'bitmunk.webui.Settings',
      name: 'Settings',
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
            resourceId: 'settings',
            hash: 'settings',
            name: 'Settings',
            html: 'settings.html',
            scripts: [
               'settings.js'
            ],
            menu: {
               priority: 0.3
            },
            css: [
               'theme/settings.css'
            ]
         }
      ]
   });
   
   bitmunk.log.debug(cat, 'did init');
};

bitmunk.resource.registerScript({
   pluginId: 'bitmunk.webui.Settings',
   resourceId: 'init.js',
   depends: {},
   init: init
});

})(jQuery);

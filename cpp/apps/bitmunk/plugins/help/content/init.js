/**
 * Bitmunk Web UI --- Help
 *
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 * 
 * Copyright (c) 2008-2010 Digital Bazaar, Inc.  All rights reserved.
 */
(function($) {

var init = function(task)
{
   // logging category
   var cat = 'bitmunk.webui.Help';
   
   bitmunk.log.debug(cat, 'will init');
   
   bitmunk.resource.register({
      type: bitmunk.resource.types.plugin,
      pluginId: 'bitmunk.webui.Help',
      name: 'Help',
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
            resourceId: 'help',
            hash: 'help',
            name: 'Help',
            html: 'help.html',
            menu: {
               priority: 0.2
            },
            scripts: [
               'help.js'
            ],
            css: [
               'themes/bm-app/help.css'
            ]
         },
         {
            type: bitmunk.resource.types.view,
            resourceId: 'about',
            hash: 'about',
            name: 'About',
            html: 'about.html',
            scripts: [
               'help.js'
            ],
            templates: [
               'about.info.html'
            ]
         },
         {
            type: bitmunk.resource.types.view,
            resourceId: 'credits',
            hash: 'credits',
            name: 'Credits',
            html: 'credits.html',
            scripts: [
               'help.js'
            ]
         },
         {
            type: bitmunk.resource.types.view,
            resourceId: 'licenses',
            hash: 'licenses',
            name: 'OpenSource Licenses',
            html: 'licenses.html',
            scripts: [
               'help.js'
            ]
         }
      ]
   });
   
   bitmunk.log.debug(cat, 'did init');
};

bitmunk.resource.registerScript({
   pluginId: 'bitmunk.webui.Help',
   resourceId: 'init.js',
   depends: {},
   init: init
});

})(jQuery);

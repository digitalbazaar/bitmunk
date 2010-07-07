/**
 * Bitmunk Web UI --- Test
 *
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 * 
 * Copyright (c) 2008-2010 Digital Bazaar, Inc.  All rights reserved.
 */
(function($) {

var init = function(task)
{
   // plugin id
   var sPluginId = 'bitmunk.webui.Test';
   // logging category
   var cat = sPluginId;
   
   bitmunk.log.debug(cat, 'will init');
   bitmunk.resource.register({
      type: bitmunk.resource.types.plugin,
      pluginId: sPluginId,
      name: 'Test',
      homepage: 'http://bitmunk.com/',
      authors: [
         {
            name: 'Digital Bazaar, Inc.',
            email: 'support@bitmunk.com'
         }
      ],
      scripts: [
         'models.js'
      ],
      resources: [
         {
            type: bitmunk.resource.types.view,
            resourceId: 'test',
            hash: 'test',
            name: 'Test',
            html: 'test.html',
            scripts: [
               'test.js'
            ],
            templates: [
               'tmpl.html'
            ],
            menu: {
               priority: 0.7
            },
            css: [
               'theme/test.css'
            ]
         },
         {
            type: bitmunk.resource.types.view,
            resourceId: 'test-dyno-stats',
            hash: 'test-dyno-stats',
            name: 'Test DynamicObject Statistics',
            html: 'dyno-stats.html',
            scripts: [
               'dyno-stats.js'
            ]
         }
      ],
      didLogin: function(task) {
         bitmunk.log.debug(cat, 'didLogin');
      },
      willLogout: function(task) {
         bitmunk.log.debug(cat, 'willLogout');
      }
   });
   
   bitmunk.log.debug(cat, 'did init');
};

bitmunk.resource.registerScript({
   pluginId: sPluginId,
   resourceId: 'init.js',
   depends: {},
   init: init
});

})(jQuery);

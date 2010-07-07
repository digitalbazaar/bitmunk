/**
 * Bitmunk Web UI --- Support
 *
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 * 
 * Copyright (c) 2008-2010 Digital Bazaar, Inc.  All rights reserved.
 */
(function($) {

var init = function(task)
{
   // logging category
   var cat = 'bitmunk.webui.CustomerSupport';
   
   bitmunk.log.debug(cat, 'will init');
   bitmunk.resource.register({
      type: bitmunk.resource.types.plugin,
      pluginId: 'bitmunk.webui.CustomerSupport',
      name: 'Customer Support',
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
            resourceId: 'support',
            hash: 'support',
            name: 'Customer Support',
            html: 'support.html',
            menu: {
               priority: 0.3
            },
            css: [
               'theme/support.css'
            ]
         }
      ]
   });
   
   bitmunk.log.debug(cat, 'did init');
};

bitmunk.resource.registerScript({
   pluginId: 'bitmunk.webui.CustomerSupport',
   resourceId: 'init.js',
   depends: {},
   init: init
});

})(jQuery);

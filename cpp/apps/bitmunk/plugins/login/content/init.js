/**
 * Bitmunk Web UI --- Login
 *
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 * 
 * Copyright (c) 2008-2010 Digital Bazaar, Inc.  All rights reserved.
 */
(function($) {

var init = function(task)
{
   // logging category
   var cat = 'bitmunk.webui.Login';
   
   bitmunk.log.debug(cat, 'will init');
   bitmunk.resource.register({
      type: bitmunk.resource.types.plugin,
      pluginId: 'bitmunk.webui.Login',
      name: 'Login',
      homepage: 'http://bitmunk.com/',
      authors: [
         {
            name: 'Digital Bazaar, Inc.',
            email: 'support@bitmunk.com'
         }
      ],
      scripts: [
         'startup.js'
      ],
      resources: [
         {
            type: bitmunk.resource.types.view,
            resourceId: 'login',
            hash: 'login',
            name: 'Login',
            html: 'login.html',
            scripts: [
               'login.js'
            ],
            // FIXME: css not loading!
            css: [
               'theme/login.css'
            ]
         }
      ],
      didLogin: function(task) {
         bitmunk.login.didLogin(task);
      },
      willLogout: function(task) {
         bitmunk.login.willLogout(task);
      }
   });
   bitmunk.log.debug(cat, 'did init');
};

bitmunk.resource.registerScript({
   pluginId: 'bitmunk.webui.Login',
   resourceId: 'init.js',
   depends: {},
   init: init
});

})(jQuery);

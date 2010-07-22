/**
 * Bitmunk Web UI --- Main
 *
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 * 
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
(function($) {

var init = function(task)
{
   // Note: This plugin is *also* registered in the main script. This will
   // overwrite those registered settings. The other low-level resources will
   // still be registered and pre-loaded.
   bitmunk.sys.register({
      type: bitmunk.resource.types.plugin,
      pluginId: bitmunk.mainPluginId,
      name: 'Bitmunk',
      homepage: 'http://bitmunk.com/',
      authors: [
         {
            name: 'Digital Bazaar, Inc.',
            email: 'support@bitmunk.com'
         }
      ],
      // scripts to load immediately
      scripts:
      [
         'models/models.js'
      ],
      // real script files for each virtual script
      subScripts:
      {
         'models/models.js':
         [
            'models/config.js',
            'models/accounts.js',
            'models/feedback.js'
         ]
      },
      resources: [
         {
            type: bitmunk.resource.types.template,
            resourceId: 'filebrowser.html',
            required: true
         }
      ]
   });
};

bitmunk.resource.registerScript({
   pluginId: 'bitmunk.webui.Main',
   resourceId: 'js/init.js',
   depends: {},
   init: init
});

})(jQuery);

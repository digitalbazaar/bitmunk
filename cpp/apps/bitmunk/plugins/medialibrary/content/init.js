/**
 * Bitmunk Web UI --- Media Library
 *
 * @author Mike Johnson
 * @author Manu Sporny <msporny@digitalbazaar.com>
 * 
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
(function($) {

var init = function(task)
{
   // logging category
   var cat = 'bitmunk.webui.MediaLibrary';
   
   bitmunk.log.debug(cat, 'will init');
   
   bitmunk.resource.register(
   {
      type: bitmunk.resource.types.plugin,
      pluginId: 'bitmunk.webui.MediaLibrary',
      name: 'MediaLibrary',
      homepage: 'http://bitmunk.com/',
      authors:
      [
         {
            name: 'Digital Bazaar, Inc.',
            email: 'support@bitmunk.com'
         }
      ],
      // scripts to load immediately
      scripts:
      [
         'startup.js'
      ],
      // real script files for each virtual script
      subScripts:
      {
         'startup.js':
         [
            'medialibrary.model.js',
            'catalog.model.js',
            'common.js'
         ]
      },
      resources: [
         {
            type: bitmunk.resource.types.view,
            resourceId: 'medialibrary',
            hash: 'library',
            name: 'Library',
            html: 'medialibrary.html',
            // scripts to lazy-load
            scripts: [
               'medialibrary.js'
            ],
            subScripts: {
               'medialibrary.js':
               [
                  'medialibrary.controller.js',
                  'medialibrary.view.js',
                  'medialibrary.wareEditor.js',
                  'medialibrary.paymentEditor.js'
               ]
            },
            css: [
               'theme/medialibrary.css'
            ],
            templates: [
               'wareRow.html',
               'payeeRow.html'
            ],
            menu: {
               priority: 0.8
            }
         },
         {
            type: bitmunk.resource.types.html,
            resourceId: 'messages.html',
            load: true
         }
      ],
      didLogin: function(task) {
         bitmunk.medialibrary.didLogin(task);
      },
      willLogout: function(task) {
         bitmunk.medialibrary.willLogout(task);
      }
   });
   
   bitmunk.log.debug(cat, 'did init');
};

bitmunk.resource.registerScript({
   pluginId: 'bitmunk.webui.MediaLibrary',
   resourceId: 'init.js',
   depends: {},
   init: init
});

})(jQuery);

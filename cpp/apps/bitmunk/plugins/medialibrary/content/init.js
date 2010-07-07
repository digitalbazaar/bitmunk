/**
 * Bitmunk Web UI --- Media Library
 *
 * @author Mike Johnson
 * @author Manu Sporny <msporny@digitalbazaar.com>
 */
(function($)
{
   // logging category
   var cat = 'bitmunk.webui.MediaLibrary';
   
   var sScriptTask = bitmunk.resource.getScriptTask(
      'bitmunk.webui.MediaLibrary', 'init.js');
      
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
         ],
         'medialibrary.js':
          [
            'medialibrary.controller.js',
            'medialibrary.view.js',
            'medialibrary.wareEditor.js',
            'medialibrary.paymentEditor.js'
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
      },
      task: sScriptTask
   });
};

bitmunk.resource.registerScript({
   pluginId: 'bitmunk.webui.Help',
   resourceId: 'help.js',
   depends: {},
   init: init
});


   sScriptTask.unblock();
   bitmunk.log.debug(cat, 'did init');
})(jQuery);

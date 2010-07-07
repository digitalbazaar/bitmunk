/**
 * Bitmunk Web UI --- Main and Context Log
 *
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 */
(function($)
{
   // logging category
   var cat = 'bitmunk.webui.LogUI';
   
   var sScriptTask = bitmunk.resource.getScriptTask(
      'bitmunk.webui.LogUI', 'init.js');
      
   bitmunk.log.debug(cat, 'will init');
   bitmunk.resource.register({
      type: bitmunk.resource.types.plugin,
      pluginId: 'bitmunk.webui.LogUI',
      name: 'Log UI',
      homepage: 'http://bitmunk.com/',
      authors: [
         {
            name: 'Digital Bazaar, Inc.',
            email: 'support@bitmunk.com'
         }
      ],
      // need to have loaded all the time 
      scripts: [
         'log.js'
      ],
      resources: [
         {
            type: bitmunk.resource.types.view,
            resourceId: 'log',
            hash: 'log',
            name: 'Log',
            html: 'log.html',
            templates: [
               'logEntries.html'
            ],
            css: [
               'themes/bm-app/log.css'
            ]
         },
         {
            type: bitmunk.resource.types.html,
            resourceId: 'contextLog.html',
            css: [
               'themes/bm-app/contextLog.css'
            ]
         }
      ],
      didLogin: function(task) {
         bitmunk.logui.didLogin(task);
      },
      willLogout: function(task) {
         bitmunk.logui.willLogout(task);
      }
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

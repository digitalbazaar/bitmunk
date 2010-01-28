/**
 * Bitmunk Web UI --- Help
 *
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 */
(function($)
{
   // logging category
   var cat = 'bitmunk.webui.Help';
   
   bitmunk.log.debug(cat, 'will init');
   
   var sScriptTask = bitmunk.resource.getScriptTask(
      'bitmunk.webui.Help', 'init.js');
      
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
      ],
      task: sScriptTask
   });
   sScriptTask.unblock();
   bitmunk.log.debug(cat, 'did init');
})(jQuery);

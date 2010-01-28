/**
 * Bitmunk Web UI --- Settings
 *
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 */
(function($)
{
   // logging category
   var cat = 'bitmunk.webui.Settings';
   
   var sScriptTask = bitmunk.resource.getScriptTask(
      'bitmunk.webui.Settings', 'init.js');
   
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
      ],
      task: sScriptTask
   });
   sScriptTask.unblock();
   bitmunk.log.debug(cat, 'did init');
})(jQuery);

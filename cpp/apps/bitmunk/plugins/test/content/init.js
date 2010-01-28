/**
 * Bitmunk Web UI --- Test
 *
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 */
(function($)
{
   // plugin id
   var sPluginId = 'bitmunk.webui.Test';
   // logging category
   var cat = sPluginId;
   
   var sScriptTask = bitmunk.resource.getScriptTask(
      sPluginId, 'init.js');
      
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
      },
      task: sScriptTask
   });
   sScriptTask.unblock();
   bitmunk.log.debug(cat, 'did init');
})(jQuery);

/**
 * Bitmunk Web UI --- Purchase
 *
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 * @author Dave Longley
 */
(function($)
{
   // logging category
   var cat = 'bitmunk.webui.Purchase';
   
   var sScriptTask = bitmunk.resource.getScriptTask(
      'bitmunk.webui.Purchase', 'init.js');
      
   bitmunk.log.debug(cat, 'will init');
   
   bitmunk.resource.register(
   {
      type: bitmunk.resource.types.plugin,
      pluginId: 'bitmunk.webui.Purchase',
      name: 'Purchase',
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
         'models.js'
      ],
      // real script files for each virtual script
      subScripts:
      {
         'models.js':
         [
            'downloadstates.model.js'
         ],
         'purchases.js':
         [
            'purchases.controller.js',
            'purchases.view.js'
         ]
      },
      resources: [
         {
            type: bitmunk.resource.types.view,
            resourceId: 'purchases',
            hash: 'purchases',
            name: 'Purchases',
            html: 'purchases.html',
            // scripts to lazy-load
            scripts: [
               'purchases.js'
            ],
            css: [
               'theme/purchases.css'
            ],
            templates: [
               'loadingRow.html',
               'purchaseRow.html'
            ],
            menu: {
               priority: 0.9
            }
         },
         {
            type: bitmunk.resource.types.html,
            resourceId: 'messages.html',
            load: true
         }
      ],
      task: sScriptTask
   });
   sScriptTask.unblock();
   bitmunk.log.debug(cat, 'did init');
})(jQuery);

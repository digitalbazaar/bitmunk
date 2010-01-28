/**
 * Bitmunk Web UI --- Support
 *
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 */
(function($)
{
   // logging category
   var cat = 'bitmunk.webui.CustomerSupport';
   
   var sScriptTask = bitmunk.resource.getScriptTask(
      'bitmunk.webui.CustomerSupport', 'init.js');
      
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
      ],
      task: sScriptTask
   });
   sScriptTask.unblock();
   bitmunk.log.debug(cat, 'did init');
})(jQuery);

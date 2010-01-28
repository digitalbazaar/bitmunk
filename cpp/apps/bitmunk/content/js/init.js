/**
 * Bitmunk Web UI --- Main
 *
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 */
(function($)
{
   var sScriptTask = bitmunk.resource.getScriptTask(
      'bitmunk.webui.Main', 'js/init.js');
   // Note: This plugin is *also* registered in the main script.  This will
   // overwrite those registered settings.  The other low-level resources will
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
            'config.js',
            'accounts.js',
            'feedback.js'
         ]
      },
      resources: [
         {
            type: bitmunk.resource.types.template,
            resourceId: 'filebrowser.html',
            load: true
         }
      ],
      task: sScriptTask
   });
   sScriptTask.unblock();
})(jQuery);

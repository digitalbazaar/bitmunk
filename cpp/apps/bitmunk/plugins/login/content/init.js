/**
 * Bitmunk Web UI --- Login
 *
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 */
(function($) {
   // logging category
   var cat = 'bitmunk.webui.Login';
   
   var sScriptTask = bitmunk.resource.getScriptTask(
      'bitmunk.webui.Login', 'init.js');
      
   bitmunk.log.debug(cat, 'will init');
   bitmunk.resource.register({
      type: bitmunk.resource.types.plugin,
      pluginId: 'bitmunk.webui.Login',
      name: 'Login',
      homepage: 'http://bitmunk.com/',
      authors: [
         {
            name: 'Digital Bazaar, Inc.',
            email: 'support@bitmunk.com'
         }
      ],
      scripts: [
         'startup.js'
      ],
      resources: [
         {
            type: bitmunk.resource.types.view,
            resourceId: 'login',
            hash: 'login',
            name: 'Login',
            html: 'login.html',
            scripts: [
               'login.js'
            ],
            css: [
               'theme/login.css'
            ]
         }
      ],
      didLogin: function(task) {
         bitmunk.login.didLogin(task);
      },
      willLogout: function(task) {
         bitmunk.login.willLogout(task);
      },
      task: sScriptTask
   });
   sScriptTask.unblock();
   bitmunk.log.debug(cat, 'did init');
})(jQuery);

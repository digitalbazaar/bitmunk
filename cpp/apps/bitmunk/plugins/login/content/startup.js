/**
 * Bitmunk Web UI --- Login Startup
 *
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 * 
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
(function($) {

var init = function(task)
{
   // log category
   var sLogCategory = 'bitmunk.webui.Login';

   // event namespace
   var sNS = 'bitmunk-webui-Login';
   
   // setup api location
   bitmunk.login = bitmunk.login || {};
   
   /**
    * Callback for initialization after login.
    * 
    * @param task the current task.
    */
   bitmunk.login.didLogin = function(task)
   {
      // set username in status bar
      $('#logged-in').html('Logged in as <span class="username">' +
         bitmunk.user.getUsername() + '</span>');
   };
   
   /**
    * Callback for initialization before logout.
    * 
    * @param task the current task.
    */
   bitmunk.login.willLogout = function(task)
   {
      // remove username from status bar
      $('#logged-in').empty();
   };
};

bitmunk.resource.registerScript({
   pluginId: 'bitmunk.webui.Login',
   resourceId: 'startup.js',
   depends: {},
   init: init
});

})(jQuery);

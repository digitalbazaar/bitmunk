/**
 * Bitmunk Web UI --- Purchase
 *
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 * @author Dave Longley
 * 
 * Copyright (c) 2008-2010 Digital Bazaar, Inc.  All rights reserved.
 */
(function($) {

var init = function(task)
{
   // logging category
   var cat = 'bitmunk.webui.Purchase';
   
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
      didLogin: function(task)
      {
         $(window).bind(
            'bitmunk-purchase-DownloadState-licenseAcquired', function(event)
         {
            // show notification
            if(bitmunk.getCurrentViewId() != 'purchases')
            {
               // set info notification
               var msg = $('#bitmunk-directive-loaded-message',
                  bitmunk.resource.get(
                     'bitmunk.webui.Purchase', 'messages.html', true))
                  .html();
               $('#messages').jGrowl(msg,
               {
                  sticky: false,
                  theme: 'info'
               });
            }
         });
         $(window).bind('bitmunk-directive-started', function(event)
         {
            var msgId = (bitmunk.getCurrentViewId() == 'purchases') ?
               '#bitmunk-directive-started-purchases-message' :
               '#bitmunk-directive-started-non-purchases-message';
            
            // set info notification
            var msg = $(msgId,
               bitmunk.resource.get(
                  'bitmunk.webui.Purchase', 'messages.html', true))
               .html();
            $('#messages').jGrowl(msg,
            {
               sticky: false,
               theme: 'info'
            });
         });
         $(window).bind('bitmunk-directive-error', function(event)
         {
            var e = event.target;
            if(e.hasAttribute('error'))
            {
               var err = e.getAttribute('error');
               e.removeAttribute('error');
               
               // set error notification
               bitmunk.log.debug(sLogCategory, 'directive error: ' + err);
               var msg = $('#bitmunk-directive-exception-message',
                  bitmunk.resource.get(
                     'bitmunk.webui.Purchase', 'messages.html', true))
                  .html();
               $('#messages').jGrowl(msg,
               {
                  sticky: false,
                  theme: 'error'
               });
            }
         });
      },
      willLogout: function(task)
      {
         $(window).unbind('bitmunk-purchase-DownloadState-licenseAcquired');
         $(window).unbind('bitmunk-directive-started');
         $(window).unbind('bitmunk-directive-error');
      }
   });
   
   bitmunk.log.debug(cat, 'did init');
};

bitmunk.resource.registerScript({
   pluginId: 'bitmunk.webui.Purchase',
   resourceId: 'init.js',
   depends: {},
   init: init
});

})(jQuery);

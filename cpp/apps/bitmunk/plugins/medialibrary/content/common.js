/**
 * Bitmunk Media Library Common
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 * 
 * @author David I. Lehn
 */
(function($) {

var init = function(task)
{
   // log category
   var sLogCategory = 'bitmunk.medialibrary.common';
   
   // current event ids
   var sNetAccessEventIds = null;
   
   // Storage for net access notification state.
   // Only one notification should be displayed at any time.
   // New type of notification will cause previous notification to be removed.
   // This object is either null or an object with following properties:
   //    type: string type of notification
   //    element: jQuery element of notification captured from beforeOpen()
   var sNetAccessNotification = null;
   
   /**
    * Handler when net access check is successful.
    */
   var netAccessSuccess = function()
   {
      bitmunk.log.debug(sLogCategory, 'successful net access check');
      // if non-success notification is displayed then remove it and display
      // success notification.
      if(sNetAccessNotification != null &&
         sNetAccessNotification.type != 'success')
      {
         // remove old notification
         sNetAccessNotification.element.find('div.close').click();
         // add success notification
         var msg = $('#net-access-success-message',
            bitmunk.resource.get(
               'bitmunk.webui.MediaLibrary', 'messages.html', true))
            .html();
         $('#messages').jGrowl(msg, {
            sticky: false,
            theme: 'ok',
            beforeOpen: function(e, m, o) {
               // store success notification info
               sNetAccessNotification = {
                  type: 'success',
                  element: e
               };
            },
            beforeClose: function(e, m, o) {
               // clear success notification
               sNetAccessNotification = null;
            }
         });
      }
   };
   
   /**
    * Handler when net access check exception occurs.
    * 
    * @param details details about the failure
    */
   var netAccessException = function(details)
   {
      bitmunk.log.warning(sLogCategory, 'failed net access check', details);
      
      // handling two types of details, real exceptions and service results
      // with exceptions. also handling null exception as default case and
      // for testing use.
      var ex = null;
      if('netaccessException' in details)
      {
         ex = details.netaccessException;
      }
      else if('type' in details &&
         details.type == 'bitmunk.catalog.NetAccess.exception' &&
         'details' in details &&
         'exception' in details.details)
      {
         ex = details.details.exception;
      }
      
      var isPortConflict = false;
      // wrap in try/catch to ignore missing fields
      try {
         isPortConflict =
            (ex != null) &&
            (ex.type ==
               'bitmunk.catalog.NetAccessTestError') &&
            (ex.cause.type == 'monarch.upnp.SoapFault') &&
            (ex.cause.details.fault.params.detail.UPnPError.errorCode ==
               '718');
      }
      catch(e) {}
      
      // check if notification needs to be updated
      if(
         // no other notification
         sNetAccessNotification == null ||
         // success showing
         sNetAccessNotification.type == 'success' ||
         // wrong type showing
         (sNetAccessNotification.type == 'portConflict' && !isPortConflict) ||
         (sNetAccessNotification.type == 'exception' && isPortConflict))
      {
         if(sNetAccessNotification != null)
         {
            // remove old notification
            sNetAccessNotification.element.find('div.close').click();
         }
         
         // message depends on exception
         var msg;
         if(isPortConflict)
         {
            // port conflict specific error message
            msg = $('#net-access-port-conflict-message',
               bitmunk.resource.get(
                  'bitmunk.webui.MediaLibrary', 'messages.html', true))
               .html();
         }
         else
         {
            // generic failure
            msg = $('#net-access-exception-message',
               bitmunk.resource.get(
                  'bitmunk.webui.MediaLibrary', 'messages.html', true))
               .html();
         }
         $('#messages').jGrowl(msg, {
            sticky: true,
            theme: 'error',
            beforeOpen: function(e, m, o) {
               // store notification info
               sNetAccessNotification = {
                  type: isPortConflict ? 'portConflict' : 'exception',
                  element: e
               };
            },
            beforeClose: function(e, m, o) {
               // clear success notification
               sNetAccessNotification = null;
            }
         });
      }
   };
   
   /**
    * Get the current server status.
    * 
    * @param task the current task.
    */
   var getNetAccessStatus = function(task)
   {
      task.block();
      
      bitmunk.api.proxy(
      {
         method: 'GET',
         url: bitmunk.api.localRoot + 'catalog/server',
         params: { nodeuser: bitmunk.user.getUserId() },
         success: function(data)
         {
            data = JSON.parse(data);
            bitmunk.log.debug(
               sLogCategory, 'got catalog server status', data);
            if('netaccessException' in data)
            {
               netAccessException(data);
            }
            task.unblock();
         },
         error: function()
         {
            bitmunk.log.warning(
               sLogCategory, 'failed to get catalog server status');
            task.fail();
         }
      });
   };
   
   /**
    * Start watching net access events.
    * 
    * @param task the current task.
    */
   var startNetAccessWatch = function(task)
   {
      task.block();
      
      bitmunk.events.subscribe(
      {
         events: [
         {
            type: 'bitmunk.catalog.NetAccess.success',
            callback: function(event)
            {
               netAccessSuccess();
            }
         },
         {
            type: 'bitmunk.catalog.NetAccess.exception',
            callback: function(event)
            {
               netAccessException(event);
            }
         }],
         success: function(data)
         {
            sNetAccessEventIds = data;
            bitmunk.log.debug(
               sLogCategory, 'subscribed to netacess events', data);
            task.unblock();
         },
         error: function()
         {
            bitmunk.log.warning(
               sLogCategory, 'failed to subscribe to netaccess events');
            task.fail();
         }
      });
   };
   
   /**
    * Stop watching net access events.
    * 
    * @param task the current task.
    */
   var stopNetAccessWatch = function(task)
   {
      if(sNetAccessEventIds !== null)
      {
         bitmunk.events.deleteObservers(sNetAccessEventIds);
         sNetAccessEventIds = null;
      }
   };
   
   /**
    * Callback for initialization after login.
    * 
    * @param task the current task.
    */
   bitmunk.medialibrary.didLogin = function(task)
   {
      task.next(getNetAccessStatus);
      task.next(startNetAccessWatch);
   };
   
   /**
    * Callback for initialization before logout.
    * 
    * @param task the current task.
    */
   bitmunk.medialibrary.willLogout = function(task)
   {
      task.next(stopNetAccessWatch);
   };
};

bitmunk.resource.registerScript({
   pluginId: 'bitmunk.webui.MediaLibrary',
   resourceId: 'common.js',
   depends: {},
   init: init
});

})(jQuery);

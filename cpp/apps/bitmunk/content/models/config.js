/**
 * Bitmunk Config Model
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 * 
 * @author Dave Longley
 */
(function($) {

var init = function(task)
{
   // category for logger
   var sLogCategory = 'models.config';
   
   // model and table names
   var sModel = 'config';
   var sTable = 'config';
   
   /**
    * Load configs on start and config changes.
    * 
    * @param task the current task.
    * @param seqNum seqNum of event causing this load or 0.
    */
   var loadConfigs = function(task, seqNum)
   {
      // add task to load configs
      task.next('load configs', function(task)
      {
         var configUrl = bitmunk.api.localRoot + 'system/config';
         
         // do initial config grab:
         task.block(8);
         
         // get purchase config
         bitmunk.api.proxy(
         {
            method: 'GET',
            url: configUrl + '/bitmunk.purchase.Purchase',
            params: { nodeuser: bitmunk.user.getUserId() },
            success: function(data, status)
            {
               // insert purchase config into table
               var cfg = JSON.parse(data)['bitmunk.purchase.Purchase'];
               bitmunk.model.update(
                  sModel, sTable, 'bitmunk.purchase.Purchase', cfg, seqNum);
               
               // fire purchase config updated event
               $.event.trigger(
                  'bitmunk-purchase-Purchase-config-changed', [cfg]);
               
               task.unblock();
            },
            error: function()
            {
               task.fail();
            }
         });
         
         // get sell config
         bitmunk.api.proxy(
         {
            method: 'GET',
            url: configUrl + '/bitmunk.sell.Sell',
            params: { nodeuser: bitmunk.user.getUserId() },
            success: function(data, status)
            {
               // insert purchase config into table
               var cfg = JSON.parse(data)['bitmunk.sell.Sell'];
               bitmunk.model.update(
                  sModel, sTable, 'bitmunk.sell.Sell', cfg, seqNum);
               
               // fire purchase config updated event
               $.event.trigger('bitmunk-sell-Sell-config-changed', [cfg]);
               
               task.unblock();
            },
            error: function()
            {
               task.fail();
            }
         });
         
         // get webui config
         bitmunk.api.proxy(
         {
            method: 'GET',
            url: configUrl + '/bitmunk.webui.WebUi',
            params: { nodeuser: bitmunk.user.getUserId() },
            success: function(data, status)
            {
               // insert webui config into table
               var cfg = JSON.parse(data)['bitmunk.webui.WebUi'];
               bitmunk.model.update(
                  sModel, sTable, 'bitmunk.webui.WebUi', cfg, seqNum);
               
               // fire webui config updated event
               $.event.trigger('bitmunk-webui-WebUi-config-changed', [cfg]);
               
               task.unblock();
            },
            error: function()
            {
               task.fail();
            }
         });
         
         // get raw config
         bitmunk.api.proxy(
         {
            method: 'GET',
            url: configUrl,
            params: { raw: true, nodeuser: bitmunk.user.getUserId() },
            success: function(data, status)
            {
               // insert raw config into table
               var cfg = JSON.parse(data);
               bitmunk.model.update(sModel, sTable, 'raw', cfg, seqNum);
               
               // fire raw config updated event
               $.event.trigger('bitmunk-raw-config-changed', [cfg]);
               
               task.unblock();
            },
            error: function()
            {
               task.fail();
            }
         });
         
         // get users catalog config
         bitmunk.api.proxy(
         {
            method: 'GET',
            url: configUrl + '/bitmunk.catalog.CustomCatalog',
            params:
            {
               nodeuser: bitmunk.user.getUserId()
            },
            success: function(data, status)
            {
               // insert raw config into table
               var cfg = JSON.parse(data)['bitmunk.catalog.CustomCatalog'];
               bitmunk.model.update(
                  sModel, sTable, 'bitmunk.catalog.CustomCatalog', cfg, seqNum);
               
               // fire raw config updated event
               $.event.trigger(
                  'bitmunk-catalog-CustomCatalog-config-changed', [cfg]);
               
               task.unblock();
            },
            error: function()
            {
               task.fail();
            }
         });
         
         // get system user node config
         bitmunk.api.proxy(
         {
            method: 'GET',
            url: configUrl + '/node',
            params:
            {
               id: 'system user',
               nodeuser: bitmunk.user.getUserId()
            },
            success: function(data, status)
            {
               // insert purchase config into table
               var cfg = JSON.parse(data)['node'];
               bitmunk.model.update(
                  sModel, sTable, 'system user:node', cfg, seqNum);
               
               // fire purchase config updated event
               $.event.trigger(
                  'bitmunk-node-system-user-config-changed', [cfg]);
               
               task.unblock();
            },
            error: function()
            {
               task.fail();
            }
         });
         
         // get system user catalog config
         bitmunk.api.proxy(
         {
            method: 'GET',
            url: configUrl + '/bitmunk.catalog.CustomCatalog',
            params:
            {
               id: 'system user',
               nodeuser: bitmunk.user.getUserId()
            },
            success: function(data, status)
            {
               // insert raw config into table
               var cfg = JSON.parse(data)['bitmunk.catalog.CustomCatalog'];
               bitmunk.model.update(
                  sModel, sTable, 'system user:bitmunk.catalog.CustomCatalog',
                  cfg, seqNum);
               
               // fire raw config updated event
               $.event.trigger(
                  'bitmunk-catalog-CustomCatalog-system-user-config-changed',
                  [cfg]);
               
               task.unblock();
            },
            error: function()
            {
               task.fail();
            }
         });
         
         // get raw system user config
         bitmunk.api.proxy(
         {
            method: 'GET',
            // FIXME: Only works with this well known id.
            url: configUrl,
            params:
            {
               id: 'custom system user',
               raw: true,
               nodeuser: bitmunk.user.getUserId()
            },
            // may not exist so don't fail in global context
            global: false,
            xhrOptions:
            {
               // error's expected don't print warnings on errors
               logWarningOnError: false
            },
            success: function(data, status)
            {
               // insert raw config into table
               var cfg = JSON.parse(data);
               bitmunk.model.update(
                  sModel, sTable, 'system user:raw', cfg, seqNum);
               
               // fire raw config updated event
               $.event.trigger('bitmunk-raw-system-user-config-changed', [cfg]);
               
               task.unblock();
            },
            error: function()
            {
               // FIXME: check for 404 vs other errors
               //task.fail();
               // create default
               // FIXME: Need better way to create default.
               var cfg =
               {
                  '_id_': 'custom system user',
                  '_group_': 'system user',
                  '_version_': 'Monarch Config 3.0'
               };
               bitmunk.model.update(
                  sModel, sTable, 'system user:raw', cfg, seqNum);
               
               // fire raw config updated event
               $.event.trigger('bitmunk-raw-system-user-config-changed', [cfg]);
               
               task.unblock();
            }
         });
      });
   };
   
   /**
    * Starts updating the config data model.
    * 
    * @param task the current task.
    */
   var start = function(task)
   {
      // register model
      bitmunk.model.register(task,
      {
         model: sModel,
         tables: [
         {
            // user config table
            name: sTable,
            eventGroups: [{
               events: ['monarch.config.Config.changed'],
               // filter events based on these details
               filter: {
                  details: {
                     configId: 'user ' + bitmunk.user.getUserId()
                  }
               },
               eventCallback: configChanged
            },
            {
               events: ['monarch.config.Config.changed'],
               // filter events based on these details
               filter: {
                  details: {
                     configId: 'system user'
                  }
               },
               eventCallback: configChanged
            }]
         }]
      });
      
      // add task to populate config table
      task.next(function(task)
      {
         loadConfigs(task, 0);
      });
   };
   
   /**
    * Called when the configuration changes.
    * 
    * @param event the event.
    * @param seqNum the sequence number assigned to this event update.
    */
   var configChanged = function(event, seqNum)
   {
      bitmunk.log.debug(sLogCategory, 'Got configChanged event', event, seqNum);
      
      // task to reload all configs
      bitmunk.task.start(
      {
         type: sLogCategory + '.configChanged',
         run: function(task)
         {
            loadConfigs(task, seqNum);
         }
      });
   };
   
   bitmunk.model.addModel(sModel, {start: start});
};

bitmunk.resource.registerScript({
   pluginId: 'bitmunk.webui.Main',
   resourceId: 'config.js',
   depends: {},
   init: init
});

})(jQuery);

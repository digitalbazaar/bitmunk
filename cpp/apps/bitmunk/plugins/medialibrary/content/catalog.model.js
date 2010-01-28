/**
 * Bitmunk Catalog Model
 *
 * @author Mike Johnson
 * @author Manu Sporny
 * @author Dave Longley
 */
(function($)
{
   // logging category
   var sLogCategory = 'models.catalog';
   
   var sScriptTask = bitmunk.resource.getScriptTask(
      'bitmunk.webui.MediaLibrary', 'catalog.model.js');

   // URLs used by this model
   var sPayeeSchemesUrl = bitmunk.api.localRoot + 'catalog/payees/schemes';
   var sWaresUrl = bitmunk.api.localRoot + 'catalog/wares';
   
   // Catalog Model namespace
   bitmunk.catalog = bitmunk.catalog || {};
   bitmunk.catalog.model =
   {
      name: 'catalog',
      
      /**
       * The "ware" table contains:
       * 
       * ware:
       * {
       *    id: the ware's ID,
       *    mediaLibraryId: the associated media library ID,
       *    payeeSchemeId: the payee scheme ID,
       *    description: the ware's description,
       *    exception: any exception associated with the ware
       * }
       */
      wareTable: 'ware',
      
      /**
       * The total number of wares in the system.
       */
      totalWares: 0,
      
      /**
       * The "payeeScheme" table contains:
       * 
       * payeeScheme:
       * {
       *    <payeeSchemeId>:
       *    {
       *       "id": the id of the payee scheme
       *       ...
       *    }
       * }
       */
      payeeSchemeTable: 'payeeScheme',

      /**
       * The total number of payee schemes in the system.
       */
      totalPayeeSchemes: 0,
      
      /**
       * Updates all of the payee schemes in the model.
       * 
       * @param psIds A list of payee scheme IDs to update or null if all of
       *              them should be updated.
       * @param task A task to use when blocking and unblocking processing.
       */
      updatePayeeSchemes: function(psIds, task)
      {
         bitmunk.log.debug(sLogCategory, 'updatePayeeSchemes', psIds);
         
         if(task)
         {
            task.block();
         }
         
         bitmunk.api.proxy(
         {
            method: 'GET',
            url: sPayeeSchemesUrl,
            params:
            {
               start: 0,
               'default': 'false',
               nodeuser: bitmunk.user.getUserId()
            },
            success: function(data, status)
            {
               //bitmunk.log.debug(
               //   sLogCategory, 'updatePayeeSchemes response', psIds);
               
               // we're replacing all payee schemes in the model, so clear it
               if(psIds === null)
               {
                  // clear the payee scheme table
                  bitmunk.model.clear(
                     bitmunk.catalog.model.name,
                     bitmunk.catalog.model.payeeSchemeTable);
               }
               
               // add each payee scheme
               var rs = JSON.parse(data);
               $.each(rs.resources, function(index, ps)
               {
                  bitmunk.model.update(
                     bitmunk.catalog.model.name,
                     bitmunk.catalog.model.payeeSchemeTable,
                     ps.id, ps, 0);
               });
               
               // update the total payee scheme count
               bitmunk.catalog.model.totalPayeeSchemes = rs.total;
               
               if(task)
               {
                  task.unblock();
               }
            },
            error: function(xhr, textStatus, errorThrown)
            {
               if(task)
               {
                  try
                  {
                     // use exception from server
                     task.userData = JSON.parse(xhr.responseText);
                  }
                  catch(e)
                  {
                     // use error message
                     task.userData = { message: textStatus };
                  }
                  
                  task.fail();
               }
            }
         });
      },

      /**
      * Updates all of the wares in the model.
      * 
      * @param fileIds A list of file IDs that are associated with the wares to
      *                receive.
      * @param task A task to use when blocking and unblocking processing.
      */
      updateWares: function(fileIds, task)
      {
         bitmunk.log.debug(sLogCategory, 'updateWares', fileIds);
         
         // create the files and media retrieval URL
         var params =
         {
            'default': 'false',
            nodeuser: bitmunk.user.getUserId()
         };
         if(fileIds && (fileIds.length > 0))
         {
            params.fileId = fileIds; 
         }
         
         // start blocking if a task was given
         if(task)
         {
            task.block();
         }
         
         // perform the call to retrieve the wares
         bitmunk.api.proxy(
         {
            method: 'GET',
            url: sWaresUrl,
            params: params,
            success: function(data, status)
            {
               //bitmunk.log.debug(sLogCategory, 'updateWares response', data);
               
               // insert wares into model
               var rs = JSON.parse(data);
               for(var i = 0; i < rs.resources.length; i++)
               {
                  // update the tables with the data
                  var ware = rs.resources[i];
                  
                  // use sequence id 0 in both update calls to overwrite
                  // current data and prevent update race conditions

                  // FIXME: clear ware table?

                  // update the ware table
                  bitmunk.model.update(
                     bitmunk.catalog.model.name,
                     bitmunk.catalog.model.wareTable, ware.id, ware, 0);
               }
               
               // update the total count for wares in the system
               // FIXME: is this incorrect? what if we were only updating
               // a single ware at this time?
               bitmunk.catalog.model.totalWares = rs.total;
                
               if(task)
               {
                  task.unblock();
               }
            },
            error: function(xhr, textStatus, errorThrown)
            {
               if(task)
               {
                  try
                  {
                     // use exception from server
                     task.userData = JSON.parse(xhr.responseText);
                  }
                  catch(e)
                  {
                     // use error message
                     task.userData = { message: textStatus };
                  }
                  
                  task.fail();
               }
            }
         });
      }
   };
   
   /**
    * Starts the Catalog Model.
    * 
    * @param task the current task.
    */
   var start = function(task)
   {
      // define event filter for user ID
      var filter = {
         details: { userId: bitmunk.user.getUserId() }
      };
      
      // register model
      bitmunk.model.register(task,
      {
         model: bitmunk.catalog.model.name,
         tables: [
         {
            // create wares table
            name: bitmunk.catalog.model.wareTable,
            eventGroups: [
            {
               events: ['bitmunk.common.Ware.updated'],
               filter: $.extend(true, {details:{isNew: true}}, filter),
               eventCallback: wareUpdated
            },
            {
               events: ['bitmunk.common.Ware.exception'],
               filter: filter,
               eventCallback: wareException
            }],
            watchRules:
            {
               getRegisterObject: createWareWatchEvents,
               reinitializeRows: function(wareIds)
               {
                  // get file ID from ware ID
                  var fileIds = [];
                  $.each(wareIds, function(i, wareId)
                  {
                     // 'bitmunk:file:' is 13 chars long
                     var shortId = wareId.substring(13);
                     var ids = shortId.split('-', 2);
                     fileIds.push(ids[1]);
                  });
                  bitmunk.catalog.model.updateWares(fileIds);
               }
            }
         },
         {
            // create payee schemes table
            name: bitmunk.catalog.model.payeeSchemeTable,
            eventGroups: [
            {
               events: ['bitmunk.common.PayeeScheme.updated'],
               filter: $.extend(true, {details:{isNew: true}}, filter),
               eventCallback: payeeSchemeUpdated
            },
            {
               events: ['bitmunk.common.PayeeScheme.exception'],
               filter: filter,
               eventCallback: payeeSchemeException
            }],
            watchRules:
            {
               getRegisterObject: createPayeeSchemeWatchEvents,
               reinitializeRows: bitmunk.catalog.model.updatePayeeSchemes
            }
         }]
      });
   };
   
   /**
    * Called when a ware exception occurs.
    * 
    * @param event the ware event.
    * @param seqNum the sequence number assigned to this event update.
    */
   var wareException = function(event, seqNum)
   {
      bitmunk.log.debug(sLogCategory, event.type, event, seqNum);
      
      // get updated ware via backend
      var wareId = event.details.wareId;
      // 'bitmunk:file:' is 13 chars long
      var fileIds = [wareId.substring(13).split('-', 2)[1]];
      var params =
      {
         'default': 'false',
         nodeuser: bitmunk.user.getUserId(),
         fileId: fileIds
      };
      
      // perform the call to retrieve the ware
      bitmunk.api.proxy(
      {
         method: 'GET',
         url: sWaresUrl,
         params: params,
         success: function(data, status)
         {
            // get ware
            var ware = JSON.parse(data).resources[0];
            
            // update the ware table
            var updated = bitmunk.model.update(
               bitmunk.catalog.model.name,
               bitmunk.catalog.model.wareTable, ware.id, ware, seqNum);
            
            // only trigger exception event if ware still has an exception
            if(updated && ware.exception)
            {
               $.event.trigger(event.type.replace(/\./g, '-'), [ware.id]);
            }
         }
      });
   };
   
   /**
    * Called when a payee scheme exception occurs.
    * 
    * @param event the payee scheme event.
    * @param seqNum the sequence number assigned to this event update.
    */
   var payeeSchemeException = function(event, seqNum)
   {
      bitmunk.log.debug(sLogCategory, event.type, event, seqNum);
   
      // get updated payee scheme via the backend
      var psId = event.details.payeeSchemeId;
      var params =
      {
         'default': 'false',
         nodeuser: bitmunk.user.getUserId()
      };
      
      // perform the call to retrieve the payee scheme
      bitmunk.api.proxy(
      {
         method: 'GET',
         url: sPayeeSchemesUrl + '/' + psId,
         params: params,
         success: function(data, status)
         {
            // get the payee scheme
            var ps = JSON.parse(data);
            
            // update the payee schemes table
            var updated = bitmunk.model.update(
               bitmunk.catalog.model.name,
               bitmunk.catalog.model.payeeSchemeTable, ps.id, ps, seqNum);
            
            // only trigger exception event if scheme still has an exception
            if(updated && ps.exception)
            {
               $.event.trigger(event.type.replace(/\./g, '-'), [ps.id]);
            }
         }
      });
   };
   
   /**
    * Creates the set of events for specific wares watched by the table.
    * 
    * @param wareIds an array of ware IDs.
    * 
    * @return the event registration object.
    */
   var createWareWatchEvents = function(wareIds)
   {
      var eventGroups = [];
      var userId = bitmunk.user.getUserId();
      
      $.each(wareIds, function(i, wareId)
      {
         // create filter on userId and wareId
         var filter = {
            details: {
               userId: userId,
               wareId: wareId
            }
         };
         
         // we do not need to watch exception events because the global
         // exception event was added when the wares table was created
         
         eventGroups.push(
         {
            events: ['bitmunk.common.Ware.updated'],
            filter: $.extend({details:{isNew: false}}, filter),
            eventCallback: wareUpdated
         });
         
         eventGroups.push(
         {
            events: ['bitmunk.common.Ware.removed'],
            filter: filter,
            eventCallback: wareRemoved
         });
      });
      
      return { eventGroups: eventGroups };
   };
   
   /**
    * Creates the set of events for specific payee schemes watched by the table.
    * 
    * @param psIds an array of payee scheme IDs.
    * 
    * @return the event registration object.
    */
   var createPayeeSchemeWatchEvents = function(psIds)
   {
      var eventGroups = [];
      var userId = bitmunk.user.getUserId();
      
      $.each(psIds, function(i, psId)
      {
         // create filter on userId and psId
         var filter = {
            details: {
               userId: userId,
               payeeSchemeId: psId
            }
         };
         
         // we do not need to watch exception events because the global
         // exception event was added when the payee schemes table was created
         
         eventGroups.push(
         {
            events: ['bitmunk.common.PayeeScheme.updated'],
            filter: $.extend({details:{isNew: false}}, filter),
            eventCallback: payeeSchemeUpdated
         });
         
         eventGroups.push(
         {
            events: ['bitmunk.common.PayeeScheme.removed'],
            filter: filter,
            eventCallback: payeeSchemeRemoved
         });
      });
      
      return { eventGroups: eventGroups };
   };
   
   /**
    * Called when a ware is added to/updated in the catalog.
    * 
    * @param event the ware event.
    * @param seqNum the sequence number assigned to this event update.
    */
   var wareUpdated = function(event, seqNum)
   {
      bitmunk.log.debug(
         sLogCategory, event.type, event.details.wareId, event, seqNum);
      
      // get ware ID
      var wareId = event.details.wareId;
      
      // FIXME: this code is repeated in wareException and similar in
      // updateWares ... consolidate it
      
      // get updated ware via backend
      // 'bitmunk:file:' is 13 chars long
      var fileIds = [wareId.substring(13).split('-', 2)[1]];
      var params =
      {
         'default': 'false',
         nodeuser: bitmunk.user.getUserId(),
         fileId: fileIds
      };
      
      // perform the call to retrieve the ware
      bitmunk.api.proxy(
      {
         method: 'GET',
         url: sWaresUrl,
         params: params,
         success: function(data, status)
         {
            // get ware
            var ware = JSON.parse(data).resources[0];
            
            // update the ware table
            bitmunk.model.update(
               bitmunk.catalog.model.name,
               bitmunk.catalog.model.wareTable, ware.id, ware, seqNum);
            
            // if the ware is new, start watching
            var et = event.type;
            if(event.details.isNew)
            {
               // FIXME: should watch only be called from controller?
               bitmunk.catalog.model.totalWares++;
               bitmunk.model.watch(
                  bitmunk.catalog.model.name,
                  bitmunk.catalog.model.wareTable);
               et = et.replace(/updated/g, 'added');
            }
            
            // trigger updated event
            $.event.trigger(et.replace(/\./g, '-'), [ware.id]);
         }
      });
   };
   
   /**
    * Called when a ware is removed from the catalog.
    * 
    * @param event the ware event.
    * @param seqNum the sequence number assigned to this event update.
    */
   var wareRemoved = function(event, seqNum)
   {
      bitmunk.log.debug(
         sLogCategory, event.type, event.details.wareId, event, seqNum);
      
      // remove from ware table
      bitmunk.model.remove(
         bitmunk.catalog.model.name, bitmunk.catalog.model.wareTable,
         event.details.wareId);
      
      // update the total ware count
      bitmunk.catalog.model.totalWares--;
      
      // trigger removed event
      $.event.trigger(event.type.replace(/\./g, '-'), [event.details.wareId]);
   };
   
   /**
    * Called when a payee scheme is added to/updated in the catalog.
    * 
    * @param event the payee scheme event.
    * @param seqNum the sequence number assigned to this event update.
    */
   var payeeSchemeUpdated = function(event, seqNum)
   {
      bitmunk.log.debug(
         sLogCategory, event.type,
         event.details.payeeSchemeId, event, seqNum);
      
      // get payee scheme ID
      var schemeId = event.details.payeeSchemeId;
      
      // update payee scheme
      bitmunk.api.proxy(
      {
         method: 'GET',
         url: sPayeeSchemesUrl + '/' + schemeId,
         params:
         {
            'default': 'false',
            nodeuser: bitmunk.user.getUserId()
         },
         success: function(data, status)
         {
            // update payee scheme
            var ps = JSON.parse(data);
            bitmunk.model.update(
               bitmunk.catalog.model.name,
               bitmunk.catalog.model.payeeSchemeTable,
               ps.id, ps, seqNum);
            
            // if the payee scheme is new, start watching
            var et = event.type;
            if(event.details.isNew)
            {
               // FIXME: should watch only be called from controller?
               bitmunk.catalog.model.totalPayeeSchemes++;
               bitmunk.model.watch(
                  bitmunk.catalog.model.name,
                  bitmunk.catalog.model.payeeSchemeTable);
               et = et.replace(/updated/g, 'added');
            }
            
            // trigger updated event
            $.event.trigger(et.replace(/\./g, '-'), [ps.id]);
         },
         error: function()
         {
            // FIXME: handle exception if payee schemes cannot be populated
         }
      });      
   };
   
   /**
    * Called when a payee scheme is removed from the catalog.
    * 
    * @param event the payee scheme event.
    * @param seqNum the sequence number assigned to this event update.
    */
   var payeeSchemeRemoved = function(event, seqNum)
   {
      bitmunk.log.debug(
         sLogCategory, event.type,
         event.details.payeeSchemeId, event, seqNum);
      
      // FIXME: i'm concerned that this value may get out of sync
      // due to concurrency issues ... people grabbing the payee schemes and
      // events occurring at the same time... i.e. if someone grabs all the
      // payee schemes ... then one is deleted before that call returns, the
      // new total will be off by 1 because it will be set by the grab-all
      // function when it comes back --dlongley
      
      // remove from payee scheme table
      bitmunk.model.remove(
         bitmunk.catalog.model.name,
         bitmunk.catalog.model.payeeSchemeTable,
         event.details.payeeSchemeId);
      
      // update the total payee scheme count
      bitmunk.catalog.model.totalPayeeSchemes--;
      
      // trigger removed event
      $.event.trigger(
         event.type.replace(/\./g, '-'), [event.details.payeeSchemeId]);
   };
   
   bitmunk.model.addModel(bitmunk.catalog.model.name, {start: start});
   sScriptTask.unblock();
})(jQuery);

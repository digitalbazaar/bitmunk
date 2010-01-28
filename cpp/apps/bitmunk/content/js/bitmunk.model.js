/**
 * Bitmunk Web UI --- Data Model Functions
 *
 * @author Dave Longley
 *
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 */
(function($)
{
   // log category
   var sLogCategory = 'bitmunk.webui.core.Model';
   
   // timeout for stopping a model if it is unused, if a user hasn't used
   // a model in sTimeout, then it is reasonable to assume they won't be
   // using it for a while longer and we can stop collecting events for it
   var sTimeout = 1000 * 60 * 5; /* 5 minute timeout */
   
   /**
    * A table of available data models and their reference counts.
    */
   var sAvailableModels = {};
   bitmunk.debug.set(sLogCategory, 'availableModels', sAvailableModels);
   
   /**
    * Stores the in-use data models. This object synchronizes data pulled
    * from the backend with the custom data models for each individual
    * plugin interface and stores it in a set of tables per data model.
    * 
    * The model is a hashtable of data model names to data tables. Using
    * this approach, each data model plugin can have N data tables. Each
    * table contains an array of rows where each contains the an ID for
    * the row data, a sequence number that is used to synchronize updates,
    * and the row data itself. There is also an index that maps IDs to row
    * indexes in the table.
    */
   var sUsedModels = {};
   bitmunk.debug.set(sLogCategory, 'usedModels', sUsedModels);
   
   /**
    * Clear model stop timeout if set.
    * 
    * @param model the name of the model.
    */
   var clearTimeout = function(model)
   {
      if(model.stopTimeoutId)
      {
         clearTimeout(model.stopTimeoutId);
         model.stopTimeoutId = null;
      }
   };
   
   // define the data model interface:
   bitmunk.model =
   {
      /**
       * Adds a data model to the list of available models.
       * 
       * @param model the name of the model.
       * @param inf the interface for the model.
       */
      addModel: function(model, inf)
      {
         inf = $.extend(inf,
         {
            // default stop method for stopping a model will simply
            // unregister it
            stop: function(task)
            {
               bitmunk.model.unregister(task, model);
            }
         });
         
         // wrap stop with log messages
         var wrappedStop = inf.stop;
         inf.stop = function(task)
         {
            task.next(function(task)
            {
               bitmunk.log.debug(sLogCategory, 'stopping model:', model);
            });
            task.next(wrappedStop);
            task.next(function(task)
            {
               bitmunk.log.debug(sLogCategory, 'stopped model:', model);
            });
         };
         
         sAvailableModels[model] =
         {
            'interface': inf,
            refCount: 0,
            running: false,
            stopTimeoutId: null
         };
      },
      
      /**
       * Explicitly stops all running models.
       * 
       * @param task the task to use to synchronously stop all models.
       */
      stop: function(task)
      {
         // clear all stop timeouts
         $.each(sAvailableModels, function(name, model)
         {
            clearTimeout(model);
         });
         
         // stop models in parallel
         var functions = [];
         $.each(sAvailableModels, function(name, model)
         {
            model.running = false;
            functions.push(model['interface'].stop);
         });
         task.parallel(functions);
      },
      
      /**
       * Attaches a view to a data model.
       * 
       * @param task the task to use to synchronously attach a view.
       * @param model the name of the data model.
       */
      attachView: function(task, model)
      {
         // clear any stop timeout
         clearTimeout(sAvailableModels[model]);
         
         // increment reference count, if 1, start model if not running
         if(++sAvailableModels[model].refCount === 1)
         {
            if(!sAvailableModels[model].running)
            {
               sAvailableModels[model].running = true;
               sAvailableModels[model]['interface'].start(task);
            }
         }
      },
      
      /**
       * Detaches a view from a data model.
       * 
       * @param model the name of the data model.
       */
      detachView: function(model)
      {
         // clear any stop timeout
         clearTimeout(sAvailableModels[model]);
         
         // decrement reference count, if 0, stop model
         // after expiration timeout
         if(--sAvailableModels[model].refCount === 0)
         {
            sAvailableModels[model].stopTimeoutId = setTimeout(function()
            {
               if(sAvailableModels[model].refCount === 0)
               {
                  sAvailableModels[model].running = false;
                  bitmunk.task.start(
                  {
                     type: 'bitmunk.view',
                     name: 'bitmunk.model.stop.' + model,
                     run: function(task)
                     {
                        sAvailableModels[model]['interface'].stop(task);
                     }
                  });
               }
            }, sTimeout);
         }
      },
      
      /**
       * Adds a table to a model, but does not register the table unless
       * specified. If adding the table fails, the associated task will fail.
       * 
       * @param task the task to use to synchronize adding the table.
       * @param options: {
       *    model: <model name>,
       *    table: <table name>,
       *    eventGroups: [{
       *       events: [ <event names> ],
       *       filter: <optional event filter>,
       *       eventCallback: <event occurred, signature(event, seqNum)>
       *    }],
       *    coalesceRules: <optional event coalesce rules>,
       *    watchRules: <optional> {
       *       getOptions: function([] of row IDs) returns: {
       *          eventGroups: <same as above>
       *          coalesceRules: <same as above>
       *       },
       *       reinitializeRows: function([] of row IDs) that reinitializes
       *          rows in the table,
       *       failure: optional function() to be called if subscription
       *          to watch events fails.
       *    },
       *    register: <true to register the table, false not to>
       * }
       */
      addTable: function(task, options)
      {
         // create table info
         task.next(function(task)
         {
            // build event group for table, there will be one observer per
            // table since each table can have different coalesce rules and
            // a single observer can have only one set of coalesce rules
            // create table info
            var tableInfo =
            {
               seqNum: 0,
               rows: [],
               index: {},
               eventGroup:
               {
                  events: [],
                  coalesceRules: options.coalesceRules
               },
               watches: {},
               watchRules: options.watchRules || {}
            };
            
            // set table info
            var model = sUsedModels[options.model];
            model.tables[options.table] = tableInfo;
            
            // build event list in event group
            $.each(options.eventGroups, function(groupIdx, group)
            {
               $.each(group.events, function(eventIdx, eventName)
               {
                  tableInfo.eventGroup.events.push({
                     type: eventName, 
                     filter: ('filter' in group ? group.filter : null),
                     callback: function(event)
                     {
                        // only call event callback once fully registered
                        if(model.registered)
                        {
                           var seqNum = tableInfo.seqNum++;
                           group.eventCallback(event, seqNum);
                        }
                     }
                  });
               });
            });
            
            bitmunk.log.debug(
               sLogCategory, 'table added:',
               options.model + '.' + options.table);
         });
         
         // if specified, subscribe for events for table
         if(options.subscribe)
         {
            task.next(function(task)
            {
               task.block();
               bitmunk.events.subscribe({
                  events: tableInfo.eventGroup.events,
                  coalesceRules: tableInfo.eventGroup.coalesceRules,
                  success: function(observers)
                  {
                     // update model's list of observers
                     model.observers.concat(observers);
                     task.unblock();
                  },
                  error: function()
                  {
                     task.fail();
                  }
               });
            });
         }
      },
      
      /**
       * Called when a data model is started to register its tables. If the
       * registration fails, task.fail() will be called.
       * 
       * The passed parameter is an object that contains groupings of related
       * events and a callback to call when the events occur. That callback
       * will be passed the event that occurred and the current sequence number
       * for the table the events are for. That sequence number must be passed
       * to the update call if data is to be added to the data table.
       * 
       * @param task the task to use to synchronize registering the model.
       * @param registerObject: {
       *    model: <model name>,
       *    tables: [{
       *       name: <name of table>,
       *       eventGroups: [{
       *          events: [ <event names> ],
       *          filter: <optional event filter>,
       *          eventCallback: <event occurred, signature(event, seqNum)>
       *       }],
       *       coalesceRules: <optional event coalesce rules>,
       *       watchRules: <optional> {
       *          getRegisterObject: function([] of row IDs) returns: {
       *             eventGroups: <same as above>
       *             coalesceRules: <same as above>
       *          },
       *          reinitializeRows: function([] of row IDs) that reinitializes
       *             rows in the table,
       *          failure: optional function() to be called if subscription
       *             to watch events fails.
       *       }
       *    }]
       */
      register: function(task, registerObject)
      {
         // initialize model
         task.next(function(task)
         {
            bitmunk.log.debug(
               sLogCategory, 'registering:', registerObject.model);
            
            var model =
            {
               registered: false,
               tables: {},
               observers: []
            };
            sUsedModels[registerObject.model] = model;
         });
         
         // add tables (without subscribing yet)
         task.next(function(task)
         {
            // add each table
            $.each(registerObject.tables, function(idx, tableOptions)
            {
               tableOptions.model = registerObject.model;
               tableOptions.table = tableOptions.name;
               tableOptions.subscribe = false;
               bitmunk.model.addTable(task, tableOptions);
            });
         });
         
         // do multi-subscribe to subscribe for events for all tables at once
         task.next(function(task)
         {
            var eventGroups = [];
            var model = sUsedModels[registerObject.model];
            $.each(model.tables, function(name, tableInfo)
            {
               eventGroups.push(tableInfo.eventGroup);
            });
            
            task.block();
            bitmunk.events.multiSubscribe({
               eventGroups: eventGroups,
               success: function(observers)
               {
                  // set model's list of observers
                  model.observers = observers;
                  
                  // now registered
                  model.registered = true;
                  bitmunk.log.debug(
                     sLogCategory, 'register success:', registerObject.model);
                  task.unblock();
               },
               error: function()
               {
                  task.next(function(task)
                  {
                     bitmunk.log.debug(
                        sLogCategory, 'register error:', registerObject.model);
                     
                     // unregister model
                     bitmunk.model.unregister(task, registerObject.model);
                  });
                  task.next(task.fail);
                  task.unblock();
               }
            });
         });
      },
      
      /**
       * Called when a data model is stopped to unregister its events.
       * 
       * @param task the task to use to synchronize unregistration.
       * @param model the name of the model to unregister.
       */
      unregister: function(task, model)
      {
         // only do unregister if model exists
         if(model in sUsedModels)
         {
            task.next(function(task)
            {
               bitmunk.log.debug(sLogCategory, 'unregister:', model);
               
               // model no longer registered
               sUsedModels[model].registered = false;
               var observers = sUsedModels[model].observers;
               delete sUsedModels[model];
               
               // unregister the backend event observers for the model
               task.block();
               bitmunk.events.deleteObservers(observers,
               {
                  complete: function() { task.unblock(); }
               });
            });
         }
      },
      
      /**
       * Called to update a data model table. The update will only be applied
       * if the current sequence number for the row is less than the passed
       * sequence number (the sequence number assigned by bitmunk.model) OR
       * if the sequence number is zero AND either there is no current sequence
       * number or it is zero. This latter case allows data to be initialized
       * in a table any number of times.
       * 
       * @param model the name of the model.
       * @param table the name of the table.
       * @param id the ID of the data row.
       * @param row the data row.
       * @param seqNum the sequence number provided by bitmunk.model.
       *
       * @return true if updated, false if not updated due to a sequence error
       *         or because the model was not registered.
       */
      update: function(model, table, id, row, seqNum)
      {
          var rval = false;

         // only do update if model is registered
         if(model in sUsedModels && sUsedModels[model].registered)
         {
            // get rows and index
            var tableInfo = sUsedModels[model].tables[table];
            var rows = tableInfo.rows;
            var index = tableInfo.index;
            
            // create row if it doesn't exist
            if(!(id in index))
            {
               index[id] = rows.length;
               rows.push({ id: id, seqNum: seqNum, data: row });
               rval = true;
            }
            else
            {
               var r = rows[index[id]];
               
               // only update row if the passed sequence number is greater
               if(r.seqNum < seqNum || (r.seqNum === 0 && seqNum === 0))
               {
                  r.seqNum = seqNum;
                  r.data = row;
                  rval = true;
               }
            }
         }
         
         return rval;
      },
      
      /**
       * Gets a data row.
       * 
       * @param model the name of the model.
       * @param table the name of the table.
       * @param id the ID of the data row.
       *
       * @return the data row or null if none.
       */
      fetch: function(model, table, id)
      {
         var rval = null;
         
         // only do fetch if model is registered
         if(model in sUsedModels && sUsedModels[model].registered)
         {
            // get rows and index
            var tableInfo = sUsedModels[model].tables[table];
            var index = tableInfo.index;
            var rows = tableInfo.rows;
            
            // return row data
            rval = (id in index) ? rows[index[id]].data : null;
         }
         
         if(rval !== null)
         {
            // clone data
            rval = $.extend(true, {}, rval);
         }
         
         return rval;
      },
      
      /**
       * Gets all data in a table.
       * 
       * @param model the name of the model.
       * @param table the name of the table.
       * 
       * @return all the data in the table or null if none.
       */
      fetchAll: function(model, table)
      {
         var rval = null;
         
         // only do fetch if model is registered
         if(model in sUsedModels && sUsedModels[model].registered)
         {
            // get table info
            var tableInfo = sUsedModels[model].tables[table];
            
            // clone each row's data
            rval = {};
            $.each(tableInfo.rows, function(i, row)
            {
               rval[row.id] = $.extend(true, {}, row.data);
            });
         }
         
         return rval;
      },
      
      /**
       * Returns a slice of the data in a table.
       * 
       * @param model the name of the model.
       * @param table the name of the table.
       * @param start the starting row index.
       * @param count the number of rows to return.
       *
       * @return the slice of data as an array, null if the model isn't
       *         registered.
       */
      slice: function(model, table, start, count)
      {
         var rval = null;
         
         // only do fetch if model is registered
         if(model in sUsedModels && sUsedModels[model].registered)
         {
            // get rows
            var tableInfo = sUsedModels[model].tables[table];
            var rows = tableInfo.rows;
            
            // clone each row's data and insert into the return array
            rval = [];
            for(var i = start; rval.length < count && i < rows.length; i++)
            {
               rval.push($.extend(true, {}, rows[i].data));
            }
         }
         
         return rval;
      },
      
      /**
       * Removes a data row.
       * 
       * @param model the name of the model.
       * @param table the name of the table.
       * @param id the ID of the data row.
       */
      remove: function(model, table, id)
      {
         // only do remove if model is registered
         if(model in sUsedModels && sUsedModels[model].registered)
         {
            // get index
            var tableInfo = sUsedModels[model].tables[table];
            var index = tableInfo.index;
            
            if(id in index)
            {
               // get rows
               var rows = tableInfo.rows;
               
               // update index by decrementing each entry's row index
               // for entries past than the row that is being deleted
               var rowIndex = index[id];
               for(var i = rowIndex + 1; i < rows.length; i++)
               {
                  index[rows[i].id]--;
               }
               
               // delete index entry
               delete index[id];
               
               // remove row
               rows = rows.splice(rowIndex, 1);
            }
            
            // delete watch
            if(id in tableInfo.watches)
            {
               if(tableInfo.rows.length === 0)
               {
                  // simply clear table
                  bitmunk.model.clear(model, table);
               }
               else
               {
                  // delete specific watch
                  var observerId = tableInfo.watches[id];
                  delete tableInfo.watches[id];
               }
            }
         }
      },
      
      /**
       * Clear all rows in a table.
       * 
       * @param model the name of the model.
       * @param table the name of the table.
       */
      clear: function(model, table)
      {
         // only do clear if model is registered
         if(model in sUsedModels && sUsedModels[model].registered)
         {
            bitmunk.log.debug(sLogCategory, 'clear:', model + '.' + table);
            
            // clear table rows and index
            var tableInfo = sUsedModels[model].tables[table];
            tableInfo.rows = [];
            tableInfo.index = {};
            
            // build list of all watch observers
            var observers = [];
            $.each(tableInfo.watches, function(rowId, observerId)
            {
               // add unique observer IDs
               if($.inArray(observerId, observers) == -1)
               {
                  observers.push(observerId);
               }
            });
            
            // unregister the backend event observers for watching the table
            bitmunk.events.deleteObservers(observers);
            
            // clear watches
            tableInfo.watches = {};
         }
      },
      
      /**
       * Starts watching any row in a table that is not current being
       * watched, according to the table's watch rows. To stop watching
       * a row, it must be removed from the table. New rows will not be
       * automatically watched.
       * 
       * Once watching has been activated, the row data will be re-acquired
       * to prevent any race condition where an event related to the row data
       * was lost between when we began registering for events and when we
       * actually registered. Any data that is reacquired will only replace
       * existing row data if no events that resulted in an update for the
       * same row data were received during the reacquistion. This prevents
       * another race condition where some row data is updated via event(s)
       * to a point that is more up to date than the data that is reacquired.
       * 
       * @param model the name of the model.
       * @param table the name of the table.
       */
      watch: function(model, table)
      {
         // only do watch if model is registered
         if(model in sUsedModels && sUsedModels[model].registered)
         {
            bitmunk.log.debug(sLogCategory, 'watch:', model + '.' + table);
            
            // get model
            var m = sUsedModels[model];
            
            // get table info
            var tableInfo = sUsedModels[model].tables[table];
            
            // get rows that are unwatched
            var unwatched = [];
            if(tableInfo.watchRules.getRegisterObject)
            {
               $.each(tableInfo.rows, function(i, row)
               {
                  if(!(row.id in tableInfo.watches))
                  {
                     unwatched.push(row.id);
                  }
               });
            }
            
            if(unwatched.length > 0)
            {
               // get register object
               var ro = tableInfo.watchRules.getRegisterObject(unwatched);
               
               // add all events to options
               var options = {};
               options.events = [];
               options.coalesceRules = ro.coalesceRules || [];
               $.each(ro.eventGroups || [], function(groupIdx, group)
               {
                  $.each(group.events, function(eventIdx, eventName)
                  {
                     options.events.push({
                        type: eventName,
                        filter: ('filter' in group ? group.filter : null),
                        callback: function(event)
                        {
                           // only call event callback once fully registered
                           if(m.registered)
                           {
                              var seqNum = tableInfo.seqNum++;
                              group.eventCallback(event, seqNum);
                           }
                        }
                     });
                  });
               });
               
               // create success callback
               options.success = function(data)
               {
                  // add observer ID to list for model
                  m.observers.push(data[0]);
                  
                  // add unwatched rows as watched now (rowId => observerId)
                  $.each(unwatched, function(index, rowId)
                  {
                     tableInfo.watches[rowId] = data[0];
                  });
                  
                  // reinitialize rows
                  bitmunk.log.debug(
                     sLogCategory, 'watch reinitializeRows:',
                     model + '.' + table, unwatched);
                  tableInfo.watchRules.reinitializeRows(unwatched);
               };
               
               // create failure callback
               options.failure = function()
               {
                  bitmunk.log.debug(
                     sLogCategory, 'watch failed:', model + '.' + table);
                  tableInfo.watchRules.failure();
               };
               
               // subscribe to events
               bitmunk.events.subscribe(options);
            }
         }
      }
   };
   
   // register plugin
   var defaults = $.extend(true, {}, bitmunk.sys.corePluginDefaults);
   bitmunk.sys.register($.extend(defaults, {
      pluginId: 'bitmunk.webui.core.Model',
      name: 'Bitmunk Model'
   }));
})(jQuery);

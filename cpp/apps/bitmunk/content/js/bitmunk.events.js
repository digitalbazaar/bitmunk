/**
 * Bitmunk Web UI --- Events
 *
 * @author Dave Longley
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 *
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
(function($)
{
   // logging category
   var sLogCategory = 'bitmunk.webui.core.Events';
   
   // the current state of the event engine.
   var state =
   {
      // the IDs of the observers being polled
      observerIds: [],
      
      // observer ID mappings to event type to filter and callback:
      // observer ID => event type => [filter => callback]
      callbacks: {},
      
      // set to true when polling should continue
      poll: false,
      
      // the long poll interval (1 second)
      pollInterval: 1000
   };
   
   // debug access
   bitmunk.debug.set(sLogCategory, 'state', state);
   
   // the base url to the events web-api
   var eventsUrl = '/api/3.0/system/events';
   
   // define bitmunk events interface:
   bitmunk.events =
   {
      /**
       * Initializes the event service.
       * 
       * @param task the task to use to initialize events synchronously.
       */
      init: function(task)
      {
         var timer = +new Date();
         task.block();
         bitmunk.events.subscribe(
         {
            events: [
            {
               type: 'bitmunk.webui.Observer.created',
               filter: { details: { userId: bitmunk.user.getUserId() } },
               callback: function(event) {}
            }],
            success: function()
            {
               timer = +new Date() - timer;
               bitmunk.log.debug('timing',
                  'event system initialized in ' + timer + ' ms');
               task.unblock();
            },
            error: function()
            {
               task.fail();
            }
         });
      },
      
      /**
       * Stops the event polling service.
       *
       * @param task the task to use to do clean up synchronously, null to
       *             do silent clean up without contacting backend.
       */
      cleanup: function(task)
      {
         // stop polling
         state.poll = false;
         
         // clean up observers on front end
         var observerIds = state.observerIds.slice();
         state.observerIds = [];
         state.callbacks = {};
         
         if(task)
         {
            // clean up observers on backend
            task.block();
            bitmunk.events.deleteObservers(observerIds,
            {
               complete: function()
               {
                  task.unblock();
               }
            });
         }
      },
      
      /**
       * Polls for events and dynamically updates the polling time.
       */
      poll: function()
      {
         var timeout = 0;
         if(state.observerIds.length > 0)
         {
            bitmunk.events.receiveEvents(
            {
               // schedule next poll
               complete: function(data, count)
               {
                  if(state.poll)
                  {
                     setTimeout(bitmunk.events.poll, timeout);
                     timeout = 0;
                  }
               },
               // handle invalid observer IDs
               error: function(xhr)
               {
                  try
                  {
                     var ex = JSON.parse(xhr.responseText);
                     if(ex.type ===
                        'bitmunk.webui.EventService.ObserverNotFound')
                     {
                        bitmunk.log.debug(
                           sLogCategory, 'removing unknown observers',
                           ex.details.observerIds);
                        bitmunk.events.deleteObservers(ex.details.observerIds);
                     }
                     else
                     {
                        // wait a bit longer before polling again
                        timeout = 1000 * 5;
                     }
                  }
                  catch(e)
                  {
                     // wait a bit longer before polling again
                     timeout = 1000 * 5;
                  }
               }
            });
         }
         else
         {
            // no more observers to poll
            state.poll = false;
         }
      },
      
      /**
       * Creates observer(s) that can listen for events. The observer IDs that
       * are created will be passed to the success callback in an array.
       * 
       * @param num the number of observers to create.
       * @param options:
       *        success: a callback for observer creation success.
       *        error: a callback for observer creation failure.
       *        complete: a callback for when the creation completes.
       */
      createObservers: function(num, options)
      {
         // set parameter defaults
         options = $.extend(
         {
            success: function() {},
            error: function() {},
            complete: function() {}
         }, options || {});
         
         // post to create observers
         bitmunk.api.proxy(
         {
            method: 'POST',
            url: eventsUrl + '/observers/create',
            params: { nodeuser: bitmunk.user.getUserId() },
            data: { count: num },
            success: function(data, textStatus)
            {
               // add new IDs into list
               data = JSON.parse(data);
               state.observerIds = state.observerIds.concat(data);
               
               // start polling if not doing so already
               if(!state.poll)
               {
                  state.poll = true;
                  bitmunk.events.poll();
               }
               else
               {
                  // send webui observer created event to reset polling to
                  // include new observer(s)
                  bitmunk.events.sendEvents(
                  {
                     send: [{
                        type: 'bitmunk.webui.Observer.created',
                        details: { userId: bitmunk.user.getUserId() }
                     }]
                  });
               }
               
               options.success(data, textStatus);
            },
            error: options.error,
            complete: options.complete
         });
      },
      
      /**
       * Deletes observer(s).
       *
       * @param observerIds the list of IDs of observers to delete.
       * @param options:
       *        complete: a callback for when the deletion completes.
       */
      deleteObservers: function(observerIds, options)
      {
         // set parameter defaults
         options = $.extend(
         {
            complete: function() {}
         }, options || {});
         
         // remove observer IDs from state, remove stored callbacks, keep
         // track of observer IDs that were removed so we only ask the
         // backend to remove those that exist
         var ids = [];
         $.each(observerIds, function()
         {
            var i = $.inArray('' + this, state.observerIds);
            if(i != -1)
            {
               state.observerIds.splice(i, 1);
               ids.push(this);
            }
            if(this in state.callbacks)
            {
               delete state.callbacks[this];
            }
         });
         
         // post to delete observers
         if(ids.length > 0)
         {
            bitmunk.api.proxy(
            {
               method: 'POST',
               url: eventsUrl + '/observers/delete',
               params: { nodeuser: bitmunk.user.getUserId() },
               data: observerIds,
               error: function()
               {
                  // ignore delete failures
               },
               complete: function(XMLHttpRequest, textStatus)
               {
                  // stop polling if no more observers
                  if(state.observerIds.length === 0)
                  {
                     state.poll = false;
                  }
                  
                  options.complete(XMLHttpRequest, textStatus);
               },
               // stop global error handler
               global: false
            });
         }
         else
         {
            // complete, no observers to remove
            options.complete();
         }
      },
      
      /**
       * Registers observer(s) for events. The observer IDs that are
       * successfully registered will be passed to the success callback in
       * an array.
       * 
       * For each coalesce rule, each object should state which parts of
       * the event data that must be the same or different in order for
       * events to be coalesced. For instance, if there are two events:
       * 
       * eventA (occurred first):
       * {
       *    "details": {
       *       "foo" : "abc",
       *       "bar" : "xyz"
       *    }
       * }
       * 
       * eventB (occurred second):
       * {
       *    "details": {
       *       "foo" : "abc"
       *    }
       * }
       * 
       * Then they could be coalesced (meaning only the latest event
       * will appear in the observer's queue) by using the given coalesce rule:
       * 
       * {
       *    "details": {
       *       "foo" : true
       * }
       * 
       * If "foo" were set to false, then events where "foo"'s value is not
       * equal would be coalesced. If details where set to true instead of
       * a map, then any event with all of the same details would be coalesced.
       * 
       * @param options:
       *        observers: [{
       *           id: <observerId>
       *           events: [{
       *              type: <eventType>,
       *              filter: <filter>,
       *              callback: <callback>
       *           }],
       *           coalesceRules: [<rule>]
       *        },
       *        success: a callback for observer registration success.
       *        error: a callback for observer registration failure.
       *        complete: a callback for when the registration completes.
       */
      registerObservers: function(options)
      {
         // set parameter defaults
         options = $.extend(
         {
            success: function() {},
            error: function() {},
            complete: function() {}
         }, options || {});
         
         // save indexes of observers to be created on-the-fly and create
         // register data
         var registerData = [];
         var observers = options.observers;
         var indexes = [];
         $.each(observers, function(i, entry)
         {
            if(entry.id === '0')
            {
               indexes.push(i);
            }
            
            var observer = {};
            observer.id = entry.id;
            observer.events = [];
            registerData.push(observer);
            
            // add events to register data
            $.each(entry.events, function()
            {
               var event = {}
               event.type = this.type;
               if(this.filter !== null)
               {
                  event.filter = this.filter;
               }
               observer.events.push(event);
            });
            
            // add coalesceRules if not null
            if(entry.coalesceRules !== null)
            {
               observer.coalesceRules = entry.coalesceRules;
            }
         });
         
         var timer = +new Date();
         
         // post to register observers for events
         bitmunk.api.proxy(
         {
            method: 'POST',
            url: eventsUrl + '/observers/register',
            params: { nodeuser: bitmunk.user.getUserId() },
            data: registerData,
            success: function(data, textStatus)
            {
               timer = +new Date() - timer;
               bitmunk.log.debug('timing',
                  'registered for events in ' + timer + ' ms');
               
               // get array of valid observer IDs (same order as observer
               // entries that were posted)
               var ids = JSON.parse(data);
               
               // add any new observer IDs into list and observers data
               $.each(indexes, function(i, index)
               {
                  state.observerIds.push(ids[index]);
                  observers[index].id = ids[index];
               });
               
               // FIXME: change state.callbacks so that different functions can be
               // used by different filters (right now we have an artificial limit
               // of 1 callback per event type)
               
               // update callback map
               $.each(observers, function(i, entry)
               {
                  // add callbacks to state
                  $.each(entry.events, function()
                  {
                     if(!(entry.id in state.callbacks))
                     {
                        state.callbacks[entry.id] = {};
                     }
                     state.callbacks[entry.id][this.type] = this.callback;
                  });
               });
               
               if(indexes.length > 0)
               {
                  // start polling if not doing so already
                  if(!state.poll)
                  {
                     state.poll = true;
                     bitmunk.events.poll();
                  }
                  else
                  {
                     // send webui observer created event to reset polling to
                     // include new observer(s)
                     bitmunk.events.sendEvents(
                     {
                        send: [{
                           type: 'bitmunk.webui.Observer.created',
                           details: { userId: bitmunk.user.getUserId() }
                        }]
                     });
                  }
               }
               
               // do callback
               options.success(ids, textStatus);
            },
            error: options.error,
            complete: options.complete
         });         
      },
      
      /**
       * Unregisters observer(s) for events. The observer IDs that are
       * successfully unregistered will be passed to the success callback in
       * an array.
       * 
       * @param options:
       *        observers: a map with the following (events&coalesce optional):
       *           <observerId>: {events: [<eventType>], coalesce: [<rule>]}
       *        complete: a callback for when the unregistration completes.
       */
      unregisterObservers: function(options)
      {
         // set parameter defaults
         options = $.extend(
         {
            success: function() {},
            error: function() {},
            complete: function() {}
         }, options || {});
         
         // update callback map
         var observers = options.observers;
         $.each(observers, function(observerId, value)
         {
            if('events' in value)
            {
               // remove specific callbacks
               var callbacks = state.callbacks[observerId];
               if(callbacks)
               {
                  $.each(value.events, function()
                  {
                     delete callbacks[this];
                  });
               }
            }
            else if(!('coalesceRules' in value) ||
                    value.coalesceRules.length === 0)
            {
               // remove all callbacks
               delete state.callbacks[observerId];
            }
         });
         
         // post to unregister observers for events/coalesce rules
         bitmunk.api.proxy(
         {
            method: 'POST',
            url: eventsUrl + '/observers/unregister',
            params: { nodeuser: bitmunk.user.getUserId() },
            data: observers,
            complete: function(XMLHttpRequest, textStatus)
            {
               options.complete(XMLHttpRequest, textStatus);
            },
            error: function()
            {
               // ignore unregister failures
            },
            // stop global error handler
            global: false
         });
      },
      
      /**
       * Subscribes to some events by registering callbacks for them. A new
       * observer will be created and its ID will be passed to the success
       * callback in an array. That ID can be used to delete the observer and
       * remove all of the registered callbacks at once.
       * 
       * Event filters are optional. If a filter is included it should
       * be an object that represents a subset of an event. For instance:
       *
       * filter: {
       *    details: {
       *       mydata: "foo"
       *    }
       * }
       *
       * Will capture this event:
       * 
       * event: {
       *    type: "testevent",
       *    details: {
       *       mydata: "foo",
       *       irrelevant: 4
       *    }
       * }
       * 
       * But not this event:
       * 
       * event: {
       *    type: "testevent",
       *    details: {
       *       mydata: "bar",
       *       irrelevant: 4
       *    }
       * }
       * 
       * Coalesce rules are also optional. See registerObservers for details
       * on coalesce rules.
       * 
       * @param options:
       *        events: [{
       *           type: <eventType>,
       *           filter: <filter>, 
       *           callback: <callback>
       *        }],
       *        coalesceRules: [<rule>]
       *        success: a callback for subscription success.
       *        error: a callback for subscription failure.
       *        complete: a callback for when the subscription completes.
       */
      subscribe: function(options)
      {
         // do multisubscribe with a single subscription
         options = {
            eventGroups: [{
               events: options.events,
               coalesceRules: options.coalesceRules
            }],
            success: options.success || function() {},
            error: options.error || function() {},
            complete: options.complete || function() {}
         };
         bitmunk.events.multiSubscribe(options);
      },
      
      /**
       * Subscribes to several groups of events by registering callbacks for
       * them. A new observer will be created for each group and all of the
       * IDs will be passed to the success callback in an array. Those IDs can
       * be used to delete the observer(s) and remove all of the registered
       * callbacks at once.
       * 
       * Event filters are optional. If a filter is included it should
       * be an object that represents a subset of an event. For instance:
       *
       * filter: {
       *    details: {
       *       mydata: "foo"
       *    }
       * }
       *
       * Will capture this event:
       * 
       * event: {
       *    type: "testevent",
       *    details: {
       *       mydata: "foo",
       *       irrelevant: 4
       *    }
       * }
       * 
       * But not this event:
       * 
       * event: {
       *    type: "testevent",
       *    details: {
       *       mydata: "bar",
       *       irrelevant: 4
       *    }
       * }
       * 
       * Coalesce rules are also optional. See registerObservers for details
       * on coalesce rules.
       * 
       * @param options:
       *        eventGroups: [{
       *           events: [{
       *              type: <eventType>,
       *              filter: <filter>, 
       *              callback: <callback>
       *           }],
       *           coalesceRules: [<rule>]
       *        }],
       *        success: a callback for subscription success.
       *        error: a callback for subscription failure.
       *        complete: a callback for when the subscription completes.
       */
      multiSubscribe: function(options)
      {
         // set parameter defaults
         options = $.extend(
         {
            success: function() {},
            error: function() {},
            complete: function() {}
         }, options || {});
         
         var timer = +new Date();
         
         // create new observer(s)
         var observerCount = options.eventGroups.length;
         bitmunk.events.createObservers(observerCount,
         {
            success: function(data)
            {
               timer = +new Date() - timer;
               bitmunk.log.debug('timing',
                  'created observers in ' + timer + ' ms');
               
               // get new observer IDs
               var ids = data;
               
               // build observer data in reverse because the event groups
               // are popped off the end of the event group array (this
               // way the order of the observer IDs matches the order of
               // the event groups
               var obs = [];
               for(var i = ids.length - 1; i >= 0; i--)
               {
                  var group = options.eventGroups.pop();
                  obs.push({
                     id: ids[i],
                     events: group.events,
                     coalesceRules: group.coalesceRules
                  });
               }
               
               // register observer(s) for events
               bitmunk.events.registerObservers(
               {
                  observers: obs,
                  success: options.success,
                  error: options.error,
                  complete: options.complete
               });
            },
            error: options.error
         });
      },
      
      /**
       * Sends events to a bitmunk node's event service.
       * 
       * @param options:
       *        send: an array of events to send:
       *           [{type: <eventType>, details: <map of event data}]
       *        success: a callback for event sending success.
       *        error: a callback for event sending failure.
       *        complete: a callback for when the sending completes.
       */
      sendEvents: function(options)
      {
         // set parameter defaults
         options = $.extend(
         {
            success: function() {},
            error: function() {},
            complete: function() {}
         }, options || {});
         
         // create post data
         var postData =
         {
            send: options.send
         };
         
         // post to send events
         bitmunk.api.proxy(
         {
            method: 'POST',
            url: eventsUrl,
            params: { nodeuser: bitmunk.user.getUserId() },
            data: postData,
            success: function(data, textStatus)
            {
               options.success(JSON.parse(data), textStatus);
            },
            error: options.error,
            complete: options.complete
         });
      },
      
      /**
       * Receives events from a bitmunk node's event service and sends
       * them out to any previously registered callback functions.
       * 
       * @param options:
       *        success: a callback for event sending success.
       *        error: a callback for event sending failure.
       *        complete: a callback for when the sending completes.
       */
      receiveEvents: function(options)
      {
         // set parameter defaults
         options = $.extend(
         {
            success: function() {},
            error: function() {},
            complete: function() {}
         }, options || {});
         
         // create post data
         var postData =
         {
            receive: state.observerIds,
            pollInterval: state.pollInterval
         };
         
         // post to receive events
         bitmunk.api.proxy(
         {
            method: 'POST',
            url: eventsUrl,
            params: { nodeuser: bitmunk.user.getUserId() },
            timeout: 300,
            data: postData,
            success: function(data, textStatus)
            {
               // events are returned as a map:
               // <observerId>: [<event>]
               // if event queues are empty, the observer ID will not
               // be in the map
               var count = 0;
               data = JSON.parse(data);
               $.each(data, function(observerId, events)
               {
                  var callbacks = state.callbacks[observerId];
                  if(callbacks)
                  {
                     count += events.length;
                     $.each(events, function()
                     {
                        callbacks[this.type](this);
                     });
                  }
               });
               
               options.success(data, count);
            },
            error: options.error,
            complete: options.complete
         });
      },
      
      /**
       * Adds events to a bitmunk node's event daemon.
       * 
       * @param options:
       *        add: an array of events and scheduling information to add:
       *           [{event: {type: <eventType>, details: <map of event data},
       *             interval: <interval in milliseconds to schedule event>},
       *             count: <number of events to schedule>,
       *             private: <true to duplicate event, false to share event>}]
       *        success: a callback for event adding success.
       *        error: a callback for event adding failure.
       *        complete: a callback for when the adding completes.
       */
      addDaemonEvents: function(options)
      {
         // set parameter defaults
         options = $.extend(
         {
            success: function() {},
            error: function() {},
            complete: function() {}
         }, options || {});
         
         // change public to "refs"
         $.each(options.add, function()
         {
            this.refs = (this['private'] ? 0 : 1);
            delete this['private'];
         });
         
         // create post data
         var postData =
         {
            add: options.add
         };
         
         // post to add daemon events
         bitmunk.api.proxy(
         {
            method: 'POST',
            url: eventsUrl,
            params: { nodeuser: bitmunk.user.getUserId() },
            data: postData,
            success: function(data, textStatus)
            {
               options.success(JSON.parse(data), textStatus);
            },
            error: options.error,
            complete: options.complete
         });
      }
   };
   
   // register plugin
   var defaults = $.extend(true, {}, bitmunk.sys.corePluginDefaults);
   bitmunk.sys.register($.extend(defaults, {
      pluginId: 'bitmunk.webui.core.Events',
      name: 'Bitmunk Events'
   }));
})(jQuery);

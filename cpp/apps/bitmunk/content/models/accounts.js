/**
 * Bitmunk Accounts Model
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 * 
 * @author Dave Longley
 */
(function($)
{
   // model name
   var sModel = 'accounts';
   
   // category for logger
   var sLogCategory = 'models.accounts';
   
   var sScriptTask = bitmunk.resource.getScriptTask(
      'bitmunk.webui.Main', 'accounts.js');
      
   // used to schedule account update polling events
   var sIntervalId = null;
   
   /**
    * Starts updating the accounts data model.
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
            // create accounts table
            name: 'accounts',
            eventGroups: [{
               events: [
                  'bitmunk.common.User.accountUpdatePoll',
                  'bitmunk.purchase.DownloadState.licensePurchased',
                  'bitmunk.purchase.DownloadState.dataPurchased'
               ],
               // filter events based on these details
               filter: {
                  details: {
                     userId: bitmunk.user.getUserId()
                  }
               },
               eventCallback: updateAccounts
            }],
            // combine similar events if these details are the same
            coalesceRules: [{
               details: {
                  // these details must be the same to coalesce
                  userId: true
               }
            }]
         }]
      });
      
      // add task to populate accounts table
      task.next(function(task)
      {
         // do initial account grab
         bitmunk.user.getAccounts(function(accounts)
         {
            // insert accounts into table
            bitmunk.model.update(
               sModel, 'accounts', bitmunk.user.getUserId(), accounts, 0);
            
            // schedule account update poll events
            sIntervalId = setInterval(
               function()
               {
                  bitmunk.events.addDaemonEvents(
                  {
                     add: [{
                        event: {
                           type: 'bitmunk.common.User.accountUpdatePoll',
                           details: {
                              userId: bitmunk.user.getUserId()
                           }
                        },
                        // dispatch 5 events on 2 minute intervals,
                        // totaling 10 minutes of events
                        interval: (2 * 60 * 1000),
                        count: 5
                     }],
                     success: function()
                     {
                        bitmunk.log.debug(sLogCategory,
                           'Scheduled account update poll events.');
                     }
                  });
               }, (10 * 60 * 1000) /*10 mins*/);
         });
      });
   };
   
   /**
    * Stops updating the accounts data model.
    * 
    * @param task the current task.
    */
   var stop = function(task)
   {
      clearInterval(sIntervalId);
      bitmunk.model.unregister(task, sModel);
   };
   
   /**
    * Called when an event that should trigger an account update is received.
    * 
    * @param event the event.
    * @param seqNum the sequence number assigned to this event update.
    */
   var updateAccounts = function(event, seqNum)
   {
      // get user's accounts
      bitmunk.user.getAccounts(function(accounts)
      {
         // insert accounts into table
         bitmunk.model.update(
            sModel, 'accounts', bitmunk.user.getUserId(), accounts, seqNum);
         
         // fire accounts updated event
         $.event.trigger('bitmunk-accounts-updated', [accounts]);
      });
   };
   
   bitmunk.model.addModel(sModel, {start: start, stop: stop});
   sScriptTask.unblock();
})(jQuery);

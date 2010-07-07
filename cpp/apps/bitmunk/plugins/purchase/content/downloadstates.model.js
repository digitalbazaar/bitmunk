/**
 * Bitmunk Download States Model
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 * 
 * @author Mike Johnson
 * @author Manu Sporny
 * @author Dave Longley
 */
(function($) {

var init = function(task)
{
   // category for logger
   var sLogCategory = 'downloadstates.model';
   
   // url to retrieve a download state
   var sDownloadStatesUrl =
      bitmunk.api.localRoot + 'purchase/contracts/downloadstates';
   
   // model and table names
   var sModelName = 'downloadStates';
   
   /**
    * FIXME: information may need to change?
    * 
    * A row in the download states table contains:
    * 
    * downloadStateId:
    * {
    *    downloadState: the download state object
    * }
    */
   
   /**
    * A row in the file progress table contains:
    * 
    * downloadStateId:
    * {
    *    files:
    *    {
    *       fileId:
    *       {
    *          size: total size of the file
    *          downloaded: total bytes in file downloaded
    *          eta: eta (in seconds)
    *          rate: total download rate for file
    *       }
    *    }
    *    size: total size of all files
    *    downloaded: total bytes of all files downloaded
    *    eta: eta (in seconds)
    *    rate: current download rate of all files
    * }
    * 
    * If the download state has no file progress yet, or does not exist, the
    * row will be null.
    */
   
   /**
    * A row in the feedback table contains:
    * 
    * downloadStateId:
    * {
    *    type: the feedback type
    *    message: the feedback message
    * }
    * 
    * If the download state has no feedback, or does not exist, the row will be
    * null.
    */
   
   /** Data Model Start & Stop **/
   
   /**
    * Starts updating the download states model.
    * 
    * @param task the current task.
    */
   var start = function(task)
   {
      // define event filter for user ID
      var userIdFilter = {
         details: {
            userId: bitmunk.user.getUserId()
         }
      };
      
      // register model
      bitmunk.model.register(task,
      {
         model: sModelName,
         tables: [
         {
            // create download states table
            name: 'downloadStates',
            eventGroups: [
            {
               events: [
                  'bitmunk.system.Directive.exception',
                  'bitmunk.eventreactor.EventReactor.exception',
                  'bitmunk.purchase.DownloadState.exception'
               ],
               filter: userIdFilter,
               eventCallback: downloadStateException
            },
            {
               events: [
                  'bitmunk.purchase.DownloadState.licenseAcquired',
                  'bitmunk.purchase.DownloadState.downloadStarted',
                  'bitmunk.purchase.DownloadState.downloadPaused',
                  'bitmunk.purchase.DownloadState.downloadCompleted',
                  'bitmunk.purchase.DownloadState.purchaseStarted',
                  'bitmunk.purchase.DownloadState.assemblyStarted',
                  'bitmunk.purchase.DownloadState.assemblyCompleted'
               ],
               filter: userIdFilter,
               eventCallback: downloadStateChanged
            },
            {
               events: [
                  'bitmunk.purchase.DownloadState.deleted'
               ],
               filter: userIdFilter,
               eventCallback: downloadStateDeleted
            }]
         },
         {
            // create file progress table
            name: 'fileProgress',
            eventGroups: [
            {
               events: [
                  'bitmunk.purchase.DownloadState.progressUpdate'
               ],
               filter: userIdFilter,
               eventCallback: fileProgressUpdate
            }],
            // combine similar events if these details are the same
            coalesceRules: [{
               details: {
                  downloadStateId: true,
                  userId: true
               }
            }]
         },
         {
            // create feedback table
            name: 'feedback',
            eventGroups: [
            {
               events: [
                  'bitmunk.purchase.DownloadState.deleted'
               ],
               filter: userIdFilter,
               eventCallback: feedbackDownloadStateDeleted
            }]
            // downloadStateChanged callback will call
            // feedbackAssmbledCompleted callback after new state is
            // retrieved.
         }]
      });
      
      // add task to start event reactor
      task.next(function(task)
      {
         // turn on the event reactor
         var userId = bitmunk.user.getUserId();
         var url = '/api/3.0/eventreactor/users/' +
            userId +
            '/downloadstate?nodeuser=' +
            userId;
         bitmunk.log.debug(sLogCategory, 'starting event reactor', userId);
         // FIXME: support tasks for btp calls
         task.block();
         bitmunk.api.proxy(
         {
            method: 'POST',
            url: url,
            success: function() { task.unblock(); },
            error: function() { task.fail(); }
         });
      });
      
      // add task to get download states
      task.next(function(task)
      {
         // get all download states
         task.block();
         bitmunk.api.proxy(
         {
            method: 'GET',
            url: sDownloadStatesUrl +
               '?incomplete=true&nodeuser=' + bitmunk.user.getUserId(),
            success: function(data, status)
            {
               //bitmunk.log.debug(sLogCategory,
               //   'Retrieved all download states.');
               
               // parse the download states
               task.userData = JSON.parse(data);
               
               // DEBUG: output
               //console.log('Download States: ' + data);
               
               task.unblock();
            },
            error: function()
            {
               // FIXME: fire failure event to UI?
               $.event.trigger(
                  'bitmunk-purchase-DownloadState-error',
                  [{
                     message: 'Failed to get all download states.'
                  }]);
               task.fail();
            }
         });
      });
      
      // add task to populate download table
      task.next(function(task)
      {
         // FIXME: correct return value from service so we don't have to
         // do downloadStates.downloadStates here
         var downloadStates = task.userData;
         for(var i = 0, length = downloadStates.downloadStates.length;
             i < length; i++)
         {
            // update the model with the download state
            var ds = downloadStates.downloadStates[i];
            
            // DEBUG: output
            //console.log('Download State: ' + JSON.stringify(ds), ds);
            
            bitmunk.model.update(
               sModelName, 'downloadStates', ds.id, ds, 0);
         }
      });
   };
   
   /** Table Registration Callbacks **/
   
   /**
    * Called when a download state exception occurs.
    * 
    * @event the download state event.
    * @seqNum the sequence number assigned to this event update.
    */
   var downloadStateException = function(event, seqNum)
   {
      bitmunk.log.warning(sLogCategory, 'Download Exception', event);
      
      // send event to UI to display exception
      $.event.trigger(event.type.replace(/\./g, '-'), [event.details]);
   };
   
   /**
    * Called when a download state update is received.
    * 
    * @event the download state event.
    * @seqNum the sequence number assigned to this event update.
    */
   var downloadStateChanged = function(event, seqNum)
   {
      // get download state ID
      var dsId = event.details.downloadStateId;
      
      // If this is a licenseAcquired event then the download state needs to be
      // fully initialized.  Otherwise, we can improve performance by only
      // requesting a partial state update.
      var detailLevel =
         (event.type == 'bitmunk.purchase.DownloadState.licenseAcquired') ?
         'all' : 'state';
      
      // get download state
      bitmunk.api.proxy(
      {
         method: 'GET',
         url: sDownloadStatesUrl + '/' + dsId +
            '?nodeuser=' + bitmunk.user.getUserId() +
            '&details=' + detailLevel,
         success: function(data)
         {
            // parse the download state
            var dsUpdates = JSON.parse(data);
            
            // get the current state data
            var ds = bitmunk.model.fetch(
               sModelName, 'downloadStates', dsId);
            
            // deep update of any download state changes
            // FIXME: Need a proper sync mechanism.  For complex data
            // structures this could leave the data out of sync.  (Ie, removed
            // object properties are not accounted for, the backend is only
            // sending explicit partial data rather than a proper diff)
            // See also ContractService::getDownloadState() 
            ds = $.extend(true, ds, dsUpdates);
            
            // update the model
            var updated = bitmunk.model.update(
               sModelName, 'downloadStates', dsId, ds, seqNum);
            
            // DEBUG: output
            //bitmunk.log.debug(sLogCategory,
            //   'Triggering event: ' + event.type.replace(/\./g, '-'), ds.id);
            
            if(event.type == 'bitmunk.purchase.DownloadState.assemblyCompleted')
            {
               // create assembly feedback
               feedbackAssemblyCompleted(ds, seqNum);
            }
            
            // if model was updated, send event to UI
            if(updated)
            {
               $.event.trigger(event.type.replace(/\./g, '-'), [ds]);
            }
         },
         error: function(data)
         {
            // FIXME: fire failure event to UI?
            $.event.trigger(
               'bitmunk-purchase-DownloadState-error',
               [{
                  downloadStateId: dsId,
                  message: 'Failed to get download state.'
               }]);
         }
      });
   };
   
   /**
    * Called when a download state is deleted.
    * 
    * @event the download state event.
    * @seqNum the sequence number assigned to this event update.
    */
   var downloadStateDeleted = function(event, seqNum)
   {
      // get download state ID
      var dsId = event.details.downloadStateId;
      
      // DEBUG: output
      //bitmunk.log.debug(sLogCategory, 'Removing download: ' + dsId);
      
      // remove download state from model
      bitmunk.model.remove(sModelName, 'downloadStates', dsId);
      
      // send event to UI to remove download state
      $.event.trigger(event.type.replace(/\./g, '-'), [dsId]);
   };
   
   /**
    * Called when a file progress update is received.
    * 
    * @event the file progress event.
    * @seqNum the sequence number assigned to this event update.
    */
   var fileProgressUpdate = function(event, seqNum)
   {
      // get download state ID
      var dsId = event.details.downloadStateId;
      
      // DEBUG: output
      //bitmunk.log.debug(sLogCategory, 'Updating progress: ' + dsId);
      
      // update the model
      bitmunk.model.update(
         sModelName, 'fileProgress', dsId, event.details, seqNum);
      
      // send dsId and progress row data to UI
      $.event.trigger(event.type.replace(/\./g, '-'), [dsId, event.details]);
   };
   
   /**
    * Add feedback when file assembly is complete.
    * 
    * @ds the download state.
    * @seqNum the sequence number assigned to this event update.
    */
   var feedbackAssemblyCompleted = function(ds, seqNum)
   {
      // get download path
      var path;
      if(ds.ware.fileInfos.length == 1)
      {
         // use full path when only one file present 
         path = ds.progress[ds.ware.fileInfos[0].id].path;
      }
      else
      {
         // use directory of first entry if more than one files present 
         path = ds.progress[ds.ware.fileInfos[0].id].directory;
      }
      
      // get feedback
      var feedback = bitmunk.model.fetch(
         sModelName, 'feedback', ds.id);
      // create if needed
      if(feedback === null)
      {
         feedback = [];
      }
      
      // add new entry
      var newFeedback =
      {
         type: 'info',
         message: 'Your download is available at <tt>${path|escape}</tt>'.
            process({path:path})
      };
      feedback.unshift(newFeedback);
      
      // update model
      bitmunk.model.update(
         sModelName, 'feedback', ds.id, feedback, seqNum);
      
      // send dsId and feedback row data to UI
      $.event.trigger(
         'bitmunk-purchase-DownloadState-feedbackUpdated',
         [ds.id, newFeedback]);
   };
      
   /**
    * Called when a download state is deleted.
    * 
    * @event the download state event.
    * @seqNum the sequence number assigned to this event update.
    */
   var feedbackDownloadStateDeleted = function(event, seqNum)
   {
      // get download state ID
      var dsId = event.details.downloadStateId;
      
      // remove feedback from model
      bitmunk.model.remove(sModelName, 'feedback', dsId);
      
      // send event to UI to remove feedback state
      $.event.trigger(
         'bitmunk-purchase-DownloadState-feedbackDeleted', [dsId]);
   };
   
   bitmunk.model.addModel(sModelName, {start: start});
};

bitmunk.resource.registerScript({
   pluginId: 'bitmunk.webui.Purchase',
   resourceId: 'downloadstates.model.js',
   depends: {},
   init: init
});

})(jQuery);

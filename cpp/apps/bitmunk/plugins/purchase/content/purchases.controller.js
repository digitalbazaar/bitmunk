/**
 * Bitmunk Purchases Controller
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 * 
 * Right now this controls the media library and the catalog. In the future
 * we might split these two apart if we decide the catalog should be an
 * optional feature that adds on to the Media Library.
 * 
 * @author Mike Johnson
 * @author Manu Sporny
 */
(function($)
{
   // log category
   var sLogCategory = 'purchases.controller';

   var sScriptTask = bitmunk.resource.getScriptTask(
      'bitmunk.webui.Purchase', 'purchases.controller.js');
   
   // URL to post a download action
   var sDownloadActionUrl = bitmunk.api.localRoot + 'purchase/contracts/';
   
   // model names
   var sAccountsModel = 'accounts';
   var sDownloadStatesModel = 'downloadStates';
   
   // Purchases Controller namespace
   bitmunk.purchases = bitmunk.purchases || {};
   bitmunk.purchases.controller = 
   {
      /**
       * Data loading methods
       * ====================
       */
      loadPurchases: function(task)
      {
         // clear view
         task.next('Purchases Prepare UI', function(task)
         {
            // DEBUG: output
            bitmunk.log.debug(sLogCategory, 'Prepare UI');
            
            // clear purchases
            bitmunk.purchases.view.clearPurchaseRows();
            
            // FIXME: show loading indicator
            //bitmunk.purchases.view.showLoading(true);
         });
         
         // add purchase rows
         task.next('Purchases Add Rows', function(task)
         {
            // TODO: add slice here for paginated purchases table
            
            // get all download states from model
            var states = bitmunk.model.fetchAll(
               sDownloadStatesModel, 'downloadStates');
            
            // add each row
            $.each(states, function(index, ds)
            {
               // DEBUG: output
               bitmunk.log.debug(sLogCategory, 'Add Row: ' + ds.id);
               
               // FIXME: get feedback to display in the row
               
               if(ds.licenseAcquired)
               {
                  // add the purchase row to view
                  bitmunk.purchases.view.addPurchaseRow(ds);
               }
               else
               {
                  // add a loading row to view
                  bitmunk.purchases.view.addLoadingRow(ds.id);
               }
            });
            
            // TODO: add page control update here
         });
      },
      
      /**
       * Starts the specified download.
       * 
       * @param options:
       *    dsId: the download state ID of the purchase to start.
       *    accountId: the ID of the account to use for the purchase.
       *    price: the maximum amount to spend on the download.
       *    fast: true if download speed is preferred over price.
       *    sellers: the maximum number of sellers to download from at once.
       * 
       * @return true if successful, false if not.
       */
      startPurchase: function(options)
      {
         var rval = false;
         
         // set defaults
         options = $.extend(options,
         {
            fast: true,
            sellers: 10
         });
         
         var accounts = bitmunk.model.fetch(
            sAccountsModel, 'accounts', bitmunk.user.getUserId());
         if(!(options.accountId in accounts))
         {
            // display warning about wrong account/no account selected
            bitmunk.purchases.view.addPurchaseFeedback(
               options.dsId, 'warning', 'Please choose an account.');
         }
         else if(+accounts[options.accountId].balance < +options.price)
         {
            // display warning about insufficient funds
            bitmunk.purchases.view.addPurchaseFeedback(
               options.dsId, 'warning',
               'Please choose an account with enough money to ' +
               'complete your purchase.');
         }
         else
         {
            rval = true;
            
            // create the buyer's purchase preferences
            var prefs =
            {
               downloadStateId: options.dsId,
               preferences:
               {
                  accountId: options.accountId,
                  price: {max: options.price},
                  fast: options.fast,
                  sellerLimit: options.sellers
               }
            };
            
            // start the download
            downloadAction(
               options.dsId, 'download', 'Failed to start download.', prefs);
         }
         
         return rval;
      },
      
      /**
       * Pauses the specified download.
       * 
       * @param dsId the download state ID of the purchase to pause.
       */
      pausePurchase: function(dsId)
      {
         // DEBUG: output
         bitmunk.log.debug(sLogCategory, 'Pause Purchase: ' + dsId);
         
         // pause the download
         downloadAction(dsId, 'pause', 'Failed to pause download.',
            null, function() { bitmunk.purchases.view.pausePurchase(dsId); });
      },
      
      /**
       * Resumes the specified download.
       * 
       * @param dsId the download state ID of the purchase to resume.
       */
      resumePurchase: function(dsId)
      {
         // DEBUG: output
         bitmunk.log.debug(sLogCategory, 'Resume Purchase: ' + dsId);
         
         // resume the download
         downloadAction(dsId, 'download', 'Failed to resume download.');
      },
      
      /**
       * Removes the specified download.
       * 
       * @param dsId the download state ID of the purchase to remove.
       */
      removePurchase: function(dsId)
      {
         // DEBUG: output
         bitmunk.log.debug(sLogCategory, 'Remove Purchase: ' + dsId);
         
         // remove the download
         downloadAction(
            dsId, 'downloadstates/delete',
            'Failed to delete download state.');
      },
      
      /**
       * Loads information about the specified download state into the
       * purchase dialog.
       * 
       * @param dsId the download state ID.
       */
      loadPurchaseDialog: function(dsId)
      {
         // DEBUG: output
         bitmunk.log.debug(sLogCategory, 'Load Dialog');
         
         // get download state, accounts and feedback
         var ds = bitmunk.model.fetch(
            sDownloadStatesModel, 'downloadStates', dsId);
         var accounts = bitmunk.model.fetch(
            sAccountsModel, 'accounts', bitmunk.user.getUserId());
         var feedback = bitmunk.model.fetch(
            sDownloadStatesModel, 'feedback', ds.id);
         
         // update view purchase dialog box
         bitmunk.purchases.view.setPurchaseDialog(ds, accounts, feedback);
         
         // show purchase dialog
         bitmunk.purchases.view.showPurchaseDialog(true);
      }
   };
   
   
   /**
    * Event Callbacks
    * ===============
    */
   
   /**
    * Event callback when a backend (Directive/Event Reactor) exception happens.
    * 
    * @param event the event that occurred.
    */
   var backendException = function(event)
   {
      // DEBUG: output
      bitmunk.log.debug(sLogCategory, 'General exception: ', event);
      
      // FIXME: put correct exception message here
      var message = 'A general exception has occurred.';
      
      // FIXME: use client-wide general feedback
      
      // add general feedback message
      //bitmunk.main.addGeneralFeedback('error', message);
   };
   
   /**
    * Event callback when a download exception occurs.
    * 
    * @param event the event that occurred.
    * @param details the details of the download exception.
    */
   var downloadException = function(event, details)
   {
      // DEBUG: output
      bitmunk.log.debug(sLogCategory, 'Download exception: ', details);
      
      // get the dsId from the exception details
      var dsId = details.downloadStateId;
      
      if(details.exception.type !=
         'bitmunk.purchase.ContractService.DownloadNotInProgress')
      {
         var message = details.exception.message;
         
         // add download feedback message to view
         bitmunk.purchases.view.addPurchaseFeedback(dsId, 'error', message);
      }
      else
      {
         // get download state
         var ds = bitmunk.model.fetch(
            sDownloadStatesModel, 'downloadStates', dsId);
         
         // check to see if download has not completed
         if(ds.remainingPieces != 0)
         {
            // re-enable the pause button
            bitmunk.purchases.view.resumePurchase(dsId);
         }
      }
   };
   
   /**
    * Event callback when download state license is acquired.
    * 
    * @param event the event that occurred.
    * @param ds the download state.
    */
   var licenseAcquired = function(event, ds)
   {
      // DEBUG: output
      bitmunk.log.debug(sLogCategory, 'License acquired: ' + ds.id);
      
      // add purchase row (automatically removes loading row)
      bitmunk.purchases.view.addPurchaseRow(ds);
   };
   
   /**
    * Event callback when download begins.
    * 
    * @param event the event that occurred.
    * @param ds the download state.
    */
   var downloadStarted = function(event, ds)
   {
      // DEBUG: output
      bitmunk.log.debug(sLogCategory, 'Starting download: ' + ds.id);
      
      // set purchase downloading
      bitmunk.purchases.view.resumePurchase(ds.id);
   };
   
   /**
    * Event callback when download is paused.
    * 
    * @param event the event that occurred.
    * @param ds the download state.
    */
   var downloadPaused = function(event, ds)
   {
      // DEBUG: output
      bitmunk.log.debug(sLogCategory, 'Paused: ' + ds.id);
      
      // pause the purchase
      bitmunk.purchases.view.pausePurchase(ds.id);
   };
   
   /**
    * Event callback when download completes.
    * 
    * @param event the event that occurred.
    * @param ds the download state.
    */
   var downloadCompleted = function(event, ds)
   {
      // DEBUG: output
      bitmunk.log.debug(sLogCategory, 'Download complete: ' + ds.id);

      // set download complete
      bitmunk.purchases.view.downloadComplete(ds.id);
   };
   
   /**
    * Event callback when purchase begins.
    * 
    * @param event the event that occurred.
    * @param ds the download state.
    */
   var purchaseStarted = function(event, ds)
   {
      // DEBUG: output
      bitmunk.log.debug(sLogCategory, 'Purchasing: ' + ds.id);

      // set purchase started
      bitmunk.purchases.view.setPurchaseStatus(ds.id, 'Purchasing', 'busy');
   };
   
   /**
    * Event callback when assembly begins.
    * 
    * @param event the event that occurred.
    * @param ds the download state.
    */
   var assemblyStarted = function(event, ds)
   {
      // DEBUG: output
      bitmunk.log.debug(sLogCategory, 'Assembling: ' + ds.id);

      // set assembly started
      bitmunk.purchases.view.setPurchaseStatus(ds.id, 'Assembling', 'busy');
   };
   
   /**
    * Event callback when assembly completes.
    * 
    * @param event the event that occurred.
    * @param ds the download state.
    */
   var assemblyCompleted = function(event, ds)
   {
      // DEBUG: output
      bitmunk.log.debug(sLogCategory, 'Assembly complete: ', ds);
      
      // set assembly complete
      bitmunk.purchases.view.purchaseComplete(ds.id);
   };
   
   /**
    * Event callback when a download was deleted.
    * 
    * @param event the event that occurred.
    * @param dsId the ID of the download state that was deleted.
    */
   var downloadDeleted = function(event, dsId)
   {
      // DEBUG: output
      bitmunk.log.debug(sLogCategory, 'Download deleted: ' + dsId);
      
      // remove the purchase row
      bitmunk.purchases.view.removePurchaseRow(dsId);
   };
   
   /**
    * Event callback when a progressUpdate or pieceFinished event occurs.
    * 
    * @param event the event that occurred.
    * @param dsId the ID of the download state that was updated.
    * @param the progress information for the download that was updated.
    */
   var progressUpdated = function(event, dsId, progress)
   {
      // DEBUG: output
      bitmunk.log.debug(sLogCategory, 'Progress updated: ', progress);
      
      // get received percentage for the whole download
      var percentage =
         ((progress.downloaded / progress.size) * 100).toFixed(0);
      
      // calculate eta:
      var eta = '';
      
      // days
      if(progress.eta >= (60 * 60 * 24))
      {
         var days = Math.floor(progress.eta / (60 * 60 * 24));
         progress.eta -= (days * 60 * 60 * 24);
         eta += days + 'd ';
      }
      // hours
      if(progress.eta >= (60 * 60))
      {
         var hours = Math.floor(progress.eta / (60 * 60));
         progress.eta -= (hours * 60 * 60);
         eta += hours + 'h ';
      }
      // minutes
      if(progress.eta >= 60)
      {
         var mins = Math.floor(progress.eta / 60);
         progress.eta -= (mins * 60);
         eta += mins + 'm ';
      }
      
      // seconds
      eta += progress.eta + 's';
      
      // set rate
      var rate = (progress.rate / 1024.0).toFixed(2) + ' KiB/s';
      
      // set size
      var downloaded = (progress.downloaded / 1024.0).toFixed(0) + '/' +
      (progress.size / 1024.0).toFixed(0) + ' KiB';
      
      // FIXME: might be more than 1 file (how do we handle this?)
      var pieces = [];
      // FIXME: offset is for silly display of all pieces for all files together
      var offset = 0;
      $.each(progress.files, function(fileId, fp)
      {
         // initialize to zero
         for(var i = 0; i < fp.pieces.count; i++)
         {
            pieces.push(0);
         }
         
         // set assigned values
         $.each(fp.pieces.assigned, function(i, piece)
         {
            pieces[offset + piece.index] = (piece.downloaded === 0) ?
               0 : Math.min(1, piece.downloaded / fp.pieces.size);
         });
         
         // set downloaded values
         $.each(fp.pieces.downloaded, function(i, index)
         {
            pieces[offset + index] = 1;
         });
         
         offset += fp.pieces.count;
      });
      
      var purchaseProgress = {
         percentage: percentage,
         eta: eta,
         rate: rate,
         // FIXME: handle more than 1 file
         pieces: pieces,
         downloaded: downloaded
      };
      
      // set purchase status and progress
      bitmunk.purchases.view.setPurchaseStatus(
         dsId, 'Downloading', 'download');
      bitmunk.purchases.view.setPurchaseProgress(dsId, purchaseProgress);
   };
   
   /**
    * Callback when a feedbackUpdated event occurs.
    * 
    * @param event the event that occurred.
    * @param dsId the ID of the download state that was updated.
    * @param the feedback information for the download that was updated.
    */
   var feedbackUpdated = function(event, dsId, feedback)
   {
      // FIXME: right now the feedback is added to the view and then removed
      // from the model. this could be a problem if you navigate to a different
      // page and back, because the feedback would be cleared.
      
      // FIXME: we need to keep the feedback stored in the model, and remove
      // it only when the user clears it in the interface
      
      // add purchase feedback message
      bitmunk.purchases.view.addPurchaseFeedback(
         dsId, feedback.type, feedback.message);
      
      // remove feedback from model
      bitmunk.model.remove(sDownloadStatesModel, 'feedback', dsId);
   };
   
   /**
    * Called when a N bitmunk directives have been queued.
    * 
    * @param event the event or null if called on startup.
    */
   var directivesQueued = function(event)
   {
      // TODO: decide to only clear the queue if it's a purchase directive
      // as we might have different directives in the future
      
      // get directive queue element
      var element = event ? event.target : $('#bitmunk-directive-queue')[0];
      if(element)
      {
         // save the old queue and clear it
         var json = element.innerHTML;
         element.innerHTML = '';
         try
         {
            // parse and handle all directives in the old queue
            var queue = JSON.parse(json);
            $.each(queue, function(i, directive)
            {
               // post directive to the backend, if successful, we'll receive
               // an event...if not, the default handler will catch the error
               bitmunk.api.proxy(
               {
                  method: 'POST',
                  url: bitmunk.api.root + 'system/directives',
                  params: { nodeuser: bitmunk.user.getUserId() },
                  data: directive
               });
            });
         }
         catch(e)
         {
            bitmunk.log.debug(
               sLogCategory, 'Exception while processing queued directives', e);
         }
      }
   };
   
   /**
    * Download Action
    * ===============
    */
   
   /**
    * This method will post a download state ID via BTP to the
    * 'purchase/contracts/<action>' URL over the local proxy.
    * 
    * This method assumes that if the post is successful, an event will be
    * received and handled regarding whatever action was taken. The download
    * state ID from that event may, in turn, be passed to this method with a
    * new url to take a new action.
    * 
    * This method saves a lot of repetitive code because each action that
    * can be taken on a download state uses the same post data and practically
    * the same URL.
    * 
    * @param dsId the download state ID to use.
    * @param action the action name to append to the 'purchase/contracts' url.
    * @param errorMsg an error message to display if the post fails.
    * @param data an optional set of data to override the post data.
    * @param callback an optional callback to call when successful.
    */
   var downloadAction = function(dsId, action, errorMsg, data, callback)
   {
      // DEBUG: output
      bitmunk.log.debug(sLogCategory,
         'Download Action: "' + action + '" ID: ' + dsId);
      
      // post via BTP
      bitmunk.api.proxy(
      {
         method: 'POST',
         url: sDownloadActionUrl + action,
         params: { nodeuser: bitmunk.user.getUserId() },
         data: (!data) ? { downloadStateId: dsId } : data,
         success: function()
         {
            // run callback if it is available
            if(callback)
            {
               callback();
            }
         },
         error: function(xhr, textStatus, errorThrown)
         {
            var message = 'There was an error while processing this download.';
            if(errorMsg)
            {
               message = errorMsg;
            }
            
            // add feedback message to view
            bitmunk.purchases.view.addPurchaseFeedback(
               dsId, 'error', errorMsg);
         }
      });
   };
   
   
   /** Bitmunk Framework **/
   
   /**
    * The show function is called whenever the page is shown. 
    */
   var show = function(task)
   {
      // show the view
      bitmunk.purchases.view.show();
      
      // bind to exception events
      $(window).bind(
         'bitmunk-system-Directive-exception',
         backendException);
      $(window).bind(
         'bitmunk-eventreactor-EventReactor-exception',
         backendException);
      $(window).bind(
         'bitmunk-purchase-DownloadState-exception',
         downloadException);
      
      // bind to download state events
      $(window).bind(
         'bitmunk-purchase-DownloadState-licenseAcquired',
         licenseAcquired);
      $(window).bind(
         'bitmunk-purchase-DownloadState-downloadStarted',
         downloadStarted);
      $(window).bind(
         'bitmunk-purchase-DownloadState-downloadPaused',
         downloadPaused);
      $(window).bind(
         'bitmunk-purchase-DownloadState-downloadCompleted',
         downloadCompleted);
      $(window).bind(
         'bitmunk-purchase-DownloadState-purchaseStarted',
         purchaseStarted);
      $(window).bind(
         'bitmunk-purchase-DownloadState-assemblyStarted',
         assemblyStarted);
      $(window).bind(
         'bitmunk-purchase-DownloadState-assemblyCompleted',
         assemblyCompleted);
      $(window).bind(
         'bitmunk-purchase-DownloadState-deleted',
         downloadDeleted);
      
      // bind to progress updates
      $(window).bind(
         'bitmunk-purchase-DownloadState-progressUpdate',
         progressUpdated);
      $(window).bind(
         'bitmunk-purchase-DownloadState-pieceFinished',
         progressUpdated);
      
      // bind to feedback updates
      $(window).bind(
         'bitmunk-purchase-DownloadState-feedbackUpdated',
         feedbackUpdated);
      
      // load any already queued purchase directives
      directivesQueued(null);
      
      // bind to handle directive events
      $(window).bind('bitmunk-directives-queued', directivesQueued);
      $(window).bind('bitmunk-directive-started', function(event)
      {
         // FIXME: set notification
         bitmunk.log.debug(sLogCategory, 'directive downloading');
      });
      $(window).bind('bitmunk-directive-error', function(event)
      {
         var e = event.target;
         if(e.hasAttribute('error'))
         {
            var msg = e.getAttribute('error');
            e.removeAttribute('error');
            
            // FIXME: set notification
            bitmunk.log.debug(sLogCategory, 'directive error: ' + msg);
         }
      });
      
      // FIXME: update view with download state information
      
      // load purchases
      task.next('Purchases', bitmunk.purchases.controller.loadPurchases);
      
      // DEBUG: output
      bitmunk.log.debug(sLogCategory, 'Show Complete');
   };
   
   /**
    * The hide function is called whenever the page is hidden.
    */
   var hide = function(task)
   {
      // hide the view
      bitmunk.purchases.view.hide();
      
      // FIXME: clear models?
      
      // unbind from exceptions
      $(window).unbind('bitmunk-system-Directive-exception');
      $(window).unbind('bitmunk-eventreactor-EventReactor-exception');
      $(window).unbind('bitmunk-purchase-DownloadState-exception');
      
      // unbind from download state events
      $(window).unbind('bitmunk-purchase-DownloadState-licenseAcquired');
      $(window).unbind('bitmunk-purchase-DownloadState-downloadStarted');
      $(window).unbind('bitmunk-purchase-DownloadState-downloadPaused');
      $(window).unbind('bitmunk-purchase-DownloadState-downloadCompleted');
      $(window).unbind('bitmunk-purchase-DownloadState-purchaseStarted');
      $(window).unbind('bitmunk-purchase-DownloadState-assemblyStarted');
      $(window).unbind('bitmunk-purchase-DownloadState-assemblyCompleted');
      $(window).unbind('bitmunk-purchase-DownloadState-deleted');
      
      // unbind from progress updates
      $(window).unbind('bitmunk-purchase-DownloadState-progressUpdate');
      $(window).unbind('bitmunk-purchase-DownloadState-pieceFinished');
      
      // unbind from feedback updates
      $(window).unbind('bitmunk-purchase-DownloadState-feedbackUpdated');
      
      // unbind from directive events
      $(window).unbind('bitmunk-directives-queued');
      $(window).unbind('bitmunk-directive-started');
      $(window).unbind('bitmunk-directive-error');
   };

   bitmunk.resource.setupResource(
   {
      pluginId: 'bitmunk.webui.Purchase',
      resourceId: 'purchases',
      models: [sAccountsModel, sDownloadStatesModel],
      show: show,
      hide: hide
   });
   
   sScriptTask.unblock();
})(jQuery);

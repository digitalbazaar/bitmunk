/**
 * Bitmunk Purchases View
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 * 
 * @author Mike Johnson
 * @author Manu Sporny
 * @author Dave Longley
 */
(function($)
{
   // log category
   var sLogCategory = 'purchases.view';
   
   var sScriptTask = bitmunk.resource.getScriptTask(
      'bitmunk.webui.Purchase', 'purchases.view.js');
   
   // Main Content Image URL
   var sContentImageUrl = '/bitmunk/images/';
   
   // Plugin Image URL
   var sPluginImageUrl = '/bitmunk/plugins/bitmunk.webui.Purchase/images/';
   
   // stores whether or not the purchase dialog is shown
   var mPurchaseDialogShown = false;
   
   // Purchases View namespace
   bitmunk.purchases.view = {};
   
   /**
    * Purchase Widget Methods
    * =======================
    */
   
   /**
    * Adds a temporary purchase row that just displays the loading
    * indicator.
    * 
    * @param dsId the ID of the download state that is being loaded.
    */
   bitmunk.purchases.view.addLoadingRow = function(dsId)
   {
      // DEBUG: output
      bitmunk.log.debug(sLogCategory, 'Add Loading Row: '+ dsId);
      
      // get loading row template
      var template = bitmunk.resource.get(
         'bitmunk.webui.Purchase', 'loadingRow.html', true);
      
      // create loading row values
      var values =
      {
         dsId: dsId
      };
      
      // create loading row template
      var loadingRow = $(template.process(values));
      loadingRow.data('dsId', dsId);
      
      // add loading row
      $('#purchaseRows').append(loadingRow);
      
      // hide empty row
      $('#purchaseRows .emptyRow').hide();
   };
   
   /**
    * Adds a purchase row using the specified download state.
    * 
    * @param ds the download state.
    */
   bitmunk.purchases.view.addPurchaseRow = function(ds)
   {
      // DEBUG: output
      bitmunk.log.debug(sLogCategory, 'Add Purchase Row: ' + ds.id);
      
      // TODO: check if media is a collection, load special template
      // get purchase row template
      var template = bitmunk.resource.get(
         'bitmunk.webui.Purchase', 'purchaseRow.html', true);
      
      // get media primary contributor
      var media = ds.contract.media;
      var primary = bitmunk.util.getPrimaryContributor(media);
      var contributor = primary ? primary.name : 'Not Available';
      
      // create template values
      var values =
      {
         dsId: ds.id,
         mediaId: media.id,
         title: media.title,
         contributor: contributor,
         typeImage:
            sContentImageUrl + 'media/' + media.type + '.png',
         typeName: media.type,
         price: (Math.ceil((+ds.totalMedPrice)*100)/100).toFixed(2)
      };
      
      // process template and set as loaded
      var purchaseRow = $(template.process(values));
      purchaseRow.data('dsId', ds.id);
      
      // replace or add purchase widget to DOM
      if($('#purchaseRow-' + ds.id).length > 0)
      {
         $('#purchaseRow-' + ds.id).replaceWith(purchaseRow);
      }
      else
      {
         $('#purchaseRows').append(purchaseRow);
      }
      
      // set status and remove button based on download state
      if(!ds.downloadStarted)
      {
         // set status
         bitmunk.purchases.view.setPurchaseStatus(
            ds.id, 'Ready To Buy', 'ready');
         
         // turn on remove button
         $('#removeButton-' + ds.id)
            .removeClass('off')
            .addClass('on')
            .one('click', ds.id, removeClicked);
      }
      else if(ds.downloadStarted && ds.remainingPieces > 0)
      {
         if(ds.downloadPaused)
         {
            // set status
            bitmunk.purchases.view.setPurchaseStatus(
               ds.id, 'Paused', 'paused');
         }
         else
         {
            // set status
            bitmunk.purchases.view.setPurchaseStatus(
               ds.id, 'Downloading', 'download');
         }
         
         // calculate download stats & progress
         var downloaded = 0;
         var total = 0;
         var pieces = [];
         var offset = 0;
         $.each(ds.progress, function(fid, fileProgress)
         {
            downloaded += fileProgress.bytesDownloaded;
            total += fileProgress.fileInfo.size;
            
            $.each(fileProgress.unassigned, function(i, fp)
            {
               pieces[offset + fp.index] = 0;
            });
            $.each(fileProgress.assigned, function(csHash, fpList)
            {
               $.each(fpList, function(i, fp)
               {
                  pieces[offset + fp.index] = 0;
               });
            });
            $.each(fileProgress.downloaded, function(csHash, fpList)
            {
               $.each(fpList, function(i, fp)
               {
                  pieces[offset + fp.index] = 1;
               });
            });
            
            offset = pieces.length;
         });
         
         var progress = {
            percentage: ((downloaded / total) * 100).toFixed(0),
            eta: '',
            rate: '0.0 KiB/s',
            pieces: pieces,
            downloaded: downloaded + '/' + total + ' KiB'
         };
         
         // set progress
         bitmunk.purchases.view.setPurchaseProgress(ds.id, progress);
         
         // turn on remove button
         $('#removeButton-' + ds.id)
            .removeClass('off')
            .addClass('on')
            .one('click', ds.id, removeClicked);
      }
      else if(ds.remainingPieces === 0)
      {
         // calculate download stats
         var total = 0;
         var pieces = [];
         var offset = 0;
         $.each(ds.progress, function(fid, fileProgress)
         {
            total += fileProgress.fileInfo.size;
            $.each(fileProgress.downloaded, function(csHash, fpList)
            {
               $.each(fpList, function(i, fp)
               {
                  pieces[offset + fp.index] = 1;
               });
            });
            
            offset = pieces.length;
         });
         
         var progress = {
            percentage: 100,
            eta: 'Complete',
            rate: '0.0 KiB/s',
            pieces: pieces,
            downloaded: total + '/' + total + ' KiB'
         };
         
         // set row progress bar
         bitmunk.purchases.view.setPurchaseProgress(ds.id, progress);
         
         if(!ds.licensePurchased || !ds.dataPurchased)
         {
            // set status
            bitmunk.purchases.view.setPurchaseStatus(
               ds.id, 'Purchasing', 'busy');
            
            // turn off remove button
            $('#removeButton-' + ds.id)
               .unbind('click')
               .removeClass('on')
               .addClass('off');
         }
         else if(ds.licensePurchased && ds.dataPurchased &&
            !ds.filesAssembled)
         {
            // set status
            bitmunk.purchases.view.setPurchaseStatus(
               ds.id, 'Assembling', 'busy');
            
            // turn off remove button
            $('#removeButton-' + ds.id)
               .unbind('click')
               .removeClass('on')
               .addClass('off');
         }
         else
         {
            // set status
            bitmunk.purchases.view.setPurchaseStatus(
               ds.id, 'Complete', 'ok');
            
            // turn on remove button
            $('#removeButton-' + ds.id)
               .removeClass('off')
               .addClass('on')
               .one('click', ds.id, removeClicked);
            
            // set remove button title
            $('#removeButton-' + ds.id)
               .attr('title', 'Remove completed purchase');
         }
      }
      
      // bind dialog open click to every column except selectCol
      $('#purchaseRow-' + ds.id + ' td:not(.removeCol)').bind(
         'click', ds.id, dialogOpenClicked);
      
      // FIXME: add feedback icon if there is feedback
      
      // hide empty row
      $('#purchaseRows .emptyRow').hide();
   };
   
   /**
    * Removes a purchase row identified by dsId.
    * 
    * @param dsId the download state ID of the purchase row to remove.
    */
   bitmunk.purchases.view.removePurchaseRow = function(dsId)
   {
      // DEBUG: output
      bitmunk.log.debug(sLogCategory, 'Remove Purchase Row: ' + dsId);
      
      // remove the purchase row
      $('#purchaseRow-' + dsId).remove();
      
      // if it was the last purchase row, display empty row
      if($('.purchaseRow').length === 0)
      {
         $('#purchaseRows .emptyRow').show();
      }
   };
   
   /**
    * Removes all purchase rows from the view.
    */
   bitmunk.purchases.view.clearPurchaseRows = function()
   {
      // remove all purchase rows
      $('.purchaseRow').remove();
      
      // FIXME: deselect purchase select
      $('#purchaseSelectAll').removeAttr('checked');
      
      // display empty row
      $('#purchaseRows .emptyRow').show();
   };
   
   /**
    * Updates the purchase row and dialog (if applicable) with the new
    * status of the download.
    * 
    * @param dsId the download state ID.
    * @param status the status of the download.
    * @param type the type of status message.
    */
   bitmunk.purchases.view.setPurchaseStatus = function(dsId, status, type)
   {
      // set default status type
      type = type ? type : 'ok';
      
      // set status in the purchase row
      $('#status-' + dsId)
         .text(status)
         .removeClass()
         .addClass('status-' + type);
      
      // if purchase is loaded in dialog update dialog status
      if(dsId == $('#purchaseDialog').data('dsId'))
      {
         // update dialog status
         $('#downloadStatus')
            .text(status)
            .removeClass()
            .addClass('status-' + type);
      }
   };
   
   /**
    * Updates the purchase row and dialog (if applicable) with the
    * progress of the download.
    * 
    * @param dsId the download state ID.
    * @param progress the progress of the download.
    */
   bitmunk.purchases.view.setPurchaseProgress = function(dsId, progress)
   {
      // set progress in the purchase row
      $('#progress-' + dsId).text(progress.percentage + '%');
      
      // if purchase is loaded in dialog update dialog progress
      if(dsId == $('#purchaseDialog').data('dsId'))
      {
         // update dialog file pieces
         if(!mPurchaseDialogShown)
         {
            $('#progressFill').html('Loading...');
         }
         
         // calculate bar width based on number of pieces
         // FIXME: probably doesn't work correctly if the number of pieces
         // is larger than the number of pixels the graph is wide.
         var barWidth = Math.floor(
            (500 - (progress.pieces.length - 1)) / progress.pieces.length);
         
         // FIXME: sparkline uses canvas to draw and current implementations
         // will not render anything while the canvas element is hidden, so
         // we set this to run on a timeout so that it will run after we
         // show everything (Note that "everything" includes elements that
         // are higher in the DOM hierarchy than elements that are shown
         // in this function so we can't simply move this call to the end)
         setTimeout(function()
         {
            $('#progressFill').sparkline(progress.pieces, {
               type: 'bar',
               height: 20,
               chartRangeMin: 0,
               chartRangeMax: 1,
               barColor: '#f6800a',
               zeroColor: '#888',
               barWidth: barWidth,
               barSpacing: 1,
               spaceColor: '#444',
               colorMap: { '1': '#f6800a' }
            });
         }, 0);
         
         // update dialog stats
         $('#downloadRate').text(progress.rate);
         $('#downloadBytes').text(progress.downloaded);
         $('#downloadEta').text(progress.eta);
         $('#downloadProgress').html(progress.percentage + '%');
         
         // show the statistics section
         $('#statisticsSection').show();
      }
   };
   
   /**
    * Sets the visibility of the purchase dialog.
    * 
    * @param show true to show the dialog, false to hide it.
    */
   bitmunk.purchases.view.showPurchaseDialog = function(show)
   {
      // toggle the overlay and purchase dialog
      $('#overlay').toggle(show);
      $('#purchaseDialog').toggle(show);
      mPurchaseDialogShown = show;
   };
   
   /**
    * Sets the fields in the purchase dialog box with information from the
    * specified download state.
    * 
    * @param ds the download state.
    * @param accounts the user's financial accounts.
    * @param feedback the feedback to display about this download state.
    */
   bitmunk.purchases.view.setPurchaseDialog = function(ds, accounts, feedback)
   {
      // DEBUG: output
      bitmunk.log.debug(sLogCategory, 'Set Purchase Dialog: ',
         ds, accounts, feedback);
      
      // set purchase dialog data
      $('#purchaseDialog').data('dsId', ds.id);
      
      // set media details in dialog
      setDialogMedia(ds);
      
      // set pool stats
      setPoolStats(ds);
      
      // set accounts
      setPurchaseAccounts(ds, accounts);
      
      // set purchase price
      var price = ds.preferences.price ? ds.preferences.price.max : 
         (Math.ceil((+ds.totalMedPrice)*100)/100).toFixed(2);
      $('#priceInput').attr('value', price);
      
      // set UI based on download state
      if(!ds.downloadStarted)
      {
         // turn on purchase button
         setButton('#purchaseButton', true, purchaseClicked, ds.id);
         
         // turn off remove button
         setButton('#removeButton', true, removeClicked, ds.id);
         
         // turn off pause and resume buttons
         setButton('#pauseButton', false);
         setButton('#resumeButton', false);
         
         // set status
         bitmunk.purchases.view.setPurchaseStatus(
            ds.id, 'Ready To Buy', 'ready');
         
         // enable buyer preferences
         bitmunk.purchases.view.enableBuyerPreferences(true);
         
         // hide statistics section
         $('#statisticsSection').hide();
      }
      else if(ds.downloadStarted && ds.remainingPieces > 0)
      {
         // turn off purchase button
         setButton('#purchaseButton', false);
         
         // turn on remove button
         setButton('#removeButton', true, removeClicked, ds.id);
         
         if(ds.downloadPaused)
         {
            // turn off pause button
            setButton('#pauseButton', false);
            
            // turn on resume button
            setButton('#resumeButton', true, resumeClicked, ds.id);
            
            // enable buyer's preferences
            bitmunk.purchases.view.enableBuyerPreferences(true);
            
            // set status
            bitmunk.purchases.view.setPurchaseStatus(
               ds.id, 'Paused', 'paused');
         }
         else
         {
            // turn off resume button
            setButton('#resumeButton', false);
            
            // turn on pause button
            setButton('#pauseButton', true, pauseClicked, ds.id);
            
            // disable buyer's preferences
            bitmunk.purchases.view.enableBuyerPreferences(true);
            
            // set status
            bitmunk.purchases.view.setPurchaseStatus(
               ds.id, 'Downloading', 'download');
         }
         
         // calculate download stats & progress
         var downloaded = 0;
         var total = 0;
         var pieces = [];
         var offset = 0;
         $.each(ds.progress, function(fileId, fileProgress)
         {
            downloaded += fileProgress.bytesDownloaded;
            total += fileProgress.fileInfo.size;
            
            $.each(fileProgress.unassigned, function(i, fp)
            {
               pieces[offset + fp.index] = 0;
            });
            $.each(fileProgress.assigned, function(csHash, fpList)
            {
               $.each(fpList, function(i, fp)
               {
                  pieces[offset + fp.index] = 1;
               });
            });
            $.each(fileProgress.downloaded, function(csHash, fpList)
            {
               $.each(fpList, function(i, fp)
               {
                  pieces[offset + fp.index] = 2;
               });
            });
            
            offset = pieces.length;
         });
         
         var percentage = ((downloaded / total) * 100).toFixed(0);
         
         downloaded = (downloaded / 1024.0).toFixed(0);
         total = (total / 1024.0).toFixed(0);
         
         var progress = {
            percentage: percentage,
            eta: '',
            rate: '0.0 KiB/s',
            pieces: pieces,
            downloaded: downloaded + '/' + total + ' KiB'
         };
         
         // set progress
         bitmunk.purchases.view.setPurchaseProgress(ds.id, progress);
      }
      else if(ds.remainingPieces === 0)
      {
         // turn off purchasebutton
         setButton('#purchaseButton', false);
         
         if(!ds.licensePurchased || !ds.dataPurchased)
         {
            // set status
            bitmunk.purchases.view.setPurchaseStatus(
               ds.id, 'Purchasing', 'busy');
            
            // turn off remove button
            setButton('#removeButton', false);
         }
         else if(ds.licensePurchased && ds.dataPurchased &&
            !ds.filesAssembled)
         {
            // set status
            bitmunk.purchases.view.setPurchaseStatus(
               ds.id, 'Assembling', 'busy');
            
            // turn off remove button
            setButton('#removeButton', false);
         }
         else
         {
            // set status
            bitmunk.purchases.view.setPurchaseStatus(ds.id, 'Complete');
            
            // turn on remove button
            setButton('#removeButton', true, removeClicked, ds.id);
            
            // set remove button text and title
            $('#removeButton')
               .text('Remove')
               .attr('title', 'Remove completed purchase');
         }
         
         // disable buyer preferences
         bitmunk.purchases.view.enableBuyerPreferences(false);
         
         // calculate download stats
         var downloaded = 0;
         var total = 0;
         var pieces = [];
         var offset = 0;
         $.each(ds.progress, function(fileId, fileProgress)
         {
            total += fileProgress.fileInfo.size;
            $.each(fileProgress.downloaded, function(csHash, fpList)
            {
               $.each(fpList, function(i, fp)
               {
                  pieces[offset + fp.index] = 2;
               });
            });
            
            offset = pieces.length;
         });
         total = (total / 1024.0).toFixed(0);
         
         var progress = {
            percentage: 100,
            eta: 'Complete',
            rate: '0.0 KiB/s',
            pieces: pieces,
            downloaded: total + '/' + total + ' KiB'
         };
         
         // set progress
         bitmunk.purchases.view.setPurchaseProgress(ds.id, progress);
      }
      
      // bind feedback clear button
      $('#clearButton')
         .unbind('click')
         .bind('click', ds.id, clearFeedbackClicked);
      
      // clear feedback
      bitmunk.purchases.view.clearPurchaseFeedback(ds.id);
      
      // check if feedback was included
      if(feedback)
      {
         // add feedback messages
         $.each(feedback, function(i, fb)
         {
            bitmunk.purchases.view.addPurchaseFeedback(
               ds.id, fb.type, fb.message);
         });
      }
      
      // hide dialog loading indicator
      setActivityIndicator('#dialogActivity', false);
   };
   
   /**
    * Enables or disables the input fields for the buyer's preferences,
    * like the account selector and price input.
    * 
    * @param enable true to enable them, false to disable.
    */
   bitmunk.purchases.view.enableBuyerPreferences = function(enable)
   {
      if(enable)
      {
         // enable input
         $('#accountSelect').removeAttr('disabled');
         $('#priceInput').removeAttr('disabled');
      }
      else
      {
         // disable input
         $('#accountSelect').attr('disabled', 'disabled');
         $('#priceInput').attr('disabled', 'disabled');
      }
   };
   
   /**
    * Pauses a purchase by setting the status text and enabling/disabling
    * the pause and resume buttons.
    * 
    * @param dsId the download state ID.
    */
   bitmunk.purchases.view.pausePurchase = function(dsId)
   {
      // DEBUG: output
      bitmunk.log.debug(sLogCategory, 'Pause Purchase: ' + dsId);
      
      // set status
      bitmunk.purchases.view.setPurchaseStatus(dsId, 'Paused', 'paused');
      
      if($('#purchaseDialog').data('dsId') == dsId)
      {
         // toggle buttons and enable buyer preferences
         setButton('#pauseButton', false);
         setButton('#resumeButton', true, resumeClicked, dsId);
         bitmunk.purchases.view.enableBuyerPreferences(true);
      }
   };
   
   /**
    * Resumes a purchase by setting the status text and enabling/disabling
    * the pause and resume buttons.
    * 
    * @param dsId the download state ID.
    */
   bitmunk.purchases.view.resumePurchase = function(dsId)
   {
      // DEBUG: output
      bitmunk.log.debug(sLogCategory, 'Resume Purchase: ' + dsId);
      
      // set purchase row status
      bitmunk.purchases.view.setPurchaseStatus(dsId, 'Resuming', 'busy');
      
      if($('#purchaseDialog').data('dsId') == dsId)
      {
         // toggle buttons and disable buyer preferences
         setButton('#resumeButton', false);
         setButton('#pauseButton', true, pauseClicked, dsId);
         bitmunk.purchases.view.enableBuyerPreferences(false);
      }
   };
   
   /**
    * Marks a purchase as downloaded.
    * 
    * @param dsId the download state ID.
    */
   bitmunk.purchases.view.downloadComplete = function(dsId)
   {
      // DEBUG: output
      bitmunk.log.debug(sLogCategory, 'Download Complete: ' + dsId);
      
      // set status
      bitmunk.purchases.view.setPurchaseStatus(
         dsId, 'Downloaded', 'download');
      
      // turn off row remove button
      $('#removeButton-' + dsId)
         .unbind('click')
         .removeClass('on')
         .addClass('off');
      
      if($('#purchaseDialog').data('dsId') == dsId)
      {
         // turn off purchase and remove buttons
         setButton('#purchaseButton', false);
         setButton('#removeButton', false);
         
         // turn off pause and resume buttons
         setButton('#resumeButton', false);
         setButton('#pauseButton', false);
      }
   };
   
   /**
    * Marks a purchase as complete.
    * 
    * @param dsId the download state ID.
    */
   bitmunk.purchases.view.purchaseComplete = function(dsId)
   {
      // DEBUG: output
      bitmunk.log.debug(sLogCategory, 'Purchase Complete: ' + dsId);

      // set status
      bitmunk.purchases.view.setPurchaseStatus(dsId, 'Complete', 'ok');
      
      // turn on row remove button
      $('#removeButton-' + dsId)
         .removeClass('off')
         .addClass('on')
         .bind('click', dsId, removeClicked)
         .attr('title', 'Remove completed purchase');
      
      if($('#purchaseDialog').data('dsId') == dsId)
      {
         // turn off purchase button
         setButton('#purchaseButton', false);
         
         // turn on remove button
         setButton('#removeButton', true, removeClicked, dsId);
         
         // set remove button text and title
         $('#removeButton')
            .text('Remove')
            .attr('title', 'Remove completed purchase');
         
         // turn off pause and resume buttons
         setButton('#resumeButton', false);
         setButton('#pauseButton', false);
      }
   };
   
   /**
    * Feedback Functions
    * ==================
    */
   
   /**
    * Adds a feedback messageand indicator for the specified download state.
    * 
    * @param dsId the download state ID.
    * @param type the type of message.
    * @param message the message to display.
    */
   bitmunk.purchases.view.addPurchaseFeedback = function(dsId, type, message)
   {
      // set feedback icon in purchase row
      var icon = $('<img src="/bitmunk/images/feedback-' + type +
         '-16.png" alt="" />');
      $('#feedback-' + dsId).html(icon);
      
      // if purchase is loaded in dialog add message to dialog
      if(dsId == $('#purchaseDialog').data('dsId'))
      {
         var feedback = $('<p class="' + type + '">' + message + '</p>');
         $('#feedback').prepend(feedback);
         
         // show feedback box
         $('#feedbackBox').show();
      }
   };
   
   /**
    * Clears all feedback messages for the specified download state.
    * 
    * @param dsId the download state ID.
    */
   bitmunk.purchases.view.clearPurchaseFeedback = function(dsId)
   {
      // remove feedback icon in purchase row
      $('#feedback-' + dsId).empty();
      
      // if purchase is loaded in dialog remove dialog feedback
      if(dsId == $('#purchaseDialog').data('dsId'))
      {
         // hide feedback box
         $('#feedbackBox').hide();
         
         // clear feedback
         $('#feedback').empty();
      }
   };
   
   /**
    * Bitmunk Framework Functions
    * ========================
    */
   
   /**
    * The show function is called whenever the page is shown. 
    */
   bitmunk.purchases.view.show = function(task)
   {
      // bind dialog close button
      $('#dialogClose').bind('click', dialogCloseClicked);
      
      // hide loading indicator
      setActivityIndicator('#purchasesActivity', false);
   };
   
   /**
    * The hide function is called whenever the page is hidden.
    */
   bitmunk.purchases.view.hide = function(task)
   {
      // clear purchase rows
      $('#purchaseRows .purchaseRow').remove();
   };
   
   /**
    * UI Manipulation Methods
    * =======================
    */
   
   /**
    * Updates the seller pool stats display in the purchase dialog.
    * 
    * @param ds the download state to display pool stats.
    */
   var setPoolStats = function(ds)
   {
      // DEBUG: output
      bitmunk.log.debug(sLogCategory, 'Set Pool Stats: ' + ds);
      
      // FIXME: add pool row for each available purchase
      //var rows = '';
      var sellers = 0;
      $.each(ds.progress, function(fileId, fp)
      {
         var pool = fp.sellerPool;
         sellers += pool.sellerDataSet.total;
         /*
         rows += '<tr><td>' + fp.fileInfo.mediaId +
            '</td><td>' +
            pool.sellerDataSet.total +
            '</td><td class="rightCol">$ ' +
            (Math.ceil((+pool.stats.minPrice)*100)/100).toFixed(2) +
            '</td><td class="rightCol">$ ' +
            (Math.ceil((+pool.stats.medPrice)*100)/100).toFixed(2) +
            '</td><td class="rightCol">$ ' +
            (Math.ceil((+pool.stats.maxPrice)*100)/100).toFixed(2) +
            '</td></tr>';
         */
      });
      
      //$('#pool-rows').html(rows);
      
      // set totals
      $('#pool-sellers').text(sellers);
      $('#pool-minTotal').text(
         '$ ' + (Math.ceil((+ds.totalMinPrice)*100)/100).toFixed(2));
      $('#pool-minTotal').attr('title', '$ ' + ds.totalMinPrice);
      $('#pool-medTotal').text(
         '$ ' + (Math.ceil((+ds.totalMedPrice)*100)/100).toFixed(2));
      $('#pool-medTotal').attr('title', '$ ' + ds.totalMedPrice);
      $('#pool-maxTotal').text(
         '$ ' + (Math.ceil((+ds.totalMaxPrice)*100)/100).toFixed(2));
      $('#pool-maxTotal').attr('title', '$ ' + ds.totalMaxPrice);
   };
   
   /**
    * This method updates the account selection box for the specified
    * download state.
    *
    * @param ds the download state to update with accounts.
    * @param accounts the user financial accounts.
    */
   var setPurchaseAccounts = function(ds, accounts)
   {
      // DEBUG: output
      bitmunk.log.debug(sLogCategory, 'Set Purchase Accounts: ' + ds.id);
      
      // check if accounts were provided
      if(accounts === null)
      {
         // disable account selector
         $('#accountSelect').attr('disabled', 'disabled');
         
         // add feedback item
         bitmunk.purchases.view.addPurchaseFeedback(
            ds.id, 'warning',
            'Your accounts are currently unavailable. Please try again.');
      }
      else
      {
         // enable account selector
         $('#accountSelect').removeAttr('disabled');
         
         // build options html from user accounts, select preferred account
         // if available
         var preferredAccountId = (ds.preferences.accountId) ? 
            ds.preferences.accountId : null;
         var optionsHtml = '';
         $.each(accounts, function(accountId, account)
         {
            // create option html, converting string to float
            optionsHtml += '<option value="' + accountId + '"' +
               (accountId == preferredAccountId ?
                  ' selected="selected">' : '>') +
               account.name + ' ($' + (+account.balance).toFixed(2) +
               ')</option>\n';
         });
         
         // FIXME: move this code to controller
         /*
         // set event handler to get updates to accounts
         accountBox.unbind('bitmunk-accounts-updated');
         accountBox.bind(
            'bitmunk-accounts-updated',
            function(event, accounts) { setPurchaseAccounts(ds.id, accounts); });
         */
         
         // clear & add options to account select
         $('#accountSelect').html(optionsHtml);
         
         // TODO: add support for "default" purchase account
         // if purchase preferences not available, use first account
         if(!preferredAccountId)
         {
            $('#accountSelect option:first').attr('selected', 'selected');
         }
      }
   };
   
   /**
    * Sets the media detail information in the purchase dialog.
    */
   var setDialogMedia = function(ds)
   {
      // get media from contract
      var media = ds.contract.media;
      
      // set dialog title
      $('#purchaseDialog-title').text('Purchase - ' + media.title);
      
      // set media title
      $('#media-title').text(media.title);
      
      // set media performer
      var primary = bitmunk.util.getPrimaryContributor(media);
      var contributor = primary ? primary.name : '';
      $('#media-contributor').text(contributor);
      
      // display all media contributors
      /*
      $.each(media.contributors, function(name, group)
      {
         $('#mediaContributors').append('<div class="detail"></div>');
         
         var detail = $('#mediaContributors .detail:last');
         detail.append('<span class="key">' + name + '</span>');
         
         $.each(group, function(position, contributor)
         {
            detail.append(
               '<span class="value">' + contributor.name + '</span>');
         });
      });
      */
      
      // set media image
      $('#media-link').attr('href',
         'http://bitmunk.com/media/' + media.id);
      $('#media-image').attr('src',
         bitmunk.api.bitmunkRoot + 'media/images/' + media.id + '?size=md');
      $('#media-image').attr('title', media.title);
      
      /*
      // set media type
      $('#mediaTypeImage').attr(
         'src', sContentImageUrl + 'media/' + media.type + '.png');
      $('#mediaTypeImage').attr('alt', media.type);
      $('#mediaTypeImage').attr('title', media.type);
      $('#mediaType').text(media.type);
      */
      
      // set media details
      //$('#mediaReleased').text(media.releaseDate);
      //$('#mediaFormat').text();
      //$('#mediaBitrate').text();
   };
   
   /**
    * Enables or disables the named button. If enabled, will bind the button
    * to the callback with the optional data.
    * 
    * @param name the name of the button.
    * @param enabled true to enable the button, false to disable.
    * @param callback function to call one time when button is clicked.
    * @param data the data to pass the callback when button is clicked.
    */
   var setButton = function(name, enabled, callback, data)
   {
      var button = $(name);
      
      // remove previous click binding
      button.unbind('click');
      
      if(enabled)
      {
         // turn on button
         button.removeClass('off');
         button.addClass('on');
         
         // bind button click
         if(data)
         {
            button.one('click', data, callback);
         }
         else
         {
            button.one('click', callback);
         }
      }
      else
      {
         // turn off button
         button.removeClass('on');
         button.addClass('off');
      }
   };
   
   /**
    * Sets the visibility and message of the specified activity indicator.
    * 
    *  @param id the ID of the activity indicator.
    *  @param enable true to show the indicator, false to hide it.
    *  @param message the activity message to display.
    */
   var setActivityIndicator = function(id, enable, message)
   {
      // DEBUG: output
      bitmunk.log.debug(sLogCategory, 'Set Activity Indicator: ' + id);
      
      // set the message
      if(message)
      {
         $(id + ' .activityMessage').text(message);
      }
      
      // set visibility
      if(enable)
      {
         $(id).fadeTo(0, 1, 
            function() { $(this).css('visibility', 'visible'); });
      }
      else
      {
         $(id).fadeTo('normal', 0,
            function() { $(this).css('visibility', 'hidden'); });
      }
   };
   
   /**
    * Starts or restarts a download using the buyer's purchase preferences.
    * 
    * @param dsId the download state ID.
    * 
    * @return true if successful, false if not.
    */
   var startDownload = function(dsId)
   {
      var rval = false;
      
      // disable buyer preferences
      bitmunk.purchases.view.enableBuyerPreferences(false);
      
      // start the download using the user's options
      rval = bitmunk.purchases.controller.startPurchase({
         dsId: dsId,
         accountId: +$('#accountSelect').val(),
         price: $('#priceInput').val()
      });
      
      if(!rval)
      {
         // re-enable buyer preferences
         bitmunk.purchases.view.enableBuyerPreferences(true);
      }
      
      return rval;
   };
      
   /**
    * Click Handlers
    * ==============
    */
   
   /**
    * Callback when the purchase dialog is opened by clicking on a row. The
    * download state ID of the row clicked is included in the event object
    * data.
    * 
    * @param event the event object.
    */
   var dialogOpenClicked = function(event)
   {
      // DEBUG: output
      bitmunk.log.debug(sLogCategory, 'Dialog Opened');
      
      // get the selected purchase row
      var dsId = event.data;
      
      // check if current purchase is already loaded in dialog
      if(dsId == $('#purchaseDialog').data('dsId'))
      {
         // show purchase dialog
         bitmunk.purchases.view.showPurchaseDialog(true);
      }
      else
      {
         // FIXME: figure out how to show background loading
         // show dialog loading indicator
         setActivityIndicator('#dialogActivity', true, 'Loading...');
         
         // load purchase into dialog
         bitmunk.purchases.controller.loadPurchaseDialog(dsId);
      }
   };
   
   /**
    * Callback when the purchase dialog is closed.
    * 
    * @param event the event object.
    */
   var dialogCloseClicked = function(event)
   {
      // DEBUG: output
      bitmunk.log.debug(sLogCategory, 'Dialog Closed');
      
      // hide purchase dialog
      bitmunk.purchases.view.showPurchaseDialog(false);
   };
   
   /**
    * Callback when the purchase button is clicked.
    *
    * @param event the event object.
    */
   var purchaseClicked = function(event)
   {
      // get download state ID from event data
      var dsId = event.data;
      
      // DEBUG: output
      bitmunk.log.debug(sLogCategory, 'Purchase clicked: ' + dsId);
      
      // turn off purchase button
      setButton('#purchaseButton', false);
      
      if(!startDownload(dsId))
      {
         // starting download failed, turn purchase button back on
         setButton('#purchaseButton', true, purchaseClicked, dsId);
      }
   };
   
   /**
    * Callback when a purchase remove button is clicked.
    *
    * @param event the event object.
    */
   var removeClicked = function(event)
   {
      // get download state ID from event data
      var dsId = event.data;
      
      // DEBUG: output
      bitmunk.log.debug(sLogCategory, 'Remove clicked: ' + dsId);
      
      // turn off row remove button
      $('#removeButton-' + dsId)
         .unbind('click')
         .removeClass('on')
         .addClass('off');
      
      // set status
      bitmunk.purchases.view.setPurchaseStatus(dsId, 'Removing', 'disabled');
      
      // remove row open dialog click binding
      $('#purchaseRow-' + dsId + ' td:not(.removeCol)').unbind('click');
      
      // check if purchase was in dialog
      if(dsId == $('#purchaseDialog').data('dsId'))
      {
         // close dialog
         bitmunk.purchases.view.showPurchaseDialog(false);
         
         // turn off buttons in dialog
         setButton('#purchaseButton', false);
         setButton('#removeButton', false);
         setButton('#pauseButton', false);
         setButton('#resumeButton', false);
      }
      
      // remove the purchase
      bitmunk.purchases.controller.removePurchase(dsId);
   };
   
   /**
    * Callback when dialog pause button is clicked.
    * 
    * @param event the event object.
    */
   var pauseClicked = function(event)
   {
      // get download state ID from event data
      var dsId = event.data;
      
      // DEBUG: output
      bitmunk.log.debug(sLogCategory, 'Pause clicked: ' + dsId);
      
      // set status
      bitmunk.purchases.view.setPurchaseStatus(dsId, 'Pausing', 'paused');
      
      // set status in dialog if purchase is loaded
      if($('#purchaseDialog').data('dsId') == dsId)
      {
         // turn off pause button
         setButton('#pauseButton', false);
      }
      
      // pause the purchase
      bitmunk.purchases.controller.pausePurchase(dsId);
   };
   
   /**
    * Callback when the dialog resume button is clicked.
    * 
    * @param event the event object.
    */
   var resumeClicked = function(event)
   {
      // get download state ID from event data
      var dsId = event.data;
      
      // DEBUG: output
      bitmunk.log.debug(sLogCategory, 'Resume clicked: ' + dsId);
      
      // set status in purchase row
      bitmunk.purchases.view.setPurchaseStatus(dsId, 'Resuming', 'busy');
      
      // set status in dialog if purchase is loaded
      if($('#purchaseDialog').data('dsId') == dsId)
      {
         // turn off resume button
         setButton('#resumeButton', false);
         
         // start download with preferences from the dialog
         if(!startDownload(dsId))
         {
            // starting download failed, turn resume button back on
            setButton('#resumeButton', true, resumeClicked, dsId);
         }
      }
      else
      {
         // resume the purchase (reuses existing buyer preferences)
         bitmunk.purchases.controller.resumePurchase(dsId);
      }
   };
   
   /**
    * This callback is called whenever a clear button is clicked.
    * 
    * @param eventObject the event object sent from the resume click.
    */
   var clearFeedbackClicked = function(event)
   {
      // get download state ID from event data
      var dsId = event.data;
      
      // DEBUG: output
      bitmunk.log.debug(sLogCategory, 'Clear Feedback clicked: ' + dsId);
      
      // clear feedback in UI
      bitmunk.purchases.view.clearPurchaseFeedback(dsId);
      
      // FIXME: remove feedback from model
      //bitmunk.model.remove(sDownloadStatesModel, 'feedback', dsId);
      //bitmunk.purchases.controller.clearFeedback(dsId);
   };
   
   sScriptTask.unblock();
})(jQuery);

/**
 * Bitmunk Media Library View
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 * 
 * @author Mike Johnson
 * @author Manu Sporny
 * @author Dave Longley
 */
(function($) {

var init = function(task)
{
   // log category
   var sLogCategory = 'bitmunk.medialibrary.view';
   
   // Plugin Image URL
   var sPluginImageUrl = '/bitmunk/plugins/bitmunk.webui.MediaLibrary/images/';
   
   /** Public Media Library View namespace **/
   bitmunk.medialibrary.view = {};
   
   /**
    * Sets the visibility and message of the specified activity indicator.
    * 
    * @param indicator the ID of the activity indicator.
    * @param enable true to show the indicator, false to hide it.
    * @param message the activity message to display.
    */
   bitmunk.medialibrary.view.setActivityIndicator = function(
      indicator, enable, message)
   {
      // set the message
      if(message)
      {
         $(indicator + ' .activityMessage').text(message);
      }
      
      // set visibility
      if(enable)
      {
         $(indicator).fadeTo(0, 1, 
            function() { $(this).css('visibility', 'visible'); });
      }
      else
      {
         $(indicator).fadeTo('normal', 0,
            function() { $(this).css('visibility', 'hidden'); });
      }
   };
   
   /**
    * Shows the library browser loading indicator.
    * 
    * @param enable true to show the loading indicator, false to hide it.
    */
   bitmunk.medialibrary.view.showBrowserLoading = function(enable)
   {
      setActivityIndicator(enable ? 'Loading...' : null);
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
   bitmunk.medialibrary.view.setButton = function(
      name, enabled, callback, data)
   {
      var button = $(name);
      
      // remove previous click binding
      button.unbind('click');
      
      if(enabled)
      {
         // turn on button
         button
            .removeClass('off')
            .addClass('on');
         
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
         button
            .removeClass('on')
            .addClass('off');
      }
   };
   
   /**
    * Adds a ware row to the media library browser table.
    * 
    * @param file the file from the media library.
    * @param media the media that matches the file.
    */
   bitmunk.medialibrary.view.addBrowserRow = function(
      file, media, ware, scheme, status)
   {
      bitmunk.log.debug(
         sLogCategory, 'addBrowserRow', file, media, ware, scheme, status);
      
      var mediaId = (media !== null) ? media.id : 0;
      var wareId = mediaId + '-' + file.id;
      var mediaTitle = (media !== null) ? media.title : file.path;
      // FIXME: The unknown media type should be 'unknown', not 'image'
      // Mike will need to generate an image for 'unknown' and add it to
      // bitmunk/content/images/media
      var mediaType = (media !== null) ? media.type : 'image';
      
      // get media primary contributor
      var primary = bitmunk.util.getPrimaryContributor(media);
      var contributor = primary ? primary.name : 'Not Available';

      // create template values
      var values =
      {
         wareId: wareId,
         fileId: file.id,
         mediaId: mediaId,
         title: mediaTitle,
         contributor: contributor,
         typeImage: '/bitmunk/images/media/' + mediaType + '.png',
         typeName: mediaType
      };
      
      // process and insert template
      var template = bitmunk.resource.get(
         'bitmunk.webui.MediaLibrary', 'wareRow.html', true);
      $('#wareRows').append(template.process(values));
      
      // hide empty row
      $('#wareRows .emptyRow').hide();
      
      // update selectAll checkbox
      bitmunk.medialibrary.view.updateSelectAll(
         $('#fileSelectAll'), $('.fileSelect'));
      
      // bind click handler for ware editor
      var data =
      {
         fileId: file.id,
         mediaId: mediaId
      };
      
      // bind ware editor open to every column except selectCol if there is
      // a media that is associated with the ware.
      if(mediaId !== 0)
      {
         $('#row-' + wareId + ' td:not(.selectCol)').bind(
            'click', data, editorOpenClicked);
      }
      
      // set ware status to busy while it loads
      bitmunk.medialibrary.view.setWareStatus(wareId, 'Loading', 'busy');
   };
   
   /**
    * Sets the status of a ware in the ware table.
    * 
    * @param wareId the ID of the ware to update.
    * @param status a string of the status state.
    * @param type the type of status message.
    */
   bitmunk.medialibrary.view.setWareStatus = function(wareId, status, type)
   {
      // set default status type
      type = type ? type : 'ok';
      
      // get row status
      $('#status-' + wareId)
         // set status in the ware row
         .text(status)
         .removeClass()
         .addClass('status-' + type);
   };
   
   /**
    * Sets the payment for a ware in the ware table.
    * 
    * @param wareId the ID of the ware to update.
    * @param scheme the related payee scheme.
    */
   bitmunk.medialibrary.view.setWarePayment = function(wareId, scheme)
   {
      // add payment information from the scheme
      var payment = 0;
      if(scheme)
      {
         $.each(scheme.payees, function(i, payee)
         {
            // FIXME: only support for flat fees at this time
            if(payee.amountType == 'flatFee')
            {
               // woo! javascript floating point math!
               payment += +payee.amount;
            }
         });
      }
      
      // format payment for display
      $('#payment-' + wareId)
         .text('$ ' + (Math.ceil((+payment)*100)/100).toFixed(2))
         .attr('title', '$ ' + payment);
   };
   
   /**
    * Adds the ware data to a ware row in the browser table.
    * 
    * @param wareId the short ware ID for finding the browser row.
    * @param file the associated file.
    * @param ware the ware data to add (can be null).
    * @param scheme the scheme data to add (can be null).
    */
   bitmunk.medialibrary.view.updateBrowserRow = function(
      wareId, file, ware, scheme)
   {
      bitmunk.log.debug(sLogCategory, 'updateBrowserRow', wareId, file, scheme);
      if(!file)
      {
         bitmunk.medialibrary.view.deleteBrowserRow(wareId);
      }
      else
      {
         // update payment information from the scheme
         bitmunk.medialibrary.view.setWarePayment(wareId, scheme);
         
         // update ware info
         if(ware)
         {
            if(ware.exception)
            {
               // set status and add feedback for ware
               bitmunk.medialibrary.view.setWareStatus(
                  wareId, 'Error', 'error');
               bitmunk.medialibrary.view.addWareFeedback(
                  file.id, 'error', ware.exception.cause.message);
            }
            else if(ware.dirty || ware.updating)
            {
               bitmunk.medialibrary.view.setWareStatus(
                  wareId, 'Pending', 'busy');
            }
            else
            {
               bitmunk.medialibrary.view.setWareStatus(wareId, 'OK', 'ok');
            }
         }
         else
         {
            // mark status as disabled
            bitmunk.medialibrary.view.setWareStatus(
               wareId, 'Disabled', 'disabled');
         }
      }
   };
   
   /**
    * Marks the ware row as deleted.
    * 
    * @param wareId the ID of the ware.
    */
   bitmunk.medialibrary.view.deleteBrowserRow = function(wareId)
   {
      bitmunk.log.debug(sLogCategory, 'deleteBrowserRow', wareId);
      
      // add deleted class to row
      $('#row-' + wareId).addClass('deleted');
      
      // set status as disabled
      bitmunk.medialibrary.view.setWareStatus(wareId, 'Disabled', 'disabled');
   };
   
   /**
    * Clears all current library browser rows.
    */
   bitmunk.medialibrary.view.clearBrowserRows = function()
   {
      // remove all ware rows
      $('.wareRow').remove();

      // FIXME: deselect ware select
      $('#fileSelectAll').removeAttr('checked');
      
      // display empty row
      $('#wareRows .emptyRow').show();
   };
   
   /**
    * Sets the ware browser message that is displayed in the list of wares.
    */
   bitmunk.medialibrary.view.setBrowserMessage = function(message)
   {
      $('#browserMessage').html(message);
   };
   
   /**
    * Updates the paging control for the ware table.
    * 
    * @param params the paging control data.
    */
   bitmunk.medialibrary.view.setPageControl = function(params)
   {
      // DEBUG: output
      bitmunk.log.debug(sLogCategory, 'Setting Page Control', params);
      
      // page total
      var pageTotal = Math.ceil(params.total/params.num);
      
      // window size (number of page links on each side of current page)
      var window = 3;
      
      // first page link in window
      var fp = Math.max(Math.ceil(params.page - window), 1);
      
      // last page link in window
      var lp = Math.min(Math.ceil(params.page + window), pageTotal);
      
      if(pageTotal > 0)
      {
         // add prev page link
         var prevLink = '&#171; Previous';
         if(params.page - 1 > 0)
         {
            prevLink = makePageLink(
               '&#171; Previous', params.page - 1, params.num,
               params.order, params.direction, params.query);
         }
         $('.paging-prev').html(prevLink);
         
         // clear page links
         $('.paging-pages').empty();
         
         // add first page link
         if(fp > 1)
         {
            $('.paging-pages').append(
               makePageLink(1, 1, params.num,
                  params.order, params.direction, params.query));
         }
         
         // add ellipses
         if(fp > 2)
         {
            $('.paging-pages').append('&hellip;');
         }
         
         // add window page links
         for(var i = fp; i <= lp; i++)
         {
            if(i == params.page)
            {
               $('.paging-pages').append(
                  '<span class="paging-selected">' + i + '</span>');
            }
            else
            {
               $('.paging-pages').append(
                  makePageLink(i, i, params.num,
                     params.order, params.direction, params.query));
            }
         }
         
         // add ellipses
         if(lp < pageTotal - 1)
         {
            $('.paging-pages').append('&hellip;');
         }
         
         // add last page link
         if(lp < pageTotal)
         {
            $('.paging-pages').append(
               makePageLink(pageTotal, pageTotal, params.num,
                  params.order, params.direction, params.query));
         }
         
         // add next page link
         var nextLink = 'Next &#187;';
         if(params.page < pageTotal)
         {
            nextLink = makePageLink(
               'Next &#187;', params.page + 1, params.num,
               params.order, params.direction, params.query);
         }
         $('.paging-next').html(nextLink);
         
         
         // add per page links
         // FIXME: adjust page var if num is too large
         $('.paging-perpage')
            .empty()
            .append(
               makePageLink(10, Math.ceil(params.page / 10), 10,
                  params.order, params.direction, params.query))
            .append(
               makePageLink(20, Math.ceil(params.page / 20), 20,
                  params.order, params.direction, params.query))
            .append(
               makePageLink(40, Math.ceil(params.page / 40), 40,
                  params.order, params.direction, params.query));
         
         // show paging
         $('.paging').show();
      }
      else
      {
         // hide paging
         $('.paging').hide();
      }
      
      // set paging links on table sort columns
      // FIXME: need to reverse direction on link if current sort col
      
      $('#sort-title').html(makePageLink(
         'Title', params.page, params.num,
         params.order, params.direction, params.query));
      if(params.order == 'title')
      {
         $('#sort-title a').addClass('sort-' + params.direction);
      }
      /*
      $('#sort-contributor').html(makePageLink(
         'Contributor', params.page, params.num,
         'contributor', params.direction, params.query));
      if(params.order == 'contributor')
      {
         $('#sort-contributor a').addClass('sort-' + params.direction);
      }
      */
   };
   
   /**
    * Update an "all" selected checkbox based on current selection state.
    * 
    * @param selectAll jQuery for checkbox controlling selects.
    * @param selects jQuery for checkboxes that selectAll depends on.
    */
   bitmunk.medialibrary.view.updateSelectAll = function(selectAll, selects)
   {
      selectAll.attr('checked',
         (selects.length > 0) && (selects.not(':checked').length === 0));
   };
   
   /**
    * Adds a feedback message and indicator for the specified ware.
    * 
    * @param fileId the ware file ID.
    * @param type the type of message (i.e. 'ok', 'warning', 'error').
    * @param message the message to display.
    */
   bitmunk.medialibrary.view.addWareFeedback = function(fileId, type, message)
   {
      // set feedback icon in ware row
      var icon = $('<img src="/bitmunk/images/feedback-' + type +
         '-16.png" alt="" />');
      $('#feedback-' + fileId).html(icon);
      
      // if ware is loaded in dialog add message to dialog
      if(fileId == $('#libraryEditor').data('fileId'))
      {
         var feedback = $('<p class="' + type + '">' + message + '</p>');
         $('#feedback-ware').prepend(feedback);
         
         // show feedback box
         $('#feedbackBox-ware').show();
      }
   };
   
   /**
    * Clears all feedback messages for the specified ware.
    * 
    * @param fileId the ware file ID.
    */
   bitmunk.medialibrary.view.clearWareFeedback = function(fileId)
   {
      // remove feedback icon in ware row
      $('#feedback-' + fileId).empty();
      
      // if purchase is loaded in dialog remove dialog feedback
      if(fileId == $('#libraryEditor').data('fileId'))
      {
         // hide feedback box
         $('#feedbackBox-ware').hide();
         
         // clear feedback
         $('#feedback-ware').empty();
      }
   };
   
   /** Bitmunk Framework **/
   
   /**
    * The show function is called whenever the page is shown. 
    */
   bitmunk.medialibrary.view.show = function()
   {
      bitmunk.log.debug(sLogCategory, 'show');
      
      // disable browser delete button
      bitmunk.medialibrary.view.setButton('#browserDelete', false);
         
      // bind fileSelectAll to check every ".fileSelect"
      $('#fileSelectAll').bind('click', function(e)
      {
         // select/unselect all files based on master selector
         $('.fileSelect').attr('checked', $(this).attr('checked'));
         
         // enable/disable browser delete button based on whether or not
         // any wares are checked
         bitmunk.medialibrary.view.setButton(
            '#browserDelete', $('.fileSelect:checked').length > 0,
            browserDeleteClicked);
      });
      
      // live automatically binds to all current/future "fileSelect" classes
      $('.fileSelect').live('click', function(e)
      {
         // select-all will be checked if all select-rows are checked
         bitmunk.medialibrary.view.updateSelectAll(
            $('#fileSelectAll'), $('.fileSelect'));
         
         // enable/disable browser delete button based on whether or not
         // any wares are checked
         bitmunk.medialibrary.view.setButton(
            '#browserDelete', $('.fileSelect:checked').length > 0,
            browserDeleteClicked);
      });
      
      // bind button handlers
      $('#browserAdd').bind('click', browserAddClicked);
      $('#browserResync').bind('click', browserResyncClicked);
   };
   
   /**
    * The hide function is called whenever the page is hidden.
    */
   bitmunk.medialibrary.view.hide = function(task)
   {
      // clear library table
      $('#libraryRows').empty();
      
      // unbind file selectors
      $('#fileSelectAll').unbind('click');
      $('.fileSelect').die('click');
      
      // unbind button handlers
      $('#browserAdd').unbind('click');
      
      // FIXME: unbind page control
   };
   
   /** Private functions **/
   
   /**
    * Shows or hides the browser activity indicator.
    * 
    * @param msg the message to display, null/undefined to hide the indicator.
    */
   var setActivityIndicator = function(msg)
   {
      bitmunk.medialibrary.view.setActivityIndicator(
         '#browserActivity', msg ? true : false, msg ? msg : '');
   };
   
   /**
    * Creates a link to be used for page control.
    */
   var makePageLink = function(text, page, num, order, dir, query)
   {
      var url = '#library?page=' + page + '&amp;num=' + num;
      
      if(order)
      {
         url += '&amp;order=' + order;
      }
      
      if(dir)
      {
         url += '&amp;dir=' + dir;
      }
      
      if(query)
      {
         url += '&amp;query=' + query;
      }
      
      return '<a class="paging-link" href="' + url + '">' + text + '</a>';
   };
   
   /**
    * Adds a feedback message to the general feedback box.
    * 
    * @param type the type of message.
    * @param message the message to display.
    */
   var addGeneralFeedback = function(type, message)
   {
      var feedback = $('<p class="' + type + '">' + message + '</p>');
      $('#feedback').prepend(feedback);

      // show feedback box
      $('#feedbackBox').slideDown('normal');
   };
   
   /**
    * Called when the browser add ware button is clicked.
    *
    * @param event the event object.
    */
   var browserAddClicked = function(event)
   {
      bitmunk.log.debug(sLogCategory, 'browserAddClicked');
      
      // open ware editor
      bitmunk.medialibrary.wareEditor.open();
   };
   
   /**
    * Called when the browser resync button is clicked.
    *
    * @param event the event object.
    */
   var browserResyncClicked = function(event)
   {
      bitmunk.log.debug(sLogCategory, 'browserResyncClicked');
      
      // resync catalog
      bitmunk.medialibrary.controller.resyncCatalog();
   };
   
   /**
    * Called when a ware row is clicked to open the ware editor.
    *
    * @param event the event object.
    */
   var editorOpenClicked = function(event)
   {
      bitmunk.log.debug(sLogCategory, 'editorOpenClicked', event.data);
      
      // open ware editor
      bitmunk.medialibrary.wareEditor.open(
         event.data.fileId, event.data.mediaId);
   };
   
   /**
    * Called when the browser delete button is clicked.
    * 
    * @param event the event object.
    */
   var browserDeleteClicked = function(event)
   {
      bitmunk.log.debug(sLogCategory, 'browserDeleteClicked');
      
      // disable browser delete button
      bitmunk.medialibrary.view.setButton('#browserDelete', false);
      
      // show the busy indicator
      setActivityIndicator('Deleting...');
      
      // get selected wares
      var wareIds = [];
      $.each($('.fileSelect:checked'), function()
      {
         var wareId = $(this).attr('id').substring(11);
         wareIds.push(wareId);
         
         // mark browser row as deleted
         bitmunk.medialibrary.view.deleteBrowserRow(wareId);
      });
      
      // controller deletes wares
      bitmunk.medialibrary.controller.deleteWares(wareIds, true);
   };
   
   /**
    * Callback when sort order in browser was changed by clicking on a table
    * header.
    * 
    * @param event the event object.
    */
   var columnSortClicked = function(event)
   {
      bitmunk.log.debug(sLogCategory, 'columnSortClicked');
      
      // FIXME: get correct data out of event object
      // get current sorting
      var col = event.data;
      
      // FIXME: tell the controller the sorting has changed
   };
};

bitmunk.resource.registerScript({
   pluginId: 'bitmunk.webui.MediaLibrary',
   resourceId: 'medialibrary.view.js',
   depends: {},
   init: init
});

})(jQuery);

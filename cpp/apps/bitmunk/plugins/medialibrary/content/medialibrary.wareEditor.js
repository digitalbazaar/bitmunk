/**
 * Bitmunk Media Library Ware Editor
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 * 
 * @author Mike Johnson
 * @author Manu Sporny
 * @author Dave Longley
 */
(function($)
{
   // log category
   var sLogCategory = 'bitmunk.medialibrary.wareEditor';
   
   var sScriptTask = bitmunk.resource.getScriptTask(
      'bitmunk.webui.MediaLibrary', 'medialibrary.wareEditor.js');
   
   // event namespace
   var sNS = '.bitmunk-medialibrary-wareEditor';
   
   // stores the file browser
   var sFileBrowser = null;
   
   // stores whether or not a reload of the library browser will be
   // necessary due to editing changes
   var sReloadNeeded = false;
   
   // Main Content Image URL
   var sContentImageUrl = '/bitmunk/images/';
   
   // urls for updating files/wares/payee schemes
   var sFilesUrl = bitmunk.api.localRoot32 + 'medialibrary/files';
   var sWaresUrl = bitmunk.api.localRoot + 'catalog/wares';
   var sSchemesUrl = bitmunk.api.localRoot + 'catalog/payees/schemes';
   
   // file and media IDs
   var sFileId = null;
   var sMediaId = null;
   
   /** Public Media Library Ware Editor namespace **/
   bitmunk.medialibrary.wareEditor = {};
   
   /**
    * Opens the ware editor and to edit a ware.
    * 
    * @param fileId the ID of the related file.
    * @param mediaId the ID of the related media.
    */
   bitmunk.medialibrary.wareEditor.open = function(fileId, mediaId)
   {
      bitmunk.log.debug(sLogCategory, 'open', fileId, mediaId);
      
      // no reload necessary yet
      sReloadNeeded = false;
      
      // get file, media, ware, and schemes
      var file = fileId ? bitmunk.model.fetch(
         bitmunk.medialibrary.model.name,
         bitmunk.medialibrary.model.fileTable,
         fileId) : null;
      var media = mediaId ? bitmunk.model.fetch(
         bitmunk.medialibrary.model.name,
         bitmunk.medialibrary.model.mediaTable,
         mediaId) : null;
      var ware = (fileId && mediaId) ? bitmunk.model.fetch(
         bitmunk.catalog.model.name,
         bitmunk.catalog.model.wareTable,
         'bitmunk:file:' + mediaId + '-' + fileId) : null;
      var schemes = bitmunk.model.fetchAll(
         bitmunk.catalog.model.name,
         bitmunk.catalog.model.payeeSchemeTable);
      
      // show the editor dialog
      show(file, media, ware, schemes);
   };
   
   /**
    * Closes the ware editor and notifies the library browser.
    */
   bitmunk.medialibrary.wareEditor.close = function()
   {
      bitmunk.log.debug(sLogCategory, 'close', sFileId, sMediaId);
      hide();
      bitmunk.medialibrary.controller.wareEditorClosed(
         sReloadNeeded, sFileId, sMediaId);
      sReloadNeeded = false;
   };
   
   /**
    * Called when the payment editor is closed.
    * 
    * @param psId the payment scheme the editor was editing.
    */
   bitmunk.medialibrary.wareEditor.paymentEditorClosed = function(psId)
   {
      // show loading indicator
      setActivityIndicator('Loading...');
      
      // update payment details
      setPaymentDetails(null, null, psId);
      
      // try to enable save button
      enableSaveButton(true);
      
      // hide loading indicator
      setActivityIndicator();
   };
   
   /** Private functions **/
   
   /**
    * Shows or hides the file browser for wares.
    * 
    * @param enable true to show the file browser, false to hide it.
    */
   var showFileBrowser = function(enable)
   {
      if(sFileBrowser === null)
      {
         // create file browser, use download path for browse path
         var cfg = bitmunk.model.fetch(
            'config', 'config', 'bitmunk.purchase.Purchase');
         sFileBrowser = bitmunk.filebrowser.create({
            id: 'wareFileBrowser',
            path: cfg.downloadPath,
            selectable: ['file'],
            fileSelected: fileSelected
         });
      }
      
      if(enable)
      {
         // hide all details so that only browser will be showing
         $('#mediaSection').hide();
         $('#fileSection').hide();
         $('#wareSection').hide();
         $('#paymentSection').hide();
         $('#editorSave').hide();
         
         sFileBrowser.show();
         $('#fileFinder').show();
      }
      else
      {
         $('#fileFinder').hide();
         sFileBrowser.hide();
         $('#editorSave').show();
      }
   };
   
   /**
    * Shows or hides the media browser for wares.
    * 
    * @param enable true to show the media browser, false to hide it.
    */
   var showMediaBrowser = function(enable)
   {
      if(sMediaBrowser === null)
      {
         // FIXME: create media browser
      }
      
      /*
      if(enable)
      {
         // hide all details so that only browser will be showing
         $('#mediaSection').hide();
         $('#fileSection').hide();
         $('#wareSection').hide();
         $('#paymentDetails').hide();
         
         sMediaBrowser.show();
         $('#mediaFinder').show();
      }
      else
      {
         $('#mediaFinder').hide();
         sMediaBrowser.hide();
      }
      */
   };
   
   /**
    * Sets the media details in the editor.
    * 
    * @param media the media with details to show.
    */
   var setMediaDetails = function(media)
   {
      $('#mediaSection').hide();
      
      if(media)
      {
         // set media link & image
         $('#media-link').attr('href',
            'http://bitmunk.com/media/' + media.id);
         $('#media-image').attr('src',
            bitmunk.api.bitmunkRoot + 'media/images/' + media.id + '?size=sm');
         $('#media-image').attr('title', 'View ' + media.title + ' on Bitmunk');
         
         // set media title
         $('#media-title').text(media.title);
         
         // set media contributor
         var primary = bitmunk.util.getPrimaryContributor(media);
         var contributor = primary ? primary.name : 'Not Available';
         $('#media-contributor').text(contributor);
         
         $('#mediaSection').show();
      }
   };
   
   /**
    * Clears the media details in the editor.
    */
   var clearMediaDetails = function()
   {
      $('#mediaSection').hide();
      
      // clear media link & image
      
      
      // clear media details
      $('#media-title').text('');
      $('#media-contributor').text('');
   };

   /**
    * Sets the file details in the editor.
    * 
    * @param file the file info with details to show.
    */
   var setFileDetails = function(file)
   {
      // hide file section
      $('#fileSection').hide();

      // hide file details
      $('#fileDetails').hide();
      
      if(file)
      {
         // set file path
         $('#file-path').val(file.path);
         
         // set file details
         $('#file-id').text(file.id);
         $('#file-extension').text(file.extension);
         $('#file-type').text('(' + file.contentType + ')');
         $('#file-size').text(bitmunk.util.formatSize(file.size));
         $('#file-bitrate').text(file.formatDetails.averageBitrate);
         
         // show file section
         $('#fileSection').show();
      }
   };
   
   /**
    * Clears the file details in the editor and shows the file browser.
    */
   var clearFileDetails = function()
   {
      $('#fileSection').hide();
      
      // clear file path
      $('#file-path').val('');
      
      // clear file details
      $('#file-id').text('');
      $('#file-content').text('');
      $('#file-extension').text('');
      $('#file-size').text('');
      $('#file-bitrate').text('');
   };
   
   /**
   * Sets the description details in the editor.
   *
   * @param ware the ware with details to show.
   */
   var setWareDetails = function(ware)
   {
      // set ware description
      $('#wareSection').hide();
      $('#ware-description').val(ware ? ware.description : '');
      $('#wareSection').show();
   };
   
   /**
   * Clears the description details in the ware editor.
   */
   var clearWareDetails = function()
   {
      $('#wareSection').hide();
      
      // clear ware description
      $('#ware-description').val('');
   };
   
   /**
    * Sets the payment details in the editor.
    * 
    * @param ware the ware currently being edited or null for a new ware.
    * @param schemes the payment schemes to use when updating the payment
    *                details.
    * @param selectedId the payee scheme id to select in the selection box or
    *                   null if the payee scheme for the ware should be 
    *                   unselected.
    */
   var setPaymentDetails = function(ware, schemes, selectedId)
   {
      // clear payment details
      clearPaymentDetails();
      
      // scheme id to label as the "default"
      var autoSellSchemeId = bitmunk.model.fetch(
         'config', 'config', 'bitmunk.catalog.CustomCatalog')
         .autoSell.payeeSchemeId;
      
      if(!ware)
      {
         // load ware from model
         var file = sFileId ? bitmunk.model.fetch(
            bitmunk.medialibrary.model.name,
            bitmunk.medialibrary.model.fileTable,
            sFileId) : null;
         ware = file ? bitmunk.model.fetch(
            bitmunk.catalog.model.name,
            bitmunk.catalog.model.wareTable,
            'bitmunk:file:' + file.mediaId + '-' + sFileId) : null;
      }
      
      // if no selected ID has been specified, use an existing one from
      // the ware
      if(ware && ware.payeeSchemeId && !selectedId)
      {
         selectedId = ware.payeeSchemeId;
      }

      if(!schemes)
      {
         // reload schemes
         schemes = bitmunk.model.fetchAll(
            bitmunk.catalog.model.name,
            bitmunk.catalog.model.payeeSchemeTable);
      }
      
      // FIXME: need a way to provide selected and default schemes
      // add each payee scheme to the scheme selector
      $.each(schemes, function(index, scheme)
      {
         $('#payment-select').append(
            '<option id="scheme-' + scheme.id + '" value="' + scheme.id +
            '">' + scheme.id + ' - ' + scheme.description +
            (scheme.id == autoSellSchemeId ? ' (default)' : '') + '</option>');
         
         if(selectedId == scheme.id)
         {
            $('#scheme-' + scheme.id).attr('selected', 'selected');
         }
      });
      
      $('#paymentSection').show();
   };
   
   /**
    * Clears the payment details in the editor.
    */
   var clearPaymentDetails = function()
   {
      $('#paymentSection').hide();
      
      // empty payment select
      $('#payment-select').empty();
   };
   
   /**
    * Adds a feedback message inside the editor dialog.
    * 
    * @param type the type of message (i.e. 'ok', 'warning', 'error').
    * @param message the message to display.
    */
   var addFeedback = function(type, message)
   {
      var feedback = $('<p class="' + type + '">' + message + '</p>');
      $('#feedback-ware').prepend(feedback);
      
      // show feedback box
      $('#feedbackBox-ware').show();
   };
   
   /**
    * Clears all feedback messages in the editor dialog.
    */
   var clearFeedback = function()
   {
      // hide feedback box and clear all feedback
      $('#feedbackBox-ware').hide();
      $('#feedback-ware').empty();
   };
   
   /**
    * Enables or disables the save button. Enabling the save button will only
    * be successful if all of the data necessary to save the current ware is
    * available in the editor.
    * 
    * @param enable true to enable the save button, false to disable it.
    */
   var enableSaveButton = function(enable)
   {
      if(!enable)
      {
         bitmunk.medialibrary.view.setButton('#editorSave', false);
      }
      else
      {
         var fs = $('#fileSection').attr('display') != 'none';
         var ms = $('#mediaSection').attr('display') != 'none';
         var ss = $('#payment-select option:selected').val() !== null;
         
         // check to make sure that a file has been selected, it has an 
         // associated media, and a payment scheme has been selected before
         // being able to click on the "Save" button
         if(fs && ms && ss)
         {
            // enable button
            bitmunk.medialibrary.view.setButton(
               '#editorSave', true, saveClicked);
         }
         else
         {
            // disable button
            bitmunk.medialibrary.view.setButton('#editorSave', false);
         }
      }
   };
   
   /**
    * Shows or hides the editor activity indicator.
    * 
    * @param msg the message to display, null/undefined to hide the indicator.
    */
   var setActivityIndicator = function(msg)
   {
      bitmunk.medialibrary.view.setActivityIndicator(
         '#editorActivity', msg ? true : false, msg ? msg : '');
   };
   
   /**
    * Shows or hides the editor deleting indicator.
    * 
    * @param enable true to show the indicator, false to hide it.
    */
   var showDeleting = function(enable)
   {
      setActivityIndicator(enable ? 'Deleting payment scheme...' : null);
      
      // FIXME: also only allow enabling if there are payee schemes
      // available for selection
      
      // enable/disable delete button
      if(enable)
      {
         $('#schemeDelete')
            .removeClass('on')
            .addClass('off');
      }
      else
      {
         $('#schemeDelete')
            .removeClass('off')
            .addClass('on');
      }
      
      // update save button
      enableSaveButton(!enable);
   };
   
   /**
    * Shows or hides the editor saving indicator.
    * 
    * @param enable true to show the indicator, false to hide it.
    */
   var showSaving = function(enable)
   {
      setActivityIndicator(enable ? 'Saving...' : null);
      
      // update save button
      enableSaveButton(!enable);
   };
   
   /**
    * Shows or hides the editor adding file indicator.
    * 
    * @param enable true to show the indicator, false to hide it.
    */
   var showAddingFile = function(enable)
   {
      setActivityIndicator(enable ? 'Adding file to media library...' : null);
      
      // update save button
      enableSaveButton(!enable);
   };
   
   /**
    * Called whenever a file has been selected in the file browser. 
    * 
    * @param path the path name of the file that was selected.
    */
   var fileSelected = function(path)
   {
      bitmunk.log.debug(sLogCategory, 'fileSelected', path);
      
      // clear editor feedback
      clearFeedback();
      
      // hide file browser, show adding file indicator
      showFileBrowser(false);
      showAddingFile(true);
      
      // hit proxy to update file info on backend
      // Note: The file will be updated asynchronously on the backend
      // so there is nothing to do in a success() handler.
      bitmunk.api.proxy(
      {
         method: 'POST',
         url: sFilesUrl,
         params: { nodeuser: bitmunk.user.getUserId() },
         data:
         {
            mediaId: 0,
            path: path
         },
         error: function(xhr, textStatus, errorThrown)
         {
            var ex;
            try
            {
               // use exception from server
               ex = JSON.parse(xhr.responseText);
            }
            catch(e)
            {
               // use error message
               ex = { message: textStatus };
            }
            
            // add editor feedback
            addFeedback('error',
               'There was a problem when attempting to add the file ' +
               'to the media library. ' + (ex.cause ?
               ex.cause.message : ex.message));
            
            // hide adding file indicator
            showAddingFile(false);
         }
      });
   };
   
   /**
    * Called when a file has been added to or updated in the media library on
    * the backend.
    * 
    * @param event the event object.
    * @param fileId the file ID associated with the event.
    */
   var fileAdded = function(event, fileId)
   {
      // get related file
      var file = bitmunk.model.fetch(
         bitmunk.medialibrary.model.name,
         bitmunk.medialibrary.model.fileTable,
         fileId);
      bitmunk.log.debug(sLogCategory, event.type, file);
      
      // we're only interested in the event if it refers to the file
      // that we've selected in our dialog
      if(sFileBrowser.selection.path == file.path)
      {
         // update IDs
         sFileId = file.id;
         sMediaId = file.mediaId;
         
         // if a media exists we can clear our file browser selection
         // and update the editor with the new details
         var media = sMediaId ? bitmunk.model.fetch(
            bitmunk.medialibrary.model.name,
            bitmunk.medialibrary.model.mediaTable,
            sMediaId) : null;
         if(media)
         {
            // fetch the related ware
            var ware = bitmunk.model.fetch(
               bitmunk.catalog.model.name,
               bitmunk.catalog.model.wareTable,
               'bitmunk:file:' + sMediaId + '-' + sFileId);
            
            bitmunk.log.debug(
               sLogCategory, 'loading selected file', file, ware);
            
            // clear file browser selection and hide browser
            sFileBrowser.clearSelection();
            showFileBrowser(false);
            
            // set dialog title
            $('#wareEditor-title').text('Edit - ' + media.title);
            
            // set details
            setMediaDetails(media);
            setFileDetails(file);
            setWareDetails(ware);
            setPaymentDetails(ware);
            clearFeedback();
            
            // enable save button
            enableSaveButton(true);
         }
         // no media was found to be associated with the file, so we must
         // reject it (we also try to automatically remove it from the media
         // library)
         else
         {
            // clear IDs
            sFileId = null;
            sMediaId = null;
            
            // FIXME: clear all details?
            clearFileDetails();
            
            // add feedback that associated media was not found
            addFeedback(
               'error', 'The file you selected, ' + file.path + 
               ', is not registered on Bitmunk. The copyright owner of the ' +
               'file must first allow it to be sold via the Bitmunk website.');
            
            // delete the previously selected file from the media library
            bitmunk.task.start(
            {
               type: sLogCategory + '.automaticallyRemoveFile',
               run: function(task)
               {
                  task.next(function(task)
                  {
                     bitmunk.medialibrary.controller.removeFile(file.id, task);
                  });
               },
               failure: function(task)
               {
                  bitmunk.medialibrary.view.addWareEditorFeedback(
                     'warning', 'Failed to automatically remove ' + file.path + 
                     'from the Media Library. You will need to manually ' + 
                     'remove the file by selecting the file and clicking ' +
                     'the "Delete" button on the main Media Library page.');
               }
            });
         }
         
         // hide file adding indicator
         showAddingFile(false);
      }
   };
   
   /**
    * Called there has been an exception when trying to add a file to the
    * media library in the backend.
    *
    * @param event the event object.
    * @param file the file info associated with the event.
    * @param exception the exception object that generated the event.
    */
   var fileException = function(event, file, exception)
   {
      bitmunk.log.debug(sLogCategory, event.type, file, exception);
      
      // we're only interested in the event if it refers to the file
      // that we've selected in our dialog
      if(sFileBrowser.selection.path == file.path)
      {
         // show the file browser
         showFileBrowser(true);
         
         // add most relevant message from exception as feedback
         var message = exception.cause ?
            exception.cause.message : exception.message;
         addFeedback('error',
            'There was an error while processing the file. ' + message +
            ' Please try selecting another file.');
         showAddingFile(false);
      }
   };
   
   /**
    * Called when a payee scheme changes.
    * 
    * @param event the event object.
    * @param psId the payee scheme ID associated with the event.
    */
   var payeeSchemesChanged = function(event, psId)
   {
      bitmunk.log.debug(sLogCategory, event.type, event, psId);
      
      // update payment details
      setPaymentDetails(null, null, psId);
      
      // try to enable save button
      enableSaveButton(true);
   };
   
   /**
    * Called when the editor close button is clicked.
    * 
    * @param event the event object.
    */
   var closeClicked = function(event)
   {
      // DEBUG: output
      bitmunk.log.debug(sLogCategory, 'closeClicked');
      
      bitmunk.medialibrary.wareEditor.close();
   };
   
   /**
    * Called when the new payment scheme button is clicked.
    * 
    * @param event the event object.
    */
   var schemeNewClicked = function(event)
   {
      // DEBUG: output
      bitmunk.log.debug(sLogCategory, 'schemeNewClicked');
      
      // open new scheme in payment editor
      bitmunk.medialibrary.paymentEditor.open(0);
   };
   
   /**
    * Callback when the payment scheme edit button is clicked.
    * 
    * @param event the event object.
    */
   var schemeEditClicked = function(event)
   {
      // DEBUG: output
      bitmunk.log.debug(sLogCategory, 'schemeEditClicked');
      
      // get the selected payment scheme ID
      var psId = $('#payment-select option:selected').val();
      if(psId === null)
      {
         // no scheme selected, so edit a new one
         psId = 0;
      }
      
      // open payment editor
      bitmunk.medialibrary.paymentEditor.open(+psId);
   };
   
   /**
    * Called when the delete payee scheme button is clicked.
    * 
    * @param event the event object.
    */
   var schemeDeleteClicked = function(event)
   {
      // DEBUG: output
      bitmunk.log.debug(sLogCategory, 'schemeDeleteClicked');
      
      // show deleting indicator
      showDeleting(true);
      
      // get the selected payee scheme ID
      var psId = +($('#payment-select option:selected').val());
      
      // hit proxy to delete the payee scheme on backend
      bitmunk.api.proxy(
      {
         method: 'DELETE',
         url: sSchemesUrl + '/' + psId,
         params: { nodeuser: bitmunk.user.getUserId() },
         success: function(data, status)
         {
            // remove payee scheme from model
            bitmunk.model.remove(
               bitmunk.catalog.model,
               bitmunk.catalog.payeeSchemeTable,
               psId);
            
            // FIXME: clear previous error feedback from trying to delete
            // payee scheme(s)?
         },
         error: function(xhr, textStatus, errorThrown)
         {
            var ex;
            try
            {
               // use exception from server
               ex = JSON.parse(xhr.responseText);
            }
            catch(e)
            {
               // use error message
               ex = { message: textStatus };
            }
            
            // add editor feedback
            addFeedback('error',
               'There was a problem when attempting to delete ' +
               'the payment scheme. ' + (ex.cause ?
               ex.cause.message : ex.message));
         },
         complete: function()
         {
            // hide deleting indicator
            showDeleting(false);
         }
      });
   };
   
   /**
    * Called when the save button is clicked in the editor.
    * 
    * @param event the event object.
    */
   var saveClicked = function(event)
   {
      bitmunk.log.debug(sLogCategory, 'saveClicked');
      
      // a reload of the library browser will be necessary
      sReloadNeeded = true;
      
      // show saving indicator
      showSaving(true);
      
      // create ware to save
      var ware =
      {
         id: 'bitmunk:file:' + sMediaId + '-' + sFileId,
         mediaId: sMediaId,
         description: $('#ware-description').val(),
         fileInfo: { id: sFileId },
         payeeSchemeId: +($('#payment-select option:selected').val())
      };
      
      // hit proxy to update ware on backend
      bitmunk.api.proxy(
      {
         method: 'POST',
         url: sWaresUrl,
         params: { nodeuser: bitmunk.user.getUserId() },
         data: ware,
         success: function(data, status)
         {
            bitmunk.medialibrary.wareEditor.close();
         },
         error: function(xhr, textStatus, errorThrown)
         {
            var ex;
            try
            {
               // use exception from server
               ex = JSON.parse(xhr.responseText);
            }
            catch(e)
            {
               // use error message
               ex = { message: textStatus };
            }
            
            // add editor feedback
            addFeedback('error',
               'There was a problem when attempting to update ' +
               'the ware. ' + (ex.cause ?
               ex.cause.message : ex.message));
         },
         complete: function()
         {
            // hide saving indicator
            showSaving(false);
         }
      });
   };
   
   /**
    * Called when the feedback clear button is clicked.
    * 
    * @param event the event object.
    */
   var feedbackClearClicked = function(event)
   {
      bitmunk.log.debug(sLogCategory, 'feedbackClearClicked');
      clearFeedback();
   };
   
   /**
    * Shows the ware editor dialog with the given file, media, ware, and
    * payee schemes.
    * 
    * @param file the file info.
    * @param media the media.
    * @param ware the ware, if any.
    * @param schemes all payee schemes.
    */
   var show = function(file, media, ware, schemes)
   {
      // show loading indicator
      setActivityIndicator('Loading...');
      
      // set IDs
      sFileId = file ? file.id : null;
      sMediaId = media ? media.id : null;
      
      // if no file has been selected yet, show the file browser
      if(!file)
      {
         // set dialog title
         $('#wareEditor-title').text('Add New File');
         
         showFileBrowser(true);
      }
      // hide file browser and set details for existing data
      else
      {
         // set dialog title
         $('#wareEditor-title').text('Edit - ' + media.title);
         
         // hide file browser
         showFileBrowser(false);
         
         // show all details
         setMediaDetails(media);
         setFileDetails(file);
         setWareDetails(ware);
         setPaymentDetails(ware, schemes);
         
         // add feedback for the ware
         if(ware && ware.exception)
         {
            addFeedback('error', ware.exception.cause ?
               ware.exception.cause.message : ware.exception.message);
         }
      }
      
      // bind to model events
      $(bitmunk.medialibrary)
         .bind('bitmunk-medialibrary-File-added' + sNS, fileAdded)
         .bind('bitmunk-medialibrary-File-updated' + sNS, fileAdded)
         .bind('bitmunk-medialibrary-File-exception' + sNS, fileException)
         .bind('bitmunk-common-PayeeScheme-added' + sNS, payeeSchemesChanged)
         .bind('bitmunk-common-PayeeScheme-updated' + sNS, payeeSchemesChanged)
         .bind('bitmunk-common-PayeeScheme-removed' + sNS, payeeSchemesChanged)
         .bind('bitmunk-catalog-CustomCatalog-config-changed' + sNS,
            payeeSchemesChanged);
      
      // bind handlers
      $('#editorClose').bind('click', closeClicked);
      $('#editorSave').bind('click', saveClicked);
      $('#file-view').bind('click', function(e)
      {
         $('#fileDetails').toggle();
      });
      $('#schemeNew').bind('click', schemeNewClicked);
      $('#schemeEdit').bind('click', schemeEditClicked);
      $('#schemeDelete').bind('click', schemeDeleteClicked);
      $('#payment-select').bind('select', function(e)
      {
         enableSaveButton(true);
      });
      $('#clearButton-ware').bind('click', feedbackClearClicked);
      
      // try to enable the save button
      enableSaveButton(true);
      
      // enable overlay and show editor
      $('#overlay').toggle(true);
      $('#wareEditor').toggle(true);
      
      // hide loading indicator
      setActivityIndicator();
   };
   
   /**
    * Hides the ware editor dialog.
    */
   var hide = function()
   {
      // disable overlay and hide editor
      $('#overlay').toggle(false);
      $('#wareEditor').toggle(false);
      
      // clear IDs
      sFileId = null;
      sMediaId = null;
      
      // clear details
      clearMediaDetails();
      clearFileDetails();
      clearWareDetails();
      clearPaymentDetails();
      
      // clear feedback
      clearFeedback();
      
      // unbind handlers
      $('#editorClose').unbind('click');
      $('#editorSave').unbind('click');
      $('#file-view').unbind('click');
      $('#schemeNew').unbind('click');
      $('#schemeEdit').unbind('click');
      $('#schemeDelete').unbind('click');
      $('#payment-select').unbind('click');
      $('#clearButton-ware').unbind('click');
      
      // unbind events
      $(bitmunk.medialibrary).unbind(sNS);
   };
   
   sScriptTask.unblock();
})(jQuery);

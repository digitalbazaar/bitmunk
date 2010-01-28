/**
 * Bitmunk Media Library Payment Scheme Editor
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 * 
 * @author Mike Johnson
 * @author Manu Sporny
 * @author Dave Longley
 */
(function($)
{
   // log category
   var sLogCategory = 'bitmunk.medialibrary.paymentEditor';
   
   var sScriptTask = bitmunk.resource.getScriptTask(
      'bitmunk.webui.MediaLibrary', 'medialibrary.paymentEditor.js');
   
   // store the number of payee rows so we can give each a unique ID
   var sPayeeRows = 0;
   
   // the ID of the payee scheme being edited
   var sPayeeSchemeId = 0;
   
   // urls for updating payee schemes
   var sSchemesUrl = bitmunk.api.localRoot + 'catalog/payees/schemes';
   
   /** Public Media Library Payment Scheme Editor namespace **/
   bitmunk.medialibrary.paymentEditor =
   {
      /**
       * Opens the payment scheme editor and to edit payment scheme.
       * 
       * @param psId the related scheme ID (can be 0 for a new scheme).
       */
      open: function(psId)
      {
         bitmunk.log.debug(sLogCategory, 'open', psId);
         
         // save payee scheme ID
         sPayeeSchemeId = psId;
         
         // get scheme
         var scheme = bitmunk.model.fetch(
            bitmunk.catalog.model.name,
            bitmunk.catalog.model.payeeSchemeTable,
            psId);
         
         // get user accounts
         var accounts = bitmunk.model.fetch(
            'accounts', 'accounts', bitmunk.user.getUserId());
         
         // show the editor dialog
         show(scheme, accounts);
      },
      
      /**
       * Closes the payment editor and notifies the ware editor.
       */
      close: function()
      {
         bitmunk.log.debug(sLogCategory, 'close', sPayeeSchemeId);
         hide();
         bitmunk.medialibrary.wareEditor.paymentEditorClosed(sPayeeSchemeId);
      },
      
      /**
       * Adds a feedback message inside the editor dialog.
       * 
       * @param schemeId the payment scheme ID.
       * @param type the type of message (i.e. 'ok', 'warning', 'error').
       * @param message the message to display.
       */
      addFeedback: function(schemeId, type, message)
      {
         // only add feeback if scheme is loaded in dialog
         if(schemeId == sPayeeSchemeId)
         {
            var feedback = $('<p class="' + type + '">' + message + '</p>');
            $('#feedback-payment').prepend(feedback);
            
            // show feedback box
            $('#feedbackBox-payment').show();
         }
      },
      
      /**
       * Clears all feedback messages for the specified payment scheme.
       * 
       * @param schemeId the payment scheme ID or null to clear any feedback.
       */
      clearFeedback: function(schemeId)
      {
         // if scheme is loaded in dialog or scheme ID is null/undefined,
        // remove feedback
         if(schemeId == sPayeeSchemeId || !schemeId)
         {
            // hide feedback box
            $('#feedbackBox-payment').hide();
            
            // clear feedback
            $('#feedback-payment').empty();
         }
      }
   };
   
   /** Private functions **/
   
   /**
    * Clears the payees in the editor.
    */
   var clearPayees = function()
   {
      // clear payees
      $('.payeeRow').remove();
      var sPayeeRows = 0;
   };
   
   /**
    * Adds a payee row to the editor.
    * 
    * @param accounts the user accounts to use, null to look them up.
    * @param payee the payee data to use in the row, null to create a new row.
    */
   var addPayeeRow = function(accounts, payee)
   {
      // get user accounts as necessary
      if(!accounts)
      {
         accounts = bitmunk.model.fetch(
            'accounts', 'accounts', bitmunk.user.getUserId());
      }
      
      // increment number of payee rows and save old value to use as an ID
      var position = sPayeeRows++;
      
      // create template values
      var values =
      {
         position: position,
         amount: payee ? payee.amount : '0.00',
         description: payee ? payee.description : ''
      };
      
      // process and insert template
      var template = bitmunk.resource.get(
         'bitmunk.webui.MediaLibrary', 'payeeRow.html', true);
      $('#payeeRows').append(template.process(values));
      
      // update selectAll checkbox
      bitmunk.medialibrary.view.updateSelectAll(
         $('#payeeSelectAll'), $('.payeeSelect'));
      
      // add accounts
      // build options html from user accounts
      var optionsHtml = '';
      $.each(accounts, function(accountId, account)
      {
         // create option html, converting string to float
         optionsHtml += '<option value="' + accountId + '">' +
            account.name + ' ($' + (+account.balance).toFixed(2) +
            ')</option>\n';
      });
      
      // get account box
      var accountBox = $('#payeeAccount-' + position);
      
      // FIXME: set event handler to get updates to accounts
      //accountBox.bind(
      //   'bitmunk-accounts-updated',
      //   function(event, accounts) { setAccounts(dsId); });
      
      // set options in account selector
      accountBox.html(optionsHtml);
      
      if(payee)
      {
         // select account with payee id
         $('#payeeAccount-' + position)[0].value = payee.id;
      }
      else
      {
         // set first account selected
         $('#payeeAccount-' + position + ' option:first').attr(
            'selected', 'selected');
      }
   };
   
   /**
    * Updates a message describing the number of wares with which the given
    * scheme is associated.
    * 
    * @param scheme the scheme to use when updating the message.
    */
   var updateSchemeAssociatedWares = function(scheme)
   {
      if(scheme && scheme.wareCount > 0)
      {
         var message = 'Modifying this payment scheme will affect ';
         if(scheme.wareCount == 1)
         {
            message += '1 ware.';
         }
         else
         {
            message += scheme.wareCount + ' wares.';
         }
         
         $('#modificationWarning')
            .html(message)
            .show();
      }
      else
      {
         $('#modificationWarning').hide();
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
         '#paymentActivity', msg ? true : false, msg ? msg : '');
   };
   
   /**
    * Called when the payee add button is clicked.
    * 
    * @param event the event object.
    */
   var payeeAddClicked = function(event)
   {
      bitmunk.log.debug(sLogCategory, 'payeeAddClicked');
      
      // show adding indicator
      setActivityIndicator('Adding payee...');
      
      // add a new payee row
      addPayeeRow();
      
      // hide adding indicator
      setActivityIndicator();
   };
   
   /**
    * Called when the payee delete button is clicked.
    * 
    * @param event the event object.
    */
   var payeeDeleteClicked = function(event)
   {
      bitmunk.log.debug(sLogCategory, 'payeeDeleteClicked');
      
      // turn off payee delete button
      bitmunk.medialibrary.view.setButton('#payeeDelete', false);
      
      // show deleting indicator
      setActivityIndicator('Deleting payee...');
      
      // remove selected rows
      $('.payeeRow:has(.payeeSelect:checked)').remove();
      if($('.payeeRow').length === 0)
      {
         // add a default payee (there should always be at least one default
         // payee selected via the payment editor
         addPayeeRow();
      }
      
      // update selectAll checkbox
      bitmunk.medialibrary.view.updateSelectAll(
         $('#payeeSelectAll'), $('.payeeSelect'));
      
      // hide deleting indicator
      setActivityIndicator();
   };
   
   /**
    * Called when the close button is clicked.
    * 
    * @param event the event object.
    */
   var closeClicked = function(event)
   {
      bitmunk.log.debug(sLogCategory, 'closeClicked');
      bitmunk.medialibrary.paymentEditor.close();
   };
   
   /**
    * Called when the save button is clicked in the editor.
    * 
    * @param task the task object.
    */
   var saveScheme = function(task)
   {
      bitmunk.log.debug(sLogCategory, 'saveScheme');
      
      // create payee scheme from payee rows
      var ps =
      {
         id: sPayeeSchemeId,
         description: $('#schemeDescription').val(),
         payees: []
      };
      $('#payeeRows .payeeRow').each(function(i, row)
      {
         ps.payees.push({
            id: +($('.payeeAccount option:selected', $(row)).val()),
            description: $('.payeeDescription', $(row)).val(),
            amountType: 'flatFee',
            min: '0',
            percentage: '0',
            amount: $('.payeeAmount', $(row)).val()
         });
      });
      
      // block during save
      task.block();
      
      // hit proxy to update the payee scheme on backend
      bitmunk.api.proxy(
      {
         method: 'POST',
         // include ID as a path parameter if editing an existing scheme
         url: (ps.id === 0) ? sSchemesUrl : (sSchemesUrl + '/' + ps.id),
         params: { nodeuser: bitmunk.user.getUserId() },
         data: ps,
         success: function(data, status)
         {
            // close editor
            data = JSON.parse(data);
            sPayeeSchemeId = data.payeeSchemeId;
            bitmunk.medialibrary.paymentEditor.close();
            task.unblock();
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
            
            // add payment editor feedback, showing specific validation
            // errors if possible
            if(ex.type == 'db.validation.ValidationError')
            {
               $.each(ex.details.errors, function(name, error)
               {
                  if(error.possibleErrors)
                  {
                     // FIXME: what are the array values in possibleErrors?
                     $.each(error.possibleErrors[0], function(i, entry)
                     {
                        bitmunk.medialibrary.paymentEditor.addFeedback(
                           ps.id, 'error', entry.message);
                     });
                  }
                  else if(error.message)
                  {
                     bitmunk.medialibrary.paymentEditor.addFeedback(
                        ps.id, 'error', error.message);
                  }
               });
            }
            else
            {
               bitmunk.medialibrary.paymentEditor.addFeedback(
                  ps.id, 'error',
                  'There was a problem when attempting to update ' +
                  'the payment scheme. ' + (ex.cause ?
                  ex.cause.message : ex.message));
            }
            task.fail();
         }
      });
   };
   
   /**
    * Callback when the save button is clicked to save config.
    *
    * @param task the task object.
    */
   var saveConfig = function(task)
   {
      bitmunk.log.debug(sLogCategory, 'save config');
      
      // get current config id
      var autoSellSchemeId = bitmunk.model.fetch(
         'config', 'config', 'bitmunk.catalog.CustomCatalog')
         .autoSell.payeeSchemeId;
      // get current UI value
      var isSet = ($('#isAutoSellScheme').fieldValue().length > 0);
      
      // new autosell id
      var newId = null;
      
      if((sPayeeSchemeId == autoSellSchemeId) && !isSet)
      {
         // disable current scheme as autosell id
         // FIXME: Set to 0, remove value, or disallow this?
         // FIXME: Does a id value need to be valid at all times after it is
         // FIXME: initialized?
         newId = 0;
      }
      else if((sPayeeSchemeId != autoSellSchemeId) && isSet)
      {
         // update autosell scheme to current id
         newId = sPayeeSchemeId;
      }
      
      if(newId !== null)
      {
         task.block();
      
         // get raw config
         var cfg = bitmunk.model.fetch('config', 'config', 'raw');
         
         // merge the page data into the raw configuration      
         var MERGE = '_merge_';
         if(!(MERGE in cfg))
         {
            cfg[MERGE] = {};
         }
         cfg[MERGE] = $.extend(true, cfg[MERGE],
         {
            'bitmunk.catalog.CustomCatalog':
            {
               autoSell:
               {
                  payeeSchemeId: newId 
               }
            }
         });
         
         // save the settings
         // FIXME: temp workaround until node config events are done...
         // backend will not send any config events to trigger this view
         // to update its display...
         bitmunk.api.proxy(
         {
            method: 'POST',
            url: bitmunk.api.localRoot + 'system/config',
            params:
            {
               nodeuser: bitmunk.user.getUserId()
            },
            data: cfg,
            success: function()
            {
               task.unblock();
            },
            error: function()
            {
               task.fail();
            }
         });
      }
   };
   
   /**
    * Called when the save button is clicked in the editor.
    * 
    * @param event the event object.
    */
   var saveClicked = function(event)
   {
      bitmunk.log.debug(sLogCategory, 'saveClicked');
      
      // turn save button off
      $('#schemeSave')
         .removeClass('on')
         .addClass('off');
      
      // show saving indicator
      setActivityIndicator('Saving...');
      
      // clear feedback
      //bitmunk.settings.view.clearFeedback('general');
      
      // use a task to save configs
      bitmunk.task.start(
      {
         type: sLogCategory + '.save',
         run: function(task)
         {
            task.next('save scheme', saveScheme);
            task.next('save config', saveConfig);
         },
         success: function(task)
         {
            // turn save button on
            $('#schemeSave')
               .removeClass('off')
               .addClass('on');
      
            // hide saving indicator
            setActivityIndicator();
         },
         failure: function(task)
         {
            // turn save button on
            $('#schemeSave')
               .removeClass('off')
               .addClass('on');
      
            // hide saving indicator
            setActivityIndicator();
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
      bitmunk.medialibrary.paymentEditor.clearFeedback();
   };
   
   /**
    * Shows the payment scheme editor dialog with the given scheme and
    * user accounts.
    * 
    * @param scheme the payee scheme.
    * @param accounts the user accounts.
    */
   var show = function(scheme, accounts)
   {
      // show loading indicator
      setActivityIndicator('Loading...');
      
      // reset max position counter to 0 because no payees have been added yet
      sMaxPayeePosition = 0;
      
      // set all details for a new payee scheme
      if(!scheme)
      {
         // set dialog title
         $('#paymentEditor-title').text('Add Payment Scheme');
         
         // set default description and add a new payee row
         $('#schemeDescription').val('New Payment Scheme');
         addPayeeRow(accounts);
         $('#isAutoSellSchemeId').selected(false);
      }
      // set all details for an existing payee scheme
      else
      {
         // set dialog title
         $('#paymentEditor-title').text('Edit - ' + scheme.description);
         
         // set existing description
         $('#schemeDescription').val(scheme.description);
         
         // add existing payee rows
         $.each(scheme.payees, function(i, payee)
         {
            addPayeeRow(accounts, payee);
         });
         
         // set if scheme is auto-sell scheme
         var autoSellSchemeId =
            bitmunk.model.fetch(
               'config', 'config', 'bitmunk.catalog.CustomCatalog')
               .autoSell.payeeSchemeId;
         $('#isAutoSellScheme').selected(scheme.id == autoSellSchemeId);
      }
      
      // update the number of wares associated with this payment scheme
      updateSchemeAssociatedWares(scheme);
      
      // disable payee delete button
      bitmunk.medialibrary.view.setButton('#payeeDelete', false);
      
      // bind handlers
      $('#schemeClose').bind('click', closeClicked);
      $('#payeeAdd').bind('click', payeeAddClicked);
      $('#schemeSave').bind('click', saveClicked);
      $('#clearButton-payment').bind('click', feedbackClearClicked);
      
      // bind payeeSelectAll to check every ".payeeSelect"
      $('#payeeSelectAll').bind('click', function(e)
      {
         // select/unselect all payees based on master selector
         $('.payeeSelect').attr('checked', $(this).attr('checked'));
         
         // enable/disable payee delete button based on whether or not
         // any payees are checked
         bitmunk.medialibrary.view.setButton(
            '#payeeDelete', $('.payeeSelect:checked').length > 0,
            payeeDeleteClicked);
      });
      
      // live automatically binds to all current/future "payeeSelect" classes
      $('.payeeSelect').live('click', function(e)
      {
         // select-all will be checked if all select-rows are checked
         bitmunk.medialibrary.view.updateSelectAll(
            $('#payeeSelectAll'), $('.payeeSelect'));
         
         // enable/disable payee delete button based on whether or not
         // any payees are checked
         bitmunk.medialibrary.view.setButton(
            '#payeeDelete', $('.payeeSelect:checked').length > 0,
            payeeDeleteClicked);
      });
      
      // enable overlay and show editor
      $('#overlay').css('z-index', '150');
      $('#paymentEditor').show();
      
      // hide loading indicator
      setActivityIndicator();
   };
   
   /**
    * Hides the payment scheme editor dialog.
    */
   var hide = function()
   {
      // disable overload and hide editor
      $('#overlay').css('z-index', '10');
      $('#paymentEditor').hide();
      
      // clear description, payees, and feedback
      $('#schemeDescription').val('');
      clearPayees();
      bitmunk.medialibrary.paymentEditor.clearFeedback();
      
      // unbind handlers
      $('#schemeClose').unbind('click');
      $('#payeeAdd').unbind('click');
      $('#schemeSave').unbind('click');
      $('#clearButton-payment').unbind('click');
      $('#payeeSelectAll').unbind('click');
      $('.payeeSelect').die('click');
   };
   
   sScriptTask.unblock();
})(jQuery);

/**
 * Bitmunk Web UI --- Settings
 *
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 * @author Manu Sporny <msporny@digitalbazaar.com>
 */
(function($)
{
   // log category
   var sLogCategory = 'bitmunk.webui.Settings';
   
   var sScriptTask = bitmunk.resource.getScriptTask(
      'bitmunk.webui.Settings', 'settings.js');
   
   // event namespace
   var sNS = '.bitmunk-webui-Settings';
   
   // model and table names
   var sModel = 'config';
   var sTable = 'config';
   
   // Main Content Image URL
   var sContentImageUrl = '/bitmunk/images/';
   
   // stores the directory browser
   var sDirectoryBrowser = null;
   
   // common merge key
   var MERGE = '_merge_';
   
   /** Settings View **/
   
   // Settings View namespace
   bitmunk.settings = bitmunk.settings || {};
   bitmunk.settings.view =
   {
      /**
       * Sets the visibility of the directory dialog.
       * 
       * @param show true to show the dialog, false to hide it.
       */
      showDirectoryDialog: function(show)
      {
         // toggle the overlay and directory dialog
         $('#overlay').toggle(show);
         $('#directoryDialog').toggle(show);
         if(show)
         {
            sDirectoryBrowser.show();
         }
         else
         {
            sDirectoryBrowser.hide();
         }
      },
      
      /**
       * Adds a feedback message to the UI.
       * 
       * @param name the name of the feedback widget.
       * @param type the type of message.
       * @param message the message to display.
       */
      addFeedback: function(name, type, message)
      {
         // FIXME: debug output
         //bitmunk.log.debug(sLogCategory, 'Adding ' + type + ' message');
         
         var feedback = $('<p class="' + type + '">' + message + '</p>');
         $('#feedback-' + name).prepend(feedback);

         // show feedback box
         $('#feedbackBox-' + name).slideDown('fast');
      },
      
      /**
       * Clears feedback messages.
       * 
       * @param name the name of the feedback widget.
       */
      clearFeedback: function(name)
      {
         // clear feedback
         $('#feedback-' + name).empty();
         
         // hide feedback box
         $('#feedbackBox-' + name).slideUp('fast');
      }
   };
   
   /** Settings Controller **/
   
   bitmunk.settings.controller =
   {
      /**
       * Called when a directory is selected in the directory browser.
       * 
       * @param path the path name of the file that was selected.
       */
      directorySelected: function(path)
      {
         // DEBUG: output
         bitmunk.log.debug(sLogCategory, 'Directory Selected: ' + path);
         
         // set element
         $('#' + sDirectoryBrowser.elementIdToSet)[0].value = path;
         
         // clear selection and hide file browser
         sDirectoryBrowser.clearSelection();
         bitmunk.settings.view.showDirectoryDialog(false);
      }
   };
   
   /**
    * Gets the purchase settings from the HTML on the page.
    *
    * @return a settings object that can be transmitted to the config service.
    */
   var getHtmlPurchaseSettings = function() 
   {
      return {
         'bitmunk.purchase.Purchase':
         {
            'downloadPath': $('#downloadPath')[0].value,
            'temporaryPath': $('#temporaryPath')[0].value,
            'maxDownloadRate': $('#maxDownloadRate')[0].value * 1024,
            'maxExcessBandwidth': $('#maxExcessBandwidth')[0].value * 1024,
            'maxPieces': +$('#maxPieces')[0].value
         }
      };
   };

   /**
    * Writes the settings from a purchase settings object into the HTML page.
    */
   var updateHtmlPurchaseSettings = function() 
   {
      var cfg = bitmunk.model.fetch(
         sModel, sTable, 'bitmunk.purchase.Purchase');
      
      //bitmunk.log.debug(sLogCategory, 'putsettings', cfg.settings);
      $('#downloadPath')[0].value = cfg.downloadPath;
      $('#temporaryPath')[0].value = cfg.temporaryPath;
      $('#maxDownloadRate')[0].value = cfg.maxDownloadRate / 1024;
      $('#maxExcessBandwidth')[0].value = cfg.maxExcessBandwidth / 1024;
      $('#maxPieces')[0].value = cfg.maxPieces;
   };

   /**
    * Gets the sell settings from the HTML on the page.
    *
    * @return a settings object that can be transmitted to the config service.
    */
   var getHtmlSellSettings = function() 
   {
      return {
         'bitmunk.sell.Sell':
         {
            'maxUploadRate': $('#maxUploadRate')[0].value * 1024
         }
      };
   };

   /**
    * Writes the settings from a sell settings object into the HTML page.
    */
   var updateHtmlSellSettings = function() 
   {
      var cfg = bitmunk.model.fetch(sModel, sTable, 'bitmunk.sell.Sell');
      
      $('#maxUploadRate')[0].value = cfg.maxUploadRate / 1024;
   };

   /**
    * Gets the webui settings from the HTML on the page.
    *
    * @return a settings object that can be transmitted to the config service.
    */
   var getHtmlWebUiSettings = function() 
   {
      return {
         'bitmunk.webui.WebUi': {
            'cache': {
               'resources': ($('#cacheResources').fieldValue().length > 0)
            },
            'loggers': {
               'console': {
                  'level': $('#consoleLogLevel option:selected').val()
               },
               'log': {
                  'capacity': +$('#mainLogCapacity').val(),
                  'level': $('#mainLogLevel option:selected').val()
               }
            }
         }
      };
   };

   /**
    * Writes the settings from a webui settings object into the HTML page.
    */
   var updateHtmlWebUiSettings = function() 
   {
      var cfg = bitmunk.model.fetch(sModel, sTable, 'bitmunk.webui.WebUi');
      
      var consoleLevel = cfg.loggers.console.level;
      $('#consoleLogLevel option[value="' + consoleLevel + '"]').selected();
      var mainLevel = cfg.loggers.log.level;
      $('#mainLogLevel option[value="' + mainLevel + '"]').selected();
      var mainCapacity = cfg.loggers.log.capacity;
      $('#mainLogCapacity')[0].value = mainCapacity;
      var cacheResources = cfg.cache.resources;
      $('#cacheResources').selected(cacheResources);
   };
   
   /**
    * Gets the catalog settings from the HTML on the page.
    *
    * @return a settings object that can be transmitted to the config service.
    */
   var getHtmlCatalogSettings = function() 
   {
      return {
         'bitmunk.catalog.CustomCatalog': {
            'autoSell': {
               'enabled': ($('#enableAutoSell').fieldValue().length > 0)
            }
         }
      };
   };

   /**
    * Writes the settings from a catalog settings object into the HTML page.
    */
   var updateHtmlCatalogSettings = function() 
   {
      var cfg = bitmunk.model.fetch(
         sModel, sTable, 'bitmunk.catalog.CustomCatalog');
      
      $('#enableAutoSell').selected(cfg.autoSell.enabled);
   };
   
   /**
    * Gets the system user catalog settings from the HTML on the page.
    *
    * @return a settings object that can be transmitted to the config service.
    */
   var getHtmlSystemUserCatalogSettings = function() 
   {
      return {
         'bitmunk.catalog.CustomCatalog': {
            'portMapping': {
               'enabled': ($('#enableUPnP').fieldValue().length > 0)
               // FIXME: add support for more than one port
               //'externalPort': +$('#externalPort').val()
            }
         }
      };
   };

   /**
    * Writes the system user settings from a catalog settings object into the
    * HTML page.
    */
   var updateHtmlSystemUserCatalogSettings = function() 
   {
      var cfg = bitmunk.model.fetch(
         sModel, sTable, 'system user:bitmunk.catalog.CustomCatalog');
      
      $('#enableUPnP').selected(cfg.portMapping.enabled);
      // FIXME: add support for external port
      //$('#externalPort')[0].value = cfg.portMapping.externalPort;
   };
   
   /**
    * Gets the system user node settings from the HTML on the page.
    *
    * @return a settings object that can be transmitted to the config service.
    */
   var getHtmlSystemUserNodeSettings = function() 
   {
      // FIXME: add support for external port
      return {
         'node': {
            'port': +$('#externalPort').val()
         }
      };
   };

   /**
    * Writes the system user settings from a catalog settings object into the
    * HTML page.
    */
   var updateHtmlSystemUserNodeSettings = function() 
   {
      // FIXME: add support for external port
      var cfg = bitmunk.model.fetch(
         sModel, sTable, 'system user:node');
      $('#externalPort')[0].value = cfg.port;
   };
   
   /**
    * This method is called when a general error is detected. It will update
    * the UI to display the appropriate error information.
    *
    * Note the global error handler may handle general error conditions.
    * 
    * @param xhr the XMLHttpRequest object that caused the error.
    * @param textStatus the text status of the HTTP request.
    * @param errorThrown the error that was thrown.
    * @param callbackData data that was added to the BTP call as extra 
    *                     information to be used for debugging.
    */
   var generalErrorOccurred =
      function(xhr, textStatus, errorThrown, callbackData)
   {
      if(callbackData && 'errorMessage' in callbackData)
      {
         bitmunk.settings.view.addFeedback(
            'general', 'error', callbackData.errorMessage);
      }
      else
      {
         bitmunk.settings.view.addFeedback(
            'general', 'error',
            'There was an error while modifying the configuration.');
      }
      
      // turn save button on
      $('#saveButton')
         .removeClass('off')
         .addClass('on')
         // bind save button click
         .one('click', saveClicked);
   };
   
   /**
    * Callback when either directory select button is clicked.
    */
   var selectClicked = function(event)
   {
      // DEBUG: output
      bitmunk.log.debug(sLogCategory, 'Directory Select Clicked');
      
      // event.data.dir refers to the initial directory to use
      var cfg = bitmunk.model.fetch(
         sModel, sTable, 'bitmunk.purchase.Purchase');
      var path = cfg[event.data.dir];
      
      // create the directory browser if it doesn't exist
      if(!sDirectoryBrowser)
      {
         sDirectoryBrowser = bitmunk.filebrowser.create({
            id: 'directoryBrowser',
            path: path,
            selectable: ['directory'],
            directorySelected: function(path)
            {
               bitmunk.settings.controller.directorySelected(path);
            }
         });
      }
      else
      {
         // set directory
         sDirectoryBrowser.setDirectory(path);
      }
      
      // save element to be set
      sDirectoryBrowser.elementIdToSet = event.data.dir;
      
      // show directory dialog
      bitmunk.settings.view.showDirectoryDialog(true);
   };
   
   /**
    * Callback when the directory dialog close button is clicked.
    */
   var directoryCloseClicked = function(event)
   {
      // DEBUG: output
      bitmunk.log.debug(sLogCategory, 'Directory Close Clicked');
      
      // hide directory dialog
      bitmunk.settings.view.showDirectoryDialog(false);
   };
   
   /**
    * Callback when the save button is clicked.
    *
    * @param eventObject the event object associated with the save button
    *                    click event.
    */
   var saveClicked = function(eventObject)
   {
      // DEBUG: output
      //bitmunk.log.debug(sLogCategory, 'Save Button Clicked');
      
      // turn save button off
      $('#saveButton')
         .removeClass('on')
         .addClass('off');
      
      // clear feedback
      bitmunk.settings.view.clearFeedback('general');
      
      // use a task to save configs
      bitmunk.task.start(
      {
         type: sLogCategory + '.saveConfigs',
         run: function(task)
         {
            task.next('save user config', function(task)
            {
               task.block();
               
               // get raw config
               var cfg = bitmunk.model.fetch(sModel, sTable, 'raw');
               
               // merge the page data into the raw configuration      
               if(!(MERGE in cfg))
               {
                  cfg[MERGE] = {};
               }
               cfg[MERGE] =
                  $.extend(true, cfg[MERGE], getHtmlPurchaseSettings());
               cfg[MERGE] =
                  $.extend(true, cfg[MERGE], getHtmlSellSettings());
               cfg[MERGE] =
                  $.extend(true, cfg[MERGE], getHtmlWebUiSettings());
               cfg[MERGE] =
                  $.extend(true, cfg[MERGE], getHtmlCatalogSettings());
               
               // DEBUG: output
               //bitmunk.log.debug(sLogCategory,
               //   'Raw merged config:' + JSON.stringify(cfg, null, 3));
               
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
            });
            
            task.next('save system user config', function(task)
            {
               task.block();
               
               // get node config for old port value
               var nodeCfg = bitmunk.model.fetch(
                  sModel, sTable, 'system user:node');
               var oldPort =
                  bitmunk.util.getPath(nodeCfg, ['port']);
               
               // get raw main config
               var cfg = bitmunk.model.fetch(
                  sModel, sTable, 'system user:raw');
               
               // merge the page data into the raw configuration      
               if(!(MERGE in cfg))
               {
                  cfg[MERGE] = {};
               }
               cfg[MERGE] =
                  $.extend(
                     true, cfg[MERGE], getHtmlSystemUserCatalogSettings());
               cfg[MERGE] =
                  $.extend(
                     true, cfg[MERGE], getHtmlSystemUserNodeSettings());
               
               var newPort =
                  bitmunk.util.getPath(cfg, [MERGE, 'node', 'port']);
               var restart = oldPort != newPort;
               
               // save the settings
               bitmunk.api.proxy(
               {
                  method: 'POST',
                  url: bitmunk.api.localRoot + 'system/config',
                  params:
                  {
                     nodeuser: bitmunk.user.getUserId(),
                     restart: restart
                  },
                  data: cfg,
                  success: function()
                  {
                     if(restart)
                     {
                        $.event.trigger('bitmunk-offline', [{
                           message: 'Restarted due to port change.',
                           port: newPort
                        }]);
                     }
                     task.unblock();
                  },
                  error: function()
                  {
                     task.fail();
                  }
               });
            });
         },
         success: function(task)
         {
            // DEBUG: output
            //bitmunk.log.debug(sLogCategory, 'Settings saved.');
            
            // turn save button on
            $('#saveButton')
               .removeClass('off')
               .addClass('on');
            
            // bind save button click
            $('#saveButton').one('click', saveClicked);
            
            // display info message about save progress
            bitmunk.settings.view.addFeedback(
               'general', 'info', 'Configuration settings have been saved.');
         },
         failure: function(task)
         {
            // FIXME: expects xhr error params
            generalErrorOccurred();
         }
      });
   };
   
   /** jQuery Page Functions **/
   
   /**
    * Called whenever the UI is shown.
    */
   var show = function(task)
   {
      // update all configs
      updateHtmlPurchaseSettings();
      updateHtmlSellSettings();
      updateHtmlWebUiSettings();
      updateHtmlCatalogSettings();
      updateHtmlSystemUserCatalogSettings();
      updateHtmlSystemUserNodeSettings();
      
      // turn save button on
      $('#saveButton')
         .removeClass('off')
         .addClass('on');
      
      // bind save button click
      $('#saveButton').one('click', saveClicked);
      
      // bind directory select buttons
      $('#downloadSelect').bind('click', {dir: 'downloadPath'}, selectClicked);
      $('#tempSelect').bind('click', {dir: 'temporaryPath'}, selectClicked);
      
      // bind directory dialog close button
      $('#directoryClose').bind('click', directoryCloseClicked);
      
      // bind to receive config change events
      $(bitmunk.settings)
         .bind('bitmunk-purchase-Purchase-config-changed' + sNS,
            updateHtmlPurchaseSettings)
         .bind('bitmunk-sell-Sell-config-changed' + sNS,
            updateHtmlSellSettings)
         .bind('bitmunk-webui-WebUi-config-changed' + sNS,
            updateHtmlWebUiSettings)
         .bind('bitmunk-catalog-CustomCatalog-config-changed' + sNS,
            updateHtmlCatalogSettings)
         .bind('bitmunk-catalog-CustomCatalog-system-user-config-changed' + sNS,
            updateHtmlSystemUserCatalogSettings)
         .bind('bitmunk-node-system-user-config-changed' + sNS,
            updateHtmlSystemUserNodeSettings);
      
      // bind log info buttons
      var infoClicked = function()
      {
         $('#logInfo').toggleClass('hidden');
      };
      $('#mainLogInfo').bind('click' + sNS, infoClicked);
      $('#consoleLogInfo').bind('click' + sNS, infoClicked);
   };
   
   /**
    * Called whenever the UI is hidden.
    */   
   var hide = function(task)
   {
      // unbind all settings events
      $(bitmunk.settings).unbind(sNS);
      
      // clear feedback
      bitmunk.settings.view.clearFeedback('general');
   };
   
   bitmunk.resource.setupResource(
   {
      pluginId: 'bitmunk.webui.Settings',
      resourceId: 'settings',
      models: ['config'],
      show: show,
      hide: hide
   });
   sScriptTask.unblock();
})(jQuery);

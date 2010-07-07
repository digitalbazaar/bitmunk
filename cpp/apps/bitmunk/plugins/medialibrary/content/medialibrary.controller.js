/**
 * Bitmunk Media Library Controller
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 * 
 * Right now this controls the media library and the catalog. In the future
 * we might split these two apart if we decide the catalog should be an
 * optional feature that adds on to the Media Library.
 * 
 * @author Mike Johnson
 * @author Manu Sporny
 * @author Dave Longley
 */
(function($) {

var init = function(task)
{
   // log category
   var sLogCategory = 'bitmunk.medialibrary.controller';
   
   // event namespace
   var sNS = '.bitmunk-medialibrary-controller';
   
   // urls to delete media library files and catalog wares
   var sFilesUrl = bitmunk.api.localRoot32 + 'medialibrary/files';
   var sWaresUrl = bitmunk.api.localRoot + 'catalog/wares';
   
   // default pagination parameters
   var sDefaults =
   {
      query: null,
      page: 1,
      num: 10,
      order: 'title',
      direction: 'asc'
   };
   
   // current pagination parameters
   var sParams =
   {
      query: null,
      page: 1,
      num: 10,
      order: 'title',
      direction: 'asc'
   };
   
   /** Public Media Library Controller namespace **/
   bitmunk.medialibrary.controller = {};
   
   /**
    * Via a task, this method loads and shows the current subset of files
    * in the user's Media Library.
    * 
    * @param task the task to use when loading the library browser.
    */
   bitmunk.medialibrary.controller.loadBrowser = function(task)
   {
      // clear ware rows from UI
      task.next('MediaLibrary Prepare UI', function(task)
      {
         // DEBUG: output
         bitmunk.log.debug(sLogCategory, 'Prepare UI');
         
         // clear library browser
         bitmunk.medialibrary.view.clearBrowserRows();
         
         // show loading indicator
         bitmunk.medialibrary.view.showBrowserLoading(true);
      });
      
      // load files from media library
      task.next('MediaLibrary Fetch Files', function(task)
      {
         // FIXME: if you use page parameters to fetch a range of files
         // and then use the same page parameters to try to get a slice
         // of those files then you will get nothing
         
         // build the query options
         // FIXME: fake query options for now
         var options = 
         {
            start: 0,
            num: 20,
            query: sParams.query,
            order: sParams.order,
            /*
            start: (sParams.page - 1) * sParams.num,
            num: sParams.num,
            query: sParams.query,
            order: sParams.order
            */
            clearModel: true
         };
         
         // FIXME: in the future we will want to load more than
         // one page of files into the model
         bitmunk.medialibrary.model.updateFiles(null, task, options);
      });
      
      // watch the files in the media library for changes
      task.next('MediaLibrary Files Watch', function(task)
      {
         // watch file table
         bitmunk.model.watch(
            bitmunk.medialibrary.model.name,
            bitmunk.medialibrary.model.fileTable);
      });
      
      // add the rows to the view
      task.next('MediaLibrary Add Rows', function(task)
      {
         // update page control
         sParams.total = bitmunk.medialibrary.model.totalFiles;
         bitmunk.medialibrary.view.setPageControl(sParams);

         // update the contents of the media library display
         if(sParams.total > 0)
         {
            // fetch a slice of the files that were added to the model
            var files = bitmunk.model.slice(
               bitmunk.medialibrary.model.name,
               bitmunk.medialibrary.model.fileTable,
               (sParams.page - 1) * sParams.num, sParams.num);

            // add each row
            $.each(files, function(index, file)
            {
               // DEBUG: output
               bitmunk.log.debug(sLogCategory, 'Add Row: ' + file.id);
               
               // get media
               var media = bitmunk.model.fetch(
                  bitmunk.medialibrary.model.name,
                  bitmunk.medialibrary.model.mediaTable,
                  file.mediaId);
               
               // add the file/media information as a library row
               bitmunk.medialibrary.view.addBrowserRow(file, media);
            });
         }
         else
         {
            bitmunk.medialibrary.view.setBrowserMessage(
               'Your Media Library is empty. ' +
               'Add new content to begin selling.');
         }
      });
      
      // load the wares from the catalog
      task.next('MediaLibrary Fetch Wares', function(task)
      {
         // get the files from the model 
         var fileIds = [];
         
         // fetch a slice of the files that were added to the model
         var files = bitmunk.model.slice(
            bitmunk.medialibrary.model.name,
            bitmunk.medialibrary.model.fileTable,
            (sParams.page - 1) * sParams.num, sParams.num);
         
         // add each file ID to the list of fileIds to fetch
         $.each(files, function(index, file)
         {
            fileIds.push(file.id);
         });
         
         // FIXME: Remove debugging info before launch
         bitmunk.log.debug(
            sLogCategory, 'loadBrowser: model.updateWares', fileIds);
         
         // fetch the wares associated with the given files
         bitmunk.catalog.model.updateWares(fileIds, task);
      });

      // watch the wares in the catalog for changes
      task.next('MediaLibrary Wares Watch', function(task)
      {
         // watch file table
         bitmunk.model.watch(
            bitmunk.catalog.model.name,
            bitmunk.catalog.model.wareTable);
      });

      // load payee schemes
      task.next('MediaLibrary Fetch Payment', function(task)
      {
         bitmunk.catalog.model.updatePayeeSchemes(null, task);
      });
      
      task.next('MediaLibrary Payment Watch', function(task)
      {
         // watch the payee schemes table
         bitmunk.model.watch(
            bitmunk.catalog.model.name,
            bitmunk.catalog.model.payeeSchemeTable);
      });
      
      task.next('MediaLibrary Update Ware Rows', function(task)
      {
         // update all of the rows in the UI
         updateWareRows();
      });
      
      // make final display changes to UI
      task.next('MediaLibrary Finalize UI', function(task)
      {
         // hide loading indicator
         bitmunk.medialibrary.view.showBrowserLoading(false);
      });
   };
   
   /**
    * Starts a task to reload the library browser.
    */
   bitmunk.medialibrary.controller.reloadBrowser = function()
   {
      // reload library to display latest changes
      bitmunk.task.start(
      {
         type: sLogCategory + '.loadBrowser',
         run: bitmunk.medialibrary.controller.loadBrowser
      });
   };
   
   /**
    * Resync the user's catalog with bitmunk.
    */
   bitmunk.medialibrary.controller.resyncCatalog = function()
   {
      // send event
      bitmunk.events.sendEvents(
      {
         send: [{
            type: 'bitmunk.common.SellerListing.sync',
            details: { userId: bitmunk.user.getUserId() }
         }]
      });
   };
   
   /**
    * Deletes the wares indicated by the file IDs.
    * 
    * @param wareIds the IDs of the wares.
    * @param removeFiles true to delete the associated files as well.
    */
   bitmunk.medialibrary.controller.deleteWares = function(wareIds, removeFiles)
   {
      bitmunk.log.debug(
         sLogCategory, 'deleteWares,removeFiles', wareIds, removeFiles);
      
      // use a task to delete wares
      bitmunk.task.start(
      {
         type: sLogCategory + '.deleteWares',
         run: function(task)
         {
            // for storing parallel tasks
            var tasks = [];
            
            task.next(function(task)
            {
               // do deletes in parallel
               $.each(wareIds, function(i, wareId)
               {
                  tasks.push(function(task)
                  {
                     deleteWare(wareId, removeFiles, task);
                  });
               });
               task.parallel(tasks);
            });
            
            task.next(function(task)
            {
               // check tasks for failure
               var exceptions = [];
               $.each(tasks, function(i, t)
               {
                  if(t.userData !== null)
                  {
                     // user data contains an exception
                     exceptions.push(t.userData);
                  }
               });
               
               if(exceptions.length > 0)
               {
                  task.userData = exceptions;
                  task.fail();
               }
            });
         },
         success: function(task)
         {
            bitmunk.medialibrary.controller.reloadBrowser();
         },
         failure: function(task)
         {
            // FIXME: add feedback some how for failures
            // task.userData may contain an array of exceptions or
            // an exception
            bitmunk.medialibrary.controller.reloadBrowser();
         }
      });
   };
   
   /**
    * Removes a specific file entry from the media library.
    * 
    * FIXME: optionally allow them to remove file from disk?
    * 
    * @param fileId the ID of the file to remove.
    * @param task the task to use to synchronize the update.
    */
   bitmunk.medialibrary.controller.removeFile = function(fileId, task)
   {
      bitmunk.log.debug(sLogCategory, 'removeFile', fileId);
      
      task.block();
      bitmunk.api.proxy(
      {
         method: 'DELETE',
         url: sFilesUrl + '/' + fileId,
         params: { nodeuser: bitmunk.user.getUserId() },
         success: function(data, status)
         {
            task.unblock();
         },
         error: function(xhr, textStatus, errorThrown)
         {
            try
            {
               // use exception from server
               task.userData = JSON.parse(xhr.responseText);
            }
            catch(e)
            {
               // use error message
               task.userData = { message: textStatus };
            }
            task.fail();
         }
      });
   };
   
   /**
    * Called when the ware editor is closed in the view.
    * 
    * @param reload true if a reload of the library browser is necessary,
    *        false if not.
    * @param fileId the related file ID.
    * @param mediaId the related media ID.
    */
   bitmunk.medialibrary.controller.wareEditorClosed = function(
      reload, fileId, mediaId)
   {
      var file = fileId ? bitmunk.model.fetch(
         bitmunk.medialibrary.model.name,
         bitmunk.medialibrary.model.fileTable, fileId) : null;
      var ware = file ? bitmunk.model.fetch(
         bitmunk.catalog.model.name,
         bitmunk.catalog.model.wareTable,
         'bitmunk:file:' + file.mediaId + '-' + file.id) : null;
      var scheme = ware ? bitmunk.model.fetch(
         bitmunk.catalog.model.name,
         bitmunk.catalog.model.payeeSchemeTable,
         ware.payeeSchemeId) : null;
      
      bitmunk.log.debug(
         sLogCategory, 'wareEditorClosed', ware, file, scheme);
      if(reload)
      {
         // reload library to display latest changes
         // (necessary to get newly added files in the correct sort order)
         bitmunk.medialibrary.controller.reloadBrowser();
      }
   };
   
   /** Private functions **/
   
   var updateWareRows = function()
   {
      // fetch a slice of the files that were added to the model
      var files = bitmunk.model.slice(
         bitmunk.medialibrary.model.name,
         bitmunk.medialibrary.model.fileTable,
         (sParams.page - 1) * sParams.num, sParams.num);
      
      // update the ware info in the row associated with each file
      $.each(files, function(index, file)
      {
         var scheme = null;
         var wareId = file.mediaId + '-' + file.id;
         var ware = bitmunk.model.fetch(
            bitmunk.catalog.model.name,
            bitmunk.catalog.model.wareTable,
            'bitmunk:file:' + wareId);
         if(ware)
         {
            scheme = bitmunk.model.fetch(
               bitmunk.catalog.model.name,
               bitmunk.catalog.model.payeeSchemeTable,
               ware.payeeSchemeId);
         }
         
         bitmunk.medialibrary.view.updateBrowserRow(wareId, file, ware, scheme);
      });
   };
   
   /**
    * Removes a ware from a user's catalog.
    * 
    * @param wareId the ID of the ware to delete.
    * @param removeFile true to delete the associated media library file as 
    *                   well.
    * @param task the task to use to synchronize the delete.
    */
   var deleteWare = function(wareId, removeFile, task)
   {
      bitmunk.log.debug(
         sLogCategory, 'deleteWare,removeFile', wareId, removeFile);
      
      task.next(function(task)
      {
         // create fully qualified URL-encoded ware ID
         var fqWareId = escape('bitmunk:file:' + wareId);
         
         task.block();
         bitmunk.api.proxy(
         {
            method: 'DELETE',
            url: sWaresUrl + '/' + fqWareId,
            params: { nodeuser: bitmunk.user.getUserId() },
            success: function(data, status)
            {
               task.unblock();
            },
            error: function(xhr, textStatus, errorThrown)
            {
               try
               {
                  // use exception from server
                  var ex = JSON.parse(xhr.responseText);
                  
                  // do not fail task if ware was simply not found
                  if(ex.type == 'bitmunk.catalog.database.WareNotFound')
                  {
                     task.unblock();
                  }
                  else
                  {
                     task.userData = ex;
                     task.fail();
                  }
               }
               catch(e)
               {
                  // use error message
                  task.userData = { message: textStatus };
                  task.fail();
               }
            }
         });
      });
      
      if(removeFile)
      {
         task.next(function(task)
         {
            // delete file as well
            var fileId = wareId.split('-', 2)[1];
            bitmunk.medialibrary.controller.removeFile(fileId, task);
         });
      }
   };
   
   /**
    * Called when a file is updated on the backend.
    * 
    * @param event the event object.
    * @param fileId the file ID associated with the event.
    * @param mediaId the media ID associated with the event.
    */
   var fileUpdated = function(event, fileId, mediaId)
   {
      var file = bitmunk.model.fetch(
         bitmunk.medialibrary.model.name,
         bitmunk.medialibrary.model.fileTable, fileId);
      var ware = file ? bitmunk.model.fetch(
         bitmunk.catalog.model.name,
         bitmunk.catalog.model.wareTable,
         'bitmunk:file:' + file.mediaId + '-' + file.id) : null;
      var scheme = ware ? bitmunk.model.fetch(
         bitmunk.catalog.model.name,
         bitmunk.catalog.model.payeeSchemeTable,
         ware.payeeSchemeId) : null;
      
      var wareId = file ?
         (file.mediaId + '-' + file.id) : mediaId + '-' + fileId;
      bitmunk.log.debug(sLogCategory, event.type, ware, file, scheme);
      bitmunk.medialibrary.view.updateBrowserRow(wareId, file, ware, scheme);
   };
   
   /**
    * Called when a file exception occurs on the backend.
    * 
    * @param event the event object.
    * @param fileInfo the file info associated with the event.
    * @param exception the exception object that generated the event.
    */
   var fileException = function(event, fileInfo, exception)
   {
      var file = fileInfo.id ? bitmunk.model.fetch(
         bitmunk.medialibrary.model.name,
         bitmunk.medialibrary.model.fileTable,
         fileInfo.id) : null;
      var ware = file ? bitmunk.model.fetch(
         bitmunk.catalog.model.name,
         bitmunk.catalog.model.wareTable,
         'bitmunk:file:' + file.mediaId + '-' + file.id) : null;
      bitmunk.log.debug(sLogCategory, event.type, file, ware, exception);
      if(ware)
      {
         // set status and add feedback for ware
         var wareId = file.mediaId + '-' + file.id;
         bitmunk.medialibrary.view.setWareStatus(wareId, 'Error', 'error');
         bitmunk.medialibrary.view.addWareFeedback(
            file.id, 'error', exception.cause ?
            exception.cause.message : exception.message);
      }
   };
   
   /**
    * Called when a ware is updated on the backend.
    * 
    * @param event the event object.
    * @param wareId the ware ID associated with the event.
    */
   var wareUpdated = function(event, wareId)
   {
      // 'bitmunk:file:' is 13 chars long
      var shortId = wareId.substring(13);
      var ids = shortId.split('-', 2);
      var file = bitmunk.model.fetch(
         bitmunk.medialibrary.model.name,
         bitmunk.medialibrary.model.fileTable, ids[1]);
      var ware = file ? bitmunk.model.fetch(
         bitmunk.catalog.model.name,
         bitmunk.catalog.model.wareTable,
         'bitmunk:file:' + file.mediaId + '-' + file.id) : null;
      var scheme = ware ? bitmunk.model.fetch(
         bitmunk.catalog.model.name,
         bitmunk.catalog.model.payeeSchemeTable,
         ware.payeeSchemeId) : null;
      
      bitmunk.log.debug(sLogCategory, event.type, ware, file, scheme);
      bitmunk.medialibrary.view.updateBrowserRow(shortId, file, ware, scheme);
   };

   /**
    * Called when a ware exception occurs on the backend.
    * 
    * @param event the event object.
    * @param wareId the ware ID associated with the event.
    */
   var wareException = function(event, wareId)
   {
      // 'bitmunk:file:' is 13 chars long
      var shortId = wareId.substring(13);
      var ids = shortId.split('-', 2);
      var ware = bitmunk.model.fetch(
         bitmunk.catalog.model.name,
         bitmunk.catalog.model.wareTable,
         wareId);
      bitmunk.log.debug(sLogCategory, event.type, wareId, ware, ids);
      if(ware)
      {
         // set status and add feedback for ware
         bitmunk.medialibrary.view.setWareStatus(shortId, 'Error', 'error');
         bitmunk.medialibrary.view.addWareFeedback(
            ids[1], 'error', ware.exception.cause ?
            ware.exception.cause.message : ware.exception.message);
      }
   };
   
   /**
    * Called when a payee scheme exception occurs on the backend.
    * 
    * @param event the event object.
    * @param psId the payee scheme ID associated with the event.
    */
   var payeeSchemeException = function(event, psId)
   {
      var ps = bitmunk.model.fetch(
         bitmunk.catalog.model.name,
         bitmunk.catalog.model.payeeSchemeTable,
         psId);

      bitmunk.log.debug(sLogCategory, event.type, psId, ps);

      if(ps)
      {
         // add feedback for payee scheme
         bitmunk.medialibrary.paymentEditor.addFeedback(
            ps.id, 'error', ps.exception.cause ?
            ps.exception.cause.message : ps.exception.message);
      }
   };
   
   /** Bitmunk Framework **/
   
   /**
    * The show function is called whenever the page is shown. 
    */
   var show = function(task)
   {
      bitmunk.log.debug(sLogCategory, 'show');
      
      // FIXME: create function to set params in namespace
      // FIXME: update loadBrowser call to use parameters
      
      // set request parameters
      var request = task.userData.request;
      sParams.query =
         request.getQueryLast('query', sDefaults.query);
      sParams.page =
         parseInt(request.getQueryLast('page', sDefaults.page), 10);
      sParams.num =
         parseInt(request.getQueryLast('num', sDefaults.num), 10);
      sParams.order =
         request.getQueryLast('order', sDefaults.order);
      sParams.direction = 
         request.getQueryLast('direction', sDefaults.direction);
      
      // bind to model events
      $(bitmunk.medialibrary.controller)
         .bind('bitmunk-medialibrary-File-updated' + sNS, fileUpdated)
         .bind('bitmunk-medialibrary-File-exception' + sNS, fileException)
         /*
         .bind('bitmunk-medialibrary-Media-updated' + sNS, mediaUpdated)
         .bind('bitmunk-medialibrary-Media-removed' + sNS, mediaRemoved)
         */
         .bind('bitmunk-common-Ware-added' + sNS, wareUpdated)
         .bind('bitmunk-common-Ware-updated' + sNS, wareUpdated)
         .bind('bitmunk-common-Ware-removed' + sNS, wareUpdated)
         .bind('bitmunk-common-Ware-exception' + sNS, wareException)
         .bind('bitmunk-common-PayeeScheme-exception' + sNS,
            payeeSchemeException);
      
      // show the view
      bitmunk.medialibrary.view.show();
      
      // load current page of library results
      task.next('MediaLibrary', bitmunk.medialibrary.controller.loadBrowser);
   };
   
   /**
    * The hide function is called whenever the page is hidden.
    */
   var hide = function(task)
   {
      // hide the view
      bitmunk.medialibrary.view.hide();
      
      // unbind events 
      $(bitmunk.medialibrary.controller).unbind(sNS);
   };
   
   // extend controller resource
   bitmunk.resource.extendResource(
   {
      pluginId: 'bitmunk.webui.MediaLibrary',
      resourceId: 'medialibrary',
      models: ['config', 'accounts', 'medialibrary', 'catalog'],
      show: show,
      hide: hide
   });
};

bitmunk.resource.registerScript({
   pluginId: 'bitmunk.webui.MediaLibrary',
   resourceId: 'medialibrary.controller.js',
   depends: {},
   init: init
});

})(jQuery);

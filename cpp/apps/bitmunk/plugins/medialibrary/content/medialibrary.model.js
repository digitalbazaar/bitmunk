/**
 * Bitmunk Media Library Model
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 *
 * @author Mike Johnson
 * @author Manu Sporny
 * @author Dave Longley
 */
(function($)
{
   // logging category
   var sLogCategory = 'models.medialibrary';
   
   var sScriptTask = bitmunk.resource.getScriptTask(
      'bitmunk.webui.MediaLibrary', 'medialibrary.model.js');
   
   // URL to access/modify media library files
   var sFilesUrl = bitmunk.api.localRoot32 + 'medialibrary/files';
   
   // MediaLibrary Model namespace
   bitmunk.medialibrary = bitmunk.medialibrary || {};
   bitmunk.medialibrary.model =
   {
      name: 'medialibrary',
      
      /**
       * The "file" table contains file infos:
       * 
       * file
       * {
       *    <fileId>:
       *    {
       *       "id": the ID of the file info.
       *       "path": the path of the file located on the local disk.
       *       "mediaId": the ID of the media associated with this file.
       *    }
       * }
       */
      fileTable: 'file',
      
      /**
       * The total number of files in the media library.
       */
      totalFiles: 0,
      
      /**
       * The "media" table contains media and contributors:
       * 
       * media
       * {
       *    <mediaId>:
       *    {
       *       "id": the ID of the media.
       *       "title": the title of the media.
       *       "type": the type of the media.
       *       "contributors":
       *       {
       *          <contributorType>:
       *          [
       *             <contributorName>,
       *             ...
       *          ]
       *       }
       *    }
       * }
       */
      mediaTable: 'media'
   };
   
   /**
    * Updates the files and media in the media library.
    *
    * @param fileIds a list of file IDs to fetch from the backend, or null
    *                if all file IDs should be fetched from the backend.
    * @param task a task that is associated with the given function.
    * @param options an options object that contains start, num, query,
    *                order, and optionally, a flag for clearing the file
    *                table and a sequence number to use for inserting
    *                the files into the model.
    */
   bitmunk.medialibrary.model.updateFiles = function(fileIds, task, options)
   {
      // set the default options
      options = $.extend({
         seqNum: 0,
         clearModel: false
         }, options || {});
      
      // create the files and media params
      var params = $.extend({
         media: 'true',
         nodeuser: bitmunk.user.getUserId()
      }, 
      { 
         start: options.start, 
         num: options.num, 
         query: options.query, 
         order: options.order
      });
      if(fileIds && (fileIds.length > 0))
      {
         params.id = fileIds; 
      }
      
      // start blocking if a task was given
      if(task)
      {
         task.block();
      }
      
      // perform the call to retrieve the files and media
      bitmunk.api.proxy(
      {
         method: 'GET',
         url: sFilesUrl,
         params: params,
         success: function(data, status)
         {
            // clear the old files from the table if the clearModel flag
            // is set
            if(options.clearModel)
            {
               bitmunk.model.clear(
                  bitmunk.medialibrary.model.name,
                  bitmunk.medialibrary.model.fileTable);
            }
            
            // insert files into model
            var rs = JSON.parse(data);
            for(var i = 0, length = rs.resources.length;
               i < length; i++)
            {
               // update the tables with the data
               var file = rs.resources[i].fileInfo;
               var media = rs.resources[i].media;

               // use sequence id 0 by default in both update calls to
               // overwrite current data and prevent update race conditions

               // FIXME: clear file and media table?

               // update the file table
               bitmunk.model.update(
                  bitmunk.medialibrary.model.name,
                  bitmunk.medialibrary.model.fileTable, file.id, file,
                  options.seqNum);
                
               // update the media table, if the media exists
               if(media)
               {
                  bitmunk.model.update(
                     bitmunk.medialibrary.model.name,
                     bitmunk.medialibrary.model.mediaTable, 
                     media.id, media, 0);
               }
            }
            
            // update the total number of files in the media library
            bitmunk.medialibrary.model.totalFiles = rs.total;

            if(task)
            {
               task.unblock();
            }
         },
         error: function()
         {
            if(task)
            {
               task.fail();
            }
         }
      });
   };
   
   /**
    * Starts updating the media library model.
    * 
    * @param task the current task.
    */
   var start = function(task)
   {
      // define event filter for user ID
      var filter = {
         details: { userId: bitmunk.user.getUserId() }
      };
      
      // register model
      bitmunk.model.register(task,
      {
         model: bitmunk.medialibrary.model.name,
         tables: [
         {
            // create file table
            name: bitmunk.medialibrary.model.fileTable,
            eventGroups: [
            // listen for all file additions and exceptions, this will cover
            // both watched and unwatched files
            {
               events: ['bitmunk.medialibrary.File.updated'],
               filter: $.extend(true, {details:{isNew: true}}, filter),
               eventCallback: fileAdded
            },
            {
               events: ['bitmunk.medialibrary.File.exception'],
               filter: filter,
               eventCallback: fileException
            }],
            watchRules:
            {
               getRegisterObject: createFileWatchEvents,
               reinitializeRows: bitmunk.medialibrary.model.updateFiles
            }
         },
         {
            // create media table
            name: bitmunk.medialibrary.model.mediaTable,
            eventGroups:
            [
               // add future media events here
            ]
         }]
      });
   };
   
   /** Table Registration Callbacks **/
   
   /**
    * Called when a new file is added.
    * 
    * @param event the file event.
    * @param seqNum the sequence number assigned to this event update.
    */
   var fileAdded = function(event, seqNum)
   {
      bitmunk.log.debug(sLogCategory, event.type, event, seqNum);
      
      // add the file to the model, watch it and trigger an event so that
      // the UI can display the newly added file
      bitmunk.task.start(
      {
         type: sLogCategory + '.getFile',
         run: function(task)
         {
            // get the file that has been added
            task.next(function(task)
            {
               bitmunk.medialibrary.model.updateFiles(
                  [event.details.fileInfo.id], task,
                  {seqNum: seqNum, clearModel: false});
            });
            
            // watch the newly added file 
            task.next(function(task)
            {
               // watch file table
               bitmunk.model.watch(
                  bitmunk.medialibrary.model.name,
                  bitmunk.medialibrary.model.fileTable);
            });
            
            // fire the file added event 
            task.next(function(task)
            {
               var type = event.type.replace(/updated/g, 'added');
               $.event.trigger(
                  type.replace(/\./g, '-'),
                  [event.details.fileInfo.id, event.details.fileInfo.mediaId]);
            });
         }
      });
   };
   
   /**
    * Called when a fileInfo exception occurs.
    * 
    * @param event the file event.
    * @param seqNum the sequence number assigned to this event update.
    */
   var fileException = function(event, seqNum)
   {
      bitmunk.log.debug(sLogCategory, event.type, event, seqNum);
      
      // FIXME: Does the exception include the fileId in the details?
      // The fileId is necessary to identify the fileInfo that had an exception.
      // If it doesn't have one, we can ignore the event.
      
      // trigger fileInfo exception event
      $.event.trigger(event.type.replace(/\./g, '-'), 
         [event.details.fileInfo, event.details.exception]);
   };
   
   /**
    * Creates the set of events for specific fileInfos watched by the table. 
    * 
    * @param fileIds an array of fileIds.
    * 
    * @return the event registration object.
    */
   var createFileWatchEvents = function(fileIds)
   {
      var eventGroups = [];
      var userId = bitmunk.user.getUserId();
      
      $.each(fileIds, function(i, fileId)
      {
         // create filter on userId and fileId
         var filter = {
            details: {
               userId: userId,
               fileInfo: { id: fileId }
            }
         };
         
         // we do not need to watch exception events because the global
         // exception event was added when the files table was created
         
         // add file updated event
         eventGroups.push(
         {
            events: ['bitmunk.medialibrary.File.updated'],
            filter: filter,
            eventCallback: fileUpdated
         });
         
         // add file removed event
         eventGroups.push(
         {
            events: ['bitmunk.medialibrary.File.removed'],
            filter: filter,
            eventCallback: fileRemoved
         });
      });
      
      // DEBUG: output
      bitmunk.log.debug(
         sLogCategory, 'Create File Watch Events', fileIds, eventGroups);
      
      return { eventGroups: eventGroups };
   };
   
   /**
    * Event callback used when a fileInfo is added/updated in the media library.
    * 
    * @param event the media library file event.
    * @param seqNum the sequence number assigned to this event update.
    */
   var fileUpdated = function(event, seqNum)
   {
      bitmunk.log.debug(
         sLogCategory, event.type, event.details.fileInfo.id, event, seqNum);
      
      // update the file in the model, trigger an event so that the UI can
      // display the updates
      bitmunk.task.start(
      {
         type: sLogCategory + '.getFile',
         run: function(task)
         {
            // get the file that has been updated
            task.next(function(task)
            {
               bitmunk.medialibrary.model.updateFiles(
                  [event.details.fileInfo.id], task,
                  {seqNum: seqNum, clearModel: false});
            });
            
            // fire the file updated event 
            task.next(function(task)
            {
               $.event.trigger(
                  event.type.replace(/\./g, '-'),
                  [event.details.fileInfo.id, event.details.fileInfo.mediaId]);
            });
         }
      });
   };
   
   /**
    * Event callback used when a fileInfo is removed from the media library.
    * 
    * @param event the media library file event.
    * @param seqNum the sequence number assigned to this event update.
    */
   var fileRemoved = function(event, seqNum)
   {
      bitmunk.log.debug(event.type, event, seqNum);
      
      // fetch file
      var file = bitmunk.model.fetch(
         bitmunk.medialibrary.model.name,
         bitmunk.medialibrary.model.fileTable,
         event.details.fileId);
      
      // remove from file table
      bitmunk.model.remove(
         bitmunk.medialibrary.model.name,
         bitmunk.medialibrary.model.fileTable,
         event.details.fileId);
      
      // trigger removed event
      $.event.trigger(
         event.type.replace(/\./g, '-'), [event.details.fileId, file]);
   };
};

bitmunk.resource.registerScript({
   pluginId: 'bitmunk.webui.Help',
   resourceId: 'help.js',
   depends: {},
   init: init
});

   
   bitmunk.model.addModel(bitmunk.medialibrary.model.name, {start: start});
   sScriptTask.unblock();
})(jQuery);

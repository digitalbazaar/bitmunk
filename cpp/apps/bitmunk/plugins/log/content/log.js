/**
 * Bitmunk Web UI --- LogView
 *
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 */
(function($)
{
   // category for LogView
   var sLogCategory = 'bitmunk.webui.LogUI';
   
   var sScriptTask = bitmunk.resource.getScriptTask(
      'bitmunk.webui.LogUI', 'log.js');
   
   // event namespace
   var sNS = '.bitmunk-webui-LogUI';
   
   /*
   The log view stores the raw log message internally.  For efficiency reasons
   it uses "chunks".  The storage uses the following structure:
      storage = [chunk-0, chunk-1, ..., chunk-N]
      chunk = [message-0, message-1, ..., message-N]
   Messages are added to the last chunk until it reaches the max chunk size.
   When the last chunk fills up, the first chunk is removed and a new chunk
   started.  This helps to avoid frequent array resizing.
   */
   // log view info
   var sLogView =
   {
      // the logger instance
      logger: null,
      // max number of log entries
      capacity: 1000,
      // chunk size of logs (see above)
      chunkSize: 50,
      // cache calculation of max chunks
      maxChunks: 200, /* capcity / chunkSize */
      // the message chunk storage
      chunks: [],
      // if log view is showing or not (used for live log updates)
      showing: false
   };
   // debug access
   bitmunk.debug.set(sLogCategory, 'logView', sLogView);
   
   /**
    * Refresh the UI log messages.
    */
   var refreshLog = function()
   {
      // get template, process with message chunks, and append to table
      var tmpl = bitmunk.resource.get(
         'bitmunk.webui.LogUI', 'logEntries.html');
      // do message interpolation
      for(var ci = 0, cl = sLogView.chunks.length; ci < cl; ci++)
      {
         var c = sLogView.chunks[ci];
         for(var mi = 0, ml = c.length; mi < ml; mi++)
         {
            bitmunk.log.prepareFull(c[mi]);
         }
      }
      var logData = {
         messageChunks: sLogView.chunks
      };
      $('#logView table tr:gt(0)').remove();
      $('#logView table').append(tmpl.data.process(logData));
   };
   
   /**
    * Clear the stored log messages and UI.
    */
   var clearLog = function()
   {
      sLogView.chunks = [];
      $('#logView table tr:gt(0)').remove();
   };
   
   var show = function(task)
   {
      bitmunk.log.debug(sLogCategory, 'show');
      sLogView.showing = true;
      refreshLog();
      $('#refreshLog').bind('click', refreshLog);
      $('#clearLog').bind('click', clearLog);
   };
   
   var hide = function(task)
   {
      bitmunk.log.debug(sLogCategory, 'hide');
      sLogView.showing = false;
      $('#refreshLog').unbind('click');
      $('#clearLog').unbind('click');
   };
   
   bitmunk.resource.setupResource(
   {
      pluginId: 'bitmunk.webui.LogUI',
      resourceId: 'log',
      show: show,
      hide: hide
   });
   
   bitmunk.logui =
   {
      didLogin: function(task)
      {
         bitmunk.log.debug(sLogCategory, 'logui didLogin');
         clearLog();
      },
      
      willLogout: function(task)
      {
         bitmunk.log.debug(sLogCategory, 'logui willLogout');
         clearLog();
      }
   };
   
   // setup main log view
   sLogView.logger = bitmunk.log.makeLogger(function(logger, message)
   {
      // Note: written in order of most likely conditionals
      // rough order:
      //   last chunk not full
      //   last chunk full and max chunks
      //   last chunk full and not max chunks
      //   no chunks
      var clen = sLogView.chunks.length;
      if(clen != 0)
      {
         // chunks exist
         // get last chunk
         var chunk = sLogView.chunks[clen-1];
         if(chunk.length < sLogView.chunkSize)
         {
            // last chunk not full, add message
            chunk.push(message);
         }
         else
         {
            // last chunk full
            if(clen == sLogView.maxChunks)
            {
               // max chunks, shift out first
               sLogView.chunks.shift();
            }
            // create new chunk
            sLogView.chunks.push([message]);
         }
      }
      else
      {
         // create new chunk
         sLogView.chunks.push([message]);
      }
   });
   // start off not logging and let initial config event setup
   bitmunk.log.setLevel(sLogView.logger, 'none');
   // add the logger
   bitmunk.log.addLogger(sLogView.logger);
   
   /**
    * Handle logging configuration events.
    */
   $(bitmunk.log).bind('bitmunk-webui-WebUi-config-changed' + sNS,
      function(e, config)
      {
         // log view level
         var level = bitmunk.util.getPath(config,
            ['loggers', 'log', 'level']);
         if(typeof(level) !== 'undefined')
         {
            bitmunk.log.setLevel(sLogView.logger, level);
         }
         
         // log view capacity
         var capacity = bitmunk.util.getPath(config,
            ['loggers', 'log', 'capacity']);
         if(typeof(capacity) !== 'undefined')
         {
            sLogView.capacity = capacity;
            sLogView.maxChunks = capacity / sLogView.chunkSize;
         }
      });
   
   sScriptTask.unblock();
})(jQuery);

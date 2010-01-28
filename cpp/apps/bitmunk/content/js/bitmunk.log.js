/**
 * Bitmunk Web UI --- Logging
 *
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 *
 * Copyright (c) 2008-2009 Digital Bazaar, Inc.  All rights reserved.
 */
(function($) 
{
   // event namespace
   var sNS = '.bitmunk-webui-core-Logging';
   
   /**
    * Bitmunk logging system.
    * 
    * Each logger level available as it's own function of the form:
    *    bitmunk.log.level(category, args...)
    * The category is an arbitrary string, and the args are the same as
    * Firebug's console.log API.  By default the call will be output as:
    *   'LEVEL [category] <args[0]>, args[1], ...'
    * This enables proper % formatting via the first argument.
    * Each category is enabled by default but can be enabled or disabled with
    * the setCategoryEnabled() function.
    */
   // log namespace object
   bitmunk.log = {};
   // list of known levels
   bitmunk.log.levels =
      ['none', 'error', 'warning', 'info', 'debug', 'verbose', 'max'];
   // info on the levels indexed by name:
   //    index: level index
   //    name: uppercased display name
   var sLevelInfo = {};
   // list of loggers
   var sLoggers = [];
   /**
    * Standard console logger.  If no console support is enabled this will
    * remain null.  Check before using.
    */
   var sConsoleLogger = null;
   
   // logger flags
   /**
    * Lock the level at the current value.  Used in cases where user config may
    * set the level such that only critical messages are seen but more verbose
    * messages are needed for debugging or other purposes.
    */
   bitmunk.log.LEVEL_LOCKED = (1 << 1);
   /**
    * Always call log function.  By default, the logging system will check the
    * message level against logger.level before calling the log function.  This
    * flag allows the function to do its own check.
    */
   bitmunk.log.NO_LEVEL_CHECK = (1 << 2);
   /**
    * Perform message interpolation with the passed arguments.  "%" style
    * fields in log messages will be replaced by arguments as needed.  Some
    * loggers, such as Firebug, may do this automatically.  The original log
    * message will be available as 'message' and the interpolated version will
    * be available as 'fullMessage'.
    */
   bitmunk.log.INTERPOLATE = (1 << 3);
   
   // setup each log level
   $.each(bitmunk.log.levels, function(i, level) {
      sLevelInfo[level] = {
         index: i,
         name: level.toUpperCase()
      };
   });
   
   /**
    * Message logger.  Will dispatch a message to registered loggers as needed.
    *
    * @param message message object
    */
   bitmunk.log.logMessage = function(message)
   {
      var len = sLoggers.length;
      var messageLevelIndex = sLevelInfo[message.level].index;
      $.each(sLoggers, function(i, logger) {
         if(logger.flags & bitmunk.log.NO_LEVEL_CHECK)
         {
            logger.f(message);
         }
         else
         {
            // get logger level
            var loggerLevelIndex = sLevelInfo[logger.level].index;
            // check level
            if(messageLevelIndex <= loggerLevelIndex)
            {
               // message critical enough, call logger
               logger.f(logger, message);
            }
         }
      });
   };
   
   /**
    * Set the 'standard' key on a message object to:
    * "LEVEL [category] " + message
    *
    * @param message a message log object
    */
   bitmunk.log.prepareStandard = function(message)
   {
      if(!('standard' in message))
      {
         message.standard =
            sLevelInfo[message.level].name +
            //' ' + +message.timestamp +
            ' [' + message.category + '] ' +
            message.message;
      }
   };
   
   /**
    * Set the 'full' key on a message object to the original message
    * interpolated via % formatting with the message arguments.
    *
    * @param message a message log object
    */
   bitmunk.log.prepareFull = function(message)
   {
      if(!('full' in message))
      {
         // copy args and insert message at the front
         var args = $.extend([], message['arguments']);
         args.unshift(message.message);
         // format the message
         message.full = bitmunk.util.format.apply(this, args);
      }
   };
   
   /**
    * Apply both preparseStandard() and prepareFull() to a message object and
    * store result in 'standardFull'.
    *
    * @param message a message log object
    */
   bitmunk.log.prepareStandardFull = function(message)
   {
      if(!('standardFull' in message))
      {
         // FIXME implement 'standardFull' logging
         bitmunk.log.prepareStandard(message);
         message.standardFull = message.standard;
      }
   };
   
   // create log level functions
   if(true)
   {
      // levels for which we want functions
      var levels = ['error', 'warning', 'info', 'debug', 'verbose'];
      $.each(levels, function(i, level) {
         // create function for this level
         bitmunk.log[level] = function(category, message/*, args...*/)
         {
            // convert arguments to real array, remove category and message
            var args = Array.prototype.slice.call(arguments).slice(2);
            // create message object
            // Note: interpolation and standard formatting is done lazily
            var msg = {
               timestamp: new Date(),
               level: level,
               category: category,
               message: message,
               'arguments': args
               /*standard*/
               /*full*/
               /*fullMessage*/
            };
            // process this message
            bitmunk.log.logMessage(msg);
         };
      });
   }
   
   /**
    * Create a new logger with specified custom logging function.
    * The logging function has a signature of:
    *    function(logger, message)
    * logger: current logger
    * message: object:
    *   level: level id
    *   category: category
    *   message: string message
    *   arguments: Array of extra arguments
    *   fullMessage: interpolated message and arguments if INTERPOLATE flag set
    * 
    * @param logFunction a logging function which takes a log message object
    *        as a parameter
    *
    * @return a logger object
    */
   bitmunk.log.makeLogger = function(logFunction)
   {
      var logger = {
         flags: 0,
         f: logFunction
      };
      bitmunk.log.setLevel(logger, 'none');
      return logger;
   };

   /**
    * Set the current log level on a logger.
    *
    * @param logger the target logger
    * @param level the new maximum log level as a string
    *
    * @return true if set, false if not
    */
   bitmunk.log.setLevel = function(logger, level)
   {
      var rval = false;
      if(logger && !(logger.flags & bitmunk.log.LEVEL_LOCKED))
      {
         $.each(bitmunk.log.levels, function(i, aValidLevel) {
            if(level == aValidLevel)
            {
               // set level
               logger.level = level;
               rval = true;
               // exit $.each() loop
               return false;
            }
            else
            {
               return true;
            }
         });
      }
      
      return rval;
   };

   /**
    * Lock the log level at its current value.
    *
    * @param logger the target logger
    * @param lock boolean lock value, default to true
    */
   bitmunk.log.lock = function(logger, lock)
   {
      if(typeof lock === 'undefined' || lock)
      {
         logger.flags |= bitmunk.log.LEVEL_LOCKED;
      }
      else
      {
         logger.flags &= ~bitmunk.log.LEVEL_LOCKED;
      }
   };

   /**
    * Add a logger.
    *
    * @param logger the logger object
    */
   bitmunk.log.addLogger = function(logger)
   {
      sLoggers.push(logger);
   };
   
   // setup the console logger if possible, else create fake console.log
   if('console' in window && 'log' in console)
   {
      var logger;
      if(console.error && console.warn && console.info && console.debug)
      {
         // looks like Firebug-style logging is available
         // level handlers map
         var levelHandlers = {
            error: console.error,
            warning: console.warn,
            info: console.info,
            debug: console.debug,
            verbose: console.debug
         };
         var f = function(logger, message)
         {
            bitmunk.log.prepareStandard(message);
            var handler = levelHandlers[message.level];
            // copy args and prepend standard message
            var args = message['arguments'].slice();
            args.unshift(message.standard);
            // apply to low-level console function
            handler.apply(console, args);
         };
         logger = bitmunk.log.makeLogger(f);
      }
      else
      {
         // only appear to have basic console.log
         var f = function(logger, message)
         {
            bitmunk.log.prepareStandardFull(message);
            console.log(message.standardFull);
         };
         logger = bitmunk.log.makeLogger(f);
      }
      bitmunk.log.setLevel(logger, 'debug');
      bitmunk.log.addLogger(logger);
      sConsoleLogger = logger;
   }
   else
   {
      // define fake console.log to avoid potential script errors
      console = {
         log: function() {}
      };
   }
   
   /*
    * Check for logging control query vars.
    *
    * console.level=<level-name>
    * Set's the console log level by name.  Useful to override defaults and
    * allow more verbose logging before a user config is loaded.
    * 
    * console.lock=<true|false>
    * Lock the console log level at whatever level it is set at.  This is run
    * after console.level is processed.  Useful to force a level of verbosity
    * that could otherwise be limited by a user config.
    */
   if(sConsoleLogger !== null)
   {
      var query = bitmunk.util.getQueryVariables();
      if('console.level' in query)
      {
         // set with last value
         bitmunk.log.setLevel(
            sConsoleLogger, query['console.level'].slice(-1)[0]);
      }
      if('console.lock' in query)
      {
         // set with last value
         var lock = query['console.lock'].slice(-1)[0];
         if(lock == 'true')
         {
            bitmunk.log.lock(sConsoleLogger);
         }
      }
   }
   
   /**
    * Handle global WebUI configuration.
    */
   $(bitmunk.log).bind('bitmunk-webui-WebUi-config-changed' + sNS,
      function(e, config)
      {
         var level = bitmunk.util.getPath(config,
            ['loggers', 'console', 'level']);
         if(typeof(level) !== 'undefined')
         {
            bitmunk.log.setLevel(sConsoleLogger, level);
         }
      });
   
   // NOTE: logging support is implicit and not a required dependency
   // plugin registered after registration code is initialized
   bitmunk._implicitPluginInfo.logging =
   {
      pluginId: 'bitmunk.webui.core.Logging',
      name: 'Bitmunk Logging'
   };
})(jQuery);

/**
 * Copyright 2008 Digital Bazaar, Inc.
 *
 * Support for Bitmunk web applications.
 *
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 */

(function($) {
   // debugging info and functions
   bitmunk.debug =
   {
      // Private storage for debugging.
      // Useful to expose data that is otherwise unviewable behind closures.
      // NOTE: remember that this can hold references to data and cause leaks!
      // format is "bitmunk._debug.<modulename>.<dataname> = data"
      // Example:
      // (function($) {
      //    var cat = 'bitmunk.test.Test'; // dugging category
      //    var sState = {...}; // local state
      //    bitmunk.debug.set(cat, 'sState', sState);
      // })(jQuery);
      
      // storage
      storage: {},
   
      /**
       * Get debug data.  Omit name for all cat data.  Omit name and cat for
       * all data.
       *
       * @param cat name of debugging category
       * @param name name of data to get (optional)
       * @return object with requested debug data or undefined
       */
      get: function(cat, name)
      {
         var rval;
         if(typeof(cat) === 'undefined')
         {
            rval = bitmunk.debug.storage;
         }
         else if(cat in bitmunk.debug.storage)
         {
            if(typeof(name) === 'undefined')
            {
               rval = bitmunk.debug.storage[cat];
            }
            else
            {
               rval = bitmunk.debug.storage[cat][name];
            }
         }
         return rval; 
      },
   
      /**
       * Set debug data.
       *
       * @param cat name of debugging category
       * @param name name of data to set
       * @param data data to set
       */
      set: function(cat, name, data)
      {
         if(!(cat in bitmunk.debug.storage))
         {
            bitmunk.debug.storage[cat] = {};
         }
         bitmunk.debug.storage[cat][name] = data;
      },
   
      /**
       * Clear debug data.  Omit name for all cat data.  Omit name and cat for
       * all data.
       *
       * @param cat name of debugging category
       * @param name name of data to clear or omit to clear entire category
       */
      clear: function(cat, name)
      {
         if(typeof(cat) === 'undefined')
         {
            bitmunk.debug.storage = {};
         }
         else if(cat in bitmunk.debug.storage)
         {
            if(typeof(name) === 'undefined')
            {
               delete bitmunk.debug.storage[cat];
            }
            else
            {
               delete bitmunk.debug.storage[cat][name];
            }
         }
      }
   };
   
   // NOTE: debug support is implicit and not a required dependency
   // plugin registered after registration code is initialized
   bitmunk._implicitPluginInfo.debug =
   {
      pluginId: 'bitmunk.webui.core.Debug',
      name: 'Bitmunk Debug'
   };
})(jQuery);

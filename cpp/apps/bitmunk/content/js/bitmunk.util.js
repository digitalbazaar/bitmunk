/**
 * Bitmunk Web UI --- Utilities
 *
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 *
 * Copyright (c) 2008-2010 Digital Bazaar, Inc.  All rights reserved.
 */
(function($)
{
   // extend forge util
   bitmunk.util = window.forge.util;
   
   /**
    * Return the primary contributor of a media. The primary contributor is
    * guessed from the available contributors.
    *
    * @param media the media
    *    media:
    *       contributors:
    *          <type>: [name, ...]
    * @param ... arguments to interpolate into the format string.
    *
    * @return the media creator contributor object or null
    */
   bitmunk.util.getPrimaryContributor = function(media)
   {
      var rval = null;
      
      if(media && 'contributors' in media)
      {
         var first = function(name)
         {
            var rval;
            if(name in media.contributors &&
               media.contributors[name].length > 0)
            {
               rval = media.contributors[name][0];
            }
            return rval;
         };
         rval =
            first('Performer') ||
            first('Copyright Owner') ||
            first('Publisher');
      }
      return rval;
   };
   
   // NOTE: util support is implicit and not a required dependency
   // plugin registered after registration code is initialized
   bitmunk._implicitPluginInfo.util =
   {
      pluginId: 'bitmunk.webui.core.Util',
      name: 'Bitmunk Util'
   };
})(jQuery);

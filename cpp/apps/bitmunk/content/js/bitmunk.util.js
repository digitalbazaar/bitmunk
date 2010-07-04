/**
 * Bitmunk Web UI --- Utilities
 *
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 *
 * Copyright (c) 2008-2010 Digital Bazaar, Inc.  All rights reserved.
 */
(function($)
{
   /* Strorage for query variables */
   var sQueryVariables = null;
   
   bitmunk.util = {};
   
   /**
    * Return the window location query variables.  Query is parsed on the first
    * call and the same object is returned on subsequent calls.  The mapping
    * is from keys to an array of values.  Parameters without values will have
    * an object key set but no value added to the value array.  Values are
    * unescaped.
    *
    * ...?k1=v1&k2=v2:
    * {
    *   "k1": ["v1"],
    *   "k2": ["v2"]
    * }
    *
    * ...?k1=v1&k1=v2:
    * {
    *   "k1": ["v1", "v2"]
    * }
    *
    * ...?k1=v1&k2:
    * {
    *   "k1": ["v1"],
    *   "k2": []
    * }
    *
    * ...?k1=v1&k1:
    * {
    *   "k1": ["v1"]
    * }
    *
    * ...?k1&k1:
    * {
    *   "k1": []
    * }
    *
    * @param query the query string to parse (optional, default to cached
    *        results from parsing window location search query)
    *
    * @return object mapping keys to variables. 
    */
   bitmunk.util.getQueryVariables = function(query)
   {
      var parse = function(q)
      {
         var rval = {};
         var kvpairs = q.split('&');
         for (var i = 0; i < kvpairs.length; i++)
         {
            var pos = kvpairs[i].indexOf('=');
            var key;
            var val;
            if (pos > 0)
            {
               key = kvpairs[i].substring(0,pos);
               val = kvpairs[i].substring(pos+1);
            }
            else
            {
               key = kvpairs[i];
               val = null;
            }
            if(!(key in rval))
            {
               rval[key] = [];
            }
            if(val !== null)
            {
               rval[key].push(unescape(val));
            }
         }
         return rval;
      };
      
      var rval;
      if(typeof(query) === 'undefined')
      {
         // cache and use window search query
         if(sQueryVariables === null)
         {
            sQueryVariables = parse(window.location.search.substring(1));
         }
         rval = sQueryVariables;
      }
      else
      {
         // parse given query
         rval = parse(query);
      }
      return rval;
   };

   /**
    * Parse a fragment into a path and query. This method will take a URI
    * fragment and break it up as if it were the main URI. For example:
    *    /bar/baz?a=1&b=2
    * results in:
    *    {
    *       path: ["bar", "baz"],
    *       query: {"k1": ["v1"], "k2": ["v2"]}
    *    }
    * 
    * @return object with a path array and query object
    */
   bitmunk.util.parseFragment = function(fragment)
   {
      // default to whole fragment
      var fp = fragment;
      var fq = '';
      // split into path and query if possible at the first '?'
      var pos = fragment.indexOf('?');
      if(pos > 0)
      {
         fp = fragment.substring(0,pos);
         fq = fragment.substring(pos+1);
      }
      // split path based on '/' and ignore first element if empty
      var path = fp.split('/');
      if(path.length > 0 && path[0] == '')
      {
         path.shift();
      }
      // convert query into object
      var query = (fq == '') ? {} : bitmunk.util.getQueryVariables(fq);
      
      return {
         pathString: fp,
         queryString: fq,
         path: path,
         query: query
      };
   };
   
   /**
    * Make a request out of a URI-like request string. This is intended to
    * be used where a fragment id (after a URI '#') is parsed as a URI with
    * path and query parts. The string should have a path beginning and
    * delimited by '/' and optional query parameters following a '?'. The
    * query should be a standard URL set of key value pairs delimited by
    * '&'. For backwards compatibility the initial '/' on the path is not
    * required. The request object has the following API, (fully described
    * in the method code):
    *    {
    *       path: <the path string part>.
    *       query: <the query string part>,
    *       getPath(i): get part or all of the split path array,
    *       getQuery(k, i): get part or all of a query key array,
    *       getQueryLast(k, _default): get last element of a query key array.
    *    }
    * 
    * @return object with request parameters.
    */
   bitmunk.util.makeRequest = function(reqString)
   {
      var frag = bitmunk.util.parseFragment(reqString);
      var req =
      {
         // full path string
         path: frag.pathString,
         // full query string
         query: frag.queryString,
         /**
          * Get path or element in path.
          * 
          * @param i optional path index.
          * 
          * @return path or part of path if i provided.
          */
         getPath: function(i)
         {
            return (typeof(i) === 'undefined') ? frag.path : frag.path[i];
         },
         /**
          * Get query, values for a key, or value for a key index.
          * 
          * @param k optional query key.
          * @param i optional query key index.
          * 
          * @return query, values for a key, or value for a key index.
          */
         getQuery: function(k, i)
         {
            var rval;
            if(typeof(k) === 'undefined')
            {
               rval = frag.query;
            }
            else
            {
               rval = frag.query[k];
               if(rval && typeof(i) !== 'undefined')
               {
                  rval = rval[i];
               }
            }
            return rval;
         },
         getQueryLast: function(k, _default)
         {
            var rval;
            var vals = req.getQuery(k);
            if(vals)
            {
               rval = vals[vals.length - 1];
            }
            else
            {
               rval = _default;
            }
            return rval;
         }
      };
      return req;
   };
   
   /**
    * Make a URI out of a path, an object with query parameters, and a
    * fragment. Uses jQuery.param() internally for query string creation.
    * If the path is an array, it will be joined with '/'.
    * 
    * @param path string path or array of strings.
    * @param query object with query parameters. (optional)
    * @param fragment fragment string. (optional)
    * 
    * @return string object with request parameters.
    */
   bitmunk.util.makeLink = function(path, query, fragment)
   {
      // join path parts if needed
      path = $.isArray(path) ? path.join('/') : path;
       
      var qstr = jQuery.param(query || {});
      fragment = fragment || '';
      return path +
         ((qstr.length > 0) ? ('?' + qstr) : '') +
         ((fragment.length > 0) ? ('#' + fragment) : '');
   };
   
   /**
    * Follow a path of keys deep into an object hierarchy and set a value.
    * If a key does not exist or it's value is not an object, create an
    * object in it's place.  This can be destructive to a object tree if
    * leaf nodes are given as non-final path keys.
    * Used to avoid exceptions from missing parts of the path.
    *
    * @param object the starting object
    * @param keys an array of string keys
    * @param value the value to set
    */
   bitmunk.util.setPath = function(object, keys, value)
   {
      // need to start at an object
      if(typeof(object) === 'object' && object !== null)
      {
         var i = 0;
         var len = keys.length;
         while(i < len)
         {
            var next = keys[i++];
            if(i == len)
            {
               // last
               object[next] = value;
            }
            else
            {
               // more
               var hasNext = (next in object);
               if(!hasNext ||
                  (hasNext && typeof(object[next]) !== 'object') ||
                  (hasNext && object[next] === null))
               {
                  object[next] = {};
               }
               object = object[next];
            }
         }
      }
   };
   
   /**
    * Follow a path of keys deep into an object hierarchy and return a value.
    * If a key does not exist, create an object in it's place.
    * Used to avoid exceptions from missing parts of the path.
    *
    * @param object the starting object
    * @param keys an array of string keys
    * @param _default value to return if path not found
    * @return the value at the path if found, else default if given, else
    *         undefined
    */
   bitmunk.util.getPath = function(object, keys, _default)
   {
      var i = 0;
      var len = keys.length;
      var hasNext = true;
      while(hasNext && i < len &&
         typeof(object) === 'object' && object !== null)
      {
         var next = keys[i++];
         hasNext = next in object;
         if(hasNext)
         {
            object = object[next];
         }
      }
      return (hasNext ? object : _default);
   };
   
   /**
    * Follow a path of keys deep into an object hierarchy and delete the
    * last one. If a key does not exist, do nothing.
    * Used to avoid exceptions from missing parts of the path.
    *
    * @param object the starting object
    * @param keys an array of string keys
    */
   bitmunk.util.deletePath = function(object, keys)
   {
      // need to start at an object
      if(typeof(object) === 'object' && object !== null)
      {
         var i = 0;
         var len = keys.length;
         var hasNext = true;
         while(i < len)
         {
            var next = keys[i++];
            if(i == len)
            {
               // last
               delete object[next];
            }
            else
            {
               // more
               if(!(next in object) ||
                  (typeof(object[next]) !== 'object') ||
                  (object[next] === null))
               {
                  break;
               }
               object = object[next];
            }
         }
      }
   };
   
   /**
    * Check if an object is empty.
    *
    * Taken from:
    * http://stackoverflow.com/questions/679915/how-do-i-test-for-an-empty-javascript-object-from-json/679937#679937
    *
    * @param object the object to check.
    */
   bitmunk.util.isEmpty = function(obj)
   {
      for(var prop in obj)
      {
         if(obj.hasOwnProperty(prop))
         {
            return false;
         }
      }
      return true;
   };
   
   /**
    * Format with simple printf-style interpolation.
    *
    * %%: literal '%'
    * %s,%o: convert next argument into a string
    *
    * @param format the string to format.
    * @param ... arguments to interpolate into the format string.
    */
   bitmunk.util.format = function(format)
   {
      var re = /%./g;
      // current match
      var match;
      // current part
      var part;
      // current arg index
      var argi = 0;
      // collected parts to recombine later
      var parts = [];
      // last index found
      var last = 0;
      // loop while matches remain
      while((match = re.exec(format)))
      {
         part = format.substring(last, re.lastIndex - 2);
         // don't add empty strings (ie, parts between %s%s)
         if(part.length > 0)
         {
            parts.push(part);
         }
         last = re.lastIndex;
         // switch on % code
         var code = match[0][1];
         switch(code)
         {
            case 's':
            case 'o':
               // check if enough arguments were given
               if(argi < arguments.length)
               {
                  parts.push(arguments[argi++ + 1]);
               }
               else
               {
                  parts.push('<?>');
               }
               break;
            // FIXME: do proper formating for numbers, etc
            //case 'f':
            //case 'd':
            case '%':
               parts.push('%');
               break;
            default:
               parts.push('<%' + code + '?>');
         }
      }
      // add trailing part of format string
      parts.push(format.substring(last));
      return parts.join('');
   };
   
   /**
    * Return the primary contributor of a media. The primary contributor is guessed
    * from the available contributors.
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
   
   /**
    * Format a number.
    *
    * http://snipplr.com/view/5945/javascript-numberformat--ported-from-php/
    */
   bitmunk.util.formatNumber = function(
      number, decimals, dec_point, thousands_sep)
   {
       // http://kevin.vanzonneveld.net
       // +   original by: Jonas Raoni Soares Silva (http://www.jsfromhell.com)
       // +   improved by: Kevin van Zonneveld (http://kevin.vanzonneveld.net)
       // +     bugfix by: Michael White (http://crestidg.com)
       // +     bugfix by: Benjamin Lupton
       // +     bugfix by: Allan Jensen (http://www.winternet.no)
       // +    revised by: Jonas Raoni Soares Silva (http://www.jsfromhell.com)    
       // *     example 1: number_format(1234.5678, 2, '.', '');
       // *     returns 1: 1234.57     
    
       var n = number, c = isNaN(decimals = Math.abs(decimals)) ? 2 : decimals;
       var d = dec_point === undefined ? "," : dec_point;
       var t = thousands_sep === undefined ?
          "." : thousands_sep, s = n < 0 ? "-" : "";
       var i = parseInt(n = Math.abs(+n || 0).toFixed(c)) + "", j = (j = i.length) > 3 ? j % 3 : 0;
       
       return s + (j ? i.substr(0, j) + t : "") + i.substr(j).replace(/(\d{3})(?=\d)/g, "$1" + t) + (c ? d + Math.abs(n - i).toFixed(c).slice(2) : "");
   };
   
   /**
    * Format a byte size.
    * 
    * http://snipplr.com/view/5949/format-humanize-file-byte-size-presentation-in-javascript/
    */
   bitmunk.util.formatSize = function(size)
   {
      if (size >= 1073741824)
      {
         size = bitmunk.util.formatNumber(size / 1073741824, 2, '.', '') +
            ' GiB';
      }
      else
      {
         if (size >= 1048576)
         {
            size = bitmunk.util.formatNumber(size / 1048576, 2, '.', '') +
               ' MiB';
         }
         else
         {
            if (size >= 1024)
            {
               size = bitmunk.util.formatNumber(size / 1024, 0) + ' KiB';
            }
            else
            {
               size = bitmunk.util.formatNumber(size, 0) + ' bytes';
            }
         }
      }
      return size;
   };
   
   // NOTE: util support is implicit and not a required dependency
   // plugin registered after registration code is initialized
   bitmunk._implicitPluginInfo.util =
   {
      pluginId: 'bitmunk.webui.core.Util',
      name: 'Bitmunk Util'
   };
})(jQuery);

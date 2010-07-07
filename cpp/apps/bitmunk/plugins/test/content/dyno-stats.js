/**
 * Bitmunk Web UI --- DynamicObject Statistics Test
 *
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 * 
 * Copyright (c) 2008-2010 Digital Bazaar, Inc.  All rights reserved.
 */
(function($) {

var init = function(task)
{
   // logging category
   var cat = 'bitmunk.webui.Test.dyno-stats';
   
   var show = function show(task)
   {
      bitmunk.log.debug(cat, 'show');
      
      // calculate stat totals
      var calcTotal = function(data, skipKeyCounts)
      {
         var totals = {
            counts: {
               live: 0,
               dead: 0,
               max: 0
            },
            bytes: {
               live: 0,
               dead: 0,
               max: 0
            }
         };
         for(var key in data)
         {
            if(!skipKeyCounts || key != 'KeyCounts')
            {
               var value = data[key];
               totals.counts.live += value.counts.live;
               totals.counts.dead += value.counts.dead;
               totals.counts.max += value.counts.max;
               totals.bytes.live += value.bytes.live;
               totals.bytes.dead += value.bytes.dead;
               totals.bytes.max += value.bytes.max;
            }
         }
         return totals;
      };
      
      // calculate totals as if keys are unique
      var calcUnique = function(data, skipKeyCounts)
      {
         var totals = {
            counts: {
               live: 0,
               dead: 0,
               max: 0
            },
            bytes: {
               live: 0,
               dead: 0,
               max: 0
            }
         };
         for(var key in data)
         {
            if(!skipKeyCounts || key != 'KeyCounts')
            {
               var value = data[key];
               totals.counts.live += (value.counts.live > 0) ? 1 : 0;
               totals.counts.dead += (value.counts.dead > 0) ? 1 : 0;
               totals.counts.max += 1;
               totals.bytes.live += value.bytes.live / value.counts.live;
               totals.bytes.dead += value.bytes.dead / value.counts.dead;
               totals.bytes.max += value.bytes.max / value.counts.max;
            }
         }
         return totals;
      };
      
      // make a stat row
      var makeRow = function(name, data)
      {
         var row = $('<tr/>');
         row.append('<td>' + name + '</td>');
         row.append('<td>' + data.counts.live + '</td>');
         row.append('<td>' + data.counts.dead + '</td>');
         row.append('<td>' + data.counts.max + '</td>');
         row.append('<td>' + data.bytes.live + '</td>');
         row.append('<td>' + data.bytes.dead + '</td>');
         row.append('<td>' + data.bytes.max + '</td>');
         return row;
      };
      
      $('#refresh-dyno-counts').click(function()
      {
         var stats = $('#dyno-counts');
         var status = $('#dyno-counts-status');
         
         stats.empty();
         status.text('Loading...');
         
         bitmunk.api.proxy(
         {
            method: 'GET',
            url: '/api/3.0/system/statistics/dyno?nodeuser=' +
                 bitmunk.user.getUserId(),
            success: function(data)
            {
               data = JSON.parse(data);
               console.info(data);
               for(var key in data)
               {
                  if(key != 'KeyCounts')
                  {
                     stats.append(makeRow(key, data[key]));
                  }
               }
               stats.append(makeRow('Total', calcTotal(data, true)));
               status.text('Updated @:' + new Date());         
            },
            error: function()
            {
               status.text('ERROR! @:' + new Date());         
            }
         });
         
         return false;
      });
      
      $('#refresh-dyno-key-counts').click(function()
      {
         var stats = $('#dyno-key-counts');
         var status = $('#dyno-key-counts-status');
         
         stats.empty();
         status.text('Loading...');
         
         bitmunk.api.proxy(
         {
            method: 'GET',
            url: '/api/3.0/system/statistics/dyno?nodeuser=' +
                 bitmunk.user.getUserId(),
            success: function(data)
            {
               data = JSON.parse(data);
               data = ('KeyCounts' in data) ? data.KeyCounts.keys : {};
               for(var key in data)
               {
                  stats.append(makeRow(key, data[key]));
               }
               stats.append(makeRow('Total', calcTotal(data, false)));
               stats.append(makeRow('Unique', calcUnique(data, false)));
               status.text('Updated @:' + new Date());         
            },
            error: function()
            {
               status.text('ERROR! @:' + new Date());         
            }
         });
         
         return false;
      });
   };
   
   var hide = function(task)
   {
      bitmunk.log.debug(cat, 'hide');
   };
   
   bitmunk.resource.extendResource(
   {
      pluginId: 'bitmunk.webui.Test',
      resourceId: 'test-dyno-stats',
      show: show,
      hide: hide
   });
};

bitmunk.resource.registerScript({
   pluginId: 'bitmunk.webui.Test',
   resourceId: 'dyno-stats.js',
   depends: {},
   init: init
});


})(jQuery);

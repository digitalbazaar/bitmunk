/**
 * Bitmunk Web UI --- Help
 *
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 */
(function($)
{
   var sScriptTask = bitmunk.resource.getScriptTask(
      'bitmunk.webui.Help', 'help.js');

   // general help page handlers
   var help =
   {
      show: function(task)
      {
         // find external URLs by looking for hrefs that don't start with '#'
         $('a').each(function(i, el)
         {
            if($(el).attr('href')[0] != '#')
            {
               // add external relation attribute 
               $(el).addClass('external');
            }
         });
      },
      hide: function(task)
      {
      }
   };
   
   // special about page handlers
   var about =
   {
      show: function(task)
      {
         // info template
         var template = bitmunk.resource.get(
            'bitmunk.webui.Help', 'about.info.html', true);
         
         // show info loading message
         var values = {
            loading: true
         };
         $('#about-info').html(template.process(values));
         
         task.next(help.show);
         task.next(function(task)
         {
            // Start call to get version from backend. On success just set the
            // HTML without worrying if page is still active.
            bitmunk.api.proxy(
            {
               method: 'GET',
               url: bitmunk.api.localRoot + 'system/version',
               params:
               {
                  nodeuser: bitmunk.user.getUserId()
               },
               success: function(data, status)
               {
                  var v = JSON.parse(data);
                  $('#about-info').html(template.process(v));
               },
               error: function()
               {
                  var v = {
                     error: 'Error loading version info.'
                  };
                  $('#about-info').html(template.process(v));
               }
            });
            $('#webUiVersion').html('unknown');
         });
      },
      hide: function(task)
      {
         task.next(help.show);
      }
   };
   
   // setup same show/hide handlers for most help pages
   var pages = ['credits', 'help', 'licenses'];
   $.each(pages, function(i, page)
   {
      bitmunk.resource.setupResource(
      {
         pluginId: 'bitmunk.webui.Help',
         resourceId: page,
         show: help.show,
         hide: help.hide
      });
   });
   bitmunk.resource.setupResource(
   {
      pluginId: 'bitmunk.webui.Help',
      resourceId: 'about',
      show: about.show,
      hide: about.hide
   });
};

bitmunk.resource.registerScript({
   pluginId: 'bitmunk.webui.Help',
   resourceId: 'help.js',
   depends: {},
   init: init
});

})(jQuery);

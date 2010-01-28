/**
 * Bitmunk Web UI --- Views
 *
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 *
 * Copyright (c) 2009 Digital Bazaar, Inc.  All rights reserved.
 */
(function($)
{
   // logging category
   var cat = 'bitmunk.webui.core.View';
   var sLogCategory = cat;
   
   /**
    * Map of ids to named views.
    */
   //var sNamedViews = {};
   // debug access
   //bitmunk.debug.set(cat, 'namedViews', sNamedViews);
   
   bitmunk.view =
   {
      /**
       * Creates an instance of a view that can be shown or hidden in
       * a DOM content element.
       * 
       * The options parameter for this function specifies the functions
       * to call when this view is either shown or hidden.
       * 
       * @param options:
       *        id: the unique identifier for the view.
       *        models: a list of all the models the view uses.
       *        show: called when a view is shown.
       *        hide: called when a view is hidden.
       */
      create: function(options)
      {
         // set options defaults
         options = $.extend({
            id: null,
            pluginId: null,
            resourceId: null,
            models: [],
            show: function(task){},
            hide: function(task){}
         }, options);
         
         // get the resource for this view
         var r = bitmunk.resource.get(options.pluginId, options.resourceId);
         $.extend(r.options, options);
         
         var view =
         {
            // set this view's ID
            id: options.id,
            
            // set resources for this view
            resource: r,
            
            /**
             * Hides a view.
             * 
             * @param task the task to use to hide the view synchronously.
             * @param node the dom content element to clear, if provided.
             */
            hide: function(task, node)
            {
               var opts = view.resource.options;
               bitmunk.log.debug(sLogCategory, 'hiding view [%s]', view.id);
               
               // first call the view's specific hide function
               task.next('view hide', opts.hide);
               
               if(node)
               {
                  // clear dom element
                  node.empty();
               }
               
               if(opts.css)
               {
                  // next disable view css
                  task.next('view disable css', function(task)
                  {
                     $.each(opts.css, function(i, css)
                     {
                        // FIXME: ref count active css nodes with jQuery data()?
                        // DOM link node
                        var linkNode =
                           bitmunk.resource.get(opts.pluginId, css, true);
                        if(linkNode)
                        {
                           // disable stylesheet, no need to remove from head
                           linkNode.disabled = true;
                        }
                     });
                     // FIXME: how to wait?
                  });
               }
               
               // next detach it from all its models
               task.next('view detachModels', function(task)
               {
                  $.each(opts.models, function(i, model)
                  {
                     bitmunk.model.detachView(model);
                  });
               });
            },
            
            /**
             * Shows a view.
             * 
             * @param task the task to use to show the view synchronously.
             * @param node the dom content element to populate with the view's
             *             html.
             * @param request the request object.
             */
            show: function(task, node, request)
            {
               var opts = view.resource.options;
               bitmunk.log.debug(sLogCategory, 'showing view [%s][%s]',
                  opts.pluginId, opts.resourceId, view);
               
               // load all of a views resources
               task.next('view load for show', function(task)
               {
                  bitmunk.resource.load(
                  {
                     pluginId: opts.pluginId,
                     resourceId: opts.resourceId,
                     recursive: true,
                     task: task
                  });
               });
               
               // next attach it to all its models
               $.each(opts.models, function(i, model)
               {
                  task.next('view attachView', function(task)
                  {
                     bitmunk.model.attachView(task, model);
                  });
               });
               
               // enable view css
               if(opts.css)
               {
                  // hide node while preparing css
                  task.next('hide node to prepare css', function(task)
                  {
                     node.hide();
                  });
                  
                  task.next('view enable css', function(task)
                  {
                     // block on all CSS
                     task.block(opts.css.length);
                     // enable CSS
                     $.each(opts.css, function(i, css)
                     {
                        // FIXME ref count active css nodes with jQuery data()?
                        // DOM link node
                        var linkNode =
                           bitmunk.resource.get(opts.pluginId, css, true);
                        var allStyleNodes = $('link[rel="stylesheet"]');
                        // make sure it's enabled
                        linkNode.disabled = false;
                        // check if link dom node needs to be added
                        if($.inArray(linkNode, allStyleNodes) == -1)
                        {
                           $('head').append(linkNode);
                        }
                           
                        var unblock = function()
                        {
                           task.unblock();
                        };
                        // Inspired by xLazyLoader:
                        // http://code.google.com/p/ajaxsoft/source/browse/
                        // trunk/xLazyLoader/jquery.xLazyLoader.js
                        if($.browser.msie)
                        {
                           linkNode.onreadystatechange = function()
                           {
                              /loaded|complete/.test(linkNode.readyState) &&
                              unblock();
                           };
                        }
                        else if($.browser.opera)
                        {
                           linkNode.onload = unblock;
                        }
                        else
                        {
                           // FF, Safari, Chrome
                           (function(){
                              try {
                                 // test for a known object property
                                 linkNode.sheet.cssRules;
                                 // above will throw exception if CSS not ready
                                 unblock();
                              } catch(e){
                                 setTimeout(arguments.callee, 20);
                              }
                           })();
                        }
                     });
                  });
               }
               
               // update and show content element
               task.next('update and show view content element', function(task)
               {
                  var viewHtml = bitmunk.resource.get(opts.pluginId, opts.html);
                  // update content and show
                  node.html(viewHtml.data).show();
                  // set the web page title
                  document.title = bitmunk.baseTitle + opts.name;
                  // pass request to show()
                  task.userData = task.userData || {};
                  task.userData.request = request;
               });
               
               // finally call specific show on the view
               task.next(opts.show);
            }
         };
         
         // return the created view
         return view;
      }
   };
   
   // register plugin
   var defaults = $.extend(true, {}, bitmunk.sys.corePluginDefaults);
   bitmunk.sys.register($.extend(defaults, {
      pluginId: 'bitmunk.webui.core.View',
      name: 'Bitmunk View'
   }));
})(jQuery);

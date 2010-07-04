/**
 * Bitmunk Web UI --- Tabbed Pane Functions
 *
 * @author Dave Longley
 *
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 */
(function($)
{
   // the logging category for tabbed pane
   var sLogCategory = 'bitmunk.tabbedpane';
   
   // define the tabbed pane interface
   bitmunk.tabbedpane = {};
   
   /**
    * Creates a new tabbed pane with the given name.
    *
    * The show and hide options take a task parameter.
    * 
    * @param options options for this tabbed pane:
    *    name: the name of the tabbed pane. (string, required)
    *    show: a fuction to call after showing this pane. (optional)
    *    hide: a function to call before hiding this pane. (optional)
    */
   bitmunk.tabbedpane.create = function(options)
   {
      // menu and content nodes for the tabbed pane
      var mMenuNode = null;
      var mContentNode = null;
      
      // the ordered menu items (with names+view ID entries)
      var mMenu = [];
      
      // set to true once the menu has been initialized
      var mMenuLoaded = false;
      
      // view for the current page
      var mCurrentView = null;
      
      // request for the current page
      var mCurrentRequest = null;
      
      // define tabbed pane object
      var tabbedPane =
      {
         id: 'pane-' + options.name,
         // name of tabbed pane
         name: options.name,
         // the current view ID
         currentViewId: null
      };
      
      tabbedPane.show = function(task)
      {
         bitmunk.log.debug(sLogCategory, '[%s] show()', tabbedPane.name);
         
         mMenuNode = $('#pane-' + tabbedPane.name + '-menu');
         mContentNode = $('#pane-' + tabbedPane.name);
         
         if(!mMenuLoaded)
         {
            // get first menuLinks element
            var menuLinks = mMenuNode.children('.menuLinks');
            
            // clear any existing links
            menuLinks.empty();
            
            // add items to the menu
            $.each(mMenu, function(i, item)
            {
               bitmunk.log.debug(sLogCategory,
                  '[%s] adding menu item [%s]=>[%s]',
                  tabbedPane.name, item.name, item.viewId);
               
               var liId =
                  'pane-' + tabbedPane.name + '-menu-' + item.viewId;
               var li = $(
                  '<li id="' + liId + '"><a class="load" href="#' +
                  item.viewId + '"><span>' + item.name +
                  '</span></a></li>');
               
               // append menu item
               menuLinks.append(li);
               
               // setup click handler to switch views
               $('a.load', li).click(function()
               {
                  bitmunk.log.verbose(sLogCategory,
                     '[%s] menu item [%s]=>[%s] clicked',
                     tabbedPane.name, item.name, item.viewId);
                  
                  // load menu item's view
                  bitmunk.load(item.viewId);
                  return false;
               });
            });
            
            mMenuLoaded = true;
         }
         
         // run optional show callback
         if(options.show)
         {
            task.next(options.show);
         }
      };
      
      tabbedPane.hide = function(task)
      {
         bitmunk.log.debug(sLogCategory, '[%s] hide()', tabbedPane.name);
         
         // clean up menu
         if(mMenuLoaded)
         {
            // clear menuLinks element
            $('#menuLinks').empty();
            mMenuLoaded = false;
         }
         
         // run optional hide callback
         if(options.hide)
         {
            task.next(options.hide);
         }
      };
      
      /**
       * Adds a menu item to this tabbed panel.
       * 
       * @param name the name of the menu item.
       * @param viewId the unique ID of the view.
       */
      tabbedPane.addMenuItem = function(name, viewId)
      {
         bitmunk.log.debug(sLogCategory,
            '[%s] adding menu item...', tabbedPane.name, name);
         mMenu.push(
         {
            name: name,
            viewId: viewId
         });
      };
      
      /**
       * Clears the menu items in this tabbed panel.
       */
      tabbedPane.clearMenu = function()
      {
         mMenuLoaded = false;
         mMenu = [];
      };
      
      /**
       * Clears this tabbed panel entirely.
       * 
       * @param task the task to use to clear the panel synchronously.
       */
      tabbedPane.clear = function(task)
      {
         task.next(function(task)
         {
            tabbedPane.clearMenu();
         });
         
         // hide the current view
         task.next('bitmunk.view.TabbedPane.clearView', function(task)
         {
            if(mCurrentView !== null)
            {
               // hide the current view
               task.next(mCurrentView.data.hide);
            }
         });
         
         // clear current view member
         task.next(function(task)
         {
            mCurrentView = null;
         });
         
         // empty content pane
         task.next(function(task)
         {
            $('#pane-' + tabbedPane.name).empty();
         });
      };
      
      /**
       * Display the loading page while another page gets ready to
       * display.
       *
       * @param task the task to use to synchronize changing the view.
       */
      tabbedPane.showLoadingPage = function(task)
      {
         bitmunk.log.debug(sLogCategory,
            '[%s] showing loading page...', tabbedPane.name);
         
         // replace content html with loading page
         var loadingHtml = bitmunk.resource.get(
            bitmunk.mainPluginId, 'loading.html', true);
         mContentNode.html(loadingHtml);
      };
      
      /**
       * Hides the current page, displays a loading page, and then
       * shows a new page.
       * 
       * @param task the task to use to synchronize changing the view.
       * @param request the view request.
       */
      tabbedPane.showNamedView = function(task, request)
      {
         task.next(function(task)
         {
            // get view
            var viewId = request.getPath(0);
            var view = bitmunk.resource.getNamedView(viewId);
            
            // ensure the view exists
            if(!view)
            {
               // handle page not found error
               bitmunk.log.error(sLogCategory,
                  '[%s] page not found [%s]', tabbedPane.name, viewId);
               view = bitmunk.resource.getNamedView('__unknown__');
            }
            
            // only change views if not showing current view
            if(view &&
               (view != mCurrentView ||
               request.query != mCurrentRequest.query))
            {
               bitmunk.log.debug(sLogCategory,
                  '[%s] showing view [%s]', tabbedPane.name, viewId);
               
               // hide the content node to prepare it
               task.next('bitmunk.view.TabbedPane.hideContentNode',
               function(task)
               {
                  mContentNode.hide();
               });
               
               // hide the current view
               task.next('bitmunk.view.TabbedPane.hideView',
               function(task)
               {
                  if(mCurrentView !== null)
                  {
                     mCurrentView.data.hide(task);
                  }
               });
               
               var loadingTimeoutId = null;
               task.next('bitmunk.view.TabbedPane.showLoadingView',
               function(task)
               {
                  // display the loading page after delay
                  // cancel once loading is complete
                  loadingTimeoutId = setTimeout(function()
                  {
                     loadingTimeoutId = null;
                     mContentNode.show();
                     tabbedPane.showLoadingPage(task);
                  }, 200);
               });
               
               // highlight the view's menu item
               task.next('bitmunk.view.TabbedPane.highlightMenuItem',
               function(task)
               {
                  // set selected menu item
                  var li =
                     $('#pane-' + tabbedPane.name + '-menu-' + viewId);
                  $('li', li.parent()).removeClass('selected');
                  li.addClass('selected');
               });
               
               // load all of a view's resources
               task.next('bitmunk.view.TabbedPane.loadView',
               function(task)
               {
                  bitmunk.resource.load(
                  {
                     pluginId: view.pluginId,
                     resourceId: view.resourceId,
                     recursive: true,
                     task: task
                  });
               });
               
               // update and show the view
               task.next('bitmunk.view.TabbedPane.showView',
               function(task)
               {
                  // cancel loading notification
                  if(loadingTimeoutId !== null)
                  {
                     clearTimeout(loadingTimeoutId);
                  }
                  mCurrentRequest = request;
                  mCurrentView = view;
                  tabbedPane.currentViewId = viewId;
                  mCurrentView.data.show(task, mContentNode, request);
               });
            }
         });
      };
      
      // return created tabbed pane
      return tabbedPane;
   };
})(jQuery);

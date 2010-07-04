/**
 * Bitmunk Web UI --- File Browser
 *
 * @author Manu Sporny
 *
 * Copyright (c) 2009 Digital Bazaar, Inc.  All rights reserved.
 */
(function($)
{
   var sLogCategory = 'bitmunk.webui.core.FileBrowser';
   
   // URL to access the filesystem
   var sFilesystemUrl = bitmunk.api.localRoot32 + 'filesystem/files';
   
   // system root directory
   // FIXME: get config or something? how? ... or does '/' just work with
   // our backend stuff for windows?
   var sRootPath = '/';
   
   /**
    * This click handler is called whenever an item is single clicked in a 
    * directory listing.
    * 
    * @param event the event object associated with the click event.
    */
   var itemClicked = function(event)
   {
      var browser = event.data;
      var item = $(event.target).closest('tr');
      var path = $(item).find('.full-path').text();
      
      if($(item).find('.filebrowser-directory').length > 0)
      {
         // highlight the directory, but do not expand it
         if(browser.highlightedItem != item)
         {
            bitmunk.log.debug(
               sLogCategory, 'highlight (directory): ' + path);
            
            if(browser.highlightedItem)
            {
               browser.highlightedItem.removeClass('filebrowser-highlight');
            }
            browser.highlightedItem = item;
            browser.highlightedItem.addClass('filebrowser-highlight');
         }
      }
      else if($(item).find('.filebrowser-file').length > 0)
      {
         // highlight the file, but do not select it
         if(browser.highlightedItem != item)
         {
            bitmunk.log.debug(
               sLogCategory, 'highlight (file): ' + path);
            
            if(browser.highlightedItem)
            {
               browser.highlightedItem.removeClass('filebrowser-highlight');
            }
            browser.highlightedItem = item;
            browser.highlightedItem.addClass('filebrowser-highlight');
         }
      }
   };
   
   /**
    * This click handler is called whenever an item is double clicked in a 
    * directory listing.
    * 
    * @param event the event object associated with the click event.
    */
   var itemDoubleClicked = function(event)
   {
      var browser = event.data;
      var item = $(event.target).closest('tr');
      var path = $(item).find('.full-path').text();
      
      if($(item).find('.non-readable').length > 0)
      {
         // the path is non-readable, ignore the click
         bitmunk.log.debug(
            sLogCategory, 'itemDoubleClicked (non-readable): ' + path);
      }
      else if($(item).find('.filebrowser-directory').length > 0)
      {
         // the path is a directory
         bitmunk.log.debug(
            sLogCategory, 'itemDoubleClicked (directory): ' + path);
         
         // set the directory in the file browser
         browser.setDirectory(path);
      }
      else if($(item).find('.filebrowser-file').length > 0)
      {
         // the path is a file
         bitmunk.log.debug(
            sLogCategory, 'itemDoubleClicked (file): ' + path);
         
         // select file
         browser.selectFile(path);
      }
   };
   
   /**
    * This click handler is called whenever the select button is clicked in
    * a file browser.
    * 
    * @param event the event object associated with the click event.
    */
   var selectClicked = function(event)
   {
      // only handle selection if something was highlighted
      var browser = event.data;
      var item = browser.highlightedItem;
      if(item)
      {
         var path = $(item).find('.full-path').text();
         if($(item).find('.non-readable').length > 0)
         {
            // the path is non-readable, ignore the click
            bitmunk.log.debug(
               sLogCategory, 'selectClicked (non-readable): ' + path);
         }
         else if($(item).find('.filebrowser-directory').length > 0)
         {
            // the path is a directory
            bitmunk.log.debug(
               sLogCategory, 'selectClicked (directory): ' + path);
            
            if($.inArray('directory', browser.selectable) != -1)
            {
               // select directory
               browser.selectDirectory(path);
            }
            else
            {
               // set the directory in the file browser
               browser.setDirectory(path);
            }
         }
         else if($(item).find('.filebrowser-file').length > 0)
         {
            // the path is a file
            bitmunk.log.debug(
               sLogCategory, 'selectClicked (file): ' + path);
            
            // select file
            browser.selectFile(path);
         }
      }
   };
   
   /**
    * Formats a given directory and file path to a relative pathname.
    * 
    * @param parent the parent directory for the given path.
    * @param path the full pathname, including the parent pathname.
    * 
    * @return a relative path based on the parent directory and the given
    *         path.
    */
   var getRelativePath = function(parent, path)
   {
      var relativePath = 'UNKNOWN';
      
      // check to see if the path is a Unix-based path or a Windows-based
      // path
      if(path[0] == '/')
      {
         // if path and parent are equal, the relative path is .
         if(path == parent)
         {
            relativePath = '.';
         }
         // if the parent contains the path, then the item is a pointer to
         // the parent directory
         else if(parent.length > path.length)
         {
            relativePath = '..';
         }
         // if parent is contained in the path, generate the relative path
         else if(path.indexOf(parent) != -1)
         {
            relativePath = path.replace(parent, '');
            if(relativePath[0] == '/')
            {
               relativePath = relativePath.substring(1, relativePath.length);
            }
         }
      }
      else
      {
         // FIXME: This probably won't work for Windows...
         relativePath = path.replace(parent, '');
      }
      
      return relativePath;
   };
   
   /**
    * Initializes the browser's UI.
    * 
    * @param browser the browser to initialize.
    */
   var init = function(browser)
   {
      // save main element
      var filebrowser = $('#' + browser.id);
      browser.mainElement = filebrowser;
      
      // create template values
      var values =
      {
         id: browser.id
      };
      
      // process and insert template
      var template = bitmunk.resource.get(
         'bitmunk.webui.Main', 'filebrowser.html', true);
      filebrowser.append(template.process(values));
      
      // save appropriate elements
      browser.directoryElement = $('#' + browser.id + '-directory');
      browser.contentElement = $('#' + browser.id + '-content');
      browser.selectButton = $('#' + browser.id + '-select');
      
      // set the browser's directory
      browser.setDirectory(browser.directory);
   };
   
   /**
    * Populates the area for displaying the path and contents of a directory.
    * If directories are selectable, no files will appear. The contents
    * parameter is an array of entries where each has the following fields:
    * 
    * type: 'file' or 'directory',
    * path: an absolute path,
    * readable: true if the path is readable, false if not
    *
    * @param browser the file browser to populate.
    * @param path the directory.
    * @param contents the array of path entries to display.
    */
   var populate = function(browser, path, contents)
   {
      // update top-level directory
      browser.directoryElement.text(path);
      
      // filter the items into a sort-able array
      var sortableItems = { directories: [], files: [] };
      
      // only include files if they are selectable
      var includeFiles = ($.inArray('file', browser.selectable) != -1);
      
      // build the sortableContents map
      $.each(contents, function(i, entry)
      {
         var item = 
         {
            path: entry.path,
            relativePath: getRelativePath(path, entry.path),
            classes: [],
            size: 0
         };
         
         // check to see if the CSS class should include non-readable
         if(!entry.readable)
         {
            item.classes.push('non-readable');
         }
         
         // check to see if the CSS class should be a directory or a file
         switch(entry.type)
         {
            case 'directory':
               item.classes.push('filebrowser-directory');
               sortableItems.directories.push(item);
               break;
            case 'file':
               if(includeFiles)
               {
                  item.classes.push('filebrowser-file');
                  item.size = entry.size;
                  sortableItems.files.push(item);
               }
               break;
         }
      });
      
      // sort the items
      var sortedItems = sortableItems;
      sortedItems.directories.sort(function(a, b)
      {
         return ((a.relativePath == b.relativePath) ? 0 : 
               ((a.relativePath > b.relativePath) ? 1 : -1 ));
      });
      sortedItems.files.sort(function(a, b)
      {
         return ((a.relativePath == b.relativePath) ? 0 : 
               ((a.relativePath > b.relativePath) ? 1 : -1 ));
      });
      
      // create filebrowser content
      var content =
         '<tbody id="' + browser.id + '-content" class="filebrowser-content">';
      
      // add directories
      $.each(sortedItems.directories, function(index, dir)
      {
         content +=
            '<tr class="filebrowser-row">' +
            '<td class="nameCol">' +
            '<span class="full-path">' + dir.path + '</span>' +
            '<span class="display-path ' +  dir.classes.join(' ') + '">' +
            dir.relativePath + '</span>' +
            '</td>' +
            '<td class="sizeCol"></td>' +
            '</tr>';
      });
      
      // add files
      $.each(sortedItems.files, function(index, file)
      {
         content +=
            '<tr class="filebrowser-row">' +
            '<td class="nameCol">' + 
            '<span class="full-path">' + file.path + '</span>' +
            '<span class="display-path ' + file.classes.join(' ') + '">' +
            file.relativePath + '</span>' +
            '</td> ' +
            '<td class="sizeCol">' + file.size + '</td>' +
            '</tr>';
      });
      
      // end filebrowser content
      content += '</tbody>';
      
      // set new directory content
      browser.contentElement.hide();
      browser.contentElement.replaceWith(content);
      browser.contentElement = $('#' + browser.id + '-content');
      
      // disable text selection in contents section
      browser.contentElement.bind('mousedown', function() { return false; });
      
      // scroll back to the top of the contents
      browser.contentElement.animate({scrollTop: 0}, 0);
      
      // show contents
      browser.contentElement.show();
   };
   
   /**
    * Sets the directory that a file browser is viewing and displays the
    * appropriate contents.
    * 
    * @param browser the browser with the directory to set.
    * @param path the path to use when retrieving a directory listing.
    */
   var setDirectory = function(browser, path)
   {
      // use a task to retrieve the directory listing so that if multiple
      // paths are clicked on, the clicks will be handled in order
      bitmunk.task.start(
      {
         type: sLogCategory + '.setDirectory',
         run: function(task)
         {
            // show busy indicator
            browser.mainElement.addClass('busy');
            
            task.block();
            bitmunk.api.proxy(
            {
               method: 'GET',
               url: sFilesystemUrl,
               params:
               {
                  path: path,
                  nodeuser: bitmunk.user.getUserId()
               },
               success: function(data, status)
               {
                  var d = JSON.parse(data);
                  populate(browser, d.path, d.contents);
                  task.unblock();
               },
               error: function(xhr, textStatus, errorThrown)
               {
                  try
                  {
                     // use exception from server
                     var ex = JSON.parse(xhr.responseText);
                     if(ex.type.indexOf('bitmunk.filesystem.') === 0)
                     {
                        // path doesn't exist, couldn't be read, or wasn't
                        // a directory, so set directory to root
                        browser.setDirectory(sRootPath);
                        task.unblock();
                     }
                     else
                     {
                        task.userData = ex;
                        task.fail();
                     }
                  }
                  catch(e)
                  {
                     // use error message
                     task.userData = { message: textStatus };
                     task.fail();
                  }
               }
            });
         },
         success: function(task)
         {
            // hide busy indicator
            browser.mainElement.removeClass('busy');
         },
         failure: function(task)
         {
            // hide busy indicator
            browser.mainElement.removeClass('busy');
            
            // FIXME: add some kind of error feedback or call an error callback
            bitmunk.log.error(
               sLogCategory, 'setDirectory failed', task.userData);
         }
      });
   };
   
   /**
    * Clears any selection in a file browser.
    * 
    * @param browser the file browser to clear the selection in.
    */
   var clearSelection = function(browser)
   {
      browser.selection.path = null;
      browser.selection.type = null;
      if(browser.highlightedItem)
      {
         browser.highlightedItem.removeClass('filebrowser-highlight');
      }
   };
   
   /**
    * Shows a browser.
    * 
    * @param browser the file browser to show.
    */
   var show = function(browser)
   {
      // bind the click handlers
      browser.mainElement.bind('click', browser, itemClicked);
      browser.mainElement.bind('dblclick', browser, itemDoubleClicked);
      browser.contentElement.bind('mousedown', function() { return false; });
      browser.selectButton.bind('click', browser, selectClicked);
      
      // show browser
      browser.mainElement.show();
   };
   
   /**
    * Hides a browser.
    * 
    * @param browser the file browser to hide.
    */
   var hide = function(browser)
   {
      // hide browser
      browser.mainElement.hide();
      
      // remove highlighting
      if(browser.highlightedItem)
      {
         browser.highlightedItem.removeClass('filebrowser-highlight');
         browser.highlightedItem = null;
      }
      
      // scroll back to the top of the contents
      browser.contentElement.animate({scrollTop: 0}, 0);
      
      // unbind click handlers
      browser.mainElement.unbind('click');
      browser.mainElement.unbind('dblclick');
      browser.contentElement.unbind('mousedown');
      browser.selectButton.unbind('click');
   };
   
   /**
    * Bitmunk file browsing subsystem.
    * 
    * The file browser operates on a div that contains the following elements:
    * 
    * - An area for displaying the current path.
    * - An area for navigating the file hierarchy.
    * - An area for displaying the contents of a path 
    *   (if the path is a directory)
    *   
    * A developer is expected to provide the outer-most div, which will be
    * cleared and re-populated with all of the elements needed to browse the
    * file system and select files or directories. 
    */
   // filebrowser namespace object
   bitmunk.filebrowser = {};
   
   /**
    * Creates a file browser that will be displayed in a DOM element with
    * the given ID. The element should be a <div> element. The contents of
    * the element will be cleared and replaced with the file browser UI. 
    *
    * @param options:
    *           id the name of the element ID to use when replacing the
    *              contents of the element.
    *           path the path to use when initializing the file browser.
    *           selectable an array with 'file' if files can be selected,
    *              'directory' if directories can be selected.
    *           fileSelected the function(path) that will be called when a
    *              file has been selected.
    *           directorySelected the function(path) that will be called when
    *              a directory has been selected.
    *           init an optional custom function(browser) that will be
    *              called to initialize the browser UI.
    *           setDirectory an optional custom function(browser, path) that
    *              will be called to update the browser contents based on
    *              the given path.
    *           clearSelection an optional custom function(browser) that
    *              can be called to clear the browser contents.
    *           show an optional custom function(browser) that can be called
    *              to show the browser.
    *           hide an optional custom function(browser) that can be called
    *              to hide the browser.
    * 
    * @return the created file browser object.
    */
   bitmunk.filebrowser.create = function(options)
   {
      // set parameter defaults
      options = $.extend(
      {
         path: '/',
         selectable: ['file'],
         fileSelected: function() {},
         directorySelected: function() {},
         init: init,
         setDirectory: setDirectory,
         clearSelection: clearSelection,
         show: show,
         hide: hide
      }, options || {});
      
      bitmunk.log.debug(sLogCategory, 'create', options);
      
      // create browser object
      var browser =
      {
         id: options.id,
         directory: options.path,
         selectable: options.selectable,
         selection:
         {
            path: null,
            type: null
         },
         mainElement: null,
         directoryElement: null,
         contentElement: null,
         selectButton: null,
         highlightedItem: null,
         fileSelected: options.fileSelected,
         directorySelected: options.directorySelected,
         setDirectory: function(path)
         {
            options.setDirectory(browser, path);
         },
         clearSelection: function()
         {
            options.clearSelection(browser);
         },
         selectFile: function(path)
         {
            browser.selection.path = path;
            browser.selection.type = 'file';
            browser.fileSelected(path);
         },
         selectDirectory: function(path)
         {
            browser.selection.path = path;
            browser.selection.type = 'directory';
            browser.directorySelected(path);
         },
         show: function()
         {
            options.show(browser);
         },
         hide: function()
         {
            options.hide(browser);
         }
      };
      
      // initialize the browser's UI
      options.init(browser);
      
      return browser;
   };
   
   // NOTE: filebrowser support is implicit and not a required dependency
   // plugin registered after registration code is initialized
   bitmunk._implicitPluginInfo.filebrowser =
   {
      pluginId: 'bitmunk.webui.core.FileBrowser',
      name: 'Bitmunk File Browser'
   };
})(jQuery);

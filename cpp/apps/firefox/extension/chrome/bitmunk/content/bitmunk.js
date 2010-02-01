/**
 * The Bitmunk extension Javascript functionality for the main Firefox UI.
 *
 * @author Manu Sporny
 * @author Dave Longley
 */

/**
 * The current state of the Bitmunk application.
 */
const STATE_UNKNOWN   = 0;
const STATE_OFFLINE   = 1;
const STATE_ONLINE    = 2;
const STATE_LOADING   = 3;
var gBitmunkState = STATE_UNKNOWN;

/**
 * The polling interval. Set to 15 seconds.
 */
const gPollInterval = 15000;

/**
 * Interval ID for polling to see if the Bitmunk app is online.
 */
var gPollIntervalId = null;

/**
 * The maximum amount of time to allow for the Bitmunk app's webservices to
 * come online before assuming failure.
 */
const gMaximumLoadTime = 15000;

/**
 * Stores the time when the Bitmunk app was started.
 */
var gBitmunkStartTime = +new Date();

/**
 * The complete BPE URL, which may be modified whenever a communication error
 * occurs.
 */
var gBpeUrl = BPE_URL + '19200';

/**
 * A queue of Bitmunk directives to be processed.
 */
var gDirectives = [];

/**
 * Called when the window first loads the bitmunk status overlay. This is used
 * to determine the current state of the bitmunk app.
 */
function onWindowLoad()
{
   _bitmunkLog('onBrowserLoad()');
   
   // do a quick poll
   pollBitmunk(function(online)
   {
      _bitmunkLog('onBrowserLoad(): in poll callback: ' + online);
      
      if(online)
      {
         gBitmunkState = STATE_ONLINE;
         
         // start regular polling
         startBitmunkPolling({interval: gPollInterval});
      }
      else
      {
         // bitmunk app is offline, do not poll until the user clicks the on
         // status overlay interface
         gBitmunkState = STATE_OFFLINE;
      }
      
      // update the status display
      updateStatusDisplay();
   });
   
   // remove the window listener
   window.removeEventListener('load', onWindowLoad, false);
}

// add an event listener for when the window loads
window.addEventListener('load', onWindowLoad, false);

/**
 * Opens a new tab or reuses an existing one that has been marked for use with
 * this extension and that still contains the BPE url.
 * 
 * @return the tab pointing at the BPE's interface.
 */
function openBitmunkTab()
{
   var rval = null;
   
   _bitmunkLog('openBitmunkTab()');
   
   // FIXME: a future may be able to keep a reference to the bitmunk tab
   // around, clearing it when the tab is closed ... but it would also need
   // to clear it if someone navigates way to a different base URL
   
   // get the window mediator
   var wm = Components
      .classes['@mozilla.org/appshell/window-mediator;1']
      .getService(Components.interfaces.nsIWindowMediator);
   
   // check each browser instance for our attribute and URL
   var found = false;
   var be = wm.getEnumerator('navigator:browser');
   while(!found && be.hasMoreElements())
   {
      var browserWindow = be.getNext();
      var tabBrowser = browserWindow.gBrowser;
      var tabs = tabBrowser.tabContainer;
      
      for(var i = 0; !found && i < tabs.childNodes.length; i++)
      {
         // get the next tab and its web browser
         var tab = tabs.childNodes[i];
         var webBrowser = tabBrowser.getBrowserAtIndex(i);
         
         // FIXME: if the port number changes, this will not match the
         // tab any more, do we care?
         
         // if the tab contains our custom attribute and its URL starts with
         // the BPE URL, then we want to use it
         if(tab.hasAttribute('bitmunk-extension') &&
            webBrowser.currentURI.spec.indexOf(gBpeUrl + '/bitmunk') != -1)
         {
            // select the tab
            tabBrowser.selectedTab = tab;
            rval = tab;
            
            // focus this browser window in case another one has focus
            browserWindow.focus();
            found = true;
            
            // if the URL is pointing at the offline page, set it to online
            var url = webBrowser.currentURI.spec;
            var lastIndex = url.lastIndexOf('#offline');
            if(lastIndex != -1 && (lastIndex + 8 == url.length))
            {
               webBrowser.loadURI(gBpeUrl + '/bitmunk');
            }
         }
      }
   }
   
   // get the current focused tab browser
   var tabBrowser = getCurrentWindow().getBrowser();
   
   if(!found)
   {
      // add a new tab pointing at the BPE and go to it
      var newTab = tabBrowser.addTab(gBpeUrl + '/bitmunk');
      newTab.setAttribute('bitmunk-extension', 'true');
      tabBrowser.selectedTab = newTab;
      rval = newTab;
   }
   
   // add an onload event listener
   var webBrowser = tabBrowser.getBrowserForTab(rval);
   var onDocumentLoad = function()
   {
      // add a listener for when the BPE goes offline
      tabBrowser.contentDocument.addEventListener('bpe-offline', function()
      {
         _bitmunkLog('BPE is going offline.');
         
         // handle updating offline status more quickly
         if(gBitmunkState == STATE_ONLINE)
         {
            // restart polling on a fast interval
            stopBitmunkPolling();
            startBitmunkPolling({interval: 1000});
         }
      }, false);
      
      // handle any queued directives
      if(gDirectives.length > 0)
      {
         // send queued directives, clear queue
         var queue = gDirectives;
         gDirectives = [];
         sendDirectives(tabBrowser.contentDocument, queue);
      }
      
      // remove event listener
      webBrowser.removeEventListener('load', onDocumentLoad, true);
   };
   webBrowser.addEventListener('load', onDocumentLoad, true);
   
   return rval;
}

/**
 * Called when a user left clicks on something in the Bitmunk status bar
 * overlay that is meant to start and view their Bitmunk app.
 * 
 * The ensuing behavior depends on the current state of the Bitmunk app:
 * 
 * STATE_ONLINE: The Bitmunk app is online, so open a tab and visit its URL.
 * STATE_OFFLINE: The Bitmunk app is offline, so attempt to start it.
 * STATE_LOADING: The Bitmunk app is starting up already, so ignore.
 * STATE_UNKNOWN: The state of the Bitmunk app is being determined, so ignore.
 */
function manageBitmunk()
{
   _bitmunkLog('manageBitmunk(): current state: ' + gBitmunkState);
   
   switch(gBitmunkState)
   {
      case STATE_OFFLINE:
         // start the bitmunk app
         startBitmunk();
         break;
      case STATE_ONLINE:
         openBitmunkTab();
         break;
      case STATE_LOADING:
      case STATE_UNKNOWN:
         // ignore
         break;
   }
}

/**
 * Checks to see if the Bitmunk application is online. The given callback
 * will be called with a single parameter: either true, if the Bitmunk
 * application is online, or false if it is not.
 * 
 * @param cb the function to call with the true/false result and, if false, a
 *           message explaining why.
 */
function pollBitmunk(cb)
{
   _bitmunkLog('pollBitmunk()');
   
   // if we're not online, the port might have changed, so refresh it
   if(gBitmunkState != STATE_ONLINE)
   {
      // get the plugin
      var plugin = Components
      .classes['@bitmunk.com/xpcom;1']
      .getService()
      .QueryInterface(
         Components.interfaces.nsIBitmunkExtension);
      
      // get the port number for the plugin
      _bitmunkLog('pollBitmunk(): getting bitmunk node port...');
      var port = {};
      plugin.getBitmunkPort(port);
      _bitmunkLog('pollBitmunk(): bitmunk node port: ' + port.value);
      gBpeUrl = BPE_URL + port.value;
   }
   
   var xhr = new XMLHttpRequest();
   if(!xhr)
   {
      _bitmunkLog('pollBitmunk(): could not create XMLHttpRequest');
      cb(false, 'Could not create XMLHttpRequest.');
   }
   else
   {
      _bitmunkLog('pollBitmunk(): creating request');
      
      // called if a connection error occurs
      xhr.onerror = function()
      {
         _bitmunkLog('pollBitmunk(): could not connect');
         cb(false, 'Could not connect to the webservice.');
      };
      
      // called whenever the request worked
      xhr.onload = function()
      {
         // an error occurred if the HTTP response status is 400+
         _bitmunkLog('pollBitmunk(): response: ' + xhr.responseText);
         var error = '';
         if(xhr.status >= 400)
         {
            error = '' + xhr.status + ' ' + xhr.statusText;
         }
         else
         {
            try
            {
               // convert content from JSON
               var obj = JSON.parse(xhr.responseText);
               if(obj.echo !== 'ping')
               {
                  error = 'Unrecognized response.';
               }
            }
            catch(e)
            {
               error = 'Unrecognized response. JSON parse error.';
            }
         }
         
         if(error.length > 0)
         {
            _bitmunkLog('pollBitmunk(): ' + error);
            cb(false, error);
         }
         else
         {
            _bitmunkLog('pollBitmunk(): success');
            cb(true);
         }
      };
      
      _bitmunkLog('pollBitmunk(): sending request...');
      
      // open the URL
      xhr.open('GET', gBpeUrl + '/api/3.0/system/test/echo?echo=ping', true);
      
      // set accept request header and send the request
      xhr.setRequestHeader('Accept', 'application/json');
      xhr.send(null);
   }
}

/**
 * Starts the Bitmunk application if it is currently offline.
 */
function startBitmunk()
{
   _bitmunkLog('startBitmunk()');
   
   // stop any existing polling
   stopBitmunkPolling();
   
   // bitmunk is now loading
   gBitmunkState = STATE_LOADING;
   updateStatusDisplay();
   
   // start the Bitmunk application if it is offline
   pollBitmunk(function(online)
   {
      if(online)
      {
         _bitmunkLog('startBitmunk(): bitmunk has already started.');
         
         // switch state back to online
         gBitmunkState = STATE_ONLINE;
         updateStatusDisplay();
         
         // open bitmunk tab
         openBitmunkTab();
         
         // restart polling on a regular interval
         startBitmunkPolling({interval: gPollInterval});
      }
      else
      {
         _bitmunkLog('startBitmunk(): starting bitmunk @ ' +new Date());
         
         // get the plugin
         var plugin = Components
            .classes['@bitmunk.com/xpcom;1']
            .getService()
            .QueryInterface(Components.interfaces.nsIBitmunkExtension);
         
         // start the bitmunk app
         var pid = {};
         if(plugin.launchBitmunk(pid))
         {
            _bitmunkLog('startBitmunk(): bitmunk PID: ' + pid.value);
            
            // reset bitmunk start time
            gBitmunkStartTime = +new Date();
            
            /* There might be a small delay between when the bitmunk process
             * starts up and when its webservices come online. For that reason,
             * we do some short-interval polling here until we change state.
             * Then we start polling at a normal interval to monitor state.
             */
            startBitmunkPolling({
               interval: 1000,
               stateChanged: function()
               {
                  // stop polling
                  stopBitmunkPolling();
                  
                  if(gBitmunkState == STATE_ONLINE)
                  {
                     _bitmunkLog('startBitmunk(): bitmunk is now ONLINE');
                     
                     try
                     {
                        // bitmunk successfully started, open tab
                        openBitmunkTab();
                     }
                     catch(e)
                     {
                        _bitmunkLog('openBitmunkTab exception: ' + e);
                     }
                     
                     // start polling at a regular interval
                     startBitmunkPolling({interval: gPollInterval});
                  }
               }
            });
         }
         else
         {
            _bitmunkLog('startBitmunk(): bitmunk failed to start');
            
            // bitmunk failed to start
            gBitmunkState = STATE_OFFLINE;
         }
      }
   });
}

/**
 * Starts polling the Bitmunk app to see if it is online or not. Polling will
 * continue as long as the state has not changed and the state hasn't changed
 * to offline.
 * 
 * @param options:
 *           interval: the polling interval.
 *           stateChanged: a function to call once the state changes.
 */
function startBitmunkPolling(options)
{
   _bitmunkLog('startBitmunkPolling()');
   
   /* Start polling, do so using setTimeout so that we wait for the network
      activity to complete before doing the next poll. Otherwise, over time,
      due to network latency, we might start spawning off more than one poll
      at a time. */
   var poll = function()
   {
      _bitmunkLog('poll()');
      
      pollBitmunk(function(online)
      {
         var oldState = gBitmunkState;
         if(online)
         {
            _bitmunkLog('poll(): setting state to ONLINE');
            gBitmunkState = STATE_ONLINE;
         }
         else
         {
            var elapsed = (+new Date()) - gBitmunkStartTime;
            if(gBitmunkState != STATE_LOADING || elapsed >= gMaximumLoadTime)
            {
               /*
               // FIXME: notify the user that the bitmunk app failed to start
               if(gBitmunkState == STATE_LOADING)
               {
               }
               */
               
               _bitmunkLog('poll(): setting state to OFFLINE');
               gBitmunkState = STATE_OFFLINE;
            }
         }
         
         // continue polling if the state has not changed to offline
         // note that it the state should never be offline when polling starts
         if(oldState == gBitmunkState || gBitmunkState != STATE_OFFLINE)
         {
            // schedule next poll
            gPollIntervalId = getCurrentWindow().setTimeout(
               poll, options.interval);
            _bitmunkLog('poll(): next poll in ' + options.interval);
         }
         else
         {
            // finished polling
            gPollIntervalId = null;
         }

         // handle state change
         if(oldState != gBitmunkState)
         {
            // update status display
            updateStatusDisplay();
            
            // handle callback
            if(options.stateChanged)
            {
               options.stateChanged();
            }
         }
      });
   };
   
   // schedule first poll
   gPollIntervalId = getCurrentWindow().setTimeout(poll, options.interval);
}

/**
 * Stops polling the Bitmunk app.
 */
function stopBitmunkPolling()
{
   // clear any old interval ID
   if(gPollIntervalId !== null)
   {
      clearTimeout(gPollIntervalId);
      gPollIntervalId = null;
   }
}

/**
 * Updates the status display based on the current state.
 */
function updateStatusDisplay()
{
   _bitmunkLog('updateStatusDisplay()');
   
   statusImage = document.getElementById('bitmunk-status-image');
   statusLabel = document.getElementById('bitmunk-status-label');
   //controlLabel = document.getElementById('bitmunk-control-label');

   // update the image and the label based on the current state
   switch(gBitmunkState)
   {
      case STATE_ONLINE:
         _bitmunkLog('updateStatusDisplay(): ONLINE');
         statusImage.src = 'chrome://bitmunk/content/bitmunk16-online.png';
         statusLabel.value = 'online';
         break;
      case STATE_OFFLINE:
         _bitmunkLog('updateStatusDisplay(): OFFLINE');
         statusImage.src = 'chrome://bitmunk/content/bitmunk16-offline.png';
         statusLabel.value = 'offline';
         break;
      case STATE_LOADING:
      case STATE_UNKNOWN:
         _bitmunkLog('updateStatusDisplay(): LOADING');
         statusImage.src = 'chrome://bitmunk/content/bitmunk16-offline.png';
         statusLabel.value = 'loading';
         break;
   }
}

/**
 * Notifies the BPE that N directives have been queued via an event.
 * 
 * @param doc the page document.
 * @param bmds the array of bmds to send.
 */
function sendDirectives(doc, bmds)
{
   _bitmunkLog('sendDirectives()');
   
   var e = doc.getElementById('bitmunk-directive-queue');
   if(e)
   {
      // get any existing directive queue
      var queue;
      var json = e.innerHTML;
      if(json === '')
      {
         // no existing queue
         queue = bmds;
      }
      else
      {
         try
         {
            // append to the current queue
            queue = JSON.parse(json);
            queue = queue.concat(bmds);
         }
         catch(ex)
         {
            // overwrite bogus queue
            queue = bmds;
         }
      }
      
      // update queue element
      e.innerHTML = JSON.stringify(queue);
      
      // send an event
      var ev = doc.createEvent('Events');
      ev.initEvent('bitmunk-directives-queued', true, true);
      e.dispatchEvent(ev);
   }
}

/**
 * Called when the browser has started to download a Bitmunk directive.
 */
function directiveStarted()
{
   _bitmunkLog('Downloading Bitmunk directive.');
   
   if(gBitmunkState == STATE_ONLINE)
   {
      openBitmunkTab();
      var doc = getCurrentWindow().getBrowser().contentDocument;
      var e = doc.getElementById('bitmunk-directive-queue');
      if(e)
      {
         var ev = doc.createEvent('Events');
         ev.initEvent('bitmunk-directive-started', true, true);
         e.dispatchEvent(ev);
      }
   }
   else
   {
      // attempt to manage bitmunk
      manageBitmunk();
   }
}

/**
 * Queues a Bitmunk directive. If the Bitmunk app is already online, the
 * directive is sent immediately. Otherwise, the directive is queued and
 * an attempt is made to start the Bitmunk app. 
 * 
 * @param bmd the bitmunk directive.
 */
function queueDirective(bmd)
{
   _bitmunkLog('Queuing Bitmunk directive: ' + bmd);
   
   var obj = null;
   try
   {
      obj = JSON.parse(bmd);
   }
   catch(e)
   {
      _bitmunkLog('Could not queue Bitmunk directive. JSON parse error.');
   }
   
   if(obj !== null)
   {
      if(gBitmunkState == STATE_ONLINE)
      {
         // open the Bitmunk tab and send the directive
         openBitmunkTab();
         var tabBrowser = getCurrentWindow().getBrowser();
         sendDirectives(tabBrowser.contentDocument, [obj]);
      }
      else
      {
         // queue the directive and attempt to manage bitmunk
         gDirectives.push(obj);
         manageBitmunk();
      }
   }
}

/**
 * Called when the browser encounters an error while downloading a Bitmunk
 * directive.
 * 
 * @param msg the error message.
 */
function directiveError(msg)
{
   _bitmunkLog('Error when downloading Bitmunk directive.');
   
   openBitmunkTab();
   var doc = getCurrentWindow().getBrowser().contentDocument;
   var e = doc.getElementById('bitmunk-directive-queue');
   if(e)
   {
      var ev = doc.createEvent('Events');
      ev.initEvent('bitmunk-directive-error', true, true);
      e.setAttribute('error', msg);
      e.dispatchEvent(ev);
   }
}

/**
 * Opens a tab to display the Bitmunk P2P Plugin help page.
 */
function showBitmunkHelp()
 {
   _bitmunkLog('Showing Bitmunk Help');
   
   var mainWindow = getCurrentWindow();
   var newTab = mainWindow.getBrowser().addTab(
      'chrome://bitmunk/content/help.html');
   mainWindow.getBrowser().selectedTab = newTab;   
}

/**
 * Opens a tab to display the Bitmunk Website.
 */
function showBitmunkWebsite()
{
   _bitmunkLog('Showing Bitmunk Website');
   
   var mainWindow = getCurrentWindow();
   var newTab = mainWindow.getBrowser().addTab('http://bitmunk.com/');
   mainWindow.getBrowser().selectedTab = newTab;   
}

/**
 * The Bitmunk Directive Observer is used to listen to Bitmunk directive
 * commands. 
 */
const bitmunkDirectiveObserver = 
{
   observe: function(subject, topic, data)
   {
      switch(topic)
      {
         case 'bitmunk-directive-started':
            directiveStarted();
            break;
         case 'bitmunk-directive':
            queueDirective(data);
            break;
         case 'bitmunk-directive-error':
            directiveError(data);
            break;
      }
   }
};

// register the Bitmunk directive observer
const service = Components.classes['@mozilla.org/observer-service;1']
   .getService(Components.interfaces.nsIObserverService);
service.addObserver(
   bitmunkDirectiveObserver, 'bitmunk-directive-started', false);
service.addObserver(
   bitmunkDirectiveObserver, 'bitmunk-directive', false);
service.addObserver(
   bitmunkDirectiveObserver, 'bitmunk-directive-error', false);

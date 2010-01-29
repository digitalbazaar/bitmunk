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
const gBitmunkState = STATE_UNKNOWN;

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
 * Opens a new tab or reuses an existing one that has been marked for use with
 * this extension and that still contains the BPE url.
 * 
 * @return the tab pointing at the BPE's interface.
 */
function getBitmunkTab()
{
   var rval = null;
   
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
         var webBrowser = tabBrowser.getBrowserAtIndex(index);
         
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
   
   // handle any queued directives
   if(gDirectives.length > 0)
   {
      // add a load event listener to handle queued directives
      var sendDirectives = function()
      {
         var doc = newTabBrowser.contentDocument;
         
         // send queued directives
         while(gDirectives.length > 0)
         {
            sendDirective(doc, gDirectives.splice(0, 1)[0]);
         }
         
         // remove event listener
         webBrowser.removeEventListener('load', sendDirectives, true);
      };
      var webBrowser = tabBrowser.getBrowserForTab(rval);
      webBrowser.addEventListener('load', sendDirectives, true);
   }
   
   return rval;
}

/**
 * Called when the browser first starts up and loads the bitmunk status
 * overlay. This is used to determine the current state of the bitmunk app.
 */
function onBrowserLoad()
{
   // do a quick poll
   pollBitmunk(function(online)
   {
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
   switch(gBitmunkState)
   {
      case STATE_OFFLINE:
         // start the bitmunk app, open the main page on success
         startBitmunk(
         {
            success: function()
            {
               // get (open) the bitmunk tab
               getBitmunkTab();
            }
         });
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
 * @param cb the function to call with the true/false result.
 */
function pollBitmunk(cb)
{
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
   
   sendRequest(
   {
      url: gBpeUrl + '/api/3.0/system/test/ping',
      success: function(xhr, obj)
      {
         cb(true);
      },
      error: function(xhr, obj)
      {
         cb(false);
      }
   });
}

/**
 * Starts the Bitmunk application if it is currently offline.
 * 
 * @param options:
 *           success: a callback to call if successful.
 */
function startBitmunk(options)
{
   // stop any existing polling
   stopBitmunkPolling();
   
   // start the Bitmunk application if it is offline
   pollBitmunk(function(online)
   {
      if(online)
      {
         _bitmunkLog('startBitmunk(): bitmunk has already started.');
         
         // restart polling
         startBitmunkPolling({interval: gPollInterval});
      }
      else
      {
         _bitmunkLog('startBitmunk(): starting bitmunk @ ' +new Date());
         
         // bitmunk is now loading
         gBitmunkState = STATE_LOADING;
         
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
                  // stop old polling
                  stopBitmunkPolling();
                  
                  if(gBitmunkState == STATE_ONLINE)
                  {
                     // bitmunk successfully started
                     if(options.success)
                     {
                        options.success();
                     }
                     
                     // start polling at a regular interval
                     startBitmunkPolling({interval: gPollInterval});
                  }
               }
            });
         }
         else
         {
            // bitmunk failed to start
            gBitmunkState = STATE_OFFLINE;
            stopBitmunkPolling();
         }
      }
   });
}

/**
 * Starts polling the Bitmunk app to see if it is online or not.
 * 
 * @param options:
 *           interval: the polling interval.
 *           stateChanged: a function to call once the state changes.
 */
function startBitmunkPolling(options)
{
   /* Start polling, do so using setTimeout so that we wait for the network
      activity to complete before doing the next poll. Otherwise, over time,
      due to network latency, we might start spawning off more than one poll
      at a time. */
   var poll = function()
   {
      pollBitmunk(function(online)
      {
         var oldState = gBitmunkState;
         if(online)
         {
            gBitmunkState = STATE_ONLINE;
         }
         else
         {
            var now = +new Date();
            if(gBitmunkState != STATE_LOADING || now > gMaximumLoadTime)
            {
               /*
               // FIXME: notify the user that the bitmunk app failed to start
               if(gBitmunkState == STATE_LOADING)
               {
               }
               */
               
               gBitmunkState = STATE_OFFLINE;
            }
         }
         
         // schedule next poll
         gPollIntervalId = getCurrentWindow().setTimeout(
            poll, options.interval);

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
   statusImage = document.getElementById('bitmunk-status-image');
   statusLabel = document.getElementById('bitmunk-status-label');
   //controlLabel = document.getElementById('bitmunk-control-label');

   // update the image and the label based on the current state
   switch(gBitmunkState)
   {
      case STATE_ONLINE:
         statusImage.src = 'chrome://bitmunk/content/bitmunk16-online.png';
         statusLabel.value = 'online';
         break;
      case STATE_OFFLINE:
         statusImage.src = 'chrome://bitmunk/content/bitmunk16-offline.png';
         statusLabel.value = 'offline';
         break;
      case STATE_LOADING:
      case STATE_UNKNOWN:
         statusImage.src = 'chrome://bitmunk/content/bitmunk16-offline.png';
         statusLabel.value = 'loading';
         break;
   }
}

/**
 * Sends a directive to the BPE via an event.
 * 
 * @param doc the page document.
 * @param bmd the directive to send.
 */
function sendDirective(doc, bmd)
{
   var e = doc.createEvent('Events');
   e.initEvent('bitmunk-directive-queued', true, true);
   e.detail = bmd;
   doc.dispatchEvent(e);
}

/**
 * Queues a Bitmunk Directive. If the Bitmunk app is already online, the
 * directive is sent immediately. Otherwise, the directive is queued and
 * an attempt is made to start the Bitmunk app. 
 * 
 * @param bmd the bitmunk directive.
 */
function queueDirective(bmd)
 {
   _bitmunkLog('Process Bitmunk Directive: ' + bmd);
   
   if(gBitmunkState == STATE_ONLINE)
   {
      // get the Bitmunk tab and send the directive
      var tab = getBitmunkTab();
      sendDirective(tab.contentDocument, bmd);
   }
   else
   {
      // queue the directive and attempt to manage bitmunk
      gDirectives.push(bmd);
      manageBitmunk();
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
      if(topic == 'bitmunk-directive')
      {
         queueDirective(data);
      }
   }
};

// register the Bitmunk directive observer
const service = Components.classes['@mozilla.org/observer-service;1']
   .getService(Components.interfaces.nsIObserverService);
service.addObserver(bitmunkDirectiveObserver, 'bitmunk-directive', false);

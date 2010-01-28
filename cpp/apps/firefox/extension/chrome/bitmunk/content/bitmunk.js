/**
 * The Bitmunk extension Javascript functionality for the main Firefox UI.
 *
 * @author Manu Sporny
 */

/**
 * Contains the global Bitmunk extension online status.
 */
var gBitmunkOnline = false;

/**
 * Flag that shows the management UI after startup.
 */
var gShowManagementUiAfterStartup = false;

/**
 * Interval ID for checking to see if the Bitmunk Software is online.
 */
var gOnlineIntervalId = null;

/**
 * The last time a software start was attempted.
 */
var gLastStartupAttempt = 0;

/**
 * The action that should be executed whenever the Bitmunk node comes online.
 */
var gStartupAction = null; 

/**
 * The complete BPE URL, which may be modified whenever a communication error
 * occurs.
 */
var gBpeUrl = BPE_URL + "19200"

/**
 * Start the Bitmunk node if it is not running and open a new tab showing 
 * the status of the Bitmunk software once the node is operational.
 */
function manageBitmunk()
{
   if(gBitmunkOnline)
   {
	   mainWindow = getCurrentWindow();
	   newTab = mainWindow.getBrowser().addTab(
	      gBpeUrl + "/bitmunk");
	   mainWindow.getBrowser().selectedTab = newTab;
	}
	else
	{
      gShowManagementUiAfterStartup = true;
      startBitmunkApplication();
   }
}

/**
 * Sets a timer to update the interface from time to time to show the person
 * on the current system the status of the Bitmunk software.
 */
function updateBitmunkStatus()
{
   sendJsonRequest(gBpeUrl + "/api/3.0/system/test/echo",
      updateBitmunkStatusDisplay, "{}");

   if(gOnlineIntervalId == null)
   {
      gOnlineIntervalId = 
         getCurrentWindow().setInterval(updateBitmunkStatus, 3000);
   }
}

/**
 * Updates the Firefox display window with the updated status.
 */
function updateBitmunkStatusDisplay(obj)
{
   var online = true;
   
   // if "type" is defined in obj, there was an exception.
   if(obj.code == 0)
   {
      online = false;
   }

   // if the old status doesn't equal the new status, update the UI
   if(gBitmunkOnline != online)
   {
      gBitmunkOnline = online;

      statusImage = document.getElementById("bitmunk-status-image");
      statusLabel = document.getElementById("bitmunk-status-label");
      controlLabel = document.getElementById("bitmunk-control-label");

      // update the image and the label
      if(online)
      {
         statusImage.src = "chrome://bitmunk/content/bitmunk16-online.png";
         statusLabel.value = "online";
      }
      else
      {
         statusImage.src = "chrome://bitmunk/content/bitmunk16-offline.png";
         statusLabel.value = "offline";
      }
   }

   // check to see if there should be any start-up actions that are queued
   if(gBitmunkOnline)
   {
      if(gStartupAction != null)
      {
         // check to see if there is a pending start-up action
         gStartupAction();
         gStartupAction = null;
      }
      else if(gShowManagementUiAfterStartup)
      {
         // check to see if the management UI should be shown after start-up
         gShowManagementUiAfterStartup = false;
         manageBitmunk();
      }
   }
   else
   {
      // get the plugin
      var plugin = Components
      .classes["@bitmunk.com/xpcom;1"]
      .getService()
      .QueryInterface(
         Components.interfaces.nsIBitmunkExtension);
   
      // retrieve the port number for the plugin, if it exists
      _bitmunkLog("launchBitmunk(): retrieving Bitmunk node port...");
      var port = {};
      plugin.getBitmunkPort(port);
      _bitmunkLog("launchBitmunk(): Bitmunk node port: " + port.value);
      gBpeUrl = BPE_URL + port.value;
   }
}

/**
 * Kicks off a thread to start the Bitmunk application if it isn't
 * running on the specified host and port.
 *
 * @param callback the callback function to call with the status of the
 *        echo test performed in this call. If the object has a code == 0,
 *        then the bitmunk application is offline.
 */
function startBitmunkApplication(callback)
{
   if(callback == null)
   {
      sendJsonRequest(gBpeUrl + "/api/3.0/system/test/echo", 
         launchBitmunk, "{}");
   }
   else
   {
      sendJsonRequest(gBpeUrl + "/api/3.0/system/test/echo", 
         callback, "{}");
   }
}

/**
 * Launches the Bitmunk application if it is currently offline.
 */
function launchBitmunk(obj)
{
   var now = new Date();

   _bitmunkLog("launchBitmunk() called: " + now.getTime());

   // Check to see if the object has a status of online. If the
   // connection failed, assume the Bitmunk application is offline
   // and start it.
   if((obj.code == 0) && ((now.getTime() - gLastStartupAttempt) > 10000))
   {
      gLastStartupAttempt = now.getTime();
      _bitmunkLog("launchBitmunk(): attempting to launch application.");

      var plugin = Components
         .classes["@bitmunk.com/xpcom;1"]
         .getService()
         .QueryInterface(
            Components.interfaces.nsIBitmunkExtension);
      
      // retrieve the port number for the plugin
      _bitmunkLog("launchBitmunk(): retrieving Bitmunk node port...");
      var port = {};
      plugin.getBitmunkPort(port);
      _bitmunkLog("launchBitmunk(): Bitmunk node port: " + port.value);
      gBpeUrl = BPE_URL + port.value;

      // launch the Bitmunk application
      var pid = {};
      if(plugin.launchBitmunk(pid));
      {
         _bitmunkLog("Bitmunk PID: " + pid.value);
      }
   }
   else
   {
      _bitmunkLog("launchBitmunk(): application launch denied.");
   }
}

/**
 * Open a tab to display the Bitmunk P2P Plugin help page.
 */
function showBitmunkProcessDirective(bmdFile)
 {
   _bitmunkLog("Process Bitmunk Directive: " + bmdFile);

   mainWindow = getCurrentWindow();
   contentWindow = mainWindow.getBrowser().contentWindow; 
   documentWindow = mainWindow.getBrowser().documentWindow; 
   var intervalId;

	/**
	 * The Bitmunk Process Observer is used to process onload events for 
	 * BMD plugin windows after the command to load the chrome window is
	 * executed.
	 */
	bitmunkProcessObserver = 
	{
	   setBmdContent: function()
	   {
	      // get the content document and the bmdFile for the document
	      mainWindow = getCurrentWindow();
         contentDocument = mainWindow.getBrowser().contentDocument;

         // get the directive element
         var directiveElem = 
            contentDocument.getElementById("queued-bitmunk-directive");

         // if the directive element was found, set the inner HTML, otherwise
         // try again in 250ms.
         if(directiveElem != null)
         {
            directiveElem.innerHTML = bmdFile;
            mainWindow.clearInterval(intervalId);
         }
	   }
	};

	// create the start-up action that should run when the bitmunk node is
	// verified to be operational
   var startupAction = function() 
   {   
      // load the Bitmunk chrome page to process the BMD file
      contentWindow.location.replace(
         gBpeUrl + "/bitmunk#purchases");
   
      // FIXME: This is a fairly nasty way to get this done, we should be 
      // waiting for an onload event, but both the content and document windows 
      // are destroyed at some point after this call. 
      intervalId = 
         mainWindow.setInterval(bitmunkProcessObserver.setBmdContent, 250);
   };
   
   // check to see if the node is online, if it is then run the startupAction,
   // otherwise wait for the node to come online and then run the startupAction.
   if(gBitmunkOnline == true)
   {
      startupAction();
   }
   else
   {
      startBitmunkApplication();
      gStartupAction = startupAction;
   }
}

/**
 * Open a tab to display the Bitmunk P2P Plugin help page.
 */
function showBitmunkHelp()
 {
   _bitmunkLog("Showing Bitmunk Help");
         
   mainWindow = getCurrentWindow();
   newTab = mainWindow.getBrowser().addTab(
      "chrome://bitmunk/content/help.html");
   mainWindow.getBrowser().selectedTab = newTab;   
}

/**
 * Open a tab to display the Bitmunk Website.
 */
function showBitmunkWebsite()
{
   _bitmunkLog("Showing Bitmunk Website");
   
   mainWindow = getCurrentWindow();
   newTab = mainWindow.getBrowser().addTab("http://bitmunk.com/");
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
      if(topic == "bitmunk-directive")
      {
         showBitmunkProcessDirective(data);
      }
   }
};

// register the Bitmunk directive observer.
const service = Components.classes["@mozilla.org/observer-service;1"]
   .getService(Components.interfaces.nsIObserverService);
service.addObserver(bitmunkDirectiveObserver, "bitmunk-directive", false);

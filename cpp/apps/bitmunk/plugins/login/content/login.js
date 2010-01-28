/**
 * Bitmunk Web UI --- Login
 *
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 */
(function($)
{
   // log category
   var sLogCategory = 'bitmunk.webui.Login';

   var sScriptTask = bitmunk.resource.getScriptTask(
      'bitmunk.webui.Login', 'login.js');
   
   // event namespace
   var sNS = '.bitmunk-webui-Login';
   
   // Plugin Image URL
   var sPluginImageUrl = '/bitmunk/plugins/bitmunk.webui.Login/images/';
   
   /**
    * Callback whenever the login button is clicked.
    * 
    */
   var loginClicked = function(eventObject)
   {
      // DEBUG: output
      //bitmunk.log.debug(sLogCategory, 'Login clicked');
      
      // turn button off
      $('#loginButton')
         .removeClass('on')
         .addClass('off');
      $('#loginDialog .formInput').enable(false);
      
      // show activity indicator and signal logging in
      $('#loginActivity .activityMessage').html('Logging in...'); 
      $('#loginActivity').fadeTo(0, 1, 
            function() { $(this).css('visibility', 'visible'); });
      
      // clear feedback
      clearFeedback('username');
      clearFeedback('password');
      clearFeedback('general');
      
      // attempt to log in using data from form
      bitmunk.user.login(
      {
         username: $('#loginUsername')[0].value,
         password: $('#loginPassword')[0].value
      });
   };
   
   /**
    * Callback whenever there is a login success.
    */
   var loginSuccess = function()
   {
      // clear password, leave username for easier re-login
      $('#loginPassword')[0].value = '';
      
      // clear all feedback
      clearFeedback('username');
      clearFeedback('password');
      clearFeedback('general');
      
      // logged in, show loading...
      $('#loginActivity .activityMessage').html('Loading...');
      
      // leave login button disabled
   };
   
   /**
    * Callback whenever there is a login failure.
    * 
    * @exception the bitmunk exception data.
    */
   var loginFailure = function(eventObject, exception)
   {
      // add feedback for failed fields
      try
      {
         addFeedback(
            'username', 'error',
            exception.details.errors.username.message);
      }
      catch(e)
      {
         // clear username feedback
         clearFeedback('username');
      }
      
      try
      {
         addFeedback(
            'password', 'error',
            exception.details.errors.password.message);
      }
      catch(e)
      {
         // clear password feedback
         clearFeedback('password');
      }
      
      // add general feedback
      try
      {
         if(exception.type != 'db.validation.ValidationError')
         /*
         {
            addFeedback(
               'general', 'error',
               'Please correct your login information.');
         }
         else
         */
         {
            addFeedback(
               'general', 'error', exception.message);
         }
      }
      catch(e)
      {
         // clear general feedback
         clearFeedback('general');
      }
      
      // hide activity indicator
      $('#loginActivity').fadeTo('normal', 0,
         function() {
            $(this).css('visibility', 'hidden');
            $('#loginActivity .activityMessage').empty(); 
         });
      
      // turn button on
      $('#loginButton')
         .addClass('on')
         .removeClass('off');
      $('#loginDialog .formInput').enable();
      
      // bind login button click
      $('#loginButton').one('click', loginClicked);
   };
   
   /**
    * Callback whenever there is a logout event.
    */
   var logout = function(eventObject)
   {
      // clear input fields
      $('#loginUsername')[0].value = '';
      $('#loginPassword')[0].value = '';
      
      // clear all feedback
      clearFeedback('username');
      clearFeedback('password');
      clearFeedback('general');
      
      // leave login button disabled
   };
   
   /**
    * Adds a feedback message to the UI.
    * 
    * @param name the name of the feedback widget.
    * @param type the type of message.
    * @param message the message to display.
    */
   var addFeedback = function(name, type, message)
   {
      // FIXME: debug output
      //bitmunk.log.debug(sLogCategory, 'Adding ' + type + ' message');
      
      var feedback = $('<p class="' + type + '">' + message + '</p>');
      $('#feedback-' + name).prepend(feedback);

      // show feedback box
      $('#feedbackBox-' + name).slideDown('fast');
   };
   
   /**
    * Clears all feedback messages.
    * 
    * @param name the name of the feedback widget.
    */
   var clearFeedback = function(name)
   {
      // clear feedback
      $('#feedback-' + name).empty();
      
      // hide feedback box
      $('#feedbackBox-' + name).slideUp('fast');
   };
   
   /** jQuery Page Functions **/
   
   /**
    * The show function is called whenever the page is shown. 
    */
   var show = function(task)
   {
      // turn login button on
      $('#loginButton')
         .addClass('on')
         .removeClass('off');
      $('#loginDialog .formInput').enable();
      
      // bind login button click
      $('#loginButton').one('click', loginClicked);
      
      // setup form-like enter handling on password field
      $('#loginPassword').keypress(function(e)
      {
         // check for enter keycode and fake a login button click
         if(e.which == 13)
         {
            $('#loginButton').click();
         }
      });
      
      // bind to events
      $(window)
         .bind('bitmunk-login-success' + sNS, loginSuccess)
         .bind('bitmunk-login-failure' + sNS, loginFailure)
         .bind('bitmunk-logout' + sNS, logout);
      
      // if username is already entered then focus on password else on username
      if($('#loginUsername')[0].value == '')
      {
         $('#loginUsername').focus();
      }
      else
      {
         $('#loginPassword').focus();
      }
   };
   
   /**
    * The hide function is called whenever the page is hidden.
    */
   var hide = function(task)
   {
      // unbind events
      $(window).unbind(sNS);
      
      // clear password, leave username for easier re-login
      $('#loginPassword')[0].value = '';
      
      // hide activity indicator
      $('#loginActivity').css('visibility', 'hidden');
      $('#loginActivity .activityMessage').empty(); 
      
      // clear all feedback
      clearFeedback('username');
      clearFeedback('password');
      clearFeedback('general');
      
      // send the interface hidden event
      $.event.trigger('bitmunk-interface-hidden');
   };
   
   bitmunk.resource.setupResource(
   {
      pluginId: 'bitmunk.webui.Login',
      resourceId: 'login',
      show: show,
      hide: hide
   });
   
   sScriptTask.unblock();
})(jQuery);

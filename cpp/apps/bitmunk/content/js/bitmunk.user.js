/**
 * Bitmunk Web UI --- User Functions
 *
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 *
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
(function($) {
   // logging category
   var cat = 'bitmunk.webui.core.User';
   
   // FIXME: move elsewhere, doesn't have to wait for document to
   // be ready to define these functions
   bitmunk.user = {};
   
   /**
    * Attempts a login given the username and password given by the user.
    *
    * @event 'bitmunk-login-success' on success.
    * @event 'bitmunk-login-failure' on login failure.
    *
    * @param options:
    *    username: the username (string)
    *    password: the password (string)
    */
   bitmunk.user.login = function(options)
   {
      var req = {
         username: options.username,
         password: options.password
      };
      
      // time login process
      var timer = +new Date();
      
      $.ajax({
         type: 'POST',
         url: bitmunk.api.localRoot + 'webui/login',
         data: JSON.stringify(req),
         contentType: 'application/json',
         processData: false,
         // expecting no content 204 response
         dataType: 'text',
         success: function(data, textStatus) 
         {
            timer = +new Date() - timer;
            bitmunk.log.debug('timing',
               'network login completed in ' + timer + ' ms');
            $.event.trigger('bitmunk-login-success');
         },
         error: function(xhr, textStatus, errorThrown) 
         {
            bitmunk.log.debug(cat, 'login failure', arguments);
            var ex;
            try
            {
               // assuming error is called with a BtpException
               ex = JSON.parse(xhr.responseText);
               bitmunk.log.debug(cat, 'login failure exception', ex);
               if(!((ex.code === 0) && ex.message))
               {
                  // Does not look like a BtpException.  Fake a useful one.
                  ex = {
                     code: 0,
                     message: 'An unknown error occurred while logging in.'
                     //type: '...'
                  };
               }
               else if(ex.cause)
               {
                  if(ex.cause.type.indexOf('User.Invalid') != -1 &&
                     ex.cause.cause &&
                     ex.cause.cause.type.indexOf('NotFound') != -1)
                  {
                     ex = {
                       code: 0,
                       message: 'The username you entered does not exist.'
                     };
                  }
                  else
                  {
                     ex = ex.cause;
                  }
               }
            }
            catch(e)
            {
               ex = 
               {
                  code: 0,
                  message: 'An unknown error occurred while logging in.'
               };
            }
            $.event.trigger('bitmunk-login-failure', [ex]);
            //$.event.trigger('bitmunk-login-failure',
            //   ['[failure:' + textStatus + ':' + errorThrown + ':' +
            //   xhr.status + ']']);
         },
         complete: function(xhr, textStatus) {
            //bitmunk.log.debug(cat, 'login complete', arguments);
         },
         xhr: window.krypto.xhr.create
      });
   };
   
   /**
    * Performs a logout. Clear local info and logout from node.
    *
    * @param details object with logout details (optional)
    *    By convention, the UI will look for the following keys:
    *       exception: an XMLHttpRequest exception 
    *       message: a message for the user 
    *
    * @event 'bitmunk-logout' on completion regardless of successful
    *        contact with the server.
    */
   bitmunk.user.logout = function(details)
   {
      $.ajax({
         type: 'POST',
         // Note: "full=true" means log out of the node too,
         // not just the webui session
         url: bitmunk.api.localRoot + 'webui/logout?full=true',
         // expecting no content 204 response
         dataType: 'text',
         complete: function(xhr, textStatus) {
            bitmunk.log.debug(cat, 'logout', xhr, textStatus);
            // ensure cookie cleared
            window.krypto.xhr.removeCookie('bitmunk-user-id');
            window.krypto.xhr.removeCookie('bitmunk-session');
            $.event.trigger('bitmunk-logout', [details]);
         },
         // do not use global failure handler
         global: false,
         xhr: window.krypto.xhr.create
      });
   };
   
   /**
    * Client assumes logged in state when user id cookie is not expired.
    * Server can expire user session and require re-login regardless of this
    * state.
    */
   bitmunk.user.isLoggedIn = function()
   {
      return window.krypto.xhr.getCookie('bitmunk-user-id') ? true : false;
   };
   
   /**
    * Get the current user id.
    *
    * @return the user id.
    */
   bitmunk.user.getUserId = function()
   {
      var cookie = window.krypto.xhr.getCookie('bitmunk-user-id');
      return cookie === null ? null : cookie.value;
   };
   
   /**
    * Get the current username.
    *
    * @return the username.
    */
   bitmunk.user.getUsername = function()
   {
      var cookie = window.krypto.xhr.getCookie('bitmunk-username');
      return cookie === null ? null : unescape(cookie.value);
   };
   
   /**
    * Get the accounts for the current user and provides them via a callback.
    * 
    * @param success the callback with signature(accounts), where the
    *           accounts will contain a hashmap key'd on account ID.
    * @param error the error callback that takes an xmlhttp request,
    *           the text status, and the error that was thrown.
    */
   bitmunk.user.getAccounts = function(success, error)
   {
      // build arguments for fetching accounts
      bitmunk.api.proxy(
      {
         method: 'GET',
         url: bitmunk.api.secureBitmunkRoot + "accounts",
         params:
         {
            owner: bitmunk.user.getUserId(),
            nodeuser: 1
         },
         success: function(data, textStatus) 
         {
            // parse account resource set
            var resourceSet = JSON.parse(data);
            
            // DEBUG: output
            //bitmunk.log.debug(cat, "retrieved accounts: ", resourceSet);
            
            // build accounts map
            var accounts = {};
            $.each(resourceSet.resources, function(i, account)
            {
               accounts[account.id] = account;
            });
            
            // pass accounts to callback
            success(accounts);
         },
         error: error
      });
   };
})(jQuery);

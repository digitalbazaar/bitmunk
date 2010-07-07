/**
 * Bitmunk Web UI --- Test
 *
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 * 
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
(function($) {

var init = function(task)
{
   // logging category
   var cat = 'bitmunk.webui.Test';
   
   // FIXME: should be in a model
   var updateAccessRules = function()
   {
      // DEBUG: output
      bitmunk.log.debug(cat, 'getting access rules...');
      $.ajax(
      {
         type: 'GET',
         url: '/api/3.0/webui/session/access/users',
         dataType: 'json',
         success: function(rules)
         {
            // DEBUG: output
            bitmunk.log.debug(cat, 'access rules retrieved', rules);
            
            // clear old access rules
            $('#accessRules').empty();
            
            // add new access rules
            $.each(rules, function(i, rule)
            {
               var ruleNode = $
                  ('<li>Grant user ID: ' + rule.userId +
                  ' @ ' + rule.ip + '</li>');
               $('#accessRules').append(ruleNode);
            });
         },
         error: function()
         {
            ruleNode.html('<li>Error: Failed to fetch user access rules.</li>');
         },
         xhr: window.krypto.xhr.create
      });
   };
      
   var show = function() 
   {
      bitmunk.log.debug(cat, 'test show');
      
      // update user access rules display
      updateAccessRules();
      
      // create audio download directive
      $('#audioDirectiveButton').click(function()
      {
         // DEBUG: output
         bitmunk.log.debug(cat, 'Creating Audio Directive...');
         
         var dNode = $('<li>Creating Audio Directive...</li>');
         $('#createdDirectives').append(dNode);
         
         bitmunk.api.proxy(
         {
            method: 'POST',
            url: '/api/3.0/system/directives?nodeuser=' +
                 bitmunk.user.getUserId(),
            data:
            {
               version: '3.0',
               type: 'peerbuy',
               data:
               {
                  mediaId: 2,
                  ware:
                  {
                     id: 'bitmunk:bundle',
                     mediaId: 2,
                     fileInfos: [
                     {
                        id: 'b79068aab92f78ba312d35286a77ea581037b109',
                        mediaId: 2
                     }]
                  },
                  sellerLimit: 10
               }
            },
            success: function(data)
            {
               data = JSON.parse(data);
               dNode.html(
                  'Created Download Id: ' + data.directiveId +
                  ' @:' + new Date());
            },
            error: function()
            {
               dNode.html('ERROR! @:' + new Date());         
            }
         });
      });
      
      // create collection download directive
      $('#collectionDirectiveButton').click(function()
      {
         // DEBUG: output
         bitmunk.log.debug(cat, 'Creating Collection Directive...');
         
         var dNode = $('<li>Creating Collection Directive...</li>');
         $('#createdDirectives').append(dNode);
         
         var directive =
         {
            version: '3.0',
            type: 'peerbuy',
            data:
            {
               mediaId: 1,
               ware:
               {
                  id: 'bitmunk:bundle',
                  mediaId: 1,
                  fileInfos: [
                     {
                        id: 'b79068aab92f78ba312d35286a77ea581037b109',
                        mediaId: 2
                     },
                     {
                        id: '5e5a503fbd8217274819616929da1c041c30b4fc',
                        mediaId: 3
                     },
                     {
                        id: '1ab6dc46818c4f497b32edf8842047f78fa448b7',
                        mediaId: 4
                     },
                     {
                        id: '944c8d2db1e709f1e438ba5f5644c9485b6b6291',
                        mediaId: 5
                     },
                     {
                        id: 'ed10444dc6d9c36fdd21556192d2ca2f574cda7c',
                        mediaId: 6
                     },
                     {
                        id: '252e18e46d73f6481f98ff5952ff4b8cfbd30781',
                        mediaId: 7
                     },
                     {
                        id: '7fe75342e25ba9fdd478614d3d3267585acdfcbc',
                        mediaId: 8
                     },
                     {
                        id: 'aa36394f53644e8c2c9aaf5cab3da877b2f03c8f',
                        mediaId: 9
                     }
                  ]
               },
               sellerLimit: 10
            }
         };
         
         bitmunk.api.proxy(
         {
            method: 'POST',
            url: '/api/3.0/system/directives?nodeuser=' +
                 bitmunk.user.getUserId(),
            data: directive,
            success: function(data)
            {
               data = JSON.parse(data);
               dNode.html(
                  'Created Download Id: ' + data.directiveId +
                  ' @:' + new Date());         
            },
            error: function()
            {
               dNode.html('ERROR! @:' + new Date());         
            }
         });
      });
      
      // create custom directive
      $('#customDirectiveButton').click(function()
      {
         // DEBUG: output
         bitmunk.log.debug(cat, 'Creating Custom Directive...');
         
         var directive = $('#customDirective').val();
         var dNode = $('<li>Creating Custom Directive...</li>');
         $('#createdDirectives').append(dNode);
         
         bitmunk.api.proxy(
         {
            method: 'POST',
            url: '/api/3.0/system/directives?nodeuser=' +
                 bitmunk.user.getUserId(),
            data: JSON.parse(directive),
            success: function(data)
            {
               data = JSON.parse(data);
               dNode.html(
                  'Created Download Id: ' + data.directiveId +
                  ' @:' + new Date());         
            },
            error: function()
            {
               dNode.html('ERROR! @:' + new Date());         
            }
         });
      });
      
      // create event subscription
      $('#subscribeButton').click(function()
      {
         // DEBUG: output
         bitmunk.log.debug(cat, 'subscribing to "TESTEVENT"...');
         
         bitmunk.events.subscribe(
         {
            events: [
            {
               type: 'TESTEVENT',
               filter: { details: {'foo': 'bar'} },
               callback: function(event)
               {
                  bitmunk.log.debug(cat, 'received "TESTEVENT"');
               },
            },
            {
               type: 'bitmunk.system.Directive.created',
               callback: function(event)
               {
                  bitmunk.log.debug(cat,
                     'directive created, id: ' + event.details.directiveId);
               }
            },
            {
               type: 'bitmunk.system.Directive.processed',
               callback: function(event)
               {
                  bitmunk.log.debug(cat,
                     'Processed directive: ' + event.details.directiveId);
               }
            },
            {
               type: 'bitmunk.system.Directive.exception',
               callback: function(event)
               {
                  bitmunk.log.debug(cat,
                     'Directive processing exception: ' +
                     JSON.stringify(event.details.exception));
               }
            }],
            success: function(data)
            {
               bitmunk.log.debug(cat, 'subscribed to "TESTEVENT"');
               $('#observerId').html(data[0]);
            },
            error: function()
            {
               bitmunk.log.debug(cat, 'failed to subscribe to "TESTEVENT"');
               $('#observerId').html('None');
            }
         });
      });
      
      // create event sending button
      $('#sendEventButton').click(function()
      {
         // DEBUG: output
         bitmunk.log.debug(cat, 'sending "TESTEVENT"...');
         
         bitmunk.events.sendEvents(
         {
            send: [{type: 'TESTEVENT', details:{'foo': 'bar'}}],
            success: function()
            {
               bitmunk.log.debug(cat, '"TESTEVENT" sent.');
            }
         });
      });
      
      // create daemon event adding button
      $('#addDaemonEventsButton').click(function()
      {
         // DEBUG: output
         bitmunk.log.debug(cat, 'scheduling 10 "TESTEVENT"s...');
         
         bitmunk.events.addDaemonEvents(
         {
            add: [{
               event: {
                  type: 'TESTEVENT',details:{'foo':'bar'}
               },
               interval: 1000,
               count: 10
            }],
            success: function()
            {
               bitmunk.log.debug(cat, '"TESTEVENT" scheduled with daemon.');
            }
         });
      });
      
      // create event unsubscribe button
      $('#unsubscribeButton').click(function()
      {
         // DEBUG: output
         bitmunk.log.debug(cat, 'unsubscribing from "TESTEVENT"...');
         
         bitmunk.events.deleteObservers([$('#observerId').html()]);
         $('#observerId').html('None');
      });
      
      // create shutdown button
      $('#testShutdownButton').click(function()
      {
         bitmunk.log.debug(cat, 'shutting down');
         bitmunk.shutdown();
      });
      
      // create restart button
      $('#testRestartButton').click(function()
      {
         bitmunk.log.debug(cat, 'restart down');
         bitmunk.restart();
      });
      
      // create access control
      $('#accessButton').click(function()
      {
         // DEBUG: output
         bitmunk.log.debug(cat, 'changing access...');
         
         var entry = {};
         entry.username = $('#accessUsername').val();
         entry.ip = $('#accessIp').val();
         var access = $('#grant')[0].checked ? 'grant' : 'revoke';
         
         var ruleNode = $('<li>Creating Access Rule...</li>');
         $('#accessRules').append(ruleNode);
         
         $.ajax(
         {
            type: 'POST',
            url: '/api/3.0/webui/session/access/' + access,
            contentType: 'application/json',
            data: JSON.stringify([entry]),
            success: function()
            {
               // DEBUG: output
               bitmunk.log.debug(cat, 'access changed.');
               
               // update user access rules display
               updateAccessRules();
            },
            error: function()
            {
               ruleNode.html('<li>Error: ' + access + ' ' + entry.username +
                  ' @ ' + entry.ip + '</li>');
            },
            xhr: window.krypto.xhr.create
         });
      });
      
      $('#templateButton').click(function() 
      {
         // DEBUG: output
         bitmunk.log.debug(cat, 'Template Test');
         
         var values = {
            key: $('#templateKey').val(),
            value: $('#templateValue').val()
         };
         
         var template = bitmunk.resource.get(
            'bitmunk.webui.Test', 'tmpl.html', true);
         var test = $(template.process(values));
         $('#templateResults').append(test);
      });
      
      $('#modelButton').click(function()
      {
         // DEBUG: output
         bitmunk.log.debug(cat, 'Model Test');
         
         bitmunk.events.sendEvents(
         {
            send: [{
               type: 'TESTEVENT',
               details:
               {
                  'id': 'test-1',
                  'value': $('#testModelInput').val()
               }
            }],
            success: function()
            {
               // DEBUG: output
               bitmunk.log.debug(cat, 'TESTEVENT Sent');
            },
            error: function()
            {
               // DEBUG: output
               bitmunk.log.debug(cat, 'TESTEVENT Error');
            }
         });
      });
      
      // bind listener for testevent events
      $('#testModelValue').bind('TESTEVENT.TEST/TESTTABLE',
         function(e, data) {
            $(this).text(data.value);
         });
      
      // create task test button
      $('#testTaskButton').click(function()
      {
         // DEBUG: output
         bitmunk.log.debug(cat, '1. starting a task...');
         
         try
         {
            bitmunk.task.start(
            {
               type: 'testtask1',
               // task run function
               run: function(task)
               {
                  bitmunk.log.debug(cat, '2. top level task.');
                  
                  task.next(function(task)
                  {
                     bitmunk.log.debug(cat, '4. doing next task.');
                     
                     task.block();
                     bitmunk.events.sendEvents(
                     {
                        send: [{type: 'foobar'}],
                        success: function()
                        {
                           bitmunk.log.debug(cat, '5. sent event.');
                           
                           task.next(function(task)
                           {
                              bitmunk.log.debug(cat,
                                 '6. doing subtask, should finish the task.');
                           });
                           
                           task.unblock();
                        },
                        error: function()
                        {
                           bitmunk.log.debug(cat, '5. failed to send event.');
                           task.fail();
                        }
                     });
                  });
                  
                  bitmunk.log.debug(cat, '3. ready to do next task.');
               },
               // callback for task success
               success: function(task)
               {
                  bitmunk.log.debug(cat, '7. task is finished.');
               },
               // callback for task failure
               failure: function(task)
               {
                  bitmunk.log.debug(cat, 'should never see me!');
               }
            });
         }
         catch(e)
         {
            console.log(e)
         };
      });
      
      // create task abort button
      $('#testTaskAbortButton').click(function()
      {
         // DEBUG: output
         bitmunk.log.debug(cat, '1. starting a task...');
         
         try
         {
            bitmunk.task.start(
            {
               type: 'testtask2',
               // task run function
               run: function(task)
               {
                  task.next(function(task)
                  {
                     bitmunk.log.debug(cat, '2. part 2.');
                  });
                  
                  task.next(function(task)
                  {
                     bitmunk.log.debug(cat, '3. part 3.');
                     
                     task.next(function(task)
                     {
                        task.next(function(task)
                        {
                           bitmunk.log.debug(cat, '5. should not see me!');
                        });
                        
                        bitmunk.log.debug(cat, '4. aborting 5.');
                        task.userData = 'foo';
                        task.fail(task.parent);
                     });
                     
                     task.next(function(task)
                     {
                        if(task.error)
                        {
                           bitmunk.log.debug(cat, '6. aborting 7.');
                           task.fail(task.parent.parent.parent);
                        }
                     });
                  });
                  
                  task.next(function(task)
                  {
                     bitmunk.log.debug(cat, '7. shouldn\'t see me!');
                  });
               },
               // callback for task success
               success: function(task)
               {
                  bitmunk.log.debug(
                     cat, 'should never see me!',
                     task.error, task.userData);
               },
               // callback for task failure
               failure: function(task)
               {
                  bitmunk.log.debug(
                     cat, '8. abort task is finished.',
                     task.error, task.userData);
               }
            });
         }
         catch(e)
         {
            console.log(e)
         };
      });
      
      // Notification Tests
   
      $('#testDefaultMessageButton').click(function() 
      {
         var msg = 'Default message test, which is just a bit longer and more' +
            ' full of itself then the other tests.  ' +
            'But we <strong>would</strong> like to see what happens with' +
            ' <em>long</em> messages and ones with' +
            ' <a class="external" href="http://bitmunk.com/">links</a>' +
            ' in them.';
         
         bitmunk.log.debug(cat, 'Default message test');
         $('#messages').jGrowl(msg, {
            sticky: ($('#testMessagesSticky').fieldValue().length > 0)
         });
      });
      
      $('#testOkMessageButton').click(function() 
      {
         bitmunk.log.debug(cat, 'Ok message test');
         $('#messages').jGrowl('Test "Ok"!', {
            theme: 'ok',
            sticky: ($('#testMessagesSticky').fieldValue().length > 0)
         });
      });
      
      $('#testInfoMessageButton').click(function() 
      {
         bitmunk.log.debug(cat, 'Info message test');
         $('#messages').jGrowl('Test "Info"!', {
            theme: 'info',
            sticky: ($('#testMessagesSticky').fieldValue().length > 0)
         });
      });
      
      $('#testWarningMessageButton').click(function() 
      {
         bitmunk.log.debug(cat, 'Warning message test');
         $('#messages').jGrowl('Test "Warning"!', {
            theme: 'warning',
            sticky: ($('#testMessagesSticky').fieldValue().length > 0)
         });
      });
      
      $('#testErrorMessageButton').click(function() 
      {
         bitmunk.log.debug(cat, 'Error message test');
         $('#messages').jGrowl('Test "Error"!', {
            theme: 'error',
            sticky: ($('#testMessagesSticky').fieldValue().length > 0)
         });
      });
   };
      
   var hide = function()
   {
      // send the interface hidden event
      $.event.trigger('bitmunk-interface-hidden');
   };
   
   bitmunk.resource.extendResource(
   {
      pluginId: 'bitmunk.webui.Test',
      resourceId: 'test',
      models: ['TEST'],
      show: show,
      hide: hide
   });
};

bitmunk.resource.registerScript({
   pluginId: 'bitmunk.webui.Test',
   resourceId: 'test.js',
   depends: {},
   init: init
});

})(jQuery);

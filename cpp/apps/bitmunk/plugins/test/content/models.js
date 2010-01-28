/**
 * Bitmunk Web UI --- Test Models
 *
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 */
(function($) 
{
   // logging category
   var cat = 'bitmunk.webui.Test.models';
   
   var sScriptTask = bitmunk.resource.getScriptTask(
      'bitmunk.webui.Test', 'models.js');
      
   bitmunk.log.debug(cat, 'will load model');
   
   var sTestModelName = 'TEST';
   
   var start = function(task)
   {
      // register model
      bitmunk.model.register(task,
      {
         model: sTestModelName,
         tables: [{
            name: 'TESTTABLE',
            eventGroups: [{
               events: ['TESTEVENT'],
               eventCallback: function(event, seqNum)
               {
                  bitmunk.log.debug(cat, 'model got event', event, seqNum);
                  bitmunk.model.update(
                     sTestModelName, 'TESTTABLE',
                     event.details.id, event.details.value, seqNum);
                  //bitmunk.log.debug(
                  //   cat, 'model test-1',
                  //   bitmunk.model.fetch('TEST', 'TESTTABLE', 'test-1'));
                  $.event.trigger('TESTEVENT', [event.details]);
               }
            }]
         }]
      });
   };
   
   bitmunk.model.addModel(sTestModelName, {start: start});
   sScriptTask.unblock();
   bitmunk.log.debug(cat, 'did load model');
})(jQuery);

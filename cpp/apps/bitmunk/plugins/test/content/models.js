/**
 * Bitmunk Web UI --- Test Models
 *
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 * 
 * Copyright (c) 2008-2010 Digital Bazaar, Inc.  All rights reserved.
 */
(function($) {

var init = function(task)
{
   // logging category
   var cat = 'bitmunk.webui.Test.models';
   
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
   
   bitmunk.log.debug(cat, 'did load model');
};

bitmunk.resource.registerScript({
   pluginId: 'bitmunk.webui.Test',
   resourceId: 'models.js',
   depends: {},
   init: init
});

})(jQuery);

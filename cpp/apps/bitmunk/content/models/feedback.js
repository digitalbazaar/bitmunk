/**
 * Bitmunk Feedback Model
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 * 
 * This model is used to store feedback messages from the backend. Its
 * tables are dynamically created by individual plugins to suit their
 * own needs. Keep in mind that dynamically created tables will persist
 * throughout the lifetime of the feedback model.
 * 
 * @author Dave Longley
 */
(function($) {

var init = function(task)
{
   // model name
   var sModel = 'feedback';
   
   // category for logger
   var sLogCategory = 'models.feedback';
   
   /**
    * Starts updating the feedback data model.
    * 
    * @param task the current task.
    */
   var start = function(task)
   {
      // register model
      bitmunk.model.register(task,
      {
         model: sModel,
         // create general feedback table, no associated events
         tables: [{name: 'general'}]
      });
   };
   
   bitmunk.model.addModel(sModel, {start: start});
};

bitmunk.resource.registerScript({
   pluginId: 'bitmunk.webui.Main',
   resourceId: 'models/feedback.js',
   depends: {},
   init: init
});

})(jQuery);

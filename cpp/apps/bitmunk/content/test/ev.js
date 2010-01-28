/**
 * Bitmunk Web UI --- Events Test
 *
 * @requires jQuery v1.2 or later (http://jquery.com/)
 * @requires json2.js (http://www.json.org/js.html)
 * @requires jQuery Comet Plugin (http://plugins.jquery.com/project/Comet)
 *
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 */
jQuery(function($)
{
   $('#start').click(function() {
      console.log('events start wasInit:%d', $.comet._bInitialized);
      $.comet.init('/api/3.0/webui/events');
   });
   
   $('#stop').click(function() {
      console.log('events stop wasInit:%d', $.comet._bInitialized);
      $.comet.disconnect();
   });
   
   $('#watch').click(function() {
      var e = $('#event')[0].value;
      console.log('will watch "%s"', e);
      $.comet.subscribe(e, function(message) {
         console.log('EVENT', message);
         $('#output').append('<p>' + message + '</p>');
      });
   });
   
   $('#trigger').click(function() {
      var e = $('#event')[0].value;
      console.log('will trigger "%s"', e);
      $.comet.publish(e);
   });
   
   $('#clear').click(function() {
      $('#output').empty();
   });
   
   /*
   $.post('/api/3.0/webui/login', {
      username: 'devuser',
      password: 'password'
   });
   */
   
   //$('selector').bind('subscription', function(event, data) {});
   //$.comet.publish();
   //$.comet.subscribe('', function() {});
   //$.comet.unsubscribe('');
});

#ifndef Preferences_h_
#define Preferences_h_

#include "Httpd.h"
#include "KeyValueBroker.h"

// A Preferences object starts an HTTP server that serves its html at its root
// and a JSON serialized representation of its keyValueBroker content at data.
//
// Form submissions from the html will be published through the keyValueBroker.
// It is important that the name attribute values used for form input elements
// correspond to the keys used in the keyValueBroker.
//
// AJAX scripts from the html can be used to populate the input elements
// of a form from the Preference object's data.
// For example, the following jQuery script will use AJAX to get the JSON data
// (an object of key/name/id, value pairs), locate each identified DOM element
// and set the value of its "value" attribute.
// It is important that the id attribute values used for elements
// correspond to the keys used in the keyValueBroker.
//
//	<script src="http://code.jquery.com/jquery-3.3.1.js"></script>
//	<script>
//		$(document).ready(function(){
//			$.ajax({
//				url: 		'data',
//				type:		'GET',
//				dataType:	'json',
//				cache:		false,
//			})
//				.done(function(idValues) {
//					for (const [id, value] of Object.entries(idValues)) {
//						$('#' + id).attr('value', value);
//					}
//				})
//				.fail(function(xhr, status, error){
//					alert('Unable to load values into form');
//					console.log('error: '  + error);
//					console.log('status: ' + status);
//					console.dir(xhr);
//				})
//	});
//	</script>

class Preferences : public Httpd {
private:
    char const *	html;
    KeyValueBroker &	keyValueBroker;
    Httpd::Uri const	uri;
    Httpd::Uri const	dataUri;
public:
    Preferences(char const * html, KeyValueBroker &);
};

#endif

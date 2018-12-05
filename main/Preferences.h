#ifndef Preferences_h_
#define Preferences_h_

#include "Httpd.h"
#include "KeyValueBroker.h"

// A Preferences object starts an HTTP server that serves html at its root (/)
// and a serialized representation of the keyValueBroker at /data.
// Form submissions from the html will be published through the keyValueBroker.
// AJAX scripts from the html can be used to populate a form from /data.
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

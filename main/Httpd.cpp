#include "Httpd.h"

template<typename T = esp_err_t>
static inline T throwIf(T t) {if (t) throw t; return t;}

template<typename T = esp_err_t, T is = -1>
static inline T throwIfIs(T t) {if (is == t) throw t; return t;}

Httpd::Config::Config(httpd_config_t const & that) : httpd_config_t(that) {}

Httpd::Config::Config() : httpd_config_t(HTTPD_DEFAULT_CONFIG()) {}

#define setter(name) Httpd::Config & Httpd::Config::name##_(decltype(name) s) \
    {name = s; return *this;}
setter(task_priority)
setter(stack_size)
setter(server_port)
setter(ctrl_port)
setter(max_open_sockets)
setter(max_uri_handlers)
setter(max_resp_headers)
setter(backlog_conn)
setter(lru_purge_enable)
setter(recv_wait_timeout)
setter(send_wait_timeout)
setter(global_user_ctx)
setter(global_user_ctx_free_fn)
setter(global_transport_ctx)
setter(global_transport_ctx_free_fn)
setter(open_fn)
setter(close_fn)
#undef setter

Httpd::Uri::Uri(
    Httpd &		httpd_,
    char const *	uri,
    httpd_method_t	method,
    Handler const &&	handler_)
:
    httpd_uri_t	{uri, method, &handlerFor, this},
    httpd	(httpd_),
    handler	(std::move(handler_))
{
    httpd_register_uri_handler(httpd, this);
}

/* static */ esp_err_t Httpd::Uri::handlerFor(httpd_req_t * req) {
    return static_cast<Uri *>(req->user_ctx)->handler(req);
}

Httpd::Uri::~Uri() {
    httpd_unregister_uri_handler(httpd, this->uri, this->method);
}

Httpd::Httpd(char const * name_, Config const & config)
:
    name	(name_),
    handle	([this, &config](){
		    httpd_handle_t result;
		    throwIf(httpd_start(&result, &config));
		    return result;
		}())
{}

Httpd::operator httpd_handle_t() const {return handle;}

/* virtual */ Httpd::~Httpd() {
    httpd_stop(handle);
}

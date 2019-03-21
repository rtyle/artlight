#pragma once

#include <functional>

#include <esp_http_server.h>
#include <http_parser.h>

/// An Httpd instance has an httpd_handle_t to an HTTP server implementation.
class Httpd {
protected:
    char const *		name;
    httpd_handle_t const	handle;

public:

    /// An Httpd::Config is an httpd_config_t
    /// with convenience setter methods that can be chained together.
    /// The result can be used to construct an Httpd object.
    struct Config : public httpd_config_t {
    public:
	Config(httpd_config_t const & that);
	Config();
	#define setter(name) Config & name##_(decltype(name));
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
    };

    /// An Httpd::Uri binds a handler to a uri/method for an Httpd instance.
    struct Uri : public httpd_uri_t {
    public:
	using Handler = std::function<esp_err_t(httpd_req_t *)>;
    private:
	Httpd &			httpd;
	Handler const		handler;
    public:
	Uri(
	    Httpd &		httpd,
	    char const *	uri,
	    httpd_method_t	method,
	    Handler const &&	handler);

	static esp_err_t handlerFor(httpd_req_t *);
	~Uri();
    };

    Httpd(char const * name, Config const & config);

    operator httpd_handle_t () const;

    virtual ~Httpd();
};

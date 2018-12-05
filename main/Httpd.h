#ifndef Httpd_h_
#define Httpd_h_

#include <functional>

#include <esp_http_server.h>
#include <http_parser.h>

class Httpd {
protected:
    char const *		name;
    httpd_handle_t const	handle;
public:
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

    struct Uri : public httpd_uri_t {
    public:
	typedef std::function<esp_err_t(httpd_req_t *)> Handler;
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

#endif

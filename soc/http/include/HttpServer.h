
#ifndef SOC_HTTP_HTTPSERVER_H
#define SOC_HTTP_HTTPSERVER_H

#include "../../net/include/TcpServer.h"
#include "HttpResponse.h"
using namespace soc::net;

namespace soc {
namespace http {

class HttpServer {
public:
  using Group = std::vector<std::string>;
  using Func1 = void(const HttpRequest &, HttpResponse &);
  using Func2 = void(const HttpRequest &, HttpResponse &, const Group &);

  using Handler = std::function<Func1>;
  using RMHandler = std::function<Func2>;

  HttpServer();
  ~HttpServer();

  void start(const InetAddress &address);
  void quit();

  void route(const std::string &url, const Handler &cb);
  void route_regex(const std::string &url_rule, const RMHandler &cb);
  void error_code(int code, const Handler &cb);
  void redirect(const std::string &src, const std::string &dest);
  bool set_mounting_html_dir(const std::string &url, const std::string &dir);
  bool set_mounting_file_dir(const std::string &url);
  void set_idle_timeout(int millsecond);
  void set_ext_mimetype_mapping(const std::string &ext,
                                const std::string &mimetype);

  void set_url_auth(const std::string &origin_url,
                    HttpAuthType = HttpAuthType::Basic);

  void enable_cgi(bool on);

#ifdef SUPPORT_SSL_LIB
  void set_https_certificate(const char *cert_file,
                             const char *private_key_file,
                             const char *password = nullptr);
#endif
private:
  void initialize_env();
  void initialize();
  bool handle_msg(TcpConnectionPtr);

  bool handle_url_route(HttpRequest *, HttpResponse &);
  bool handle_redirect(HttpRequest *, HttpResponse &);
  bool handle_bad_request(HttpRequest *, HttpResponse &);
  bool handle_cgi_bin(HttpRequest *, HttpResponse &);
  bool handle_file_dir_index_of(bool &, HttpRequest *, HttpResponse &);
  bool handle_url_auth(HttpRequest *, HttpResponse &);
  bool handle_regex_match_url(HttpRequest *, HttpResponse &);
  bool handle_static_resource(bool, HttpRequest *, HttpResponse &);

  void mapping_mimetype(const std::string_view &, std::string &);

private:
  enum {
    start_url_route,
    start_url_redirect,
    start_url_route_regex,
    start_url_cgi,
    start_url_file_directory,
    start_url_locate_resource,
    start_url_error
  };

  std::unique_ptr<TcpServer> server_;

  bool enable_cgi_;
  std::string cgi_bin_;

  std::unordered_map<std::string, Handler> url_handlers_;
  std::unordered_map<int, Handler> code_handlers_;
  std::unordered_map<std::string, RMHandler> url_rgx_handlers_;
  std::unordered_map<uint64_t, std::string> mimetype_tb_;
  std::unordered_map<std::string, std::string> redirect_tb_;
  std::unordered_map<std::string, HttpAuthType> auth_tb_;

  std::vector<std::pair<std::string, std::string>> mount_dir_tb_;
  std::vector<std::string> url_rules_;
  std::vector<std::string> fdir_tb_;
};
} // namespace http
} // namespace soc
#endif

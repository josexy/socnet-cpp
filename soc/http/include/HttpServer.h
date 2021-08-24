#ifndef SOC_HTTP_HTTPSERVER_H
#define SOC_HTTP_HTTPSERVER_H

#include "../../net/include/TcpServer.h"
#include "HttpService.h"

using namespace soc;
using namespace soc::net;

namespace soc {
namespace http {

class HttpServer : private HttpSessionServer {
public:
  class DefaultErrorService : public HttpErrorService {
  public:
    bool doError(int code, const HttpRequest &req, HttpResponse &resp) {
      return false;
    }
  };

  HttpServer();
  ~HttpServer();

  void start();
  void quit();

  void addMountDir(const std::string &url, const std::string &dir);

  template <class ErrorService> void setErrorService() {
    if constexpr (std::is_base_of_v<HttpErrorService, ErrorService>) {
      services_.add("#", std::make_shared<ErrorService>());
    }
  }

  template <class Service> void addService(const std::string &url) {
    if constexpr (std::is_base_of_v<HttpService, Service>) {
      services_.add(url, std::make_shared<Service>());
    }
  }

  template <class Service>
  void addUrlPatternService(const std::string &url_pattern) {
    if constexpr (std::is_base_of_v<HttpService, Service>) {
      urlp_services_.add(url_pattern, std::make_shared<Service>());
    }
  }

  void removeErrorService() { setErrorService<DefaultErrorService>(); }
  void removeService(const std::string &url) { services_.remove(url); }
  void removeUrlPatternService(const std::string &url_pattern) {
    urlp_services_.remove(url_pattern);
  }

private:
  void initialize();
  bool onMessage(TcpConnection *);
  BaseService *getErrorService() const;

  bool dispatchMountDir(const HttpRequest &, HttpResponse &);
  bool dispatchUrlPattern(const HttpRequest &, HttpResponse &);
  void dispatchPhpProcessor(const std::string &, const HttpRequest &,
                            HttpResponse &);
  bool dispatchFile(std::string_view, std::string_view, std::string,
                    const HttpRequest &, HttpResponse &);

  std::string mappingMimeType(const std::string_view &);
  bool getIndexPageFileName(std::string &);

  void setIdleTime(int millsecond);
  void setCertificate(const std::string &cert_file,
                      const std::string &private_key_file,
                      const std::string &password = "");

  HttpSession *createSession();
  void associateRequestSession(const HttpRequest &, HttpResponse &);

  HttpSession *associateSession(HttpRequest *) override;
  void handleSessionTimeout(HttpSession *);

private:
  std::unique_ptr<TcpServer> server_;
  std::vector<std::string> default_pages_;

  HttpMap<std::string, std::string> mount_dir_;
  HttpMap<std::string, std::shared_ptr<HttpSession>> sessions_;
  HttpMap<std::string, std::shared_ptr<BaseService>> services_;
  HttpMap<std::string, std::shared_ptr<HttpService>> urlp_services_;
};

} // namespace http
} // namespace soc
#endif

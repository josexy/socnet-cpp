#ifndef SOC_HTTP_HTTPSESSIONSERVER_H
#define SOC_HTTP_HTTPSESSIONSERVER_H

namespace soc {
namespace http {
class HttpSession;
class HttpRequest;
class HttpSessionServer {
public:
  virtual ~HttpSessionServer() {}

  virtual HttpSession *associateSession(HttpRequest *) = 0;
};

} // namespace http
} // namespace soc

#endif
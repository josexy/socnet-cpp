#ifndef SOC_HTTP_HTTPSERVICE_H
#define SOC_HTTP_HTTPSERVICE_H

#include "HttpResponse.h"

namespace soc {
namespace http {

class BaseService {
public:
  virtual void service(const HttpRequest &req, HttpResponse &resp) = 0;
};

class HttpService : public BaseService {
public:
  friend class HttpServer;

  using MatchGroup = std::vector<std::string>;

  virtual ~HttpService() {}

  virtual void doHead(const HttpRequest &req, HttpResponse &resp) {
    doGet(req, resp);
  }
  virtual void doGet(const HttpRequest &req, HttpResponse &resp) {
    resp.setCode(HttpStatus::METHOD_NOT_ALLOWED);
  }
  virtual void doPost(const HttpRequest &req, HttpResponse &resp) {
    resp.setCode(HttpStatus::METHOD_NOT_ALLOWED);
  }

private:
  void service(const HttpRequest &req, HttpResponse &resp) override;
  void service0(const HttpRequest &req, HttpResponse &resp, MatchGroup &match);
};

class HttpErrorService : public BaseService {
public:
  friend class HttpServer;

  virtual bool doError(int code, const HttpRequest &req,
                       HttpResponse &resp) = 0;

private:
  virtual void service(const HttpRequest &req, HttpResponse &resp) override;
  std::string baseErrorPage(int code);
};

} // namespace http
} // namespace soc

#endif

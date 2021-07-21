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
    resp.code(405);
  }
  virtual void doGet(const HttpRequest &req, HttpResponse &resp) {
    resp.code(405);
  }
  virtual void doPost(const HttpRequest &req, HttpResponse &resp) {
    resp.code(405);
  }

private:
  void service(const HttpRequest &req, HttpResponse &resp) override;
  void service(const HttpRequest &req, HttpResponse &resp,
               const MatchGroup &match);
};

class HttpErrorService : public BaseService {
public:
  friend class HttpServer;

  virtual void doError(int code, const HttpRequest &req,
                       HttpResponse &resp) = 0;

private:
  virtual void service(const HttpRequest &req, HttpResponse &resp) override;
};

} // namespace http
} // namespace soc

#endif

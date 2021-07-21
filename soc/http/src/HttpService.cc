#include "../include/HttpService.h"
using namespace soc::http;

void HttpService::service(const HttpRequest &req, HttpResponse &resp) {
  switch (req.method()) {
  case HttpMethod::HEAD:
    doHead(req, resp);
    break;
  case HttpMethod::GET:
    doGet(req, resp);
    break;
  case HttpMethod::POST:
    doPost(req, resp);
    break;
  default:
    break;
  }
}

void HttpService::service(const HttpRequest &req, HttpResponse &resp,
                          const MatchGroup &match) {
  req.match_ = match;
  service(req, resp);
}

void HttpErrorService::service(const HttpRequest &req, HttpResponse &resp) {
  doError(resp.code(), req, resp);
}
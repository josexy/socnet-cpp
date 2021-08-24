#include "../include/HttpService.h"
using namespace soc::http;

void HttpService::service(const HttpRequest &req, HttpResponse &resp) {
  switch (req.getMethod()) {
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

void HttpService::service0(const HttpRequest &req, HttpResponse &resp,
                           MatchGroup &match) {
  req.match_.swap(match);
  service(req, resp);
}

void HttpErrorService::service(const HttpRequest &req, HttpResponse &resp) {
  int code = resp.getCode();
  if (!doError(code, req, resp)) // Unhandled error code
    resp.setHeader("Content-Type", "text/html").setBody(baseErrorPage(code));
}

std::string HttpErrorService::baseErrorPage(int code) {
  std::string text = std::to_string(code) + " " + Status_Code.at(code);
  return R"(<html><head><title>)" + text +
         R"(</title></head><body><center><h1>)" + text +
         R"(</h1></center><hr><center>socnet</center></body></html>)";
}
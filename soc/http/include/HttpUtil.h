#ifndef SOC_HTTP_HTTPUTIL_H
#define SOC_HTTP_HTTPUTIL_H

#include "../../utility/include/AppConfig.h"

namespace soc {
namespace http {

inline constexpr uint64_t hash_ext(const char *s, size_t i, size_t l,
                                   uint64_t h) noexcept {
  return i == l ? h : hash_ext(s, i + 1, l, (h * 31) + *(s + i));
}
inline uint64_t hash_ext(const std::string_view &s) noexcept {
  return hash_ext(s.data(), 0, s.size(), 0);
}
inline constexpr uint64_t operator""_h(const char *s, size_t n) noexcept {
  return hash_ext(s, 0, n, 0);
}

static const std::unordered_map<short, const char *> Status_Code = {
    {100, "Continue"},
    {200, "OK"},
    {301, "Moved Permanently"},
    {302, "Found"},
    {304, "Not Modified"},
    {307, "Temporary Redirect"},
    {400, "Bad Request"},
    {401, "Unauthorized"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {405, "Method Not Allowed"},
    {500, "Internal Server Error"},
    {503, "Service Unavailable"},
    {505, "HTTP Version Not Suppported"}};

static const std::unordered_map<uint64_t, const char *> Content_Type{
    {"html"_h, "text/html"},
    {"htm"_h, "text/html"},
    {"css"_h, "text/css"},
    {"csv"_h, "text/csv"},
    {"txt"_h, "text/plain"},
    {"ttf"_h, "font/ttf"},
    {"otf"_h, "font/otf"},
    {"svg"_h, "image/svg+xml"},
    {"gif"_h, "image/gif"},
    {"bmp"_h, "image/bmp"},
    {"jpg"_h, "image/jpeg"},
    {"jpeg"_h, "image/jpeg"},
    {"webp"_h, "image/webp"},
    {"png"_h, "image/png"},
    {"ico"_h, "image/x-icon"},
    {"wav"_h, "audio/x-wav"},
    {"mp3"_h, "audio/mp3"},
    {"mpeg"_h, "video/mpeg"},
    {"mp4"_h, "video/mp4"},
    {"avi"_h, "video/avi"},
    {"7z"_h, "application/x-7z-compressed"},
    {"rss"_h, "application/rss+xml"},
    {"tar"_h, "application/x-tar"},
    {"gz"_h, "application/gzip"},
    {"zip"_h, "application/zip"},
    {"xhtml"_h, "application/xhtml+xml"},
    {"xml"_h, "application/xml"},
    {"js"_h, "application/x-javascript"},
    {"pdf"_h, "application/pdf"},
    {"json"_h, "application/json"}};

enum class HttpMethod { GET = 0x01, POST = 0x02, HEAD = 0x03 };

enum class HttpVersion {
  HTTP_1_0 = 0x10,
  HTTP_1_1 = 0x11,
  HTTP_2_0 = 0x20 /*not support HTTP 2.0*/
};

} // namespace http
} // namespace soc
#endif

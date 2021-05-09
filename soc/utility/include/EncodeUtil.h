#ifndef SOC_UTILITY_ENCODEUTIL_H
#define SOC_UTILITY_ENCODEUTIL_H

#include <functional>
#include <string>

namespace soc {
namespace net {
class Buffer;
}

struct EncodeUtil {
  // Gzip compress algorithm for http response body
  static void gzipcompress(void *in_data, size_t in_data_size, net::Buffer *buf,
                           const std::function<void(size_t)> &f,
                           const std::function<void()> &g);

  // MD5
  static unsigned char *md5_hash(void *, size_t, unsigned char[16]);
  static std::string md5_hash(const std::string_view &);
  static bool md5_hash_equal(const std::string_view &,
                             const std::string_view &);
  static std::string md5_hash_file(const char *filename);

  // Base64 encode/decode
  static std::string base64_encode(const std::string_view &);
  static std::string base64_decode(const std::string_view &);

  // Url encode/decode
  static unsigned char hex2dec(char c);
  static char dec2hex(char c);

  static std::string url_decode(const char *value, size_t size);
  static std::string url_decode(const std::string_view &value);
  static std::string url_decode(const char *value);

  static std::string url_encode(const char *value, size_t size);
  static std::string url_encode(const std::string_view &value);
  static std::string url_encode(const char *value);

  static void tolowers(char *s, size_t size);
  static void touppers(char *s, size_t size);

  static std::string generate_randrom_string(size_t);
};
} // namespace soc

#endif

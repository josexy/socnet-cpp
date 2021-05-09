#include "../include/EncodeUtil.h"

#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <string.h>
#include <zlib.h>

#include "../../net/include/Buffer.h"
using namespace soc;

void EncodeUtil::gzipcompress(void *in_data, size_t in_data_size,
                              net::Buffer *buf,
                              const std::function<void(size_t)> &f,
                              const std::function<void()> &g) {
  std::vector<uint8_t> buffer;

  const size_t BUFSIZE = 128 * 1024;
  uint8_t temp_buffer[BUFSIZE];

  z_stream strm;
  strm.zalloc = 0;
  strm.zfree = 0;
  strm.next_in = reinterpret_cast<uint8_t *>(in_data);
  strm.avail_in = in_data_size;
  strm.next_out = temp_buffer;
  strm.avail_out = BUFSIZE;

  int windowsBits = 15;
  int GZIP_ENCODING = 16;
  deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
               windowsBits | GZIP_ENCODING, 8, Z_DEFAULT_STRATEGY);

  while (strm.avail_in != 0) {
    ::deflate(&strm, Z_NO_FLUSH);
    if (strm.avail_out == 0) {
      buffer.insert(buffer.end(), temp_buffer, temp_buffer + BUFSIZE);
      strm.next_out = temp_buffer;
      strm.avail_out = BUFSIZE;
    }
  }

  int deflate_res = Z_OK;
  while (deflate_res == Z_OK) {
    if (strm.avail_out == 0) {
      buffer.insert(buffer.end(), temp_buffer, temp_buffer + BUFSIZE);
      strm.next_out = temp_buffer;
      strm.avail_out = BUFSIZE;
    }
    deflate_res = ::deflate(&strm, Z_FINISH);
  }

  buffer.insert(buffer.end(), temp_buffer,
                temp_buffer + BUFSIZE - strm.avail_out);
  ::deflateEnd(&strm);

  f(buffer.size());
  g();
  buf->append<uint8_t>(buffer.begin(), buffer.end());
}

unsigned char *EncodeUtil::md5_hash(void *data, size_t n,
                                    unsigned char md[16]) {
  return ::MD5(reinterpret_cast<const unsigned char *>(data), n, md);
}

std::string EncodeUtil::md5_hash(const std::string_view &data) {
  unsigned char md[16];
  ::MD5(reinterpret_cast<const unsigned char *>(data.data()), data.size(), md);
  std::string v(32, 0);
  for (int i = 0, j = 0; i < 16 && j < 32; ++i, j += 2)
    ::sprintf(v.data() + j, "%02x", md[i]);
  return v;
}

bool EncodeUtil::md5_hash_equal(const std::string_view &data,
                                const std::string_view &hashv) {
  std::string hashv2 = md5_hash(data);
  return 0 == ::memcmp(hashv2.data(), hashv.data(), hashv2.size());
}

std::string EncodeUtil::md5_hash_file(const char *filename) {
  FILE *fp = NULL;
  fp = ::fopen(filename, "rb");
  if (fp == NULL)
    return "";

  MD5_CTX ctx;
  unsigned char md[16];
  char buffer[1024];
  int n;
  ::MD5_Init(&ctx);
  while ((n = ::fread(buffer, sizeof(char), 1024, fp)) > 0) {
    ::MD5_Update(&ctx, buffer, n);
    ::memset(buffer, 0, sizeof(buffer));
  }
  ::MD5_Final(md, &ctx);
  ::fclose(fp);

  std::string v(32, 0);
  for (int i = 0, j = 0; i < 16 && j < 32; ++i, j += 2)
    sprintf(v.data() + j, "%02x", md[i]);
  return v;
}

std::string EncodeUtil::base64_encode(const std::string_view &data) {
  BIO *bmem = NULL;
  BIO *b64 = NULL;
  BUF_MEM *bptr = NULL;
  b64 = ::BIO_new(::BIO_f_base64());
  ::BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
  bmem = ::BIO_new(::BIO_s_mem());
  b64 = ::BIO_push(b64, bmem);
  ::BIO_write(b64, data.data(), data.size());
  BIO_flush(b64);
  BIO_get_mem_ptr(b64, &bptr);

  std::string value(bptr->data, bptr->length);
  ::BIO_free_all(b64);
  return value;
}

std::string EncodeUtil::base64_decode(const std::string_view &data) {
  BIO *b64 = NULL;
  BIO *bmem = NULL;
  b64 = ::BIO_new(BIO_f_base64());
  ::BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
  bmem = ::BIO_new_mem_buf(data.data(), data.size());
  bmem = ::BIO_push(b64, bmem);

  std::string value(data.size(), 0);
  int n = ::BIO_read(bmem, value.data(), data.size());
  ::BIO_free_all(bmem);
  value.resize(n);
  return value;
}

unsigned char EncodeUtil::hex2dec(char c) {
  if (c >= '0' && c <= '9')
    return c - '0'; // [0-9]
  else if (c >= 'a' && c <= 'z')
    return c - 'a' + 10; // a-f [10-15]
  else if (c >= 'A' && c <= 'Z')
    return c - 'A' + 10; // A-F [10-15]
  return c;
}

char EncodeUtil::dec2hex(char c) {
  if (c >= 0 && c <= 9)
    return c + '0'; // ['0'-'9']
  else if (c >= 10 && c <= 15)
    return c - 10 + 'A'; // ['A'-'Z']
  return c;
}

std::string EncodeUtil::url_decode(const char *value, size_t size) {
  std::string escaped;
  for (size_t i = 0; i < size; ++i) {
    if (value[i] == '%' && i + 2 < size && isxdigit(value[i + 1]) &&
        isxdigit(value[i + 2])) {
      // merge two char to one byte
      // %AB => 0xAB
      unsigned char byte =
          ((unsigned char)hex2dec(value[i + 1]) << 4) | hex2dec(value[i + 2]);
      i += 2;
      escaped += byte;
    } else {
      escaped += value[i];
    }
  }
  return escaped;
}

std::string EncodeUtil::url_decode(const std::string_view &value) {
  return url_decode(value.data(), value.size());
}

std::string EncodeUtil::url_decode(const char *value) {
  return url_decode(value, strlen(value));
}

std::string EncodeUtil::url_encode(const char *value, size_t size) {
  std::string escaped;

  for (size_t i = 0; i < size; i++) {
    unsigned char c = (unsigned char)value[i];
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      escaped += c;
      continue;
    }
    // split one byte to two char
    // 0xAB => %AB
    char buf[5]{0};
    sprintf(buf, "%%%c%c", toupper(dec2hex(c >> 4)), toupper(dec2hex(c & 15)));
    escaped += buf;
  }

  return escaped;
}

std::string EncodeUtil::url_encode(const std::string_view &value) {
  return url_encode(value.data(), value.size());
}

std::string EncodeUtil::url_encode(const char *value) {
  return url_encode(value, strlen(value));
}

void EncodeUtil::tolowers(char *s, size_t size) {
  for (size_t i = 0; i < size; i++)
    if (isupper(s[i]))
      s[i] = tolower(s[i]);
}

void EncodeUtil::touppers(char *s, size_t size) {
  for (size_t i = 0; i < size; i++)
    if (islower(s[i]))
      s[i] = toupper(s[i]);
}

std::string EncodeUtil::generate_randrom_string(size_t n) {
  std::string str;
  str.resize(n);
  constexpr static const char *alpha =
      "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
  for (size_t i = 0; i < n; ++i)
    str[i] = alpha[::rand() % 62];
  return str;
}
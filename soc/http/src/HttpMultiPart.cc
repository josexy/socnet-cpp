#include "../include/HttpMultiPart.h"

#include "../include/HttpUtil.h"
#include <string.h>

using namespace soc::http;

auto HttpMultiPart::get(const std::string &name) const
    -> std::optional<std::string> {
  if (auto it = form_.find(name); it != form_.end())
    return std::make_optional(it->second);
  return std::nullopt;
}

auto HttpMultiPart::file(const std::string &name) const
    -> std::optional<const std::reference_wrapper<const std::vector<Part>>> {
  if (auto it = files_.find(name); it != files_.end()) {
    return std::optional<const std::reference_wrapper<const std::vector<Part>>>(
        it->second);
  }
  return std::nullopt;
}

void HttpMultiPart::parse(const std::string_view &body) {
  size_t i = 0, bl = bd_.size(), len = body.size();
  std::string name, filename, type, form_file_data;
  bool file_mark = false;
  const char *boundary_s = bd_.data();
  State state = start_body;

  while (i < len) {
    switch (state) {
    case start_body: {
      // boundary begin --boundary
      if (i + 1 < len && body[i] == '-' && body[i + 1] == '-') {
        i += 2;
        state = start_boundary;
      }
      break;
    }
    case start_boundary: {
      if (strncmp(body.data() + i, boundary_s, bl) == 0) {
        i += bl;
        // --boundary\r\n
        if (i + 1 < len && body[i] == CR && body[i + 1] == LF) {
          i += 2;
          state = end_boundary;
        } else if (i + 1 < len && body[i] == '-' && body[i + 1] == '-') {
          // --boundary--
          i += 2;
          state = end_body;
        } else {
          // bad body, ignored
        }
      }
      break;
    }
    case end_boundary: {
      if (hash_ext(body.data(), i, 19) == "Content-Disposition"_h) {
        state = start_content_disposition;
        i += 19; // skip "Content-Disposition"
      } else {
        // bad body
      }
      break;
    }
    case start_content_disposition: {
      // name="mike"\r\n
      // name="file1"; filename="test.txt"\r\n
      i += 13; // skip ": form-data; "
      bool start_name_filename = false;
      bool is_name = true;
      while (i < len) {
        if (i + 1 < len && body[i] == CR && body[i + 1] == LF) {
          i += 2;
          state = end_content_disposition;
          break;
        }
        if (i + 1 < len && body[i] == '=' && body[i + 1] == '\"') {
          i += 2;
          start_name_filename = true;
        } else if (body[i] == '\"') {
          start_name_filename = false;
          is_name = !is_name; // name/filename
          i++;
        } else if (start_name_filename) {
          if (is_name)
            name += body[i++];
          else
            filename += body[i++];
        } else
          i++;
      }
      break;
    }
    case end_content_disposition: {
      // html form content data
      if (i + 1 < len && body[i] == CR && body[i + 1] == LF) {
        i += 2;
        file_mark = false;
        state = start_content_data;
      } else {
        // file type
        if (hash_ext(body.data(), i, 12) == "Content-Type"_h) {
          i += 14; // skip "Content-Type"
          file_mark = true;
          state = start_content_type;
        } else {
          // bad body
        }
      }
      break;
    }
    case start_content_type: {
      while (i < len) {
        if (i + 1 < len && body[i] == CR && body[i + 1] == LF) {
          i += 2;
          state = end_content_type;
          break;
        } else
          type += body[i++];
      }
      break;
    }
    case end_content_type: {
      if (i + 1 < len && body[i] == CR && body[i + 1] == LF) {
        i += 2;
        state = start_content_data;
      } else {
        // bad body
      }
      break;
    }
    case start_content_data: {
      while (i < len) {
        if (i + 1 < len && body[i] == '-' && body[i + 1] == '-' &&
            strncmp(body.data() + i + 2, boundary_s, bl) == 0) {
          state = end_content_data;
          break;
        } else {
          if (i + 1 < len && body[i] == CR && body[i + 1] == LF) {
            // skip \r, save \n as original new line
            i++;
          }
          form_file_data += body[i++];
        }
      }
      break;
    }
    case end_content_data: {
      // pop \n
      if (form_file_data.size() && form_file_data.back() == LF)
        form_file_data.pop_back();
      if (!file_mark) // form
        form_.emplace(name, form_file_data);
      else { // file
        if (auto x = files_.find(name); x != files_.end()) {
          x->second.emplace_back(std::move(name), std::move(filename),
                                 std::move(type), std::move(form_file_data));
        } else {
          files_.emplace(name, std::move(std::vector{Part(name, filename, type,
                                                          form_file_data)}));
        }
      }
      name = filename = type = form_file_data = "";
      file_mark = false;
      state = start_body;
      break;
    }
    case end_body: {
      i += 2; // skip \r\n
      break;
    }
    default: {
      i++; // ignored
      break;
    }
    }
  }
}

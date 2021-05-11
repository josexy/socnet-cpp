#include "../soc/http/include/HttpServer.h"
#include "../soc/utility/include/Logger.h"
#include <signal.h>
#include <sys/stat.h>

using namespace soc;
using namespace soc::http;

HttpServer app;

void signal_handler(int) { app.quit(); }

int main() {
  signal(SIGINT, signal_handler);

  size_t max_limit_file_size = 10 * 1024 * 1024;
  std::string save_fir_dir = "upload_files/";
  mkdir(save_fir_dir.c_str(), 0755);

  auto do_work = [&](const std::vector<HttpMultiPart::Part> &parts) {
    for (auto &x : parts) {
      LOG_INFO("filename: %s, filetype: %s", x.file_name.c_str(),
               x.file_type.c_str());
      if (x.data_str.size() > max_limit_file_size) {
        LOG_ERROR("%s", "file size limit 10M!");
        return false;
      } else {
        if (!x.file_name.empty()) {
          std::ofstream ofs(save_fir_dir + x.file_name,
                            std::ios_base::binary | std::ios_base::out);
          ofs.write(x.data_str.data(), x.data_str.size());
          ofs.close();
          LOG_INFO("%s", "save file successfully!");
        }
      }
    }
    return true;
  };

  app.route("/post", [&](const auto &req, auto &resp) {
    bool ok = true;
    do {
      if (auto file1 = req.multipart().file("file1"); file1.has_value()) {
        ok = do_work(file1.value());
        if (!ok)
          break;
      }
      if (auto file2 = req.multipart().file("file2"); file2.has_value()) {
        ok = do_work(file2.value());
      }
    } while (0);
    if (ok)
      resp.body_html("html/post.html").send();
    else
      resp.body("upload file size limit 10M!").send();
  });

  app.start(InetAddress(5555));
  return 0;
}

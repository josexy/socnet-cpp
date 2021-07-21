#ifndef SOC_UTILITY_LOGGER_H
#define SOC_UTILITY_LOGGER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
namespace soc {

static constexpr const size_t kMaxLineBuffer = 512;

class Logger {
public:
  enum { LV_INFO, LV_WARN, LV_DEBUG, LV_ERROR };
  enum {
    F_None = 0,
    F_Black = 30,
    F_Red = 31,
    F_Green = 32,
    F_Orange = 33,
    F_Blue = 34,
    F_Solferino = 35,
    F_Cyan = 36,
    F_LightGray = 37
  };

  enum { C_None = 0, C_Highlight = 1, C_Italic = 3, C_UnderLine = 4 };

  static Logger &getLogger() {
    static Logger logger;
    return logger;
  }

  template <int fc = F_None, int cc = C_None, int lv = LV_INFO, class... Args>
  void format(const char *fmt, int LINE, const char *FILE_NAME,
              Args &&...args) {
    int len = ::snprintf(nullptr, 0, fmt, args...);
    char buffer[kMaxLineBuffer]{0};
    ::snprintf(buffer, len + 1, fmt, args...);

    char times[25]{0};
    time_t t = ::time(nullptr);
    ::strftime(times, 25, "%Y-%m-%d %H:%M:%S", ::localtime(&t));

    char lvs[9]{0};
    if (lv == LV_INFO)
      ::strcpy(lvs, "INFO");
    else if (lv == LV_WARN)
      ::strcpy(lvs, "WARN");
    else if (lv == LV_DEBUG)
      ::strcpy(lvs, "DEBUG");
    else if (lv == LV_ERROR)
      ::strcpy(lvs, "ERROR");
    else
      ::strcpy(lvs, "UNKNOWN");

    if (fc != F_None && cc != C_None)
      ::fprintf(stdout, "\033[%d;%dm[%s]:[%s]:[%d]:[%s] %s\033[0m\n", fc, cc,
                lvs, times, LINE, FILE_NAME, buffer);
    else if (fc == F_None && cc == C_None)
      ::fprintf(stdout, "[%s]:[%s]:[%d]:[%s] %s\n", lvs, times, LINE, FILE_NAME,
                buffer);
    else {
      ::fprintf(stdout, "\033[%dm[%s]:[%s]:[%d]:[%s] %s\033[0m\n", fc ? fc : cc,
                lvs, times, LINE, FILE_NAME, buffer);
    }
    ::fflush(stdout);
  }

private:
  Logger() = default;
  Logger(const Logger &) = delete;
  Logger &operator=(const Logger &) = delete;
  Logger(Logger &&) = delete;
  Logger &operator=(Logger &&) = delete;
};

#define LOG_TEMPLATE(fmt, F, C, L, LN, FN, ...)                                \
  Logger::getLogger().format<Logger::F, Logger::C, Logger::L>((fmt), LN, FN,   \
                                                              __VA_ARGS__)
#define LOG_INFO(fmt, ...)                                                     \
  LOG_TEMPLATE((fmt), F_Green, C_Highlight, LV_INFO, __LINE__, __FILE__,       \
               __VA_ARGS__)

#define LOG_WARN(fmt, ...)                                                     \
  LOG_TEMPLATE((fmt), F_Orange, C_Highlight, LV_WARN, __LINE__, __FILE__,      \
               __VA_ARGS__)

#define LOG_DEBUG(fmt, ...)                                                    \
  LOG_TEMPLATE((fmt), F_None, C_Highlight, LV_DEBUG, __LINE__, __FILE__,       \
               __VA_ARGS__)

#define LOG_ERROR(fmt, ...)                                                    \
  LOG_TEMPLATE((fmt), F_Red, C_Highlight, LV_ERROR, __LINE__, __FILE__,        \
               __VA_ARGS__)

} // namespace soc

#endif

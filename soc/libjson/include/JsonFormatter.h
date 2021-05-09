#ifndef LIBJSON_JSONFORMATTRT_H
#define LIBJSON_JSONFORMATTRT_H

#include <memory>

#include "JsonArray.h"
#include "JsonObject.h"

namespace libjson {
class JsonFormatter {
public:
  explicit JsonFormatter(int indent = 4, bool escape = false)
      : __indent(indent), __escape(escape) {}

  void set_source(JsonObject *object) {
    __object = object;
    __array = nullptr;
  }
  void set_source(JsonArray *array) {
    __array = array;
    __object = nullptr;
  }

  std::string format() {
    __fmt.str("");
    __fmt.clear();

    if (__object) {
      if (__indent <= 0)
        return __object->toString();
      recursive_formatter(1, __object, false);
    } else if (__array) {
      if (__indent <= 0)
        return __array->toString();
      recursive_formatter(1, __array, false);
    } else
      throw JsonError("the source of JsonObject or JsonArray is null");
    return __fmt.str();
  }

protected:
  void append_space_character(int c) {
    for (int i = 0; i < c; i++)
      __fmt << " ";
  }

  void recursive_formatter(int, JsonObject *, bool);
  void recursive_formatter(int, JsonArray *, bool);

private:
  int __indent;
  bool __escape;
  JsonObject *__object;
  JsonArray *__array;
  std::stringstream __fmt;
  static constexpr const char *NEWLINE = "\n";
};

} // namespace libjson
#endif

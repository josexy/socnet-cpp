#ifndef LIBJSON_JSONARRAY_H
#define LIBJSON_JSONARRAY_H

#include <memory>
#include <vector>

#include "JsonBoolean.h"
#include "JsonNull.h"
#include "JsonNumber.h"
#include "JsonObject.h"
#include "JsonString.h"

namespace std {
template <class T> struct remove_pointer<shared_ptr<T>> { using type = T; };
}; // namespace std

namespace libjson {

class JsonArray : public JsonValue {
public:
  friend class JsonFormatter;

  JsonArray() : JsonValue(JsonType::Array) {}
  ~JsonArray() {}

  bool empty() const { return __lstValue.empty(); }

  // Return unformatted json text
  std::string toString() {
    std::stringstream ss;
    size_t index = 0, size = __lstValue.size();
    ss << "[";
    for (auto token : __lstValue) {
      ss << token->toString();
      if (index + 1 < size)
        ss << ",";
      index++;
    }
    ss << "]";
    return ss.str();
  }
  JsonArray *toJsonArray() { return this; }

  std::shared_ptr<JsonValue> get(int index) {
    if (index < 0 || index >= (int)__lstValue.size())
      return nullptr;
    return __lstValue[index];
  }

  auto add(const std::shared_ptr<JsonValue> &object) {
    __lstValue.emplace_back(object);
    return object;
  }
  auto add(JsonValue *value) { return add(std::shared_ptr<JsonValue>(value)); }
  auto add() { return add(std::make_shared<JsonNull>()); }
  auto add(int value) { return add(std::make_shared<JsonNumber>(value, true)); }
  auto add(double value) {
    return add(std::make_shared<JsonNumber>(value, false));
  }
  template <class T, class E = std::enable_if_t<std::is_same<T, bool>::value>>
  auto add(T value) {
    return add(std::make_shared<JsonBoolean>(value));
  }
  auto add(const std::string &value) {
    return add(std::make_shared<JsonString>(value));
  }

  // The types that can be added are string, int, double, bool, JsonValue
  // pointer and its derived class pointer or smart pointer object.
  // For JsonValue pointer, get its original value type by remove_pointer_t
  template <class T> void add_values(T val) {
    if (std::is_same_v<int, std::remove_reference_t<T>>) {
      add(val);
    } else if (std::is_same_v<double, std::remove_reference_t<T>>) {
      add(val);
    } else if (std::is_same_v<const char *, std::remove_reference_t<T>> ||
               std::is_same_v<std::string, std::remove_reference_t<T>>) {
      add(val);
    } else if (std::is_same_v<bool, std::remove_reference_t<T>>) {
      add(val);
    } else if (std::is_base_of_v<JsonValue, std::remove_pointer_t<T>>) {
      add(val);
    }
  }

  // Add multiple types of elements to JsonArray
  template <class T, class... Args>
  void add_values(T &&val, const Args... args) {
    add_values(std::forward<T>(val));
    add_values(std::forward<const Args>(args)...);
  }

  size_t size() const noexcept { return __lstValue.size(); }

private:
  std::vector<std::shared_ptr<JsonValue>> __lstValue;
};
} // namespace libjson
#endif

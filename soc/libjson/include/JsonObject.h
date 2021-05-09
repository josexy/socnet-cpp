#ifndef LIBJSON_JSONOBJECT_H
#define LIBJSON_JSONOBJECT_H

#include "JsonArray.h"
#include "JsonBoolean.h"
#include "JsonNull.h"
#include "JsonNumber.h"
#include "JsonString.h"

namespace libjson {
class JsonObject : public JsonValue {
public:
  using key_value_pair =
      std::pair<std::shared_ptr<JsonString>, std::shared_ptr<JsonValue>>;

  friend class JsonFormatter;

  JsonObject() : JsonValue(JsonType::Object) {}

  ~JsonObject() {}

  bool empty() const { return __kvObjects.empty(); }

  // Return unformatted json text
  std::string toString() {
    std::stringstream ss;
    ss << "{";
    size_t index = 0, size = __kvObjects.size();
    for (auto &[key, value] : __kvObjects) {
      ss << key->toString();
      ss << ":";
      ss << value->toString();
      if (index + 1 < size)
        ss << ",";
      index++;
    }
    ss << "}";
    return ss.str();
  }
  auto add(const std::shared_ptr<JsonString> &key,
           const std::shared_ptr<JsonValue> &value) {
    auto p = std::make_pair(key, value);
    __kvObjects.emplace_back(p);
    return p;
  }

  auto add(JsonString *key, JsonValue *value) {
    return add(std::shared_ptr<JsonString>(key),
               std::shared_ptr<JsonValue>(value));
  }
  auto add(const string &key, JsonValue *value) {
    return add(std::make_shared<JsonString>(key),
               std::shared_ptr<JsonValue>(value));
  }
  auto add(const string &key, const string &value) {
    return add(std::make_shared<JsonString>(key),
               std::make_shared<JsonString>(value));
  }
  auto add(const string &key, double value) {
    return add(std::make_shared<JsonString>(key),
               std::make_shared<JsonNumber>(value, false));
  }
  auto add(const string &key, int value) {
    return add(std::make_shared<JsonString>(key),
               std::make_shared<JsonNumber>(value, true));
  }
  auto add(const string &key) {
    return add(std::make_shared<JsonString>(key), std::make_shared<JsonNull>());
  }
  template <class T, class E = std::enable_if_t<std::is_same<T, bool>::value>>
  auto add(const string &key, T value) {
    return add(std::make_shared<JsonString>(key),
               std::make_shared<JsonBoolean>(value));
  }

  key_value_pair get(int index) {
    if (index < 0 || index >= (int)__kvObjects.size())
      return make_pair<std::shared_ptr<JsonString>, std::shared_ptr<JsonValue>>(
          nullptr, nullptr);
    return __kvObjects[index];
  }

  std::shared_ptr<JsonValue> find(const string &key) {
    for (auto &[k, v] : __kvObjects) {
      if (key.compare(k->value<string>()) == 0)
        return v;
    }
    return nullptr;
  }
  JsonObject *toJsonObject() { return this; }

private:
  std::vector<key_value_pair> __kvObjects;
};
} // namespace libjson
#endif

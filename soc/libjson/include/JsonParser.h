#ifndef LIBJSON_JSONPARSER_H
#define LIBJSON_JSONPARSER_H

#include "JsonLexer.h"
#include "JsonObject.h"

namespace libjson {
class JsonParser {
public:
  explicit JsonParser(const std::string &json, bool escape = true)
      : lexer(json, escape) {
    getTokens();
    __tokenLength = lexer.token_list.size();
    __tokenIndex = 0;
  }
  explicit JsonParser(std::ifstream &ifs, bool escape = true)
      : lexer(ifs, escape) {
    getTokens();
    __tokenLength = lexer.token_list.size();
    __tokenIndex = 0;
  }

  ~JsonParser() {}

  auto parse() {
    if (lexer.token_list.empty())
      return std::make_tuple(__object_ptr, __array_ptr, __value_ptr);

    handle_begin_end_mismatch_exception();

    auto token = lexer.token_list[__tokenIndex++];
    if (token->tag() == Tag::JSON_OBJECT_BEGIN) {
      handle_after_object_array_exception(token.get());
      __object_ptr = std::make_shared<JsonObject>();
      parseJsonObject(__object_ptr);

    } else if (token->tag() == Tag::JSON_ARRAY_BEGIN) {
      handle_after_object_array_exception(token.get());
      __array_ptr = std::make_shared<JsonArray>();
      parseJsonArray(__array_ptr);

    } else {
      if (lexer.token_list.size() == 1) {
        switch (token->tag()) {
        case Tag::JSON_NUMBER:
        case Tag::JSON_NULL:
        case Tag::JSON_FLOAT:
        case Tag::JSON_TRUE:
        case Tag::JSON_FALSE:
        case Tag::JSON_STRING: {
          __value_ptr = std::make_shared<JsonValue>(token.get());
        } break;
        default:
          throw JsonSyntaxError(token->line(), token->col(), "invalid token");
          break;
        }
      } else {
        throw JsonSyntaxError(token->line(), token->col(), "invalid token");
      }
    }
    return std::make_tuple(__object_ptr, __array_ptr, __value_ptr);
  }
  bool isObject() const { return __object_ptr != nullptr; }
  bool isArray() const { return __array_ptr != nullptr; }

protected:
  void getTokens() {
    try {
      while (!lexer.reader.eof()) {
        lexer.analysis();
      }
    } catch (JsonError &e) {
      cout << e.what() << endl;
      exit(-1);
    }
  }
  void parseJsonArray(std::shared_ptr<JsonArray> &);
  void parseJsonObject(std::shared_ptr<JsonObject> &);

  void handle_begin_end_mismatch_exception() {
    auto first = lexer.token_list[__tokenIndex];
    auto last = lexer.token_list[lexer.token_list.size() - 1];

    switch (first->tag()) {
    case Tag::JSON_OBJECT_BEGIN:
      if (last->tag() != Tag::JSON_OBJECT_END)
        throw JsonSyntaxError(last->line(), last->col(),
                              "object { missing } character");
      break;
    case Tag::JSON_ARRAY_BEGIN:
      if (last->tag() != Tag::JSON_ARRAY_END)
        throw JsonSyntaxError(last->line(), last->col(),
                              "array [ missing ] character");
      break;
    default:
      break;
    }
  }

  void handle_after_object_array_closure_exception(Token *curToken) {
    // ]:  ][  ]{   }:  }[  }{
    if (__tokenIndex < (int)lexer.token_list.size()) {
      auto nextToken = lexer.token_list[__tokenIndex];
      switch (nextToken->tag()) {
      case Tag::JSON_KEY_VALUE_SEP:
      case Tag::JSON_OBJECT_BEGIN:
      case Tag::JSON_ARRAY_BEGIN:
      case Tag::JSON_NUMBER:
      case Tag::JSON_NULL:
      case Tag::JSON_FLOAT:
      case Tag::JSON_STRING:
      case Tag::JSON_TRUE:
      case Tag::JSON_FALSE: {
        switch (curToken->tag()) {
        case Tag::JSON_ARRAY_END:
        case Tag::JSON_OBJECT_END:
          throw JsonSyntaxError(nextToken->line(), nextToken->col(),
                                "error syntax: ]/} " + nextToken->toString());
          break;
        default:
          break;
        }
      } break;
      default:
        break;
      }
    } else {
    }
  }
  void handle_after_object_array_exception(Token *curToken) {
    // [,   {,   [:  {: [}  {]
    if (__tokenIndex < (int)lexer.token_list.size()) {
      auto nextToken = lexer.token_list[__tokenIndex];
      switch (nextToken->tag()) {
      case Tag::JSON_SEP:
      case Tag::JSON_KEY_VALUE_SEP: {
      __throw__here:
        throw JsonSyntaxError(nextToken->line(), nextToken->col(),
                              "after [ or { invalid character occurried " +
                                  nextToken->toString());
      } break;
      default:
        break;
      }
      // [}   {]
      if ((curToken->tag() == Tag::JSON_OBJECT_BEGIN &&
           nextToken->tag() == Tag::JSON_ARRAY_END) ||
          (curToken->tag() == Tag::JSON_ARRAY_BEGIN &&
           nextToken->tag() == Tag::JSON_OBJECT_END)) {
        goto __throw__here;
      }
    }
  }

  void handle_missing_separator_exception(Token *curToken) {
    switch (curToken->tag()) {
    case Tag::JSON_NULL:
    case Tag::JSON_NUMBER:
    case Tag::JSON_FLOAT:
    case Tag::JSON_STRING:
    case Tag::JSON_TRUE:
    case Tag::JSON_FALSE: {
      if (__tokenIndex < (int)lexer.token_list.size()) {
        auto nextToken = lexer.token_list[__tokenIndex];
        switch (nextToken->tag()) {
        case Tag::JSON_ARRAY_END:
        case Tag::JSON_OBJECT_END:
        case Tag::JSON_SEP:
          break;
        default:
          throw JsonSyntaxError(
              curToken->line(), curToken->col() + curToken->len() - 1,
              "between " + curToken->toString() + " and " +
                  nextToken->toString() + " missing end character ,/]/}. ");
          break;
        }
      } else {
        throw JsonSyntaxError(
            curToken->line(), curToken->col() + curToken->len() - 1,
            "the json document missing final character before " +
                curToken->toString());
      }
    } break;
    default:
      break;
    }
  }

  void handle_after_separators_exception() {
    if (__tokenIndex < (int)lexer.token_list.size()) {
      auto nextToken = lexer.token_list[__tokenIndex];
      // ,, / ,} / ,]  / ,:
      switch (nextToken->tag()) {
      case Tag::JSON_SEP:
      case Tag::JSON_OBJECT_END:
      case Tag::JSON_ARRAY_END:
      case Tag::JSON_KEY_VALUE_SEP:
        throw JsonSyntaxError(nextToken->line(), nextToken->col(),
                              "after ',' some invalid characters occurried " +
                                  nextToken->toString());
        break;
      default:
        break;
      }
    }
  }

  void handle_key_not_string_exception(Token *curToken) {
    if (curToken->tag() != Tag::JSON_STRING)
      throw JsonSyntaxError(curToken->line(), curToken->col(),
                            "object key is not string type " +
                                curToken->toString());
  }

  void handle_object_missing_separator_exception(Token *curToken) {
    if (__tokenIndex < (int)lexer.token_list.size()) {
      auto nextToken = lexer.token_list[__tokenIndex];
      if (nextToken->tag() != Tag::JSON_KEY_VALUE_SEP)
        throw JsonSyntaxError(
            curToken->line(), curToken->col() + curToken->len() - 1,
            "between" + curToken->toString() + " and " + nextToken->toString() +
                " missing : separator");
    }
  }

  void handle_object_missing_value() {
    if (__tokenIndex < (int)lexer.token_list.size()) {
      auto nextToken = lexer.token_list[__tokenIndex];
      switch (nextToken->tag()) {
      case Tag::JSON_NUMBER:
      case Tag::JSON_NULL:
      case Tag::JSON_STRING:
      case Tag::JSON_FLOAT:
      case Tag::JSON_TRUE:
      case Tag::JSON_FALSE:
      case Tag::JSON_OBJECT_BEGIN:
      case Tag::JSON_ARRAY_BEGIN:
        break;
      default:
        throw JsonSyntaxError(nextToken->line(), nextToken->col(),
                              "object missing a value before " +
                                  nextToken->toString());
        break;
      }
    }
  }

private:
  int __tokenIndex;
  int __tokenLength;
  JsonLexer lexer;
  std::shared_ptr<JsonObject> __object_ptr = nullptr;
  std::shared_ptr<JsonArray> __array_ptr = nullptr;
  std::shared_ptr<JsonValue> __value_ptr = nullptr;
};

} // namespace libjson
#endif

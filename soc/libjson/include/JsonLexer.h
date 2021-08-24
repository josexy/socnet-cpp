#ifndef LIBJSON_JSONLEXER_H
#define LIBJSON_JSONLEXER_H

#include <stdio.h>

#include <memory>
#include <vector>

#include "JsonError.h"
#include "JsonReader.h"
#include "JsonToken.h"
#include "JsonUtil.h"

namespace libjson {

class JsonLexer {
public:
  friend class JsonParser;

  JsonLexer(const std::string &text, bool escape = true) try
      : escape(escape), reader(text) {
  } catch (std::exception &e) {
    fprintf(stderr, "%s\n", e.what());
    exit(-1);
  }
  JsonLexer(std::ifstream &ifs, bool escape = true) try
      : escape(escape), reader(ifs) {
  } catch (std::exception &e) {
    fprintf(stderr, "%s\n", e.what());
    exit(-1);
  }

  ~JsonLexer() {}

  void backC() {
    colNumber--;
    reader.back();
  }

  char getC() {
    colNumber++;
    return reader.get();
  }
  std::string getS(int c) {
    colNumber += c;
    return reader.gets(c);
  }
  void analysis() {
    char c;
    // Skip whitespace
    while (!reader.eof()) {
      c = getC();
      if (isspace(c)) {
        continue;
      } else if (isnewline(c)) {
        colNumber = 1;
        lineNumber++;
        continue;
      } else
        break;
    }
    int start_col = 0;
    switch (c) {
    // String literal "..."
    case '\"': {
      start_col = colNumber;
      c = getC();
      std::string s;
      // end with \"
      while (!is_string_begin_end(c)) {
        s.append(1, c);
        // Prevent the occurrence of \" and cause premature
        // termination
        if (c == '\\') {
          c = getC();
          if (!is_valid_next_escape_character(c)) {
            throw JsonSyntaxError(lineNumber, start_col,
                                  "invalid escape character '" +
                                      std::string(1, c) + "'");
          }
          s.append(1, c);
        }
        c = getC();
      }
      if (escape) {
        try {
          s = escape_string(s);
        } catch (JsonError &e) {
          throw JsonSyntaxError(lineNumber, colNumber, e.what());
        }
      }
      return token_list.push_back(
          std::make_shared<String>(s, lineNumber, start_col, s.length()));
    }
    // Comment // ... /* ... */ will not be saved
    case '/': {
      start_col = colNumber;
      c = getC();
      if (!(c == '/' || c == '*'))
        throw JsonSyntaxError(lineNumber, colNumber,
                              "invalid comment // /* */");
      std::string s;
      if (c == '/') {
        c = getC();
        // // ...
        while (c != EOF && !isnewline(c)) {
          s.append(1, c);
          c = getC();
        }
        return;
      } else if (c == '*') {
        c = getC();
        // /* ... */
        while (!(c == '*' && reader.peek() == '/')) {
          if (isnewline(c))
            lineNumber++;
          s.append(1, c);
          c = getC();
        }
        // skip '/'
        getC();
        return;
      }
    }
    // true
    case 't': {
      start_col = colNumber;
      std::string s;
      s.append(1, c);
      s += getS(3);
      if (s == "true")
        return token_list.push_back(std::make_shared<Identifier>(
            Tag::JSON_TRUE, s, lineNumber, start_col, 4));
      else
        throw JsonSyntaxError(lineNumber, colNumber, "not [true] identifier");
    }
    // false
    case 'f': {
      start_col = colNumber;
      std::string s;
      s.append(1, c);
      s += getS(4);
      if (s == "false")
        return token_list.push_back(std::make_shared<Identifier>(
            Tag::JSON_FALSE, s, lineNumber, start_col, 5));
      else
        throw JsonSyntaxError(lineNumber, colNumber, "not [false] identifier");
    }
    // null
    case 'n': {
      start_col = colNumber;
      std::string s;
      s.append(1, c);
      s += getS(3);
      if (s == "null")
        return token_list.push_back(std::make_shared<Identifier>(
            Tag::JSON_NULL, s, lineNumber, start_col, 4));
      else
        throw JsonSyntaxError(lineNumber, colNumber, "not [null] identifier");
    }
    case ':':
      return token_list.push_back(std::make_shared<Token>(
          Tag::JSON_KEY_VALUE_SEP, lineNumber, colNumber, 1));
    case ',':
      return token_list.push_back(
          std::make_shared<Token>(Tag::JSON_SEP, lineNumber, colNumber, 1));
    case '{':
      return token_list.push_back(std::make_shared<Token>(
          Tag::JSON_OBJECT_BEGIN, lineNumber, colNumber, 1));
    case '[':
      return token_list.push_back(std::make_shared<Token>(
          Tag::JSON_ARRAY_BEGIN, lineNumber, colNumber, 1));
    case '}':
      return token_list.push_back(std::make_shared<Token>(
          Tag::JSON_OBJECT_END, lineNumber, colNumber, 1));
    case ']':
      return token_list.push_back(std::make_shared<Token>(
          Tag::JSON_ARRAY_END, lineNumber, colNumber, 1));
    default:
      break;
    }
    // integer or float
    if (isnumber(c) || c == '-' || c == '+') {
      start_col = colNumber;

      if (c == '0' && isnumber(reader.peek()))
        throw JsonSyntaxError(
            lineNumber, colNumber,
            "leading zeros in decimal integer literals are not "
            "permitted");
      std::string s;
      s.append(1, c);
      c = getC();
      while (isnumber(c)) {
        s.append(1, c);
        c = getC();
      }
      // If there is a decimal point or exponent E,
      // then it is treated as a floating point
      if (c == '.' || isexponent(c))
        goto __fraction_exponent;
      // Go back one character to continue processing the next token
      backC();
      return token_list.push_back(
          std::make_shared<Number>(static_cast<int>(toAny<double>(s)),
                                   lineNumber, start_col, s.length()));
    __fraction_exponent:
      // 10e3 10e-3 10e+3  1.1e3 1.1e-3 1.1e+3
      s.append(1, c);
      c = getC();
      while (isnumber(c) || isexponent(c) || c == '+' || c == '-') {
        s.append(1, c);
        c = getC();
      }
      // error : 123.  123.e  123e
      if (s.back() == '.' || isexponent(s.back()))
        throw JsonSyntaxError(lineNumber, colNumber, "invalid float number");
      backC();
      return token_list.push_back(std::make_shared<Float>(
          toAny<double>(s), lineNumber, start_col, s.length()));
    } else {
      // For other characters, treat them as exceptions here
      if (c != EOF)
        throw JsonSyntaxError(lineNumber, start_col,
                              "unknown character in a valid json document: " +
                                  std::string(1, c));
    }
  }

  int line_number() const { return lineNumber; }

private:
  bool escape;
  int lineNumber = 1;
  int colNumber = 1;

  JsonReader reader;
  std::vector<std::shared_ptr<Token>> token_list;
};
} // namespace libjson
#endif

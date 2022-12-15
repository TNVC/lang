#pragma once

namespace db {

  enum statement_t {
    STATEMENT_ADD,
    STATEMENT_SUB,
    STATEMENT_MUL,
    STATEMENT_DIV,

    STATEMENT_SIN,
    STATEMENT_COS,
    STATEMENT_SQRT,

    STATEMENT_OPEN,
    STATEMENT_CLOSE,
    STATEMENT_END,
    STATEMENT_ERROR,

    STATEMENT_ASSIGNMENT,
    STATEMENT_IF,
    STATEMENT_ELSE,
    STATEMENT_WHILE,
    STATEMENT_SEMICOLON,
    STATEMENT_COMPOUND,
    STATEMENT_START_BRACE,
    STATEMENT_END_BRACE,

    STATEMENT_FUN,
    STATEMENT_COLON,
    STATEMENT_TYPE,
    STATEMENT_VOID,
    STATEMENT_PARAMETER,
    STATEMENT_RETURN,
    STATEMENT_CALL,

    STATEMENT_VAR,
    STATEMENT_VAL,

    STATEMENT_COMMA,

    STATEMENT_OUT,
    STATEMENT_IN,
    STATEMENT_LESS,
    STATEMENT_GREATER,

    STATEMENT_OUTPUT,
    STATEMENT_INPUT,
    STATEMENT_LESS_OR_EQUAL,
    STATEMENT_GREATER_OR_EQUAL,
    STATEMENT_UNION,
    STATEMENT_ARROW,

    STATEMENT_NOT,
    STATEMENT_AND,
    STATEMENT_OR,

    STATEMENT_NOT_EQUAL,
    STATEMENT_EQUAL,

    STATEMENT_NEW_LINE,

    //    STATEMENT_ANNOTATION,//
    STATEMENT_STATIC,//

    STATEMENT_START_SQUARE_BRACE,
    STATEMENT_END_SQUARE_BRACE,
    STATEMENT_INT,

    STATEMENT_POW,
  };

  const char *const STATEMENT_NAMES[] =
    {
      "+",
      "-",
      "*",
      "/",

      "sin",
      "cos",
      "sqrt",

      "(",
      ")",
      "end",
      "error",

      "=",
      "if",
      "else",
      "while",
      ";",
      "cmd",
      "{",
      "}",

      "fun",
      ":",
      "Double",
      "Void",
      "param",
      "return",
      "()",

      "var",
      "val",


      ",",

      "out",
      "in",
      "\\<",
      "\\>",

      "\\<\\<",
      "\\>\\>",
      "\\<=",
      "\\>=",
      "::",
      "-\\>",
      "!",
      "&",
      "|",
      "==",
      "!=",

      "new line",

      "static",
      "[",
      "]",
      "int",

      "^"
  };

  struct PositionInfo {
    int line;
    int position;
  };

}

#include "TokenAnalysis.h"
#include "StringPool.h"
#include "DSL.h"

#include <malloc.h>
#include <ctype.h>
#include <string.h>
#include "StringsUtils.h"
#include "SystemLike.h"
#include "Error.h"

#pragma GCC diagnostic ignored "-Wpedantic"

#define EVAL(STATEMENT)                                                 \
  do                                                                    \
    {                                                                   \
      tokens[tokensSize  ]           = ST(db::STATEMENT_ ## STATEMENT); \
      tokens[tokensSize++]->position = position;                        \
    } while (0)

#define CASE(CHAR, CODE, OFFSET)                           \
  case CHAR: EVAL(CODE); UPDATE_POSITION(OFFSET); break;

#define DEFAULT(CODE, OFFSET)                                        \
  default: --source; EVAL(CODE); UPDATE_POSITION(OFFSET); break;

#define UPDATE_LINE()                           \
  do                                            \
    {                                           \
      ++position.line;                          \
      position.position = 1;                    \
    } while (0)

#define UPDATE_POSITION(OFFSET)                 \
  do                                            \
    {                                           \
      position.position += OFFSET;              \
    } while (0)

const size_t DEFAULT_TOKENS_SIZE =  16;
const size_t GROWTH_FACTOR       =   2;
const size_t MAX_NAME_SIZE       =  32;
const size_t MAX_STRING_SIZE     = 128;

struct Statement {
  const char *name;
  db::statement_t value;
  int size;
};

const Statement STATEMENTS[] =
  {
    {";"     , db::STATEMENT_SEMICOLON  , 1},
    {"{"     , db::STATEMENT_START_BRACE, 1},
    {"}"     , db::STATEMENT_END_BRACE  , 1},
    {"("     , db::STATEMENT_OPEN       , 1},
    {")"     , db::STATEMENT_CLOSE      , 1},
    {"-"     , db::STATEMENT_SUB        , 1},
    {"+"     , db::STATEMENT_ADD        , 1},
    {"*"     , db::STATEMENT_MUL        , 1},
    {"/"     , db::STATEMENT_DIV        , 1},
    {":"     , db::STATEMENT_COLON      , 1},
    {"="     , db::STATEMENT_ASSIGNMENT , 1},
    {","     , db::STATEMENT_COMMA      , 1},
    {">"     , db::STATEMENT_GREATER    , 1},
    {"<"     , db::STATEMENT_LESS       , 1},
    {"!"     , db::STATEMENT_NOT        , 1},
    {"|"     , db::STATEMENT_OR         , 1},
    {"&"     , db::STATEMENT_AND        , 1},
    {"endl"  , db::STATEMENT_NEW_LINE   , 4},
    {"out"   , db::STATEMENT_OUT        , 3},
    {"in"    , db::STATEMENT_IN         , 2},
    {"val"   , db::STATEMENT_VAL        , 3},
    {"var"   , db::STATEMENT_VAR        , 3},
    {"sqrt"  , db::STATEMENT_SQRT       , 4},
    {"sin"   , db::STATEMENT_SIN        , 3},
    {"cos"   , db::STATEMENT_COS        , 3},
    {"tan"   , db::STATEMENT_TAN        , 3},
    {"fun"   , db::STATEMENT_FUN        , 3},
    {"Void"  , db::STATEMENT_VOID       , 4},
    {"Double", db::STATEMENT_TYPE       , 6},
    {"return", db::STATEMENT_RETURN     , 6},
    {"if"    , db::STATEMENT_IF         , 2},
    {"else"  , db::STATEMENT_ELSE       , 4},
    {"while" , db::STATEMENT_WHILE      , 5},
    {"static", db::STATEMENT_STATIC     , 6},
    {"diff"  , db::STATEMENT_DIFF       , 4},
  };

const int STATEMENTS_SIZE = 35;

static db::Token *resizeTokens(db::Token *tokens, size_t newSize, int *error = nullptr);

static db::Token createNumber(db::number_t value)
{
  return db::createNode({.number = value}, db::type_t::NUMBER);
}

static db::Token createName(db::name_t value)
{
  return db::createNode({.name = value}, db::type_t::NAME);
}

static db::Token createString(db::string_t value)
{
  return db::createNode({.string = value}, db::type_t::STRING);
}

static db::Token createStatement(db::statement_t value)
{
  return db::createNode({.statement = value}, db::type_t::STATEMENT);
}

static db::Token *resizeTokens(db::Token *tokens, size_t newSize, int *error)
{
  if (!tokens) ERROR(nullptr);

  db::Token *temp =
    (db::Token *)recalloc(tokens, newSize, sizeof(db::Token));
  if (!temp) { free(tokens); ERROR(nullptr); }

  return temp;
}

db::Token *db::getTokens(const char *source, db::StringPool *pool, int *error)
{
  if (!source) ERROR(nullptr);

  db::Token *tokens =
    (db::Token *)calloc(DEFAULT_TOKENS_SIZE, sizeof(db::Token));
  if (!tokens) ERROR(nullptr);

  size_t tokensSize     = 0;
  size_t tokensCapacity = DEFAULT_TOKENS_SIZE;

  db::PositionInfo position{1, 1};

  for ( ; *source; ++source)
    {
      if (tokensSize == tokensCapacity)
        {
          tokensCapacity *= GROWTH_FACTOR;
          tokens = resizeTokens(tokens, tokensCapacity, error);

          if (!tokens) ERROR(nullptr);
        }

      switch (*source)
        {
        case ' ': case '\t': case '\n':
          {
            ++position.position;
            if (*source == '\n') UPDATE_LINE();

            break;
          }

        case '+': EVAL(ADD); UPDATE_POSITION(1);  break;
        case '-':
          {
            ++source;
            switch (*source)
              {
              case '>': EVAL(ARROW); UPDATE_POSITION(2); break;
              default: --source; EVAL(SUB); UPDATE_POSITION(1); break;
              }

            break;
          }
        case '*': EVAL(MUL); UPDATE_POSITION(1); break;
        case '/':
          {
            ++source;
            switch (*source)
              {
              case '/':
                {
                  UPDATE_LINE();
                  while (*source && *source != '\n') ++source;
                  break;
                }
              case '*':
                {
                  for ( ; *source && !(*source == '*' && source[1] == '/'); ++source)
                    {
                      ++position.position;
                      if (*source == '\n') UPDATE_LINE();
                    }
                  if (*source)
                    ++source;

                  break;
                }
              default: --source; EVAL(DIV); break;
              }

            break;
          }

        case '(': EVAL(OPEN ); UPDATE_POSITION(1); break;
        case ')': EVAL(CLOSE); UPDATE_POSITION(1); break;
        case '{': EVAL(START_BRACE); UPDATE_POSITION(1); break;
        case '}': EVAL(  END_BRACE); UPDATE_POSITION(1); break;

        case ':':
          {
            ++source;
            switch (*source)
              {
              case ':': EVAL(UNION); UPDATE_POSITION(2); break;
              default: --source; EVAL(COLON); UPDATE_POSITION(1); break;
              }

            break;
          }
        case ';': EVAL(SEMICOLON); UPDATE_POSITION(1); break;

        case ',': EVAL(COMMA); UPDATE_POSITION(1); break;

        case '>':
          {
            ++source;
            switch (*source)
              {
              case '=': EVAL(GREATER_OR_EQUAL); UPDATE_POSITION(2); break;
              case '>': EVAL(INPUT); UPDATE_POSITION(2); break;
              default: --source; EVAL(GREATER); UPDATE_POSITION(1); break;
              }

            break;
          }
        case '<':
          {
            ++source;
            switch (*source)
              {
              case '=': EVAL(LESS_OR_EQUAL); UPDATE_POSITION(2); break;
              case '<': EVAL(OUTPUT); UPDATE_POSITION(2); break;
              default: --source; EVAL(LESS); UPDATE_POSITION(1); break;
              }

            break;
          }

        case '=':
          {
            ++source;
            switch (*source)
              {
              case '=': EVAL(EQUAL); UPDATE_POSITION(2); break;
              default: --source; EVAL(ASSIGNMENT); UPDATE_POSITION(1); break;
              }

            break;
          }

          case '!':
            {
              ++source;
              switch (*source)
                {
                case '=': EVAL(NOT_EQUAL); UPDATE_POSITION(2); break;
                default: EVAL(NOT); UPDATE_POSITION(1); break;
                }

              break;
            }
          case '|':
            {
              ++source;
              switch (*source)
                {
                case '|': EVAL(OR); UPDATE_POSITION(2); break;
                default: --source; EVAL(ERROR); UPDATE_POSITION(1); break;
                }

              break;
            }
          case '&':
            {
              ++source;
              switch (*source)
                {
                case '&': EVAL(AND); UPDATE_POSITION(2); break;
                default: --source; EVAL(ERROR); UPDATE_POSITION(1); break;
                }

              break;
            }

          case '[':
           {
             EVAL(START_SQUARE_BRACE); UPDATE_POSITION(1);
             break;
           }

          case ']':
            {
              EVAL(END_SQUARE_BRACE); UPDATE_POSITION(1);
              break;
            }

          case '\"':
            {
              db::PositionInfo start = position;

              ++source;
              char buffer[MAX_STRING_SIZE] = "";
              int offset = 0;
              for ( ; *source && *source != '\"'; ++source)
                {
                  ++position.position;
                  if (*source == '\n') UPDATE_LINE();
                  buffer[offset++] = *source;
                }

              char *string = db::addString(pool, buffer, error);

              tokens[tokensSize  ]           = STR(string);
              tokens[tokensSize++]->position = start;

              break;
            }

        case 'a' ... 'z': case 'A' ... 'Z': case '_': case '$':
          {
            bool hasName = true;
            for (int i = 0; i < STATEMENTS_SIZE; ++i)
              {
                if (!strncmp(source, STATEMENTS[i].name, (size_t)STATEMENTS[i].size))
                  {
                    switch (source[STATEMENTS[i].size])
                      {
                      case 'a' ... 'z': case 'A' ... 'Z': case '_': case '$':
                        continue;
                      default: break;
                      }
                    tokens[tokensSize  ]           = ST(STATEMENTS[i].value);
                    tokens[tokensSize++]->position = position;
                    source += STATEMENTS[i].size - 1;

                    UPDATE_POSITION(STATEMENTS[i].size);
                    hasName = false;
                    break;
                  }
              }

            if (hasName)
              {
                char buffer[MAX_NAME_SIZE] = "";
                int offset = 0;
                sscanf(source, "%[_$a-zA-Z]%n", buffer, &offset);

                char *string = db::addString(pool, buffer, error);

                tokens[tokensSize  ]           = NAM(string);
                tokens[tokensSize++]->position = position;

                source += offset - 1;
                UPDATE_POSITION(offset);
              }

            break;
          }

        case '0' ... '9':
          {
            double num = 0;
            int offset = 0;
            sscanf(source, "%lg%n", &num, &offset);
            tokens[tokensSize  ]           = NUM(num);
            tokens[tokensSize++]->position = position;

            source += offset - 1;

            UPDATE_POSITION(offset);
            break;
          }
        default: break;
        }
    }

  if (tokensSize == tokensCapacity)
    {
      tokens = resizeTokens(tokens, tokensSize+1, error);

      if (!tokens) ERROR(nullptr);
    }

  tokens[tokensSize  ]           = ST(db::STATEMENT_END);
  tokens[tokensSize++]->position = position;

  return resizeTokens(tokens, tokensSize, error);
}

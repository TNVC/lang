#include "TokenAnalysis.h"
#include "DSL.h"

#include <malloc.h>
#include <ctype.h>
#include <string.h>
#include "StringsUtils.h"
#include "SystemLike.h"
#include "Error.h"

#define EVAL_STATEMENT(STATEMENT, OFFSET)               \
  do { stNum = STATEMENT; offset = OFFSET; } while (0)

const size_t DEFAULT_TOKENS_SIZE = 16;
const size_t GROWTH_FACTOR       =  2;
const size_t MAX_NAME_SIZE       = 32;

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
    {"fun"   , db::STATEMENT_FUN        , 3},
    {"Void"  , db::STATEMENT_VOID       , 4},
    {"Double", db::STATEMENT_TYPE       , 6},
    {"return", db::STATEMENT_RETURN     , 6},
    {"if"    , db::STATEMENT_IF         , 2},
    {"else"  , db::STATEMENT_ELSE       , 4},
    {"while" , db::STATEMENT_WHILE      , 5},
  };

const int STATEMENTS_SIZE = 32;

static void skipSpaces (
                        const char **source,
                        db::PositionInfo *position,
                        int *error = nullptr
                       );

static void skipComments(
                         const char **source,
                         db::PositionInfo *position,
                         int *error = nullptr
                        );

static bool getStatement(
                         const char **source,
                         db::PositionInfo *position,
                         db::Token *token,
                         int *error = nullptr
                        );

static bool getName    (
                        const char **source,
                        db::PositionInfo *position,
                        db::Token *token,
                        int *error = nullptr
                       );

static bool getNumber  (
                        const char **source,
                        db::PositionInfo *position,
                        db::Token *token,
                        int *error = nullptr
                       );

static bool getString  (
                        const char **source,
                        db::PositionInfo *position,
                        db::Token *token,
                        int *error = nullptr
                       );

static bool getError   (
                        const char **source,
                        db::PositionInfo *position,
                        db::Token *token,
                        int *error = nullptr
                       );

static bool isStatement(const char *name, int *error = nullptr);

static db::Token *resizeTokens(db::Token *tokens, size_t newSize, int *error = nullptr);

static db::Token createNumber(db::number_t value)
{
  return db::createNode({.number = value}, db::type_t::NUMBER);
}

static db::Token createName(db::name_t value)
{
  return db::createNode({.name = value}, db::type_t::NAME);
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

db::Token *db::getTokens(const char *source, int *error)
{
  if (!source) ERROR(nullptr);

  db::Token *tokens =
    (db::Token *)calloc(DEFAULT_TOKENS_SIZE, sizeof(db::Token));
  if (!tokens) ERROR(nullptr);

  size_t tokensSize     = 0;
  size_t tokensCapacity = DEFAULT_TOKENS_SIZE;

  db::PositionInfo position{1, 1};

  int errorCode = 0;
  while (*source)
    {
      if (tokensSize == tokensCapacity)
        {
          tokensCapacity *= GROWTH_FACTOR;
          tokens = resizeTokens(tokens, tokensCapacity, error);

          if (!tokens) ERROR(nullptr);
        }

      skipSpaces(&source, &position, &errorCode);

      bool wasRead = false;
      if (!wasRead)
        wasRead = getStatement(&source, &position, &tokens[tokensSize], &errorCode);
      if (!wasRead)
        wasRead = getNumber   (&source, &position, &tokens[tokensSize], &errorCode);
      if (!wasRead)
        wasRead = getName     (&source, &position, &tokens[tokensSize], &errorCode);
      if (!wasRead)
        wasRead = getString   (&source, &position, &tokens[tokensSize], &errorCode);
      if (!wasRead)
        {
          wasRead = getError  (&source, &position, &tokens[tokensSize], &errorCode);
          ++tokensSize;
          break;
        }

      skipSpaces(&source, &position, &errorCode);

      ++tokensSize;
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

static void skipSpaces(
                       const char **source,
                       db::PositionInfo *position,
                       int *error
                      )
{
  if (!source || !*source || !position) ERROR();

  while (isspace(**source) && **source)
    {
      if (**source == '\n')
        {
          ++position->line;
          position->position = 1;
        }
      else ++position->position;

      ++*source;
    }

  skipComments(source, position, error);
}

static void skipComments(
                         const char **source,
                         db::PositionInfo *position,
                         int *error
                        )
{
  if (!source || !*source || !position) ERROR();

  if (**source != '/' || *(*source + 1) != '/')
    return;

  while (**source != '\n' && **source)
    ++*source;

  if (**source == '\n')
    ++*source;

  ++position->line;
  position->position = 1;

  skipSpaces(source, position, error);
}

static bool getStatement(
                         const char **source,
                         db::PositionInfo *position,
                         db::Token *token,
                         int *error
                        )
{
  if (!source || !*source || !position || !token) ERROR(false);

  skipSpaces(source, position, error);

  char name[MAX_NAME_SIZE] = "";
  int offset               =  1;

  db::statement_t stNum = db::STATEMENT_ERROR;
  sscanf(*source, "%[a-zA-Z]", name);

  for (int i = 0; i < STATEMENTS_SIZE; ++i)
      if (STATEMENTS[i].size == 1)
        { if (STATEMENTS[i].name[0] == **source) EVAL_STATEMENT(STATEMENTS[i].value , 1); }
      else
        { if (!strcmp(STATEMENTS[i].name, name)) EVAL_STATEMENT(STATEMENTS[i].value , STATEMENTS[i].size); }

  if (stNum == db::STATEMENT_ERROR) return false;

   *token            = ST(stNum);
  (*token)->position = *position;

  position->position += offset;
  *source            += offset;

  return true;
}

static bool getName(
                    const char **source,
                    db::PositionInfo *position,
                    db::Token *token,
                    int *error
                   )
{
  if (!source || !*source || !position || !token) ERROR(false);

  skipSpaces(source, position, error);

  char name[MAX_NAME_SIZE] = "";
  int offset               = -1;

  if (sscanf(*source, "%[_a-zA-Z]%n", name, &offset) != 1) return false;

  if (isStatement(name)) return false;

   *token            = VAR(name);
  (*token)->position = *position;

  position->position += offset;
  *source            += offset;

  return true;
}

static bool getNumber(
                      const char **source,
                      db::PositionInfo *position,
                      db::Token *token,
                      int *error
                     )
{
  if (!source || !*source || !position || !token) ERROR(false);

  skipSpaces(source, position, error);

  double value  = NAN;
  int    offset =  -1;

  if (sscanf(*source, "%lg%n", &value, &offset) != 1) return false;
  if (!isfinite(value)) return false;

   *token            = NUM(value);
  (*token)->position = *position;

  position->position += offset;
  *source            += offset;

  return true;
}

static bool getString  (
                        const char **source,
                        db::PositionInfo *position,
                        db::Token *token,
                        int *error
                       )
{
  if (!source || !*source || !position || !token) ERROR(false);

  skipSpaces(source, position, error);

  char string[MAX_NAME_SIZE] = "";
  int offset                 = -1;

  if (sscanf(*source, " \"%[^\"]\"%n", string, &offset) != 1) return false;

  *token             = VAR(string);
  (*token)->type = db::type_t::STRING;
  (*token)->position = *position;

  position->position += offset;
  *source            += offset;

  return true;
}

static bool getError(
                     const char **source,
                     db::PositionInfo *position,
                     db::Token *token,
                     int *error
                    )
{
  if (!source || !*source || !position || !token) ERROR(false);

   *token            = ST(db::STATEMENT_ERROR);
  (*token)->position = *position;

  return true;
}

static bool isStatement(const char *name, int *error)
{
  if (!name) ERROR(false);

  for (int i = 0; i < STATEMENTS_SIZE; ++i)
    if (!strcmp(name, STATEMENTS[i].name)) return true;

  return false;
}

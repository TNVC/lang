#include "Translator.h"

#include "ErrorHandler.h"
#include "StringsUtils.h"
#include "Error.h"
#include "Assert.h"
#include "DSL.h"

#pragma GCC diagnostic ignored "-Wswitch-enum"

#define CASE(STATEMENT, NAME)                                         \
  case db::STATEMENT_ ## STATEMENT: fprintf(target, " " #NAME " "); break;

static const char *getNodePrefix(db::Token token, int *error);

static void printSpaces(int count, FILE *target);

static void saveToken(const db::Token token, FILE *target, int *error = nullptr);

static void printSpaces(int count, FILE *target)
{
  assert(count >= 0);
  assert(target);

  fprintf(target, "%*s", count, "");
}

void db::saveTranslator(
                        Translator *translator,
                        FILE *target,
                        int *error
                       )
{
  if (!translator || !translator->grammar.root || !target) ERROR();

  int errorCode = 0;
  saveToken(translator->grammar.root, target, &errorCode);
  if (errorCode) ERROR();
}

static void saveToken(const db::Token token, FILE *target, int *error)
{
  assert(token);
  assert(target);

  static int tabs = 0;

  const char *prefix = getNodePrefix(token, error);

  printSpaces(tabs, target);
  fprintf(target, " %s { ", prefix);

  tabs += 2;

  switch (token->type)
    {
    case db::type_t::STATEMENT:
      {
        switch (STATEMENT(token))
          {
            CASE(ADD, ADD);
            CASE(SUB, SUB);
            CASE(MUL, MUL);
            CASE(DIV, DIV);
            CASE(SIN , SIN );
            CASE(COS , COS );
            CASE(SQRT, SQRT);
            CASE(ASSIGNMENT, EQ);
            CASE(IF  , IF  );
            CASE(ELSE, ELSE);
            CASE(WHILE, WHILE);
            CASE(COMPOUND, ST);
            CASE(FUN, FUNC);
            CASE(STATIC, STATIC)
            CASE(TYPE, TYPE);
            CASE(VOID, VOID);
            CASE(PARAMETER, PARAM);
            CASE(RETURN, RET);
            CASE(CALL, CALL);
          case db::STATEMENT_VAL:
            CASE(VAR, VAR);
            CASE(OUT, OUT);
            CASE(IN , IN );
            CASE(EQUAL    , IS_EE);
            CASE(NOT_EQUAL, IS_NE);
            CASE(LESS     , IS_BT);
            CASE(GREATER  , IS_GT);
            CASE(   LESS_OR_EQUAL, IS_BE);
            CASE(GREATER_OR_EQUAL, IS_GE);
            CASE(OR , OR );
            CASE(AND, AND);
            CASE(NEW_LINE, ENDL);
            CASE(INT, MOD);
          default: break;
          }
        break;
      }
    case db::type_t::NAME:
      { fprintf(target, " \"%s\" ",   NAME(token)); break; }
    case db::type_t::NUMBER:
      { fprintf(target, " %lg "   , NUMBER(token)); break; }
    case db::type_t::STRING:
      { fprintf(target, " '%s' "  , STRING(token)); break; }
    default: break;
    }

  if (token->left )
    {
      fprintf(target, "\n");
      saveToken(token->left , target, error);
    }
  else if (IS_STATEMENT(token) || (IS_NAME(token) && token->right))
    {
      fprintf(target, "\n");
      printSpaces(tabs, target);
      fprintf(target, "  { NIL } \n");
    }

  if (token->right)
    saveToken(token->right, target, error);
  else if (IS_STATEMENT(token) || (IS_NAME(token) && token->right))
    {
      printSpaces(tabs, target);
      fprintf(target, "  { NIL } \n");
    }

  tabs -= 2;
  if (IS_STATEMENT(token) || (IS_NAME(token) && (token->left || token->right)))
    printSpaces(tabs, target);
  fprintf(target, "  } %c \n", *prefix ? '$' : ' ');
}

static const char *getNodePrefix(db::Token token, int *error)
{
  if (!token) ERROR(nullptr);

  if (IS_STRING(token) || IS_ENDL(token))
    return " $db::str ";
  if (IS_SQRT(token) || IS_INT(token))
    return " $db::math ";

  if (IS_EQUAL(token)   || IS_NOT_EQUAL(token) ||
      IS_LESS(token)    || IS_GREATER(token)   ||
      IS_LESS_EQ(token) || IS_GREATER_EQ(token))
    return " $db::relation ";

  if (IS_OR(token) || IS_AND(token))
    return " $db::logic ";

  return "";
}

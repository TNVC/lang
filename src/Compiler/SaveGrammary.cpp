#include "Translator.h"

#include "Error.h"
#include "Assert.h"
#include "DSL.h"

#define CASE(STATEMENT, NAME)                                         \
  case db::STATEMENT_ ## STATEMENT: fprintf(target, " " #NAME " "); break;

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

/*
void loadTranslator(
                    Translator *translator,
                    FILE *source,
                    int *error
                    );*/
static void saveToken(const db::Token token, FILE *target, int *error)
{
  assert(token);
  assert(target);

  static int tabs = 0;

  printSpaces(tabs, target);
  fprintf(target, " { ");

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

            CASE(SIN , $db::math SIN  $);
            CASE(COS , $db::math COS  $);
            CASE(SQRT, $db::math SQRT $);

            CASE(ASSIGNMENT, EQ);

            CASE(IF  , IF  );
            CASE(ELSE, ELSE);

            CASE(WHILE, WHILE);
            CASE(COMPOUND, ST);
            CASE(FUN, FUNC);

            CASE(TYPE, $db::type DOUBLE $);
            CASE(VOID, $db::type VOID $);

            CASE(PARAMETER, PARAM);
            CASE(RETURN, RET);
            CASE(CALL, CALL);

          case db::STATEMENT_VAL:
            CASE(VAR, VAR);

          CASE(OUT, $db::io OUT $);
          CASE(IN , $db::io IN  $);


          CASE(EQUAL    , $db::relation IS_EE $);
          CASE(NOT_EQUAL, $db::relation IS_NE $);
          CASE(LESS     , $db::relation IS_BT $);
          CASE(GREATER  , $db::relation IS_GT $);

          CASE(OR , $db::logic OR  $);
          CASE(AND, $db::logic AND $);

          CASE(NEW_LINE, $db::str ENDL $);

          case db::STATEMENT_COLON:
          case db::STATEMENT_START_BRACE:
          case db::STATEMENT_END_BRACE:
          case db::STATEMENT_COMMA:
          case db::STATEMENT_NOT:
          case db::STATEMENT_SEMICOLON:
          case db::STATEMENT_OPEN:
          case db::STATEMENT_CLOSE:
          case db::STATEMENT_END:
          case db::STATEMENT_ERROR:
          default: break;
          }
        break;
      }
    case db::type_t::NAME:
      { fprintf(target, " \"%s\" "              ,   NAME(token)); break; }
    case db::type_t::NUMBER:
      { fprintf(target, " %lg "                 , NUMBER(token)); break; }
    case db::type_t::STRING:
      { fprintf(target, " $db::str \"%s\" $ "   , STRING(token)); break; }
    default: break;
    }

  if (token->left )
    {
      fprintf(target, "\n");
      saveToken(token->left , target, error);
    }
  else if (IS_STATEMENT(token) || IS_NAME(token))
    {
      if (token->right)
        {
          fprintf(target, "\n");
          printSpaces(tabs, target);
        }
      fprintf(target, " { NIL } ");
      if (token->right) fprintf(target, "\n");
    }

  if (token->right)
    {
      saveToken(token->right, target, error);
      fprintf(target, "\n");
    }
  else if (IS_STATEMENT(token) || IS_NAME(token))
    {
      if (token->left)
        printSpaces(tabs, target);
      fprintf(target, " { NIL } \n");
    }

  tabs -= 2;
  if (IS_STATEMENT(token) || (IS_NAME(token) && (token->left || token->right)))
    printSpaces(tabs, target);
  fprintf(target, " } \n");
}

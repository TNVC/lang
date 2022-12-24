#include "Translator.h"

#include "DSL.h"
#include "ErrorHandler.h"
#include "Error.h"

#pragma GCC diagnostic ignored "-Wswitch-enum"

#define TRANSLATE(FORMAT, ...)                        \
  if (token->left )                                   \
    disassemblerToken(token->left , target, error);   \
  fprintf(target, FORMAT __VA_OPT__(,) __VA_ARGS__);  \
  if (token->right)                                   \
    disassemblerToken(token->right, target, error);   \
                                                      \
  break;

#define HANDLE_ERROR(MESSAGE, ...)                    \
  do                                                  \
    {                                                 \
      handleError(MESSAGE __VA_OPT__(,) __VA_ARGS__); \
      ERROR();                                        \
    } while (0)

static void disassemblerToken(db::Token token, FILE *target, int *error);

void db::disassemblerGrammar(const db::Translator *translator, FILE *target, int *error)
{
  if (!translator || !translator->grammar.root) ERROR();

  int errorCode = 0;
  disassemblerToken(translator->grammar.root, target, &errorCode);
  if (errorCode) ERROR();
}

static void disassemblerToken(db::Token token, FILE *target, int *error)
{
  if (!token || !target || !error) {
    *token = {};
    ERROR();
  }

  switch (token->type)
    {
    case db::type_t::NUMBER: TRANSLATE(" %lg "   , NUMBER(token));
    case db::type_t::NAME:   TRANSLATE(" %s "    , NAME(token)  );
    case db::type_t::STRING: TRANSLATE(" \"%s\" ", STRING(token));
    case db::type_t::STATEMENT:
      {
        switch (STATEMENT(token))
          {
          case db::STATEMENT_ADD:  TRANSLATE("%c"  , '+'   );
          case db::STATEMENT_SUB:  TRANSLATE("%c"  , '-'   );
          case db::STATEMENT_MUL:  TRANSLATE("%c"  , '*'   );
          case db::STATEMENT_DIV:  TRANSLATE("%c"  , '/'   );
          case db::STATEMENT_SIN:  TRANSLATE(" %s ", "sin" );
          case db::STATEMENT_COS:  TRANSLATE(" %s ", "cos" );
          case db::STATEMENT_TAN:  TRANSLATE(" %s ", "tan" );
          case db::STATEMENT_SQRT: TRANSLATE(" %s ", "sqrt");
          case db::STATEMENT_LESS:      TRANSLATE("%c", '<' );
          case db::STATEMENT_GREATER:   TRANSLATE("%c", '>' );
          case db::STATEMENT_EQUAL:     TRANSLATE("%s", "==");
          case db::STATEMENT_NOT_EQUAL: TRANSLATE("%s", "!=");
          case db::STATEMENT_AND:       TRANSLATE("%s", "&&");
          case db::STATEMENT_OR:        TRANSLATE("%s", "||");
          case db::STATEMENT_NEW_LINE:  TRANSLATE(" %s ", "newl");

          case db::STATEMENT_ASSIGNMENT: TRANSLATE(" %c  ", '=');

          case db::STATEMENT_POW: handleError("POW isn`t allowed"); ERROR();

          case db::STATEMENT_INT:
            {
              if (!token->left) HANDLE_ERROR("Int hasn`t argument");

              fprintf(target, "[");

              disassemblerToken(token->left, target, error);
              if (*error) ERROR();

              fprintf(target, "]");

              break;
            }

          case db::STATEMENT_DIFF:
            {
              if (!token->left) HANDLE_ERROR("Diff hasn`t argument");

              fprintf(target, " diff(");

              disassemblerToken(token->left, target, error);
              if (*error) ERROR();

              fprintf(target, ")");

              break;
            }

          case db::STATEMENT_RETURN:
            {
              fprintf(target, "return ");

              if (token->left)
                {
                  disassemblerToken(token->left, target, error);
                  if (*error) ERROR();
                }

              fprintf(target, ";\n");

              break;
            }

          case db::STATEMENT_COMPOUND:
            {
              db::Token temp = token;
              for ( ; temp; temp = temp->right)
                {
                  if (!temp->left) HANDLE_ERROR("Statement hasn`t instruction");

                  if (IS_COMP(temp->left))
                    fprintf(target, " { \n");

                  disassemblerToken(temp->left, target, error);
                  if (*error) ERROR();

                  if (IS_ASSIGN(temp->left))
                    fprintf(target, ";\n");

                  if (IS_COMP(temp->left))
                    fprintf(target, " } \n");
                }

              break;
            }

          case db::STATEMENT_IF:
            {
              fprintf(target, " if (");

              if (!token->left)             HANDLE_ERROR("If hasn`t conditional");
              if (!token->right ||
                  (IS_ELSE(token->right) &&
                   (!token->right->left ||
                    !token->right->right))) HANDLE_ERROR("If hasn`t body");

              disassemblerToken(token->left, target, error);
              if (*error) ERROR();

              fprintf(target, ") {\n");

              if (IS_ELSE(token->right))
                disassemblerToken(token->right->left, target, error);
              else
                disassemblerToken(token->right      , target, error);
              if (*error) ERROR();

              fprintf(target, "}\n");

              if (IS_ELSE(token->right))
                {
                  fprintf(target, " else {\n");

                  disassemblerToken(token->right->right, target, error);
                  if (*error) ERROR();

                  fprintf(target, "}\n");
                }

              break;
            }

          case db::STATEMENT_WHILE:
            {
              fprintf(target, " while (");

              if (!token->left) ERROR();
              disassemblerToken(token->left , target, error);
              if (*error) ERROR();

              fprintf(target, ") {\n");

              disassemblerToken(token->right, target, error);
              if (*error) ERROR();

              fprintf(target, "}\n");
              break;
            }

          case db::STATEMENT_FUN:
            {
              if (!IS_NAME(token->left))
                HANDLE_ERROR("Function hasn`t name");
              if (!IS_TYPE(token->left->right) &&
                  !IS_VOID(token->left->right))
                HANDLE_ERROR("Function %s hasn`t return type", NAME(token->left));
              if (!IS_COMP(token->right))
                HANDLE_ERROR("Function %s hasn`t body", NAME(token->left));

              fprintf(target, "fun %s(", NAME(token->left));

              db::Token temp = token->left->left;
              for ( ; temp; temp = temp->right)
                {
                  if (!IS_VAR(temp->left) || !IS_NAME(temp->left->left))
                    HANDLE_ERROR("Invalid argument: %s", NAME(token->left));

                  fprintf(target, "%s", NAME(temp->left->left));

                  if (temp->left->right)
                    {
                      fprintf(target, " = ");
                      disassemblerToken(temp->left->right, target, error);
                      if (*error) ERROR();
                    }

                  if (temp->right)
                    fprintf(target, ", ");
                }

              fprintf(target, "): %s {\n",
                      IS_TYPE(token->left->right) ?
                      "Double" : "Void");

              disassemblerToken(token->right, target, error);
              if (*error) ERROR();

              fprintf(target, "}\n");
              break;
            }

          case db::STATEMENT_VAR:
            {
              if (!IS_NAME(token->left))
                HANDLE_ERROR("Declaration of variable hasn`t name");
              if (!token->right)
                HANDLE_ERROR("Declaration of variable hasn`t value");

              fprintf(target, "var %s: Double = ", NAME(token->left));

              disassemblerToken(token->right, target, error);
              if (*error) ERROR();

              fprintf(target, ";\n");

              break;
            }

          case db::STATEMENT_CALL:
            {
              if (!IS_NAME(token->left))
                HANDLE_ERROR("Call hasn`t name");

              fprintf(target, "%s(", NAME(token->left));

              db::Token temp = token->left->left;
              for ( ; temp; temp = temp->right)
                {
                  if (!temp->left)
                    HANDLE_ERROR("Invalid call: %s", NAME(token->left));

                  disassemblerToken(temp->left, target, error);
                  if (*error) ERROR();

                  if (temp->right)
                    fprintf(target, ", ");
                }

              fprintf(target, ")");

              break;
            }

          case db::STATEMENT_OUT:
            {
              if (!token->left) HANDLE_ERROR("Out have to at least one argument");

              fprintf(target, "out <<");

              db::Token temp = token->left;
              for ( ; temp; temp = temp->right)
                {
                  if (!temp->left)
                    HANDLE_ERROR("Invalid out");

                  disassemblerToken(temp->left, target, error);
                  if (*error) ERROR();

                  if (temp->right)
                    fprintf(target, "<<");
                }

              fprintf(target, ";\n");

              break;
            }

          case db::STATEMENT_IN:
            {
              if (!token->left) HANDLE_ERROR("In have to at least one argument");

              fprintf(target, "in >>");

              db::Token temp = token->left;
              for ( ; temp; temp = temp->right)
                {
                  if (!temp->left)
                    HANDLE_ERROR("Invalid in");

                  disassemblerToken(temp->left, target, error);
                  if (*error) ERROR();

                  if (temp->right)
                    fprintf(target, ">>");
                }

              fprintf(target, ";\n");

              break;
            }

          default:
            {
              break;
              handleError("Unknown statement type at %s:%d", __FILE__, __LINE__);
              ERROR();
            }
          }

        break;
      }
    default:
      {
        handleError("Unknown token type at %s:%d", __FILE__, __LINE__);
        ERROR();
      }
    }

  return;
}

#include "Translator.h"

#include <math.h>
#include "DSL.h"
#include "Assert.h"
#include "ErrorHandler.h"
#include "Error.h"

#pragma GCC diagnostic ignored "-Wswitch-enum"

#undef Left
#undef Right

#define Left  token->left
#define Right token->right

#define dLeft  diff(copyLeft )
#define dRight diff(copyRight)

#define copyLeft  db::createNode(token->left )
#define copyRight db::createNode(token->right)

#define HANDLE_ERROR(MESSAGE, ...)                    \
  do                                                  \
    {                                                 \
      handleError(MESSAGE __VA_OPT__(,) __VA_ARGS__); \
      ERROR();                                        \
    } while (0)

#define SIMPLY_UNARY(NAME, OP)                    \
  do                                              \
    {                                             \
      if (!token->left)                           \
        HANDLE_ERROR("Expected that " #NAME       \
                     " has one argument");        \
                                                  \
      simplyToken(token->left, error);            \
      if (*error) ERROR();                        \
                                                  \
      if (!IS_NUM(token->left ))                  \
        break;                                    \
                                                  \
      token->type = db::type_t::NUMBER;           \
      NUMBER(token) = OP(NUMBER(token->left));    \
                                                  \
      db::Token left  = token->left;              \
      token->left = nullptr;                      \
                                                  \
      db::removeNode(left );                      \
    } while (0)

#define SIMPLY_BINARY(NAME, OP)                       \
  do                                                  \
    {                                                 \
      if (!token->left || !token->right)              \
        HANDLE_ERROR("Expected that " #NAME           \
                     " has two arguments");           \
                                                      \
      simplyToken(token->left, error);                \
      if (*error) ERROR();                            \
      simplyToken(token->right, error);               \
      if (*error) ERROR();                            \
                                                      \
      if (!IS_NUM(token->left ) ||                    \
          !IS_NUM(token->right))                      \
        break;                                        \
                                                      \
      token->type = db::type_t::NUMBER;               \
      NUMBER(token) =                                 \
        NUMBER(token->left) OP NUMBER(token->right);  \
                                                      \
      db::Token left  = token->left ;                 \
      token->left  = nullptr;                         \
      db::Token right = token->right;                 \
      token->right = nullptr;                         \
                                                      \
      db::removeNode(left );                          \
      db::removeNode(right);                          \
    } while (0)

#define SIMPLY_LINARY(NAME, OP)                           \
  do                                                      \
    {                                                     \
      if (!token->left)                                   \
        HANDLE_ERROR("Expected that " #NAME               \
                     " has at least one argument");       \
                                                          \
      simplyToken(token->left, error);                    \
      if (*error) ERROR();                                \
                                                          \
      if (token->right)                                   \
        {                                                 \
          simplyToken(token->right, error);               \
          if (*error) ERROR();                            \
                                                          \
          if (!IS_NUM(token->left ) ||                    \
              !IS_NUM(token->right))                      \
            break;                                        \
                                                          \
          token->type = db::type_t::NUMBER;               \
          NUMBER(token) =                                 \
            NUMBER(token->left) OP NUMBER(token->right);  \
                                                          \
          db::Token left  = token->left ;                 \
          token->left  = nullptr;                         \
          db::Token right = token->right;                 \
          token->right = nullptr;                         \
                                                          \
          db::removeNode(left );                          \
          db::removeNode(right);                          \
        }                                                 \
      else if (IS_NUM(token->left))                       \
        {                                                 \
          token->type = db::type_t::NUMBER;               \
          NUMBER(token) = OP NUMBER(token->left);         \
                                                          \
          db::Token left = token->left;                   \
          token->left = nullptr;                          \
                                                          \
          db::removeNode(left);                           \
        }                                                 \
    } while (0)

static db::Token createNumber(db::number_t value)
{
  return db::createNode({.number = value}, db::type_t::NUMBER);
}

static db::Token diff(db::Token token);

void simplyToken(db::Token token, int *error);

bool isConst(const db::Token token);

void db::simplyGrammar(db::Translator *translator, int *error)
{
  if (!translator || !translator->grammar.root) ERROR();

  int errorCode = 0;
  simplyToken(translator->grammar.root, &errorCode);
  if (errorCode) ERROR();

}

bool isConst(const db::Token token)
{
  assert(token);

  if (token->left  && !isConst(token->left )) return false;
  if (token->right && !isConst(token->right)) return false;

  if (!token->left  &&
      !token->right &&
      IS_NUM(token)) return true;

  return true;
}

void simplyToken(db::Token token, int *error)
{
  if (!token || !error) ERROR();

  if (!IS_STATEMENT(token) &&
      !(IS_NAME(token) && token->left)) return;

  switch (STATEMENT(token))
    {

    case db::STATEMENT_ADD: SIMPLY_LINARY(ADD , +    ); break;
    case db::STATEMENT_SUB: SIMPLY_LINARY(SUB , -    ); break;
    case db::STATEMENT_MUL: SIMPLY_BINARY(MUL , *    ); break;
    case db::STATEMENT_DIV: SIMPLY_BINARY(DIV , /    ); break;
    case db::STATEMENT_SIN:  SIMPLY_UNARY(SIN , sin  ); break;
    case db::STATEMENT_COS:  SIMPLY_UNARY(COS , cos  ); break;
    case db::STATEMENT_TAN:  SIMPLY_UNARY(TAN , tan  ); break;
    case db::STATEMENT_INT:  SIMPLY_UNARY(INT , (int)); break;
    case db::STATEMENT_SQRT: SIMPLY_UNARY(SQRT, sqrt ); break;
    case db::STATEMENT_DIFF: diff(token); break;
    default:
      {
        if (token->left ) simplyToken(token->left , error);
        if (*error) ERROR();
        if (token->right) simplyToken(token->right, error);
        if (*error) ERROR();
        break;
      }
    }
}

static db::Token diff(db::Token token)
{
  assert(token);

  switch (token->type)
    {
    case db::type_t::NUMBER:
      {
        NUMBER(token) = 0;
        return token;
      }
    case db::type_t::NAME:
      {
        if (token->left)
          return token;
        NUMBER(token) = 1;
        return token;
      }
    case db::type_t::STATEMENT:
      switch (STATEMENT(token))
        {
        case db::STATEMENT_ADD:
        case db::STATEMENT_SUB:
          {
            if (token->left )
              diff(token->left );
            if (token->right)
              diff(token->right);
            return token;
          }
        case db::STATEMENT_MUL:
          {
            STATEMENT(token) = db::STATEMENT_ADD;

            token->left  = MUL(dLeft , Left );
            token->right = MUL(dRight, Right);

            return token;
          }
        case db::STATEMENT_DIV:
          {
            token->left  = SUB(
                               MUL(dLeft, Right),
                               MUL(Left, dRight));
            token->right = MUL(copyRight, copyRight);

            return token;
          }
        case db::STATEMENT_SIN:
          {
            STATEMENT(token) = db::STATEMENT_MUL;

            token->left  = COS(Left);
            token->right = dLeft;

            return token;
          }
        case db::STATEMENT_COS:
          {
            STATEMENT(token) = db::STATEMENT_MUL;

            token->left  = NUM(-1);
            token->right = MUL(SIN(Right), dRight);

            return token;
          }
        case db::STATEMENT_SQRT:
          //          return DIV(dRight, MUL(NUM(2), SQRT(CopyRight)));
        default: return nullptr;
        }
    default: return nullptr;
    }
}

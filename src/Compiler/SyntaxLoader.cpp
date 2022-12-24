#include "Translator.h"

#include "ErrorHandler.h"
#include "StringsUtils.h"
#include "Error.h"
#include "Assert.h"
#include "DSL.h"
#include <ctype.h>
#include <string.h>

#include "Logging.h"

#pragma GCC diagnostic ignored "-Wswitch-enum"

#define CHECK_CHAR(CHAR)                                       \
  do                                                           \
    {                                                          \
      if (!fscanf(source, " %c", &ch) || ch != CHAR)           \
        {                                                      \
          printf("%s\n", buffer);                              \
          handleError("Expected " #CHAR " %d", __LINE__);      \
          ERROR(nullptr);                                      \
        }                                                      \
    } while (0)

#define HANDLE_ERROR(MESSAGE, ...)                    \
  do                                                  \
    {                                                 \
      handleError(MESSAGE __VA_OPT__(,) __VA_ARGS__); \
                                                      \
      ERROR();                                        \
    } while (0)

#define CHECK_UNARY(TOKEN, NAME)                                \
  do                                                            \
    {                                                           \
      if (!token->left) HANDLE_ERROR("Invalid .std file: "      \
                                     #NAME " argument is NIL"); \
    } while (0)

#define CHECK_BINARY(TOKEN, NAME)                                       \
  do                                                                    \
    {                                                                   \
      if (!token->left ) HANDLE_ERROR("Invalid .std file: "             \
                                      #NAME " first argument is NIL");  \
      if (!token->right) HANDLE_ERROR("Invalid .std file: "             \
                                      #NAME " sedcond argument is NIL"); \
    } while (0)


const int MAX_SIZE    = 128;
const int MAX_ID_SIZE =   8;

struct Statement {
  const char *name;
  db::statement_t value;
  int size;
};

const Statement STATEMENTS[] =
  {
    {"ST"   , db::STATEMENT_COMPOUND  , 2},
    {"IF"   , db::STATEMENT_IF        , 2},
    {"ELSE" , db::STATEMENT_ELSE      , 4},
    {"VAR"  , db::STATEMENT_VAR       , 3},
    {"WHILE", db::STATEMENT_WHILE     , 5},
    {"FUNC" , db::STATEMENT_FUN       , 4},
    {"RET"  , db::STATEMENT_RETURN    , 3},
    {"CALL" , db::STATEMENT_CALL      , 4},
    {"PARAM", db::STATEMENT_PARAMETER , 5},
    {"EQ"   , db::STATEMENT_ASSIGNMENT, 2},
    {"VOID" , db::STATEMENT_VOID      , 4},
    {"TYPE" , db::STATEMENT_TYPE      , 4},
    {"ADD"  , db::STATEMENT_ADD       , 3},
    {"SUB"  , db::STATEMENT_SUB       , 3},
    {"MUL"  , db::STATEMENT_MUL       , 3},
    {"DIV"  , db::STATEMENT_DIV       , 3},
    {"POW"  , db::STATEMENT_POW       , 3},
    {"COS"  , db::STATEMENT_COS       , 3},
    {"SIN"  , db::STATEMENT_SIN       , 3},
    {"TAN"  , db::STATEMENT_TAN       , 3},
    {"OUT"  , db::STATEMENT_OUT       , 3},
    {"IN"   , db::STATEMENT_IN        , 2},

    {"ENDL"  , db::STATEMENT_NEW_LINE  , 4},
    {"SQRT"  , db::STATEMENT_SQRT      , 4},
    {"STATIC", db::STATEMENT_STATIC    , 6},
    {"TYPE"  , db::STATEMENT_TYPE      , 4},
    {"IS_EE" , db::STATEMENT_EQUAL     , 5},
    {"IS_NE" , db::STATEMENT_NOT_EQUAL , 5},
    {"IS_BT" , db::STATEMENT_LESS      , 5},
    {"IS_GT" , db::STATEMENT_GREATER   , 5},
    {"MOD"   , db::STATEMENT_INT       , 3},
    {"AND"   , db::STATEMENT_AND       , 3},
    {"OR"    , db::STATEMENT_OR        , 2},
    {"DIFF"  , db::STATEMENT_DIFF      , 4},
  };

const int STATEMENTS_SIZE = 33;

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

static void findGlobalVariables(db::Translator *translator, int *error);

static void findFunctions(db::Translator *translator, db::Token token, int *error);

static void checkFunction(db::Translator *translator, db::Token function, int *error);

static db::Token loadToken(FILE *source, db::StringPool *pool, bool *fail, int *error);

void db::loadTranslator(
                        Translator *translator,
                        FILE *source,
                        int *error
                       )
{
  if (!translator || !source) ERROR();

  bool hasntTokens = false;
  int codeError = 0;

  translator->grammar.root
    = loadToken(source, &translator->stringPool, &hasntTokens, &codeError);
  if (codeError) ERROR();

  db:: dumpTree(&translator->grammar, 0, getLogFile());
  if (!translator->grammar.root)          HANDLE_ERROR("File hasn`t tree");
  if (!IS_COMP(translator->grammar.root)) HANDLE_ERROR("Root of tree isn`t statement");

  db::addVarTable(translator, &codeError);
  if (codeError) ERROR();

  findGlobalVariables(translator, &codeError);
  if (codeError)
    {
      db::removeVarTable(translator, &codeError);
      ERROR();
    }

  findFunctions(translator, translator->grammar.root, &codeError);
  if (codeError)
    {
      db::removeVarTable(translator, &codeError);
      ERROR();
    }
}

static db::Token loadToken(FILE *source, db::StringPool *pool, bool *fail, int *error)
{
  assert(source);
  assert(pool);
  assert(fail);
  assert(error);

  db::Token token = nullptr;

  char ch = '\0';
  if (!fscanf(source, " %c", &ch)) FAIL(nullptr);

  bool isOptional = false;
  if (ch == '$')
    {
      isOptional = true;

      char id[MAX_ID_SIZE] = "";
      fscanf(source, "%2s", id);
      if (!strcmp(id, "db"))
        fscanf(source, "%*[^ \t\n]");
      else
        {
          int depth = 1;

          while (depth)
            {
              fscanf(source, "%*[^$]$");

              ch = (char)getc(source);

              if (!isspace(ch)) depth += 1;
              else              depth -= 1;
            }

          return nullptr;
        }

      if (!fscanf(source, " %c", &ch)) ERROR(nullptr);
    }

  if (ch != '{')
    {
      ungetc(ch, source);
      FAIL(nullptr);
    }

  char buffer[MAX_SIZE] = "";

  bool isString = false;

  fscanf(source, " %c", &ch);
  if (ch == '\"')
    {
      if (!fscanf(source, "%[^\"]\" ", buffer)) ERROR(nullptr);
    }
  else if (ch == '\'')
    {
      isString = true;
      if (!fscanf(source, "%[^']' ", buffer)) ERROR(nullptr);
    }
  else
    {
      ungetc(ch, source);
      if (!fscanf(source, " %s ", buffer)) ERROR(nullptr);
    }

  if (!stricmp("NIL", buffer))
    {
      if (!fscanf(source, " %c", &ch) || ch != '}')
        {
          handleError("Expected } ");
          ERROR(nullptr);
        }

      return nullptr;
    }

  for (int i = 0; i < STATEMENTS_SIZE; ++i)
    if (!stricmp(STATEMENTS[i].name, buffer))
      {
        token = ST(STATEMENTS[i].value);
        break;
      }

  if (!token)
    {
      double value = 0;
      int offset = 0;
      sscanf(buffer, "%lg%n", &value, &offset);

      char *string = (buffer[offset] ?
                      db::addString(
                                    pool,
                                    buffer,
                                    error
                                   ) :
                      nullptr);

      if (buffer[offset] &&  isString)
        token = STR(string);
      else if (buffer[offset] && !isString)
        token = NAM(string);
      else
        token = NUM(atof(buffer));
    }

  token->left  = loadToken(source, pool, fail, error);
  if (*error) { db::removeNode(token); ERROR(nullptr); }
  token->right = loadToken(source, pool, fail, error);
  if (*error) { db::removeNode(token); ERROR(nullptr); }

  CHECK_CHAR('}');

  if (isOptional)
    CHECK_CHAR('$');

  return token;
}

static void findGlobalVariables(db::Translator *translator, int *error)
{
  db::Token temp = translator->grammar.root;
  for ( ; temp; temp = temp->right)
    if (IS_VAR(temp->left))
        if (!db::addVariable(
                             NAME(temp->left->left),
                             false,
                             translator,
                             0
                            ))
          HANDLE_ERROR("Redeclared of global variable: '%s'",
                       NAME(temp->left->left)
                      );
}

static void findFunctions(db::Translator *translator, db::Token token, int *error)
{
  int errorCode = 0;

  if (IS_FUN(token))
    {
      if (!IS_NAME(token->left))
        HANDLE_ERROR("Invalid .std file: No function name");
      if (!IS_TYPE(token->left->right) &&
          !IS_VOID(token->left->right))
          HANDLE_ERROR("Invalid .std file: No return type");

        db::Token param = token->left->left;
        for ( ; param; param = param->right)
          if (!IS_VAR(param->left))
            HANDLE_ERROR("Invalid .std file: "
                         "Invalid function parameters");
          else if (!IS_NAME(param->left->left))
            HANDLE_ERROR("Invalid .std file: "
                         "Invalid name of function parameters");

        bool isType = IS_TYPE(token->left->right);
        translator->status.returnType = (isType ?
                                         db::ReturnType::Type :
                                         db::ReturnType::Void);

        if (!IS_COMP(token->right))
          HANDLE_ERROR("Invalid .std file: "
                       "Invalid body of function: %s", NAME(token->left));

        //checkFunction(translator, token->right, &errorCode);
        //if (errorCode) ERROR();

        if (!db::addFunction(
                             NAME(token->left),
                             token,
                             translator,
                             error
                             ))
          HANDLE_ERROR("Redeclared of function: '%s'",
                       NAME(token->left));
    }

  if (token->left )
    findFunctions(translator, token->left , &errorCode);
  if (errorCode) ERROR();
  if (token->right)
    findFunctions(translator, token->right, &errorCode);
  if (errorCode) ERROR();
}

static void checkFunction(db::Translator *translator, db::Token token, int *error)
{
  assert(translator);
  assert(token);
  assert(error);

  switch (token->type)
    {
    case db::type_t::NUMBER:
    case db::type_t::NAME:
    case db::type_t::STRING:
        break;
    case db::type_t::STATEMENT:
      {
        switch (STATEMENT(token))
          {
          case db::STATEMENT_COMPOUND:
            {
              if (!token->left) HANDLE_ERROR("Invalid .std file: "
                                             "Left branch of statement is NIL");
              checkFunction(translator, token->left, error);

              if (token->right)
                checkFunction(translator, token->right, error);
              break;
            }
          case db::STATEMENT_WHILE:
            {
              if (!token->left ) HANDLE_ERROR("Invalid .std file: "
                                              "Condition of while is NIL");
              if (!token->right) HANDLE_ERROR("Invalid .std file: "
                                              "Body of while is NIL");


              checkFunction(translator, token->left , error);
              checkFunction(translator, token->right, error);
              break;
            }
          case db::STATEMENT_IF:
            {
              if (!token->left ) HANDLE_ERROR("Invalid .std file: "
                                              "Condition of if is NIL");
              if (!token->right) HANDLE_ERROR("Invalid .std file: "
                                              "Body of if is NIL");

              checkFunction(translator, token->left , error);
              checkFunction(translator, token->right, error);
              break;
            }
          case db::STATEMENT_ELSE:
            {
              if (!token->left ) HANDLE_ERROR("Invalid .std file: "
                                              "True-body of else is NIL");
              if (!token->right) HANDLE_ERROR("Invalid .std file: "
                                              "False-body of else is NIL");

              checkFunction(translator, token->left , error);
              checkFunction(translator, token->right, error);
              break;
            }
          case db::STATEMENT_VAL: case db::STATEMENT_VAR:
            {
              ////add vartable in while comp else if

              break;
            }
          case db::STATEMENT_INT:
            CHECK_UNARY(token, Int);        break;
          case db::STATEMENT_AND:
            CHECK_BINARY(token, And);       break;
          case db::STATEMENT_OR:
            CHECK_BINARY(token, Or);        break;
          case db::STATEMENT_NOT_EQUAL:
            CHECK_BINARY(token, Not equal); break;
          case db::STATEMENT_EQUAL:
            CHECK_BINARY(token, Equal);     break;
          case db::STATEMENT_SQRT:
            CHECK_UNARY(token, Sqrt);       break;
          case db::STATEMENT_POW:
            CHECK_UNARY(token, Pow);        break;
          case db::STATEMENT_LESS:
            CHECK_BINARY(token, Less);      break;
          case db::STATEMENT_GREATER:
            CHECK_BINARY(token, Greater);   break;
          case db::STATEMENT_ADD:///
          case db::STATEMENT_SUB:///
            break;
          case db::STATEMENT_MUL:
            CHECK_BINARY(token, Mul);       break;
          case db::STATEMENT_DIV:
            CHECK_BINARY(token, Div);       break;
          case db::STATEMENT_SIN:
            CHECK_UNARY(token, Sin);        break;
          case db::STATEMENT_COS:
            CHECK_UNARY(token, Cos);        break;
          case db::STATEMENT_ASSIGNMENT:
            CHECK_BINARY(token, Assign);    break;

          case db::STATEMENT_OUT:
            {
              if (!IS_PARAM(token->left)) HANDLE_ERROR("Invalid .std file: "
                                                       "Out arguments is NIL");

              checkFunction(translator, token->left, error);

              break;
            }
          case db::STATEMENT_IN:
            {
              if (!IS_PARAM(token->left)) HANDLE_ERROR("Invalid .std file: "
                                                       "In arguments is NIL");

              checkFunction(translator, token->left, error);

              break;
            }
          case db::STATEMENT_CALL:
            {

              if (!IS_NAME(token->left)) HANDLE_ERROR("Invalid .std file: "
                                                      "Call nothing");

              if (token->left->left &&
                  !IS_PARAM(token->left->left)) HANDLE_ERROR("Invalid .std file: "
                                                             "Incorrect call arguments");

              checkFunction(translator, token->left->left, error);

              break;
            }


          case db::STATEMENT_PARAMETER:
            {
              if (!token->left) HANDLE_ERROR("Invalid .std file: "
                                             "Left branck of argument is NIL");

              checkFunction(translator, token->left, error);

              if (token->right)
                checkFunction(translator, token->right, error);

              break;
            }

          case db::STATEMENT_RETURN:
            {
              if (token->left) checkFunction(translator, token->left , error);

              break;
            }
            ////
          case db::STATEMENT_GREATER_OR_EQUAL:
          case db::STATEMENT_LESS_OR_EQUAL:
            ////
          case db::STATEMENT_STATIC:
          case db::STATEMENT_FUN:
            ////
          case db::STATEMENT_NEW_LINE:
          case db::STATEMENT_TYPE:
          case db::STATEMENT_VOID:
            ////
          case db::STATEMENT_END_SQUARE_BRACE:
          case db::STATEMENT_START_SQUARE_BRACE:
          case db::STATEMENT_ARROW:
          case db::STATEMENT_UNION:
          case db::STATEMENT_OUTPUT:
          case db::STATEMENT_INPUT:
          case db::STATEMENT_NOT:
          case db::STATEMENT_COMMA:
          case db::STATEMENT_START_BRACE:
          case db::STATEMENT_END_BRACE:
          case db::STATEMENT_COLON:
          case db::STATEMENT_OPEN:
          case db::STATEMENT_CLOSE:
          case db::STATEMENT_END:
          case db::STATEMENT_ERROR:
          case db::STATEMENT_SEMICOLON:
          default: break;
          }

        break;
      }
    default:
      HANDLE_ERROR("Unknown type of token");
    }
}

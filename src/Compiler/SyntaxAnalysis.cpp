#include "SyntaxAnalysis.h"
#include "Translator.h"

#include "ErrorHandler.h"
#include "Error.h"
#include "DSL.h"


#define REMOVE_ONE(TOKEN)                       \
  do                                            \
    {                                           \
      if (TOKEN)                                \
        db::removeNode(TOKEN);                  \
    } while (0)

#define REMOVE_TWO(TOKEN_1, TOKEN_2)            \
  do                                            \
    {                                           \
      REMOVE_ONE(TOKEN_1);                      \
      REMOVE_ONE(TOKEN_2);                      \
    } while (0)

#define REMOVE_THREE(TOKEN_1, TOKEN_2, TOKEN_3)          \
  do                                                     \
    {                                                    \
      REMOVE_TWO(TOKEN_1, TOKEN_2);                      \
      REMOVE_ONE(TOKEN_3);                               \
    } while (0)

#define HANDLE_ERROR(MESSAGE, FIRST, SECOND, THIRD)               \
  do                                                              \
    {                                                             \
      handleError(MESSAGE " Line: %d at Position: %d!",           \
                  TOKEN(translator)->position.line,               \
                  TOKEN(translator)->position.position);          \
                                                                  \
      REMOVE_THREE(FIRST, SECOND, THIRD);                         \
      ERROR(nullptr);                                             \
    } while (0)

#define TOKEN(TRANSLATOR) (*TRANSLATOR->tokens)
#define INCREASE_TOKENS(TRANSLATOR) (++TRANSLATOR->tokens)
#define FAIL(...)                               \
  do                                            \
    {                                           \
      if (fail) *fail = true;                   \
                                                \
      return __VA_ARGS__;                       \
    } while (0)


#define CMD(LEFT, RIGHT)                                \
  db::createNode(                                       \
                 db::treeValue_t {.statement =          \
                   db::STATEMENT_COMPOUND},             \
                 db::type_t::STATEMENT, LEFT, RIGHT)

#define PARAM(LEFT, RIGHT)                          \
  db::createNode(                                   \
                 db::treeValue_t {.statement =      \
                   db::STATEMENT_PARAMETER},        \
                 db::type_t::STATEMENT, LEFT, RIGHT)

#define VARIABLE(LEFT, RIGHT)                       \
  db::createNode(                                   \
                 db::treeValue_t {.statement =      \
                   db::STATEMENT_VAR},              \
                 db::type_t::STATEMENT, LEFT, RIGHT)

#define CHECK_OPEN(TRANSLATOR, FIRST_TOKEN)                           \
  do                                                                  \
    {                                                                 \
      if (!IS_OPEN(TOKEN(TRANSLATOR)))                                \
        {                                                             \
          handleError("Expected ( before Line: %d at Position: %d!",  \
                      TOKEN(TRANSLATOR)->position.line,               \
                      TOKEN(TRANSLATOR)->position.position);          \
                                                                      \
          db::removeNode(FIRST_TOKEN);                                \
          FAIL(nullptr);                                              \
        }                                                             \
      db::removeNode(TOKEN(TRANSLATOR));                              \
      INCREASE_TOKENS(TRANSLATOR);                                    \
    } while (0)

#define CHECK_CLOSE(TRANSLATOR, FIRST_TOKEN, SECOND_TOKEN)            \
  do                                                                  \
    {                                                                 \
     if (!IS_CLOSE(TOKEN(TRANSLATOR)))                                \
        {                                                             \
          handleError("Expected ) before Line: %d at Position: %d!",  \
                      TOKEN(TRANSLATOR)->position.line,               \
                      TOKEN(TRANSLATOR)->position.position);          \
                                                                      \
          db::removeNode( FIRST_TOKEN);                               \
          db::removeNode(SECOND_TOKEN);                               \
          ERROR(nullptr);                                             \
        }                                                             \
     db::removeNode(TOKEN(TRANSLATOR));                               \
     INCREASE_TOKENS(TRANSLATOR);                                     \
    } while (0)

#define CHECK_ARGS(...)                                           \
  do                                                              \
    {                                                             \
      if (!translator || !translator->tokens || !fail || !error)  \
        FAIL(__VA_ARGS__);                                        \
    } while (0)


#define CHECK(IS_IT, SEQUENCE, FOR_FREE)                                \
  do                                                                    \
    {                                                                   \
      if (!IS_IT(TOKEN(translator)))                                    \
        {                                                               \
          handleError("Expected " SEQUENCE " at Line: %d at Position: %d!", \
                      TOKEN(translator)->position.line,                 \
                      TOKEN(translator)->position.position);            \
                                                                        \
          db::removeNode(token);                                        \
          if (FOR_FREE) db::removeNode(FOR_FREE);                       \
          FAIL(nullptr);                                                \
        }                                                               \
      db::removeNode(TOKEN(translator));                                \
      INCREASE_TOKENS(translator);                                      \
    } while (0)

typedef db::Token FunType(
                          db::Translator *translator,
                          bool *fail,
                          int *error
                         );

static FunType getGlobal                  ;

static FunType getInstruction             ;
static FunType getCompoundInstruction     ;
static FunType getInstructionExpression   ;
static FunType getInstructionChoice       ;
static FunType getInstructionLoop         ;
static FunType getInstructionJump         ;
static FunType getInstructionVoid         ;

static FunType getInput                   ;
static FunType getOutput                  ;

static FunType getDeclaration             ;
static FunType getFunctionDeclaration     ;
static FunType getVariableDeclaration     ;

static FunType getParameterList           ;
static FunType getArgumentList            ;
static FunType getFunctionBody            ;

static FunType getExpression              ;

static FunType getLogicAndExpression      ;
static FunType getLogicOrExpression       ;
static FunType getEqualExpression         ;
static FunType getRelationExpression      ;

static FunType getMultiplicativeExpression;
static FunType getAdditiveExpression      ;
static FunType getFunctionExpression      ;
static FunType gePostfixtExpression       ;
static FunType getPrimaryExpression       ;
static FunType getNumber                  ;
static FunType getVariable                ;
static FunType getValue                   ;

static db::Token createStatement(db::statement_t value)
{
  return db::createNode({.statement = value}, db::type_t::STATEMENT);
}

void db::getGrammarly(db::Translator *translator, int *error)
{
  if (!translator || !translator->tokens) ERROR();

  db::Token *startToken = translator->tokens;

  bool fail = false;
  int errorCode = 0;
  db::addVarTable(translator, &errorCode);
  if (errorCode) ERROR();

  translator->grammar.root = getGlobal(translator, &fail, &errorCode);

  if (!IS_END(TOKEN(translator)) && !fail && !errorCode)
      handleError("Not found terminator at Line: %d at Position: %d!",
                  TOKEN(translator)->position.line,
                  TOKEN(translator)->position.position);

  bool hasMain = searchFunction("main", translator);
  if (!hasMain)
    handleError("Not found main function!");

  if (!IS_END(TOKEN(translator)) || fail || errorCode || !hasMain)
    {
      db::destroyTree(&translator->grammar);

      int i = 0;
      for ( ; !IS_END(translator->tokens[i]); ++i)
        db::removeNode(translator->tokens[i]);
      db::removeNode(translator->tokens[i]);

      translator->tokens = startToken;
      ERROR();
    }

  db::removeNode(TOKEN(translator));
  translator->tokens = startToken;
}

static db::Token getGlobal(db::Translator *translator, bool *fail, int *error)
{
  CHECK_ARGS(nullptr);

  db::Token token = getDeclaration(translator, fail, error);
  if (*fail || *error)
    {
      handleError("Expected at least one instruction");
      FAIL(nullptr);
    }
  token = CMD(token, nullptr);
  db::Token *freePosition = &token->right;

  while (!IS_END(TOKEN(translator)))
    {
      db::Token tempToken = getDeclaration(translator, fail, error);
      if (*fail || *error)
        HANDLE_ERROR("Expected an instruction", token, nullptr, nullptr);

      *freePosition = CMD(tempToken, nullptr);
      freePosition = &(*freePosition)->right;
    }

  return token;
}

static db::Token getInstruction(db::Translator *translator, bool *fail, int *error)
{
  CHECK_ARGS(nullptr);

  bool hasntInstruction = false;
  db::Token token = getCompoundInstruction(translator, &hasntInstruction, error);
  if (*error) ERROR(nullptr);
  if (hasntInstruction)
    {
      hasntInstruction = false;
      token = getInstructionChoice(translator, &hasntInstruction, error);
      if (*error) ERROR(nullptr);
    }
  if (hasntInstruction)
    {
      hasntInstruction = false;
      token = getInstructionLoop(translator, &hasntInstruction, error);
      if (*error) ERROR(nullptr);
    }
  if (hasntInstruction)
    {
      hasntInstruction = false;
      token = getInstructionVoid(translator, &hasntInstruction, error);
      if (*error) ERROR(nullptr);
    }
  if (hasntInstruction)
    {
      hasntInstruction = false;
      token = getInstructionExpression(translator, &hasntInstruction, error);
      if (*error) ERROR(nullptr);
    }
  if (hasntInstruction)
    {
      hasntInstruction = false;
      token = getInstructionJump(translator, &hasntInstruction, error);
      if (*error) ERROR(nullptr);
    }
  if (hasntInstruction)
    {
      hasntInstruction = false;
      token = getInput(translator, &hasntInstruction, error);
      if (*error) ERROR(nullptr);
    }
  if (hasntInstruction)
    {
      hasntInstruction = false;
      token = getOutput(translator, &hasntInstruction, error);
      if (*error) ERROR(nullptr);
    }
  if (hasntInstruction)
    {
      hasntInstruction = false;
      token = getVariableDeclaration(translator, fail, error);
      if (*error) ERROR(nullptr);
    }

  return token;
}

static db::Token getCompoundInstruction(db::Translator *translator, bool *fail, int *error)
{
  CHECK_ARGS(nullptr);

  db::Token token = nullptr;

  if (IS_START_BRACE(TOKEN(translator)))
    {
      db::removeNode(TOKEN(translator));
      INCREASE_TOKENS(translator);

      db::addVarTable(translator, error);
      if (*error) ERROR(nullptr);

      db::Token *freePosition = &token;

      bool hasntCommand = false;
      while (!hasntCommand)
        {
          db::Token instruction = getInstruction(translator, &hasntCommand, error);
          if (*error)
            {
              if (token) db::removeNode(token);
              db::removeVarTable(translator, error);
              ERROR(nullptr);
            }
          if (!instruction) break;

          *freePosition = CMD(instruction, nullptr);
          freePosition = &(*freePosition)->right;
        }

      if (!token)
        {
          handleError("Expected at least one instruction at Line: %d at Position: %d!",
                      TOKEN(translator)->position.line,
                      TOKEN(translator)->position.position);

          db::removeVarTable(translator, error);
          ERROR(nullptr);
        }

      if (!IS_END_BRACE(TOKEN(translator)))
        {
          handleError("Expected an } at Line: %d at Position: %d!",
                      TOKEN(translator)->position.line,
                      TOKEN(translator)->position.position);

          db::removeVarTable(translator, error);
          db::removeNode(token);
          ERROR(nullptr);
        }
      db::removeVarTable(translator, error);
      db::removeNode(TOKEN(translator));
      INCREASE_TOKENS(translator);
    }
  else
    FAIL(nullptr);

  return token;
}

static db::Token getInstructionExpression(db::Translator *translator, bool *fail, int *error)
{
  CHECK_ARGS(nullptr);

  db::Token token = getExpression(translator, fail, error);
  if (*error) ERROR(nullptr);
  if (*fail )  FAIL(nullptr);

  if (!IS_SEM(TOKEN(translator)))
    HANDLE_ERROR("Expected ;", token, nullptr, nullptr);
  db::removeNode(TOKEN(translator));
  INCREASE_TOKENS(translator);

  return token;
}

static db::Token getInstructionLoop(db::Translator *translator, bool *fail, int *error)
{
  CHECK_ARGS(nullptr);

  db::Token whileToken = TOKEN(translator);

  if (IS_WHILE(whileToken))
    {
      INCREASE_TOKENS(translator);

      CHECK_OPEN(translator, whileToken);

      db::Token expression = getExpression(translator, fail, error);
      if (*fail || *error)
        {
          db::removeNode(whileToken);
          ERROR(nullptr);
        }

      CHECK_CLOSE(translator, whileToken, expression);

      db::Token instruction = getInstruction(translator, fail, error);
      if (*fail || *error)
        {
          db::removeNode(whileToken);
          db::removeNode(expression);
          ERROR(nullptr);
        }

      db::setChildren(whileToken, expression, instruction);
    }
  else
    FAIL(nullptr);

  return whileToken;
}

static db::Token getInstructionChoice(db::Translator *translator, bool *fail, int *error)
{
  CHECK_ARGS(nullptr);

  db::Token ifToken = TOKEN(translator);

  if (IS_IF(ifToken))
    {
      INCREASE_TOKENS(translator);
      CHECK_OPEN(translator, ifToken);

      db::addVarTable(translator, error);
      if (*error) ERROR(nullptr);

      db::Token expression = getExpression(translator, fail, error);
      if (*fail || *error)
        {
          db::removeVarTable(translator, error);
          db::removeNode(ifToken);
          ERROR(nullptr);
        }

      CHECK_CLOSE(translator, ifToken, expression);

      db::Token instruction = getInstruction(translator, fail, error);
      if (*fail || *error)
        {
          db::removeVarTable(translator, error);
          db::removeNode(ifToken);
          db::removeNode(expression);
          ERROR(nullptr);
        }

      db::setChildren(ifToken, expression, instruction);
      db::removeVarTable(translator, error);

      if (IS_ELSE(TOKEN(translator)))
        {
          db::addVarTable(translator, error);
          if (*error)
            {
              db::removeNode(ifToken);
              ERROR(nullptr);
            }

          db::Token elseToken = TOKEN(translator);
          INCREASE_TOKENS(translator);

          db::Token elseInstruction = getInstruction(translator, fail, error);
          if (*error || *fail)
            {
              db::removeVarTable(translator, error);
              db::removeNode(ifToken);
              ERROR(nullptr);
            }

          db::setChildren(elseToken, ifToken->right, elseInstruction);
          ifToken->right = elseToken;
          db::removeVarTable(translator, error);
        }
    }
  else
    FAIL(nullptr);

  return ifToken;
}

static db::Token getInstructionJump(db::Translator *translator, bool *fail, int *error)
{
  CHECK_ARGS(nullptr);

  db::Token token = TOKEN(translator);

  if (IS_RETURN(token))
    {
      INCREASE_TOKENS(translator);

      db::Token expression = getExpression(translator, fail, error);
      if (*fail || *error)
        {
          db::removeNode(token);
          ERROR(nullptr);
        }

      if (!IS_SEM(TOKEN(translator)))
        HANDLE_ERROR("Expected ;", token, nullptr, nullptr);
      db::removeNode(TOKEN(translator));
      INCREASE_TOKENS(translator);

      token->right = expression;
    }
  else
    FAIL(nullptr);

  return token;
}

static db::Token getInstructionVoid(db::Translator *translator, bool *fail, int *error)
{
  CHECK_ARGS(nullptr);

  db::Token token = TOKEN(translator);

  if (!IS_NAME(token) || !IS_OPEN(translator->tokens[1]))
    FAIL(nullptr);

  int errorCode = 0;
  db::Token funToken =
    db::searchFunction(
                       NAME(TOKEN(translator)),
                       translator,
                       &errorCode
                      );

  if (!funToken)
    {
      handleError("Unknown function found: '%s' at Line: %d at Position: %d!",
                  NAME(TOKEN(translator)),
                  TOKEN(translator)->position.line,
                  TOKEN(translator)->position.position);
      db::removeNode(token);
      FAIL(nullptr);
    }
  if (!IS_VOID(funToken->left->right))
      FAIL(nullptr);

  INCREASE_TOKENS(translator);

  db::Token callNode = ST(db::STATEMENT_CALL);
  callNode->left = token;

  CHECK(IS_OPEN , "(", nullptr);

  bool hasntArguments = false;
  db::Token arguments = getArgumentList(translator, &hasntArguments, error);
  if (*error)
    {
      db::removeNode(callNode);
      ERROR(nullptr);
    }
  token->left = arguments;

  int paramCount = 0;
  for (db::Token temp = funToken->left->left; temp; temp = temp->right)
    ++paramCount;

  int argCount = 0;
  for (db::Token temp = arguments; temp; temp = temp->right)
    ++argCount;

  bool correctArguments = (paramCount == argCount);

  if (!correctArguments)
    {
      handleError("Incorrect call of function at Line: %d at Position: %d!",
                  TOKEN(translator)->position.line,
                  TOKEN(translator)->position.position);
      db::removeNode(callNode);
      ERROR(nullptr);
    }

  CHECK(IS_CLOSE, ")", callNode);
  CHECK(IS_SEM  , ";", callNode);

  return callNode;
}

static db::Token getDeclaration(db::Translator *translator, bool *fail, int *error)
{
  CHECK_ARGS(nullptr);

  bool hasntDeclaration = false;
  db::Token declaration = getFunctionDeclaration(translator, &hasntDeclaration, error);
  if (*error) ERROR(nullptr);
  if (hasntDeclaration)
    declaration = getVariableDeclaration(translator, fail, error);

  return declaration;
}

static db::Token getFunctionDeclaration(db::Translator *translator, bool *fail, int *error)
{
  CHECK_ARGS(nullptr);

  db::Token token = TOKEN(translator);

  if (!IS_FUN(token)) FAIL(nullptr);

  INCREASE_TOKENS(translator);
  db::Token name = TOKEN(translator);

  if (!IS_NAME(name))
    {
      handleError("Expected name at Line: %d at Position: %d!",
                  TOKEN(translator)->position.line,
                  TOKEN(translator)->position.position);

      db::removeNode(token);
      FAIL(nullptr);
    }
  INCREASE_TOKENS(translator);

  CHECK(IS_OPEN       , "("          , name);

  bool hasParam = false;
  db::Token parameters = getParameterList(translator, &hasParam, error);
  if (*error) ERROR(nullptr);
  name->left = parameters;

  CHECK(IS_CLOSE      , ")"          , name);

  db::Token returnType = nullptr;

  if (IS_COL(TOKEN(translator)))
    {
      db::removeNode(TOKEN(translator));
      INCREASE_TOKENS(translator);

      returnType = TOKEN(translator);

      if (!IS_TYPE(returnType) && !IS_VOID(returnType))
        {
          handleError("Expected return type at Line: %d at Position: %d!",
                      TOKEN(translator)->position.line,
                      TOKEN(translator)->position.position);

          db::removeNode(token);
          db::removeNode(name);
          ERROR(nullptr);
        }
      INCREASE_TOKENS(translator);

    }
  else
    returnType = ST(db::STATEMENT_VOID);

  addVarTable(translator, error);
  for (db::Token temp = parameters; temp; temp = temp->right)
    addVariable(NAME(temp->left->left), false, translator, 0, "", error);
  if (*error)
    {
      db::removeNode(token);
      db::removeNode(name);
      ERROR(nullptr);
    }

  token->left  = name;
  token->left->right = returnType;

  if (!addFunction(NAME(name), token, translator))
    {
      handleError("Redeclared of function at Line: %d at Position: %d!",
                  TOKEN(translator)->position.line,
                  TOKEN(translator)->position.position);

      db::removeNode(token);
      ERROR(nullptr);
    }

  db::Token body = getFunctionBody(translator, fail, error);

  if (*fail || *error)
    {
      handleError("Expected function body at Line: %d at Position: %d!",
                  TOKEN(translator)->position.line,
                  TOKEN(translator)->position.position);

      db::removeNode(token);
      db::removeVarTable(translator, error);
      ERROR(nullptr);
    }

  token->right = body;

  db::removeVarTable(translator, error);
  if (*error)
    {
      db::removeNode(token);
      ERROR(nullptr);
    }

  return token;
}

static db::Token getVariableDeclaration(db::Translator *translator, bool *fail, int *error)
{
  CHECK_ARGS(nullptr);

  db::Token token = TOKEN(translator);
  if (!IS_VAR(token) && !IS_VAL(token)) FAIL(nullptr);
  bool isConst = IS_VAL(token);

  INCREASE_TOKENS(translator);
  db::Token name = TOKEN(translator);

  if (!IS_NAME(name))
    {
      handleError("Expected name at Line: %d at Position: %d!",
                  TOKEN(translator)->position.line,
                  TOKEN(translator)->position.position);

      db::removeNode(token);
      ERROR(nullptr);
    }
  INCREASE_TOKENS(translator);

  if (IS_COL(TOKEN(translator)))
    {
      db::removeNode(TOKEN(translator));
      INCREASE_TOKENS(translator);
      CHECK(IS_TYPE, "value type", name);
    }

  CHECK(IS_ASSIGN, "=", name);

  db::Token value = getExpression(translator, fail, error);

  if (!value)
    {
      handleError("Expected value at Line: %d at Position: %d!",
                  TOKEN(translator)->position.line,
                  TOKEN(translator)->position.position);

      db::removeNode(token);
      db::removeNode(name);
      ERROR(nullptr);
    }

  if (!addVariable(NAME(name), isConst, translator, 0))
    {
      handleError("Redeclared of variable at Line: %d at Position: %d!",
                  TOKEN(translator)->position.line,
                  TOKEN(translator)->position.position);

      db::removeNode(name);
      db::removeNode(value);
      db::removeNode(token);
      ERROR(nullptr);
    }

  db::setChildren(token, name, value);

  if (!IS_SEM(TOKEN(translator)))
    {
      handleError("Expected ; at Line: %d at Position: %d!",
                  TOKEN(translator)->position.line,
                  TOKEN(translator)->position.position);

      db::removeNode(token);
      ERROR(nullptr);
    }
  db::removeNode(TOKEN(translator));
  INCREASE_TOKENS(translator);

  return token;
}

static db::Token getArgumentList(db::Translator *translator, bool *fail, int *error)
{
  CHECK_ARGS(nullptr);

  db::Token token = nullptr;
  db::Token *freePosition = &token;

  db::Token expression = getExpression(translator, fail, error);
  if (*error) ERROR(nullptr);
  while (expression)
    {
      *freePosition = PARAM(expression, nullptr);
      freePosition = &(*freePosition)->right;

      if (IS_COMMA(TOKEN(translator)))
        {
          db::removeNode(TOKEN(translator));
          INCREASE_TOKENS(translator);
        }
      bool hasntExpression = false;
      expression = getExpression(translator, &hasntExpression, error);
    }

  if (!token) FAIL(nullptr);

  return token;
}

static db::Token getParameterList(db::Translator *translator, bool *fail, int *error)
{
  CHECK_ARGS(nullptr);

  db::Token token = nullptr;
  db::Token *freePosition = &token;

  while (IS_NAME(TOKEN(translator)))
    {
      db::Token parameter = TOKEN(translator);
      INCREASE_TOKENS(translator);

      if (!IS_COL(TOKEN(translator)))
        {
          handleError("Expected : at Line: %d at Position: %d!",
                      TOKEN(translator)->position.line,
                      TOKEN(translator)->position.position);

          db::removeNode(parameter);
          ERROR(nullptr);
        }
      db::removeNode(TOKEN(translator));
      INCREASE_TOKENS(translator);

      if (!IS_TYPE(TOKEN(translator)))
        {
          handleError("Expected argument type at Line: %d at Position: %d!",
                      TOKEN(translator)->position.line,
                      TOKEN(translator)->position.position);

          db::removeNode(parameter);
          ERROR(nullptr);
        }
      db::removeNode(TOKEN(translator));
      INCREASE_TOKENS(translator);

      *freePosition = PARAM(VARIABLE(parameter, nullptr), nullptr);
      freePosition = &(*freePosition)->right;

      if (IS_COMMA(TOKEN(translator)))
        {
          db::removeNode(TOKEN(translator));
          INCREASE_TOKENS(translator);
        }
    }

  if (!token) FAIL(nullptr);

  return token;
}

static db::Token getFunctionBody(db::Translator *translator, bool *fail, int *error)
{
  CHECK_ARGS(nullptr);

  db::Token token = nullptr;

  if (IS_START_BRACE(TOKEN(translator)))
    {
      db::removeNode(TOKEN(translator));
      INCREASE_TOKENS(translator);

      db::Token *freePosition = &token;

      bool hasntCommand = false;
      while (!hasntCommand)
        {
          db::Token instruction = getInstruction(translator, &hasntCommand, error);
          if (*error)
            {
              if (token) db::removeNode(token);
              ERROR(nullptr);
            }
          if (!instruction) break;

          *freePosition = CMD(instruction, nullptr);
          freePosition = &(*freePosition)->right;
        }

      if (!token)
        {
          handleError("Expected at least one instruction at Line: %d at Position: %d!",
                      TOKEN(translator)->position.line,
                      TOKEN(translator)->position.position);

          ERROR(nullptr);
        }

      if (!IS_END_BRACE(TOKEN(translator)))
        {
          handleError("Expected an } at Line: %d at Position: %d!",
                      TOKEN(translator)->position.line,
                      TOKEN(translator)->position.position);

          db::removeNode(token);
          ERROR(nullptr);
        }
      db::removeNode(TOKEN(translator));
      INCREASE_TOKENS(translator);
    }
  else
    FAIL(nullptr);

  return token;
}

static db::Token getInput(db::Translator *translator, bool *fail, int *error)
{
  CHECK_ARGS(nullptr);

  db::Token token = TOKEN(translator);
  db::Token *freePosition = &token->left;

  if (!IS_IN(token)) FAIL(nullptr);

  INCREASE_TOKENS(translator);

  CHECK(IS_GREATER, ">>", token);
  CHECK(IS_GREATER, ">>", token);

  db::Token variable = nullptr;

  while (IS_NAME(TOKEN(translator)))
    {
      variable = getVariable(translator, fail, error);

      if (*error) { db::removeNode(token); ERROR(nullptr); }
      *freePosition = PARAM(variable, nullptr);
      freePosition = &(*freePosition)->right;

      if (IS_GREATER(TOKEN(translator)))
        {
          CHECK(IS_GREATER, ">>", token);
          CHECK(IS_GREATER, ">>", token);
          variable = nullptr;
        }
    }

  if (!variable)
    {
      handleError("Expected a variable at Line: %d at Position: %d!",
                  TOKEN(translator)->position.line,
                  TOKEN(translator)->position.position);

      db::removeNode(token);
      ERROR(nullptr);
    }

  CHECK(IS_SEM, ";", token);

  return token;
}

static db::Token getOutput(db::Translator *translator, bool *fail, int *error)
{
  CHECK_ARGS(nullptr);

  db::Token token = TOKEN(translator);
  db::Token *freePosition = &token->left;

  if (!IS_OUT(token)) FAIL(nullptr);

  INCREASE_TOKENS(translator);

  db::Token expression = nullptr;

  do
    {
      if (IS_LESS(TOKEN(translator)))
        {
          CHECK(IS_LESS, "<<", token);
          CHECK(IS_LESS, "<<", token);
        }
      else
        break;

      expression = getExpression(translator, fail, error);

      if (*error) { db::removeNode(token); ERROR(nullptr); }

      if (*fail &&
          !IS_STRING(TOKEN(translator)) &&
          !IS_ENDL(TOKEN(translator)))
        {
          handleError("Expected an expression or string at Line: %d at Position: %d!",
                      TOKEN(translator)->position.line,
                      TOKEN(translator)->position.position);

          db::removeNode(token);
          ERROR(nullptr);
        }

      if (!expression)
        {
          expression = TOKEN(translator);
          INCREASE_TOKENS(translator);
          *fail = false;
        }

      *freePosition = PARAM(expression, nullptr);
      freePosition = &(*freePosition)->right;

    } while (expression);

  if (!token->left) { db::removeNode(token); ERROR(nullptr); }

  CHECK(IS_SEM, ";", token);

  return token;
}

static db::Token getExpression(db::Translator *translator, bool *fail, int *error)
{
  CHECK_ARGS(nullptr);

  db::Token token = getLogicOrExpression(translator, fail, error);
  if (*error) ERROR(nullptr);
  if (*fail )  FAIL(nullptr);

  if (IS_ASSIGN(TOKEN(translator)))
    {
      if (!IS_NAME(token))
        {
          handleError("Expected variable before Line: %d at Position: %d!",
                      TOKEN(translator)->position.line,
                      TOKEN(translator)->position.position);

          db::removeNode(token);
          ERROR(nullptr);
        }
      db::Variable *var = searchVariable(NAME(token), translator);

      if (var->isConst)
        {
          handleError("Value cannot be change at Line: %d at Position: %d!",
                      TOKEN(translator)->position.line,
                      TOKEN(translator)->position.position);

          db::removeNode(token);
          ERROR(nullptr);
        }

      db::Token opToken = TOKEN(translator);
      INCREASE_TOKENS(translator);

      db::Token tempToken = getLogicOrExpression(translator, fail, error);

      token = db::setChildren(opToken, token, tempToken);
    }

  if (token && (*fail || *error))
    db::removeNode(token);

  return token;
}

static db::Token getLogicOrExpression(db::Translator *translator, bool *fail, int *error)
{
  CHECK_ARGS(nullptr);

  db::Token token = getLogicAndExpression(translator, fail, error);
  if (*error) ERROR(nullptr);

  while (IS_OR(TOKEN(translator)) && IS_OR(*(translator->tokens+1)))
    {
      db::Token opToken = TOKEN(translator);
      INCREASE_TOKENS(translator);
      db::removeNode(TOKEN(translator));
      INCREASE_TOKENS(translator);

      db::Token tempToken = getLogicAndExpression(translator, fail, error);

      if (*fail || *error)
        {
          db::removeNode(token);
          db::removeNode(opToken);
          ERROR(nullptr);
        }

      token = db::setChildren(opToken, token, tempToken);
    }

  return token;
}

static db::Token getLogicAndExpression(db::Translator *translator, bool *fail, int *error)
{
  CHECK_ARGS(nullptr);

  db::Token token = getEqualExpression(translator, fail, error);
  if (*error) ERROR(nullptr);

  while (IS_AND(TOKEN(translator)) && IS_AND(*(translator->tokens+1)))
    {
      db::Token opToken = TOKEN(translator);
      INCREASE_TOKENS(translator);
      db::removeNode(TOKEN(translator));
      INCREASE_TOKENS(translator);

      db::Token tempToken = getEqualExpression(translator, fail, error);

      if (*fail || *error)
        {
          db::removeNode(token);
          db::removeNode(opToken);
          ERROR(nullptr);
        }

      token = db::setChildren(opToken, token, tempToken);
    }

  return token;
}

static db::Token getEqualExpression(db::Translator *translator, bool *fail, int *error)
{
  CHECK_ARGS(nullptr);

  db::Token token = getRelationExpression(translator, fail, error);
  if (*error) ERROR(nullptr);

  while ((IS_ASSIGN(TOKEN(translator)) || IS_NOT(TOKEN(translator)))
         && IS_ASSIGN(*(translator->tokens+1)))
    {
      db::Token opToken = TOKEN(translator);
      if (IS_NOT(opToken))
        STATEMENT(opToken) = db::STATEMENT_NOT_EQUAL;
      else
        STATEMENT(opToken) = db::STATEMENT_EQUAL;
      INCREASE_TOKENS(translator);
      db::removeNode(TOKEN(translator));
      INCREASE_TOKENS(translator);

      db::Token tempToken = getRelationExpression(translator, fail, error);

      if (*fail || *error)
        {
          db::removeNode(token);
          db::removeNode(opToken);
          ERROR(nullptr);
        }

      token = db::setChildren(opToken, token, tempToken);
    }

  return token;
}

static db::Token getRelationExpression(db::Translator *translator, bool *fail, int *error)
{
  CHECK_ARGS(nullptr);

  db::Token token = getAdditiveExpression(translator, fail, error);
  if (*error) ERROR(nullptr);

  while (
         (IS_LESS(TOKEN(translator)) && !IS_LESS(*(translator->tokens+1))) ||
         (IS_GREATER(TOKEN(translator)) &&
          !IS_GREATER(*(translator->tokens+1)))
        )

    {
      db::Token opToken = TOKEN(translator);
      INCREASE_TOKENS(translator);

      db::Token tempToken = getAdditiveExpression(translator, fail, error);

      if (*fail || *error)
        {
          db::removeNode(token);
          db::removeNode(opToken);
          ERROR(nullptr);
        }

      token = db::setChildren(opToken, token, tempToken);
    }

  return token;
}

static db::Token getAdditiveExpression(db::Translator *translator, bool *fail, int *error)
{
  CHECK_ARGS(nullptr);

  db::Token token = getMultiplicativeExpression(translator, fail, error);
  if (*error) ERROR(nullptr);

  while (IS_ADD(TOKEN(translator)) || IS_SUB(TOKEN(translator)))
    {
      db::Token opToken = TOKEN(translator);
      INCREASE_TOKENS(translator);

      db::Token tempToken = getMultiplicativeExpression(translator, fail, error);

      if (*fail || *error)
        {
          if (token) db::removeNode(token);
          db::removeNode(opToken);
          ERROR(nullptr);
        }

      token = db::setChildren(opToken, token, tempToken);
    }

  return token;
}

static db::Token getMultiplicativeExpression(db::Translator *translator, bool *fail, int *error)
{
  CHECK_ARGS(nullptr);

  db::Token token = getFunctionExpression(translator, fail, error);
  if (*error) ERROR(nullptr);

  while (IS_MUL(TOKEN(translator)) || IS_DIV(TOKEN(translator)))
    {
      db::Token opToken = TOKEN(translator);
      INCREASE_TOKENS(translator);

      db::Token tempToken = getFunctionExpression(translator, fail, error);

      if (*fail || *error)
        {
          db::removeNode(token);
          ERROR(nullptr);
        }

      token = db::setChildren(opToken, token, tempToken);
    }

  return token;
}

static db::Token getFunctionExpression(db::Translator *translator, bool *fail, int *error)
{
  CHECK_ARGS(nullptr);

  db::Token token = TOKEN(translator);

  if (IS_FUNCTION(token))
    {
      INCREASE_TOKENS(translator);

      db::Token expression = gePostfixtExpression(translator, fail, error);

      if (*fail || *error)
        {
          db::removeNode(token);
          ERROR(nullptr);
        }

      token->left = expression;
    }
  else
    token = gePostfixtExpression(translator, fail, error);

  return token;
}

static db::Token gePostfixtExpression(db::Translator *translator, bool *fail, int *error)
{
  CHECK_ARGS(nullptr);

  db::Token token = TOKEN(translator);

  if (IS_NAME(token) && IS_OPEN(translator->tokens[1]))
    {
      int errorCode = 0;
      db::Token funToken =
        db::searchFunction(
                           NAME(TOKEN(translator)),
                           translator,
                           &errorCode
                           );

      if (!funToken)
        {
          handleError("Unknown function found: '%s!' at Line: %d at Position: %d!",
                      NAME(TOKEN(translator)),
                      TOKEN(translator)->position.line,
                      TOKEN(translator)->position.position);
          ERROR(nullptr);
        }
      if (IS_VOID(funToken->left->right))
        {
          handleError("Use Void-type value in expression: '%s' at Line: %d at Position: %d!",
                      NAME(TOKEN(translator)),
                      TOKEN(translator)->position.line,
                      TOKEN(translator)->position.position);
          ERROR(nullptr);
        }
      INCREASE_TOKENS(translator);

      db::Token callNode = ST(db::STATEMENT_CALL);
      callNode->left = token;

      CHECK(IS_OPEN , "(", callNode);

      bool hasntArguments = false;
      db::Token arguments = getArgumentList(translator, &hasntArguments, error);
      if (*error)
        {
          db::removeNode(callNode);
          ERROR(nullptr);
        }
      token->left = arguments;

      int paramCount = 0;
      for (db::Token temp = funToken->left->left; temp; temp = temp->right)
        ++paramCount;

      int argCount = 0;
      for (db::Token temp = arguments; temp; temp = temp->right)
        ++argCount;

      bool correctArguments = (paramCount == argCount);

      if (!correctArguments)
        {
          handleError("Incorrect call of function at Line: %d at Position: %d!",
                      TOKEN(translator)->position.line,
                      TOKEN(translator)->position.position);
          db::removeNode(callNode);
          ERROR(nullptr);
        }

      CHECK(IS_CLOSE, ")", callNode);

      return callNode;
    }

  return getPrimaryExpression(translator, fail, error);
}

static db::Token getPrimaryExpression(db::Translator *translator, bool *fail, int *error)
{
  CHECK_ARGS(nullptr);

  db::Token token = TOKEN(translator);

  if (IS_OPEN(token))
    {
      db::removeNode(token);
      INCREASE_TOKENS(translator);

      token = getExpression(translator, fail, error);
      if (*error) ERROR(nullptr);

      if (!IS_CLOSE(TOKEN(translator)))
        {
          handleError("Expected ')' at Line: %d at Position: %d!",
                      TOKEN(translator)->position.line,
                      TOKEN(translator)->position.position);
          if (token) db::removeNode(token);
          ERROR(nullptr);
        }
      db::removeNode(TOKEN(translator));
      INCREASE_TOKENS(translator);
    }
  else
    {
      bool hasntVariable = false;
      token = getNumber(translator, &hasntVariable, error);
      if (*error) ERROR(nullptr);
      if (hasntVariable)
        {
          hasntVariable = false;
          token = getValue(translator, &hasntVariable, error);
          if (*error) ERROR(nullptr);
        }
      if (hasntVariable)
        token = getVariable(translator, fail, error);
      if (*error) ERROR(nullptr);
    }

  return token;
}

static db::Token getNumber(db::Translator *translator, bool *fail, int *error)
{
  CHECK_ARGS(nullptr);

  if (!IS_NUM(TOKEN(translator))) FAIL(nullptr);

  db::Token token = TOKEN(translator);
  INCREASE_TOKENS(translator);

  return token;
}

static db::Token getVariable(db::Translator *translator, bool *fail, int *error)
{
  CHECK_ARGS(nullptr);

  if (!IS_NAME(TOKEN(translator))) FAIL(nullptr);

  db::Token token = TOKEN(translator);
  db::Variable *var = db::searchVariable(NAME(token), translator);

  if (!var)
    {
      handleError("Unknown variable: %s at Line: %d at Position: %d!",
                  NAME(token),
                  TOKEN(translator)->position.line,
                  TOKEN(translator)->position.position);

      ERROR(nullptr);
    }
  INCREASE_TOKENS(translator);
  if (var->isConst)
    {
      handleError("Found value: %s at Line: %d at Position: %d!",
                  NAME(token),
                  TOKEN(translator)->position.line,
                  TOKEN(translator)->position.position);

      db::removeNode(token);
       ERROR(nullptr);
    }

  return token;
}

static db::Token getValue(db::Translator *translator, bool *fail, int *error)
{
  CHECK_ARGS(nullptr);

  if (!IS_NAME(TOKEN(translator))) FAIL(nullptr);

  db::Token token = TOKEN(translator);
  db::Variable *val = db::searchVariable(NAME(token), translator);

  if (!val)
    {
      handleError("Unknown value: %s at Line: %d at Position: %d!",
                  NAME(token),
                  TOKEN(translator)->position.line,
                  TOKEN(translator)->position.position);

      ERROR(nullptr);
    }
  if (!val->isConst)
    FAIL(nullptr);
  INCREASE_TOKENS(translator);

  return token;
}

#include "SyntaxAnalysis.h"
#include "Translator.h"

#include <string.h>
#include <stdarg.h>
#include "ErrorHandler.h"
#include "Error.h"
#include "DSL.h"

#include "SyntaxAnalysisStatic.h"

const int MAX_NAME_SIZE = 64;

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

static FunType getStaticBlock             ;

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

void db::getGrammarly(db::Translator *translator, int *error)
{
  if (!translator || !translator->tokens) ERROR();

  db::Token *startToken = translator->tokens;

  bool fail = false;
  int errorCode = 0;
  db::addVarTable(translator, &errorCode);
  if (errorCode) ERROR();

  translator->status.returnType = db::ReturnType::None;

  translator->grammar.root = getGlobal(translator, &fail, &errorCode);

  if (!IS_END(TOKEN(translator)) && !fail && !errorCode)
      handleError("Not found terminator at Line: %d at Position: %d!",
                  TOKEN(translator)->position.line,
                  TOKEN(translator)->position.position);

  db::Token hasMain = searchFunction("main", translator);
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

  upStatic(translator, &errorCode);
  if (errorCode) ERROR();
  updateMain(hasMain, translator, &errorCode);
  if (errorCode) ERROR();
}

static db::Token getGlobal(db::Translator *translator, bool *fail, int *error)
{
  CHECK_ARGS(nullptr);

  db::Token token = getDeclaration(translator, fail, error);
  if (*fail || *error) HANDLE_ERROR("Expected at least one instruction", false);

  token = CMD(token, nullptr);
  db::Token *freePosition = &token->right;

  while (!IS_END(TOKEN(translator)))
    {
      db::Token tempToken = getDeclaration(translator, fail, error);
      if (*fail || *error)
        HANDLE_ERROR("Expected an declaration", false, token);

      *freePosition = CMD(tempToken, nullptr);
      freePosition = &(*freePosition)->right;
    }

  return token;
}

static db::Token getInstruction(db::Translator *translator, bool *fail, int *error)
{
  CHECK_ARGS(nullptr);

  bool hasntInstruction = true;
  db::Token token = nullptr;
  TRY_GET_CHAR(hasntInstruction, token, getCompoundInstruction  );
  TRY_GET_CHAR(hasntInstruction, token, getInstructionChoice    );
  TRY_GET_CHAR(hasntInstruction, token, getInstructionLoop      );
  TRY_GET_CHAR(hasntInstruction, token, getInstructionVoid      );
  TRY_GET_CHAR(hasntInstruction, token, getInstructionExpression);
  TRY_GET_CHAR(hasntInstruction, token, getInstructionJump      );
  TRY_GET_CHAR(hasntInstruction, token, getInput                );
  TRY_GET_CHAR(hasntInstruction, token, getOutput               );
  TRY_GET_CHAR(hasntInstruction, token, getVariableDeclaration  );
  if (hasntInstruction) FAIL(nullptr);

  return token;
}

static db::Token getCompoundInstruction(db::Translator *translator, bool *fail, int *error)
{
  CHECK_ARGS(nullptr);

  db::Token token = nullptr;

  if (IS_START_BRACE(TOKEN(translator)))
    {
      CLEAN_TOKEN(translator);

      db::addVarTable(translator, error);
      if (*error) ERROR(nullptr);

      db::Token *freePosition = &token;

      bool hasntCommand = false;
      while (!hasntCommand)
        {
          db::Token instruction = getInstruction(translator, &hasntCommand, error);
          if (*error) CLEAN_RESOURCES(true, token);
          if (!instruction) break;

          *freePosition = CMD(instruction, nullptr);
          freePosition = &(*freePosition)->right;
        }

      if (!token)
        HANDLE_ERROR("Expected at least one instruction", true);

      if (!IS_END_BRACE(TOKEN(translator)))
        HANDLE_ERROR("Expected at }", true, token);
      db::removeVarTable(translator, error);
      CLEAN_TOKEN(translator);
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
    HANDLE_ERROR("Expected ;", false, token);
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
      if (*fail || *error) CLEAN_RESOURCES(false, whileToken);

      CHECK_CLOSE(translator, whileToken, expression);

      db::Token instruction = getInstruction(translator, fail, error);
      if (*fail || *error) CLEAN_RESOURCES(false, whileToken, expression);

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
      if (*fail || *error) CLEAN_RESOURCES(true, ifToken);

      CHECK_CLOSE(translator, ifToken, expression);

      db::Token instruction = getInstruction(translator, fail, error);
      if (*fail || *error) CLEAN_RESOURCES(true, ifToken, expression);

      db::setChildren(ifToken, expression, instruction);
      db::removeVarTable(translator, error);

      if (IS_ELSE(TOKEN(translator)))
        {
          db::addVarTable(translator, error);
          if (*error) CLEAN_RESOURCES(false, ifToken);

          db::Token elseToken = TOKEN(translator);
          INCREASE_TOKENS(translator);

          db::Token elseInstruction = getInstruction(translator, fail, error);
          if (*error || *fail) CLEAN_RESOURCES(true, ifToken, elseToken);

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
      if (translator->status.returnType == db::ReturnType::None)
        HANDLE_ERROR("Return forbidden", false);

      INCREASE_TOKENS(translator);

      db::Token expression = getExpression(translator, fail, error);
      if (*error) CLEAN_RESOURCES(false, token);
      if (*fail && translator->status.returnType == db::ReturnType::Type)
        HANDLE_ERROR("Expected return value", false, token);
      if (!*fail && translator->status.returnType == db::ReturnType::Void)
        HANDLE_ERROR("Return value forbidden", false, token, expression);
      *fail = false;

      if (!IS_SEM(TOKEN(translator)))
        HANDLE_ERROR("Expected ;", false, token);
      db::removeNode(TOKEN(translator));
      INCREASE_TOKENS(translator);

      token->left = expression;
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
    HANDLE_ERROR_WITH_NAME(
                           "Unknown function found: '%s'",
                           NAME(TOKEN(translator)),
                           token
                          );
  if (!IS_VOID(funToken->left->right))
      FAIL(nullptr);

  INCREASE_TOKENS(translator);

  db::Token callNode = ST(db::STATEMENT_CALL);
  callNode->left = token;

  CHECK(IS_OPEN , "(", nullptr);

  bool hasntArguments = false;
  db::Token arguments = getArgumentList(translator, &hasntArguments, error);
  if (*error) CLEAN_RESOURCES(false, callNode);

  bool hasFail = false;
  arguments = callFunction(funToken->left->left, arguments, &hasFail);
  token->left = arguments;

  if (hasFail)
    HANDLE_ERROR("Incorrect call of function", false, callNode);
  CHECK(IS_CLOSE, ")", callNode);
  CHECK(IS_SEM  , ";", callNode);

  return callNode;
}

static db::Token getStaticBlock(db::Translator *translator, bool *fail, int *error)
{
  CHECK_ARGS(nullptr);

  db::Token token = TOKEN(translator);

  if (!IS_STATIC(token)) FAIL(nullptr);

  STATEMENT(token) = db::statement_t::STATEMENT_FUN;

  INCREASE_TOKENS(translator);

  addVarTable(translator, error);
  if (*error) CLEAN_RESOURCES(false, token);

  db::Token body = getFunctionBody(translator, fail, error);

  if (*fail || *error)
    HANDLE_ERROR("Expected static body", true, token);

  static int countOfStatic = 0;
  char name[MAX_NAME_SIZE] = "";
  sprintf(name, "$static_%d", countOfStatic++);

  char *string = db::addString(&translator->stringPool, name, error);

  token->left  = NAM(string);
  token->left->right = ST(db::statement_t::STATEMENT_VOID);
  token->right = body;

  db::addStatic(token, translator, error);

  db::removeVarTable(translator, error);
  if (*error) CLEAN_RESOURCES(false, token);

  return token;
}

static db::Token getDeclaration(db::Translator *translator, bool *fail, int *error)
{
  CHECK_ARGS(nullptr);

  bool hasntDeclaration = true;
  db::Token declaration = nullptr;
  TRY_GET_CHAR(hasntDeclaration, declaration, getFunctionDeclaration);
  TRY_GET_CHAR(hasntDeclaration, declaration, getVariableDeclaration);
  TRY_GET_CHAR(hasntDeclaration, declaration, getStaticBlock        );
  if (hasntDeclaration) FAIL(nullptr);

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
    HANDLE_ERROR("Expected name", false, token);

  INCREASE_TOKENS(translator);

  CHECK(IS_OPEN , "(", name);

  bool hasParam = false;
  db::Token parameters = getParameterList(translator, &hasParam, error);
  if (*error) CLEAN_RESOURCES(false, token, name);
  name->left = parameters;

  CHECK(IS_CLOSE, ")", name);

  db::Token returnType = nullptr;

  bool isTryVoid = false;

  if (IS_COL(TOKEN(translator)))
    {
      CLEAN_TOKEN(translator);

      returnType = TOKEN(translator);

      if (!IS_TYPE(returnType) && !IS_VOID(returnType))
        HANDLE_ERROR("Expected return type", false, token, name);
      INCREASE_TOKENS(translator);

      isTryVoid = IS_VOID(returnType);
    }
  else
    returnType = ST(db::STATEMENT_VOID);

  addVarTable(translator, error);
  for (db::Token temp = parameters; temp; temp = temp->right)
    addVariable(NAME(temp->left->left), false, translator, 0, error);
  if (*error) CLEAN_RESOURCES(false, token, name);

  token->left  = name;
  token->left->right = returnType;

  if (!addFunction(NAME(name), token, translator))
    HANDLE_ERROR("Redeclared of function", true, token);

  translator->status.returnType = (IS_TYPE(returnType) ?
                                  db::ReturnType::Type :
                                  db::ReturnType::Void);
  db::Token body = getFunctionBody(translator, fail, error);
  translator->status.returnType = db::ReturnType::None;

  if (*error || (*fail && !IS_ASSIGN(TOKEN(translator))))
    HANDLE_ERROR("Expected function body", true, token);

  if (*fail && !isTryVoid)
    {
      *fail = false;
      CLEAN_TOKEN(translator);
      STATEMENT(token->left->right) = db::STATEMENT_TYPE;

      db::Token expression = getExpression(translator, fail, error);

      if (*fail || *error)
        HANDLE_ERROR("Expected function body", true, token);

      body = CMD(ST(db::STATEMENT_RETURN), nullptr);
      body->left->left = expression;

      CHECK(IS_SEM, ";", body);
    }
  else if (*fail)
    HANDLE_ERROR("Return type is Void", true, token);

  token->right = body;

  db::removeVarTable(translator, error);
  if (*error) CLEAN_RESOURCES(false, token);

  if (!strcmp(NAME(name), "main"))
    translator->status.hasMain = true;

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
    HANDLE_ERROR("Expected name", false, token);
  INCREASE_TOKENS(translator);

  if (IS_COL(TOKEN(translator)))
    {
      CLEAN_TOKEN(translator);
      CHECK(IS_TYPE, "value type", name);
    }

  CHECK(IS_ASSIGN, "=", name);

  db::Token value = getExpression(translator, fail, error);

  if (!value)
    HANDLE_ERROR("Expected value", false, token, name);

  if (!addVariable(NAME(name), isConst, translator, 0))
    HANDLE_ERROR("Redeclared of variable", false, token, value, name);

  db::setChildren(token, name, value);

  if (!IS_SEM(TOKEN(translator)))
    HANDLE_ERROR("Expected ;", false, token);
  CLEAN_TOKEN(translator);

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

      if (IS_COMMA(TOKEN(translator))) CLEAN_TOKEN(translator);
      else break;

      bool hasntExpression = false;
      expression = getExpression(translator, &hasntExpression, error);
      if (hasntExpression)
        HANDLE_ERROR("Expected argument", false, token);
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
      db::Token expression = nullptr;
      db::Token parameter = TOKEN(translator);
      INCREASE_TOKENS(translator);

      if (!IS_COL(TOKEN(translator)))
        HANDLE_ERROR("Expected :", false, token, parameter);

      CLEAN_TOKEN(translator);

      if (!IS_TYPE(TOKEN(translator)))
        HANDLE_ERROR("Expected argument type", false, token, parameter);
      CLEAN_TOKEN(translator);

      if (IS_ASSIGN(TOKEN(translator)))
        {
          CLEAN_TOKEN(translator);

          expression = getExpression(translator, fail, error);
          if (*error) CLEAN_RESOURCES(false, token, parameter);
          if (*fail)
            HANDLE_ERROR("Expected argument value", false, token, parameter);
        }

      *freePosition = PARAM(VARIABLE(parameter, expression), nullptr);
      freePosition = &(*freePosition)->right;

      if (IS_COMMA(TOKEN(translator)))
        CLEAN_TOKEN(translator);
      else break;

      if (!IS_NAME(TOKEN(translator)))
        HANDLE_ERROR("Expected parameter type", false, token);
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
      CLEAN_TOKEN(translator);

      db::Token *freePosition = &token;

      bool hasntCommand = false;
      while (!hasntCommand)
        {
          db::Token instruction = getInstruction(translator, &hasntCommand, error);
          if (*error) CLEAN_RESOURCES(false, token);
          if (!instruction) break;

          *freePosition = CMD(instruction, nullptr);
          freePosition = &(*freePosition)->right;
        }

      if (!token)
        HANDLE_ERROR("Expected at least one instruction", false);

      if (!IS_END_BRACE(TOKEN(translator)))
        HANDLE_ERROR("Expected }", false, token);
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

  CHECK(IS_INPUT, ">>", token);

  db::Token variable = nullptr;

  while (IS_NAME(TOKEN(translator)))
    {
      variable = getVariable(translator, fail, error);

      if (*error) { db::removeNode(token); ERROR(nullptr); }
      *freePosition = PARAM(variable, nullptr);
      freePosition = &(*freePosition)->right;

      if (IS_INPUT(TOKEN(translator)))
        {
          CHECK(IS_INPUT, ">>", token);
          variable = nullptr;
        }
      else
        break;
    }

  if (!variable)
    HANDLE_ERROR("Expected a variable", false, token);

  CHECK(IS_SEM, ";", nullptr);

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
      if (IS_OUTPUT(TOKEN(translator)))
          CHECK(IS_OUTPUT, "<<", token);
      else
        break;

      expression = getExpression(translator, fail, error);

      if (*error) { db::removeNode(token); ERROR(nullptr); }

      if (*fail &&
          !IS_STRING(TOKEN(translator)) &&
          !IS_ENDL(TOKEN(translator)))
        HANDLE_ERROR("Expected an expression or string", false, token);

      if (!expression)
        {
          expression = TOKEN(translator);
          INCREASE_TOKENS(translator);
          *fail = false;
        }

      *freePosition = PARAM(expression, nullptr);
      freePosition = &(*freePosition)->right;

    } while (expression);

  if (!token->left) CLEAN_RESOURCES(false, token);

  CHECK(IS_SEM, ";", nullptr);

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
        HANDLE_ERROR("Expected variable", false, token);
      db::Variable *var = searchVariable(NAME(token), translator);

      if (var->isConst)
        HANDLE_ERROR("Value cannot be change", false, token);

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

  while (IS_OR(TOKEN(translator)))
    {
      db::Token opToken = TOKEN(translator);
      INCREASE_TOKENS(translator);

      db::Token tempToken = getLogicAndExpression(translator, fail, error);
      if (*fail || *error) CLEAN_RESOURCES(false, token, opToken);

      token = db::setChildren(opToken, token, tempToken);
    }

  return token;
}

static db::Token getLogicAndExpression(db::Translator *translator, bool *fail, int *error)
{
  CHECK_ARGS(nullptr);

  db::Token token = getEqualExpression(translator, fail, error);
  if (*error) ERROR(nullptr);

  while (IS_AND(TOKEN(translator)))
    {
      db::Token opToken = TOKEN(translator);
      INCREASE_TOKENS(translator);

      db::Token tempToken = getEqualExpression(translator, fail, error);
      if (*fail || *error) CLEAN_RESOURCES(false, token, opToken);

      token = db::setChildren(opToken, token, tempToken);
    }

  return token;
}

static db::Token getEqualExpression(db::Translator *translator, bool *fail, int *error)
{
  CHECK_ARGS(nullptr);

  db::Token token = getRelationExpression(translator, fail, error);
  if (*error) ERROR(nullptr);

  while (IS_EQUAL(TOKEN(translator)) || IS_NOT_EQUAL(TOKEN(translator)))
    {
      db::Token opToken = TOKEN(translator);
      INCREASE_TOKENS(translator);

      db::Token tempToken = getRelationExpression(translator, fail, error);
      if (*fail || *error) CLEAN_RESOURCES(false, token, opToken);

      token = db::setChildren(opToken, token, tempToken);
    }

  return token;
}

static db::Token getRelationExpression(db::Translator *translator, bool *fail, int *error)
{
  CHECK_ARGS(nullptr);

  db::Token token = getAdditiveExpression(translator, fail, error);
  if (*error) ERROR(nullptr);

  while (IS_LESS(      TOKEN(translator)) ||
         IS_GREATER(   TOKEN(translator)) ||
         IS_LESS_EQ(   TOKEN(translator)) ||
         IS_GREATER_EQ(TOKEN(translator)))

    {
      db::Token opToken = TOKEN(translator);
      INCREASE_TOKENS(translator);

      db::Token tempToken = getAdditiveExpression(translator, fail, error);
      if (*fail || *error) CLEAN_RESOURCES(false, token, opToken);

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
      if (*fail || *error) CLEAN_RESOURCES(false, token, opToken);

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
      if (*fail || *error) CLEAN_RESOURCES(false, token, opToken);

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
      if (*fail || *error) CLEAN_RESOURCES(false, token);

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
        HANDLE_ERROR_WITH_NAME(
                               "Unknown function found: '%s'",
                               NAME(TOKEN(translator)),
                               false
                              );
      if (IS_VOID(funToken->left->right))
        HANDLE_ERROR_WITH_NAME(
                               "Use Void-type value in expression: '%s'",
                               NAME(TOKEN(translator)),
                               false
                              );
      INCREASE_TOKENS(translator);

      db::Token callNode = ST(db::STATEMENT_CALL);
      callNode->left = token;

      CHECK(IS_OPEN , "(", callNode);

      bool hasntArguments = false;
      db::Token arguments = getArgumentList(translator, &hasntArguments, error);
      if (*error) CLEAN_RESOURCES(false, callNode);

      bool hasFail = false;
      arguments = callFunction(funToken->left->left, arguments, &hasFail);
      token->left = arguments;

      if (hasFail)
        HANDLE_ERROR("Incorrect call of function", false, callNode);

      CHECK(IS_CLOSE, ")", callNode);

      return callNode;
    }

  return getPrimaryExpression(translator, fail, error);
}

static db::Token getPrimaryExpression(db::Translator *translator, bool *fail, int *error)
{
  CHECK_ARGS(nullptr);

  db::Token token = TOKEN(translator);

  if (IS_OPEN(token) || IS_START_SQUARE_BRACE(token))
    {
      bool isSquare = IS_START_SQUARE_BRACE(token);

      CLEAN_TOKEN(translator);

      token = getExpression(translator, fail, error);
      if (*fail || *error) ERROR(nullptr);

      if (!isSquare && !IS_CLOSE(TOKEN(translator)))
        HANDLE_ERROR("Expected )", false, token);
      else if (isSquare && !IS_END_SQUARE_BRACE(TOKEN(translator)))
        HANDLE_ERROR("Expected ]", false, token);

      CLEAN_TOKEN(translator);

      if (isSquare)
        {
          db::Token temp = ST(db::STATEMENT_INT);

          temp->left = token;
          token = temp;
        }
    }
  else
    {
      bool hasntVariable = true;
      TRY_GET_CHAR(hasntVariable, token, getNumber  );
      TRY_GET_CHAR(hasntVariable, token, getValue   );
      TRY_GET_CHAR(hasntVariable, token, getVariable);
      if (hasntVariable) FAIL(nullptr);
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
    HANDLE_ERROR_WITH_NAME(
                           "Unknown variable: '%s'",
                           NAME(token),
                           false
                          );
  INCREASE_TOKENS(translator);
  if (var->isConst)
    HANDLE_ERROR_WITH_NAME(
                           "Found value: '%s'",
                           NAME(token),
                           false,
                           token
                          );

  return token;
}

static db::Token getValue(db::Translator *translator, bool *fail, int *error)
{
  CHECK_ARGS(nullptr);

  if (!IS_NAME(TOKEN(translator))) FAIL(nullptr);

  db::Token token = TOKEN(translator);
  db::Variable *val = db::searchVariable(NAME(token), translator);

  if (!val)
    HANDLE_ERROR_WITH_NAME(
                           "Unknown value: '%s'",
                           NAME(token),
                           false
                          );
  if (!val->isConst)
    FAIL(nullptr);
  INCREASE_TOKENS(translator);

  return token;
}

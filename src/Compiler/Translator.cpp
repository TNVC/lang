#include "Translator.h"

#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include "DSL.h"
#include "SystemLike.h"
#include "Fiofunctions.h"
#include "StringsUtils.h"
#include "ErrorHandler.h"
#include "Error.h"
#include "Assert.h"

#pragma GCC diagnostic ignored "-Wswitch-enum"

#define   INT(VALUE) (int)(VALUE)
#define FRACT(VALUE) (int)((VALUE - INT(VALUE))*10000)

#define CHECK_ARGUMENTS()                                               \
  if (!translator || !token || !target) ERROR(false)

#define HANDLE_ERROR(MESSAGE, ...)                    \
  do                                                  \
    {                                                 \
      handleError(MESSAGE __VA_OPT__(,) __VA_ARGS__); \
      ERROR(false);                                   \
    } while (0)

#define TRANSLATE_UNARY(STATEMENT)                                      \
  do                                                                    \
    {                                                                   \
      if (token->left)                                                  \
        {                                                               \
          if (!translateToken(token->left, translator, target, error))  \
            ERROR(false);                                               \
        }                                                               \
      else                                                              \
        HANDLE_ERROR("Expected that " #STATEMENT                        \
                     " has at least one argument");                     \
      fprintf(target, #STATEMENT "\n");                                 \
    } while (0)

#define TRANSLATE_BINARY(STATEMENT)                                     \
  do                                                                    \
    {                                                                   \
      if (token->right)                                                 \
        {                                                               \
          if (!translateToken(token->right, translator, target, error)) \
            ERROR(false);                                               \
        }                                                               \
      else                                                              \
        HANDLE_ERROR("Expected that " #STATEMENT                        \
                     " has first argument");                            \
      if (token->left )                                                 \
        {                                                               \
          if (!translateToken(token->left , translator, target, error)) \
            ERROR(false);                                               \
        }                                                               \
      else                                                              \
        HANDLE_ERROR("Expected that " #STATEMENT                        \
                     " has second argument");                           \
      fprintf(target, #STATEMENT "\n");                                 \
    } while (0)                                                         \

#define TRANSLATE_LINARY(STATEMENT)                                     \
  do                                                                    \
    {                                                                   \
      if (token->right)                                                 \
        {                                                               \
          if (!translateToken(token->right, translator, target, error)) \
            ERROR(false);                                               \
          if (!translateToken(token->left , translator, target, error)) \
            ERROR(false);                                               \
        }                                                               \
      else                                                              \
        {                                                               \
          if (!translateToken(token->left , translator, target, error)) \
            ERROR(false);                                               \
          fprintf(target, "PUSH 0\n");                                  \
          fprintf(target, "PUSH 0\n");                                  \
        }                                                               \
      fprintf(target, #STATEMENT "\n");                                 \
    } while (0)


#define START_TRANSLATE(MESSAGE)                \
  do                                            \
    {                                           \
      fprintf(target, ";"#MESSAGE"\n");         \
    } while (0)

#define END_TRANSLATE()                         \
  do                                            \
    {                                           \
      fprintf(target, ";End\n\n");              \
    } while (0)

const int BUFFER_SIZE  = 16;

const int  VIDEO_MEMORY_START =  0;
const int GLOBAL_MEMORY_START = 128;
const int  STACK_MEMORY_START = 256;

const char *const     RETURN_INT_ADDRESS = "rax";
const char *const   RETURN_FRACT_ADDRESS = "rbx";
const char *const  STACK_POINTER_ADDRESS = "rcx";
const char *const   STACK_BOTTOM_ADDRESS = "rdx";
const char *const STACK_FUNCTION_ADDRESS = "rfx";

static int allocateVariable(db::Token block, db::Translator *translator, int startIndex, FILE *target);
static int allocateParameters(db::Token block, db::Translator *translator, FILE *target);
static int allocateBlock(db::Token block, db::Translator *translator, int startIndex, FILE *target);
static int allocateInstruction(db::Token token, db::Translator *translator, int startIndex, int blockNumber, FILE *target);

static bool translateArgument(db::Token block, db::Translator *translator, FILE *target);

static bool translateToken(
                           const db::Token token,
                           db::Translator *translator,
                           FILE *target,
                           int *error = nullptr
                          );

static void translatrGlobaleVariable(db::Translator *translator, FILE *target, int *error = nullptr);

static void translateFunctions(db::Translator *translator, FILE *target, int *error = nullptr);

typedef bool FunType(
                     db::Translator *translator,
                     db::Token token,
                     FILE *target,
                     int *error
                    );

static FunType translateStatement ;
static FunType translateIf        ;
static FunType translateWhile     ;
static FunType translateReturn    ;
static FunType translateCall      ;
static FunType translateIn        ;
static FunType translateOut       ;
static FunType translateInt       ;
static FunType translateVariable  ;
static FunType translateAssignment;
static FunType translateNumber    ;
static FunType translateName      ;
static FunType translateString    ;

static bool translateNumber(
                            db::Translator *translator,
                            db::Token token,
                            FILE *target,
                            int *error
                           )
{
  CHECK_ARGUMENTS();

  fprintf(target, "PUSH %d\nPUSH %d\n",
          FRACT(NUMBER(token)),
          INT(NUMBER(token)));

  return true;
}

static bool translateName(
                          db::Translator *translator,
                          db::Token token,
                          FILE *target,
                          int *error
                         )
{
  CHECK_ARGUMENTS();

  db::Variable *var = db::searchVariable(NAME(token), translator, false);

  if (!var) HANDLE_ERROR("Unknown variable: %s", NAME(token));

  if (var->isGlobal)
    fprintf(target, "PUSH [%d]\nPUSH [%d]\n",
            GLOBAL_MEMORY_START+(var->number*2+1),
            GLOBAL_MEMORY_START+(var->number*2  ));
  else
    fprintf(target, "PUSH [%d+%s]\nPUSH [%d+%s]\n",
            STACK_MEMORY_START+(var->number*2+2),
            STACK_BOTTOM_ADDRESS,
            STACK_MEMORY_START+(var->number*2+1),
            STACK_BOTTOM_ADDRESS);

  return true;
}


static bool translateString(
                            db::Translator *translator,
                            db::Token token,
                            FILE *target,
                            int *error
                           )
{
  CHECK_ARGUMENTS();

  const char *temp = STRING(token);
  int i = 0;
  for ( ; *temp; ++temp, ++i)
    fprintf(target, "PUSH %d; %c\nPOP [%d]\n",
            *temp, isprint(*temp) ? *temp : '~', i);
  fprintf(target, "PUSH %d; %c\nPOP [%d]\n",
          *temp, isprint(*temp) ? *temp : '~', i);

  return true;
}

static bool translateStatement(
                               db::Translator *translator,
                               db::Token token,
                               FILE *target,
                               int *error
                              )
{
  CHECK_ARGUMENTS();

  if (token->left )
    {
      if (!translateToken(token->left , translator, target, error))
        ERROR(false);
    }
  else
    HANDLE_ERROR("Statement hasn`t instruction");
  if (token->right)
    {
      if (!translateToken(token->right, translator, target, error))
        ERROR(false);
    }

  return true;
}

static bool translateCall(
                          db::Translator *translator,
                          db::Token token,
                          FILE *target,
                          int *error
                         )
{
  CHECK_ARGUMENTS();///////////

  if (!IS_NAME(token->left)) HANDLE_ERROR("Call hasn`t function name");

  START_TRANSLATE(Call);

  fprintf(target, ";Save stack pointer\nPUSH %s\n",
          STACK_BOTTOM_ADDRESS);

  /*
    printSpaces(tabs, target);
    fprintf(target,
    ";Turn on real-calc\n"
    "PUSH 1\n"
    "POP rex\n");
  */

  if (token->left->left)
    if (!translateArgument(token->left->left, translator, target))
      ERROR(false);

  fprintf(target, "CALL FUN_%s\n", NAME(token->left));

  fprintf(target, ";Pop stack pointer\n");
  fprintf(target, "POP %s\n\n", STACK_BOTTOM_ADDRESS);

  fprintf(target, "PUSH %s\n", RETURN_FRACT_ADDRESS);
  fprintf(target, "PUSH %s\n",   RETURN_INT_ADDRESS);

  END_TRANSLATE();

  return true;
}

static bool translateReturn(
                            db::Translator *translator,
                            db::Token token,
                            FILE *target,
                            int *error
                           )
{
  CHECK_ARGUMENTS();

  START_TRANSLATE(Return);

  if (token->left)
    {
      if (!translateToken(token->left, translator, target, error))
        ERROR(false);

      fprintf(target, "POP %s\n",   RETURN_INT_ADDRESS);
      fprintf(target, "POP %s\n", RETURN_FRACT_ADDRESS);
    }

  fprintf(target, ";Update stack pointer\n");
  fprintf(target, "PUSH %s\n", STACK_BOTTOM_ADDRESS);
  fprintf(target, "POP %s\n", STACK_POINTER_ADDRESS);

  fprintf(target, ";Pop return address\n");
  fprintf(target, "PUSH [%d+%s]\n",
          STACK_MEMORY_START,
          STACK_POINTER_ADDRESS);

  fprintf(target, "RET\n");

  END_TRANSLATE();

  return true;
}

static bool translateIf(
                        db::Translator *translator,
                        db::Token token,
                        FILE *target,
                        int *error
                       )
{
  CHECK_ARGUMENTS();

  if (!token->left ) HANDLE_ERROR("If hasn`t conditional");

  if (!token->right) HANDLE_ERROR("If hasn`t body");

  if (IS_ELSE(token->right) &&
      (!token->right->left || !token->right->right))
    HANDLE_ERROR("Else hasn`t body");

  START_TRANSLATE(If);

  static int ifCount = 0;
  int currentIfNumber = ifCount++;

  if (!translateToken(token->left, translator,target, error))
    ERROR(false);

  fprintf(target, "PUSH 0\n");
  fprintf(target, "PUSH 0\n");
  fprintf(target, "JE :ELSE_%6.6d\n\n", currentIfNumber);

  if (IS_ELSE(token->right))
    {
      if (!translateToken(token->right->left, translator,target, error))
        ERROR(false);
    }
  else
    {
      if (!translateToken(token->right, translator,target, error))
        ERROR(false);
    }

  fprintf(target, "JMP END_IF_%6.6d\n", currentIfNumber);
  fprintf(target, "ELSE_%6.6d:\n", currentIfNumber);

  if (IS_ELSE(token->right))
    if (!translateToken(token->right->right, translator,target, error))
      ERROR(false);

  fprintf(target, "END_IF_%6.6d:\n", currentIfNumber);

  END_TRANSLATE();

  return true;
}


static bool translateOut(
                         db::Translator *translator,
                         db::Token token,
                         FILE *target,
                         int *error
                        )
{
  CHECK_ARGUMENTS();

  if (!token->left) HANDLE_ERROR("Out hasn`t arguments");

  START_TRANSLATE(Out);

  db::Token temp = token->left;
  for ( ; temp; temp = temp->right)
    {
      if (!translateToken(temp->left, translator, target, error))
        ERROR(false);

      if (IS_STRING(temp->left))
        fprintf(target, "SHOW\n");

      else if(IS_ENDL(temp->left))
        {
          fprintf(target, "PUSH %d; \\n\n", '\n');
          fprintf(target, "POP [0]\n");
          fprintf(target, "PUSH %d; \\0\n", '\0');
          fprintf(target, "POP [1]\n");
          fprintf(target, "SHOW\n");
        }
      else
        fprintf(target, "OUT\n");
    }

  END_TRANSLATE();

  return true;
}

static bool translateIn(
                        db::Translator *translator,
                        db::Token token,
                        FILE *target,
                        int *error
                       )
{
  CHECK_ARGUMENTS();

  if (!token->left) HANDLE_ERROR("In hasn`t arguments");

  START_TRANSLATE(In);

  db::Token temp = token->left;
  for ( ; temp; temp = temp->right)
    {
      db::Variable *var = searchVariable(NAME(temp->left), translator, false);

      if (!var) HANDLE_ERROR("Unknown variable: %s", NAME(temp->left));

      fprintf(target, "IN\n");

      if (var->isGlobal)
        {
          fprintf(target, "POP [%d]\n", GLOBAL_MEMORY_START+(var->number*2  ));
          fprintf(target, "POP [%d]\n", GLOBAL_MEMORY_START+(var->number*2+1));
        }
      else
        {
          fprintf(target, "POP [%d+%s]\n",
                  STACK_MEMORY_START+(var->number*2+1),/////////////
                  STACK_BOTTOM_ADDRESS);
          fprintf(target, "POP [%d+%s]\n",
                  STACK_MEMORY_START+(var->number*2+2),////////////
                  STACK_BOTTOM_ADDRESS);
        }
    }
  END_TRANSLATE();

  return true;
}

static bool translateInt(
                         db::Translator *translator,
                         db::Token token,
                         FILE *target,
                         int *error
                        )
{
  CHECK_ARGUMENTS();

  if (!token->left) HANDLE_ERROR("Int hasn`t value");

  START_TRANSLATE(Int);

  if (!translateToken(token->left, translator, target, error))
    ERROR(false);

  fprintf(target,
          ";Turn off real-calc\n"
          "PUSH 0\n"
          "POP rex\n");
  fprintf(target, "SWAP\n");
  fprintf(target, "POP [0]\n");
  fprintf(target, "PUSH 0\n");
  fprintf(target, "SWAP\n");
  fprintf(target,
          ";Turn on real-calc\n"
          "PUSH 1\n"
          "POP rex\n");

  END_TRANSLATE();

  return true;
}

static bool translateWhile(
                           db::Translator *translator,
                           db::Token token,
                           FILE *target,
                           int *error
                          )
{
  CHECK_ARGUMENTS();

  if (!token->left)           HANDLE_ERROR("While hasn`t conditional");
  if (!IS_COMP(token->right)) HANDLE_ERROR("While hasn`t body");

  START_TRANSLATE(While);

  static int whileCount = 0;

  fprintf(target, "WHILE_%6.6d:\n\n", whileCount);

  if (!translateToken(token->left, translator, target, error))
    ERROR(false);

  fprintf(target, "PUSH 0\n");
  fprintf(target, "PUSH 0\n");
  fprintf(target, "JE :END_WHILE_%6.6d\n\n", whileCount);

  if (!translateToken(token->right, translator, target, error))
    ERROR(false);

  fprintf(target, "JMP WHILE_%6.6d\n", whileCount);
  fprintf(target, "END_WHILE_%6.6d:\n", whileCount++);

  END_TRANSLATE();

  return true;
}

static bool translateVariable(
                              db::Translator *translator,
                              db::Token token,
                              FILE *target,
                              int *error
                             )
{
  CHECK_ARGUMENTS();

  if (!IS_NAME(token->left)) HANDLE_ERROR("Variable hasn`t name");
  if (!token->right)         HANDLE_ERROR("Variable hasn`t value");

  START_TRANSLATE(Local Var/Val);

  db::Variable *var = searchVariable(NAME(token->left), translator, false);

  if (!var) HANDLE_ERROR("Unknown variable: %s", NAME(token->left));

  if (!translateToken(token->right, translator, target, error))
    ERROR(false);

  fprintf(target, "POP [%d+%s]\n",
          STACK_MEMORY_START+(var->number*2+1),//////////////
          STACK_BOTTOM_ADDRESS);
  fprintf(target, "POP [%d+%s]\n",
          STACK_MEMORY_START+(var->number*2+2),///////////////
          STACK_BOTTOM_ADDRESS);

  END_TRANSLATE();

  return true;
}

static bool translateAssignment(
                                db::Translator *translator,
                                db::Token token,
                                FILE *target,
                                int *error
                               )
{
  CHECK_ARGUMENTS();

  START_TRANSLATE(Assignment);

  db::Variable *var = searchVariable(NAME(token->left), translator, false);

  if (!var) HANDLE_ERROR("Unknown variable: %s", NAME(token->left));

  translateToken(token->right, translator,target, error);
  fprintf(target, "COPY\n");

  if (var->isGlobal)
    {
      fprintf(target, "POP [%d]\n", GLOBAL_MEMORY_START+(var->number*2  ));
      fprintf(target, "POP [%d]\n", GLOBAL_MEMORY_START+(var->number*2+1));
    }
  else
    {
      fprintf(target, "POP [%d+%s]\n",
              STACK_MEMORY_START+(var->number*2+1),
              STACK_BOTTOM_ADDRESS);
      fprintf(target, "POP [%d+%s]\n",
              STACK_MEMORY_START+(var->number*2+2),
              STACK_BOTTOM_ADDRESS);
    }
  END_TRANSLATE();

  return true;
}

bool db::translate(db::Translator *translator, FILE *target, int *error)
{
  if (!translator) ERROR(false);

  fprintf(target,
          ";Turn on real-calc\n"
          "PUSH 1\n"
          "POP rex\n"
          ";End\n\n");
  fprintf(target,
          ";Init stack pointer\n"
          "PUSH 0\n"
          "POP %s\n"
          "PUSH 0\n"
          "POP %s\n"
          "PUSH 0\n"
          "POP %s\n"
          ";End\n\n",
          STACK_POINTER_ADDRESS,
          STACK_BOTTOM_ADDRESS,
          STACK_FUNCTION_ADDRESS);

  int errorCode = 0;

  translatrGlobaleVariable(translator, target, &errorCode);
  if (errorCode) ERROR(false);

  fprintf(target,
          ";Call main\n"
          "CALL FUN_main\n"
          ";End\n\n");

  fprintf(target,
          "HLT\n"
          ";End\n\n");

  translateFunctions(translator, target, &errorCode);
  if (errorCode) ERROR(false);

  return true;
}

static bool translateToken(
                           const db::Token token,
                           db::Translator *translator,
                           FILE *target,
                           int *error
                          )
{
  if (!token) ERROR(false);

  switch (token->type)
    {
    case db::type_t::NUMBER:
      if (!translateNumber(translator, token, target, error))
        ERROR(false);
        break;
    case db::type_t::NAME:
      if (!translateName  (translator, token, target, error))
        ERROR(false);
      break;
    case db::type_t::STRING:
      if (!translateString(translator, token, target, error))
        ERROR(false);
      break;
    case db::type_t::STATEMENT:
      {
        switch (STATEMENT(token))
          {
          case db::STATEMENT_ADD:       TRANSLATE_LINARY(ADD    ); break;
          case db::STATEMENT_SUB:       TRANSLATE_LINARY(SUB    ); break;
          case db::STATEMENT_MUL:       TRANSLATE_BINARY(MUL    ); break;
          case db::STATEMENT_DIV:       TRANSLATE_BINARY(DIV    ); break;
          case db::STATEMENT_SIN:       TRANSLATE_UNARY(SIN     ); break;
          case db::STATEMENT_COS:       TRANSLATE_UNARY(COS     ); break;
          case db::STATEMENT_POW:       TRANSLATE_BINARY(POW    ); break;
          case db::STATEMENT_SQRT:      TRANSLATE_UNARY(SQRT    ); break;
          case db::STATEMENT_AND:       TRANSLATE_BINARY(AND    ); break;
          case db::STATEMENT_OR:        TRANSLATE_BINARY(OR     ); break;
          case db::STATEMENT_NOT_EQUAL: TRANSLATE_BINARY(NEQL   ); break;
          case db::STATEMENT_EQUAL:     TRANSLATE_BINARY(EQL    ); break;
          case db::STATEMENT_LESS:      TRANSLATE_BINARY(LESS   ); break;
          case db::STATEMENT_GREATER:   TRANSLATE_BINARY(GREATER); break;

          case db::STATEMENT_COMPOUND:
            if (!translateStatement(translator, token, target, error))
              ERROR(false);
            break;
          case db::STATEMENT_ASSIGNMENT:
            if (!translateAssignment(translator, token, target, error))
              ERROR(false);
            break;
          case db::STATEMENT_IF:
            if (!translateIf(translator, token, target, error))
              ERROR(false);
            break;
          case db::STATEMENT_WHILE:
            if (!translateWhile(translator, token, target, error))
              ERROR(false);
            break;
          case db::STATEMENT_RETURN:
            if (!translateReturn(translator, token, target, error))
              ERROR(false);
            break;
          case db::STATEMENT_CALL:
            if (!translateCall(translator, token, target, error))
              ERROR(false);
            break;
          case db::STATEMENT_VAL:
          case db::STATEMENT_VAR:
            if (!translateVariable(translator, token, target, error))
              ERROR(false);
            break;
          case db::STATEMENT_OUT:
            if (!translateOut(translator, token, target, error))
              ERROR(false);
            break;
          case db::STATEMENT_IN:
            if (!translateIn(translator, token, target, error))
              ERROR(false);
            break;
          case db::STATEMENT_INT:
            if (!translateInt(translator, token, target, error))
              ERROR(false);
            break;
          default: break;
          }
        break;
      }
    default:
      {
        handleError("Unknown type at File: %s at Line: %d", __FILE__, __LINE__);
        ERROR(false);
      }
    }

  return true;
}

static void
translatrGlobaleVariable(db::Translator *translator, FILE *target, int *error)
{
  if (!translator || !target) ERROR();

  unsigned errorCode = 0;
  db::VarTable *table = stack_get(&translator->varTables, (unsigned)0, &errorCode);
  if (errorCode) ERROR();

  fprintf(target, ";Start Global Var/Val initilization\n");

  db::Token token = translator->grammar.root;
  for (int i = 0; token; token = token->right)
    {
      if (!(IS_VAR(token->left) || IS_VAL(token->left))) continue;

      int num = table->table[i++].number;

      fprintf(target, ";%s\n", NAME(token->left->left));
      translateToken(token->left->right, translator, target, error);
      fprintf(
              target,
              "POP [%d]\n"
              "POP [%d]\n",
              GLOBAL_MEMORY_START+(num*2  ),
              GLOBAL_MEMORY_START+(num*2+1)
             );
    }

  fprintf(target, ";End Global Var/Val initilization\n\n");
}

static void translateFunctions(db::Translator *translator, FILE *target, int *error)
{
  if (!translator || !target) ERROR();

  for (size_t i = 0; i < translator->functions.size; ++i)
    {
      db::Function *function = translator->functions.table+i;

      int offset = 1;
      fprintf(target,
              ";Function\nFUN_%s:\n"
              ";Save return address\nPOP [%d+%s]\n",
              function->name,
              STACK_MEMORY_START,
              STACK_POINTER_ADDRESS);

      db::addVarTable(translator);

      offset +=
        allocateParameters(function->token->left->left, translator, target);
      translator->status.stackOffset = offset;
      offset +=
        allocateVariable  (function->token->right     , translator, offset, target);
      fprintf(target,
              ";Save stack pointer\n"
              "PUSH %s\n" "POP %s\n" "PUSH %s\n" "POP %s\n"

              ";Turn off real-calc\n"
              "PUSH 0\n" "POP rex\n"

              ";Update stack pointer\n"
              "PUSH %d\n" "PUSH %s\n" "ADD\n" "POP %s\n"

              ";Turn on real-calc\n"
              "PUSH 1\n" "POP rex\n",
              STACK_POINTER_ADDRESS, STACK_BOTTOM_ADDRESS,
              STACK_POINTER_ADDRESS, STACK_FUNCTION_ADDRESS,
              offset, STACK_POINTER_ADDRESS, STACK_POINTER_ADDRESS);

      int errorCode = 0;
      translateToken(function->token->right, translator, target, &errorCode);
      db::removeVarTable(translator);

      if (errorCode) ERROR();

      fprintf(target,
              ";Update stack pointer\n"
              "PUSH %s\nPOP %s\n"
              ";Pop return address\n"
              "PUSH [%d+%s]\n" "RET\n;End\n\n",
              STACK_BOTTOM_ADDRESS, STACK_POINTER_ADDRESS,
              STACK_MEMORY_START, STACK_POINTER_ADDRESS);
    }
}

static bool translateArgument(db::Token block, db::Translator *translator, FILE *target)
{
  if (!block) return false;

  if (block->right)
    if (!translateArgument(block->right, translator, target))
      return false;

  if (block->left )
    if (!translateToken(block->left , translator, target))
      return false;

  return true;
}

static int allocateParameters(db::Token block, db::Translator *translator, FILE *target)
{
  int parameterIndex = 0;

  for (db::Token temp = block; temp; temp = temp->right)
    {
      db::addVariable(
                      NAME(temp->left->left),
                      false,
                      translator,
                      parameterIndex
                     );

      fprintf(target,
              ";Get %d parameter\n"
              "POP [%d+%s]\n"
              "POP [%d+%s]\n",
              parameterIndex,
              1+STACK_MEMORY_START+parameterIndex*2,
              STACK_POINTER_ADDRESS,
              1+STACK_MEMORY_START+parameterIndex*2+1,
              STACK_POINTER_ADDRESS);

      ++parameterIndex;
    }

  return parameterIndex*2;
}

static int allocateVariable(db::Token block, db::Translator *translator, int startIndex, FILE *target)
{
  assert(startIndex >= 0);

  int variableIndex = allocateBlock(block, translator, startIndex, target);
  allocateBlock(nullptr, nullptr, 0, nullptr);

  return variableIndex*2;
}

static int allocateBlock(db::Token block, db::Translator *translator, int startIndex, FILE *target)
{
  static int numOfBlock = 0;
  int currentBlockNum = numOfBlock++;

  if (!block) { numOfBlock = 0; allocateInstruction(nullptr, nullptr, 0, 0, nullptr); return -1; }

  if (!IS_COMP(block)) return allocateInstruction(block, translator, startIndex, currentBlockNum, target);

  int finalVariableCount = 0;

  for (db::Token temp = block; temp; temp = temp->right)
    finalVariableCount = allocateInstruction(temp->left, translator, startIndex, currentBlockNum, target);

  return finalVariableCount;
}

static int allocateInstruction(db::Token token, db::Translator *translator, int startIndex, int blockNumber, FILE *target)
{
  static int variableCount = 0;
  char buffer[BUFFER_SIZE] = "";

  if (!token) { variableCount = 0; return -1; }

  if (IS_STATEMENT(token))
    switch (STATEMENT(token))
      {
      case db::STATEMENT_COMPOUND:
        allocateBlock(token, translator, startIndex, target);
        break;
      case db::STATEMENT_IF:
        if (IS_ELSE(token->right))
          {
            allocateBlock(token->right->right, translator, startIndex, target);
            allocateBlock(token->right->left , translator, startIndex, target);
          }
        else
          allocateBlock(token->right, translator, startIndex, target);
        break;
      case db::STATEMENT_WHILE:
        allocateBlock(token->right, translator, startIndex, target);
        break;
      case db::STATEMENT_VAL: case db::STATEMENT_VAR:
        {
          sprintf(buffer, "%d_", blockNumber);

          db::addVariable(
                          NAME(token->left),
                          IS_VAL(token),
                          translator,
                          variableCount+startIndex/2
                         );

          fprintf(target,
                  ";Allocate local var/val\n"
                  ";%d_%s [%d+%s]\n"
                  ";%d_%s [%d+%s]\n",
                  blockNumber, NAME(token->left),
                  startIndex+STACK_MEMORY_START+variableCount*2,
                  STACK_POINTER_ADDRESS,
                  blockNumber, NAME(token->left),
                  startIndex+STACK_MEMORY_START+variableCount*2+1,
                  STACK_POINTER_ADDRESS);

          ++variableCount;

          break;
        }
      default: break;
      }

  return variableCount;
}

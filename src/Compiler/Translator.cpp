#include "Translator.h"
#include "TokenAnalysis.h"
#include "SyntaxAnalysis.h"

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

#define   INT(VALUE) (int)(VALUE)
#define FRACT(VALUE) (int)((VALUE - INT(VALUE))*10000)

#define TRANSLATE(STATEMENT)                                             \
  do                                                                    \
    {                                                                   \
      if (token->right)                                                 \
        translateToken(token->right, translator, target, error);        \
      if (token->left )                                                 \
        translateToken(token->left , translator, target, error);        \
      printSpaces(tabs, target);                                        \
      fprintf(target, #STATEMENT "\n");                                  \
    } while (0)

#define START_TRANSLATE(MESSAGE)                \
  do                                            \
    {                                           \
      tabs += TAB;                              \
      printSpaces(tabs, target);                \
      fprintf(target, ";"#MESSAGE"\n");         \
    } while (0)

#define END_TRANSLATE()                         \
  do                                            \
    {                                           \
      printSpaces(tabs, target);                \
      fprintf(target, ";End\n\n");              \
      tabs -= TAB;                              \
    } while (0)

#define   TABS "%*s"
const int TAB = 2;

const int HASNT_PREFIX =  0;
const int BUFFER_SIZE  = 16;

const int DEFAULT_GROWTH_FACTOR = 2;

const int  VIDEO_MEMORY_START =  0;
const int GLOBAL_MEMORY_START = 128;
const int  STACK_MEMORY_START = 256;

const char *const    RETURN_INT_ADDRESS = "rax";
const char *const  RETURN_FRACT_ADDRESS = "rbx";
const char *const STACK_POINTER_ADDRESS = "rcx";
const char *const  STACK_BOTTOM_ADDRESS = "rdx";

static int sizeOfPrefix(const char *string);

static int allocateVariable(db::Token block, db::Translator *translator, int startIndex, FILE *target);
static int allocateParameters(db::Token block, db::Translator *translator, FILE *target);
static int allocateBlock(db::Token block, db::Translator *translator, int startIndex, FILE *target);
static int allocateInstruction(db::Token token, db::Translator *translator, int startIndex, int blockNumber, FILE *target);

static void translateArgument(db::Token block, const db::Translator *translator, FILE *target);

static void translateToken(
                           const db::Token token,
                           const db::Translator *translator,
                           FILE *target,
                           int *error = nullptr
                          );

static void translateFunctions(db::Translator *translator, FILE *target, int *error = nullptr);

static void printSpaces(int count, FILE *target);

static void printSpaces(int count, FILE *target)
{
  assert(count >= 0);

  fprintf(target, "%*s", count, "");
}

void db::initTranslator(db::Translator *translator, int *error)
{
  if (!translator) ERROR();

  unsigned errorCode = 0;
  stack_init(&translator->varTables, 10, &errorCode);
  if (errorCode) ERROR();
}

void db::getTranslator(
                       db::Translator *translator,
                       const char *sourceName,
                       int *error
                      )
{
  if (!translator || !sourceName) ERROR();

  char *buffer = nullptr;
  readFile(&buffer, sourceName);
  if (!buffer) ERROR();

  int hasError = 0;
  translator->tokens = db::getTokens(buffer, &hasError);
  if (hasError || !translator->tokens) { free(buffer); ERROR(); }

  free(buffer);
         /*
  db::Token *tokens = translator->tokens;
  int i = 0;
  for ( ; !IS_END(tokens[i]); ++i)
    {
      switch (tokens[i]->type)
        {
        case db::type_t::STATEMENT:
          {
            printf("OP : '%s' at L:%d P:%d\n", STATEMENT_NAMES[STATEMENT(tokens[i])], tokens[i]->position.line, tokens[i]->position.position);
            break;
          }
        case db::type_t::NUMBER:
          {
            printf("NUM: %lg at L:%d P:%d\n", NUMBER(tokens[i]), tokens[i]->position.line, tokens[i]->position.position);

            break;
          }
        case db::type_t::NAME:
          {
            printf("VAR: '%s' at L:%d P:%d\n", NAME(tokens[i]), tokens[i]->position.line, tokens[i]->position.position);

            break;
          }
        case db::type_t::STRING:
          {
            printf("STR: '%s' at L:%d P:%d\n", STRING(tokens[i]), tokens[i]->position.line, tokens[i]->position.position);

            break;
          }
        default:
          break;
        }
    }
  printf("OP : '%s' at L:%d P:%d\n", STATEMENT_NAMES[STATEMENT(tokens[i])], tokens[i]->position.line, tokens[i]->position.position);
        */
   db::getGrammarly(translator, error);

   free(translator->tokens);
}

void db::removeTranslator(db::Translator *translator, int *error)
{
  if (!translator) ERROR();

  db::removeVarTable(translator);

  for (size_t i = 0; i < translator->functions.size; ++i)
    free(translator->functions.table[i].name);
  free(translator->functions.table);

  db::destroyTree(&translator->grammar);

  unsigned errorCode = 0;
  stack_destroy(&translator->varTables, &errorCode);
  if (errorCode) ERROR();
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
          ";End\n\n",
          STACK_POINTER_ADDRESS,
          STACK_BOTTOM_ADDRESS);

  int errorCode = 0;
  translateToken(translator->grammar.root, translator, target, &errorCode);
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

static void translateToken(
                           const db::Token token,
                           const db::Translator *translator,
                           FILE *target,
                           int *error
                          )
{
  static int    ifCount = 0;
  static int whileCount = 0;

  static int tabs = 0;

  if (!token) ERROR();

  switch (token->type)
    {
    case db::type_t::NUMBER:
      {
        printSpaces(tabs, target);
        fprintf(target, "PUSH %d\n", FRACT(NUMBER(token)));
        printSpaces(tabs, target);
        fprintf(target, "PUSH %d\n", INT(NUMBER(token)));
        break;
      }
    case db::type_t::NAME:
      {
        db::Variable *var = searchVariable(NAME(token), translator, false, true);

        if (var->isGlobal)
          {
            printSpaces(tabs, target);
            fprintf(target, "PUSH [%d]\n", GLOBAL_MEMORY_START+(var->number*2+1));
            printSpaces(tabs, target);
            fprintf(target, "PUSH [%d]\n", GLOBAL_MEMORY_START+(var->number*2  ));
          }
        else
          {
            printSpaces(tabs, target);
            fprintf(target, "PUSH [%d+%s]\n",
                    STACK_MEMORY_START+(var->number*2+2),
                    STACK_BOTTOM_ADDRESS);
            printSpaces(tabs, target);
            fprintf(target, "PUSH [%d+%s]\n",
                    STACK_MEMORY_START+(var->number*2+1),
                    STACK_BOTTOM_ADDRESS);
          }
        break;
      }
    case db::type_t::STRING:
      {
        const char *temp = STRING(token);
        int i = 0;
        for ( ; *temp; ++temp, ++i)
          {
            printSpaces(tabs, target);
            fprintf(target, "PUSH %d; %c\n", *temp, isprint(*temp) ? *temp : '~');
            printSpaces(tabs, target);
            fprintf(target, "POP [%d]\n", i);
          }
        printSpaces(tabs, target);
        fprintf(target, "PUSH %d; %c\n", *temp, isprint(*temp) ? *temp : '~');
        printSpaces(tabs, target);
        fprintf(target, "POP [%d]\n", i);

        break;
      }
    case db::type_t::STATEMENT:
      {
        switch (STATEMENT(token))
          {
          case db::STATEMENT_COMPOUND:
            {
              if (token->left )
                translateToken(token->left , translator, target, error);
              if (token->right)
                translateToken(token->right, translator, target, error);
              break;
            }
          case db::STATEMENT_ADD:
            {
              if (token->right)
                {
                  translateToken(token->right, translator, target, error);
                  translateToken(token->left , translator, target, error);
                }
              else
                {
                  translateToken(token->left , translator, target, error);
                  printSpaces(tabs, target);
                  fprintf(target, "PUSH 0\n");
                  printSpaces(tabs, target);
                  fprintf(target, "PUSH 0\n");
                }
              printSpaces(tabs, target);
              fprintf(target, "ADD\n");
              break;
            }
          case db::STATEMENT_SUB:
            {
              if (token->right)
                {
                  translateToken(token->right, translator, target, error);
                  translateToken(token->left , translator, target, error);
                }
              else
                {
                  translateToken(token->left , translator, target, error);
                  printSpaces(tabs, target);
                  fprintf(target, "PUSH 0\n");
                  printSpaces(tabs, target);
                  fprintf(target, "PUSH 0\n");
                }
              printSpaces(tabs, target);
              fprintf(target, "SUB\n");
              break;
            }
          case db::STATEMENT_MUL:
            TRANSLATE(MUL); break;
          case db::STATEMENT_DIV:
            TRANSLATE(DIV); break;
          case db::STATEMENT_SIN:
            TRANSLATE(SIN); break;
          case db::STATEMENT_COS:
            TRANSLATE(COS); break;
          case db::STATEMENT_SQRT:
            TRANSLATE(SQRT); break;
          case db::STATEMENT_AND:
            TRANSLATE(AND); break;
          case db::STATEMENT_OR:
            TRANSLATE(OR); break;
          case db::STATEMENT_NOT_EQUAL:
            TRANSLATE(NEQL); break;
          case db::STATEMENT_EQUAL:
            TRANSLATE(EQL); break;
          case db::STATEMENT_LESS:
            TRANSLATE(LESS); break;
          case db::STATEMENT_GREATER:
            TRANSLATE(GREATER); break;
          case db::STATEMENT_ASSIGNMENT:
            {
              START_TRANSLATE(Assignment);

              db::Variable *var = searchVariable(NAME(token->left), translator, false, true);

              translateToken(token->right, translator,target, error);
              printSpaces(tabs, target);
              fprintf(target, "COPY\n");

              if (var->isGlobal)
                {
                  printSpaces(tabs, target);
                  fprintf(target, "POP [%d]\n", GLOBAL_MEMORY_START+(var->number*2  ));
                  printSpaces(tabs, target);
                  fprintf(target, "POP [%d]\n", GLOBAL_MEMORY_START+(var->number*2+1));
                }
              else
                {
                  printSpaces(tabs, target);
                  fprintf(target, "POP [%d+%s]\n",
                          STACK_MEMORY_START+(var->number*2+1),
                          STACK_BOTTOM_ADDRESS);
                  printSpaces(tabs, target);
                  fprintf(target, "POP [%d+%s]\n",
                          STACK_MEMORY_START+(var->number*2+2),
                          STACK_BOTTOM_ADDRESS);
                }
              END_TRANSLATE();
              break;
            }
          case db::STATEMENT_IF:
            {
              START_TRANSLATE(If);

              int currentIfNumber = ifCount++;

              translateToken(token->left, translator,target, error);
              printSpaces(tabs, target);
              fprintf(target, "PUSH 0\n");
              printSpaces(tabs, target);
              fprintf(target, "PUSH 0\n");
              printSpaces(tabs, target);
              fprintf(target, "JE :ELSE_%6.6d\n\n", currentIfNumber);

              if (IS_ELSE(token->right))
                translateToken(token->right->left , translator, target, error);
              else
                translateToken(token->right, translator, target, error);

              printSpaces(tabs, target);
              fprintf(target, "JMP END_IF_%6.6d\n", currentIfNumber);
              printSpaces(tabs, target);
              fprintf(target, "ELSE_%6.6d:\n", currentIfNumber);

              if (IS_ELSE(token->right))
                translateToken(token->right->right, translator, target, error);

              printSpaces(tabs, target);
              fprintf(target, "END_IF_%6.6d:\n", currentIfNumber);

              END_TRANSLATE();
              break;
            }
          case db::STATEMENT_WHILE:
            {
              START_TRANSLATE(While);

              printSpaces(tabs, target);
              fprintf(target, "WHILE_%6.6d:\n\n", whileCount);

              translateToken(token->left, translator, target, error);
              printSpaces(tabs, target);
              fprintf(target, "PUSH 0\n");
              printSpaces(tabs, target);
              fprintf(target, "PUSH 0\n");
              printSpaces(tabs, target);
              fprintf(target, "JE :END_WHILE_%6.6d\n\n", whileCount);

              translateToken(token->right, translator, target, error);
              printSpaces(tabs, target);
              fprintf(target, "JMP WHILE_%6.6d\n", whileCount);
              printSpaces(tabs, target);
              fprintf(target, "END_WHILE_%6.6d:\n", whileCount++);

              END_TRANSLATE();
              break;
            }
          case db::STATEMENT_RETURN:
            {
              START_TRANSLATE(Return);

              translateToken(token->right, translator, target, error);
              printSpaces(tabs, target);
              fprintf(target, "POP %s\n",   RETURN_INT_ADDRESS);
              printSpaces(tabs, target);
              fprintf(target, "POP %s\n", RETURN_FRACT_ADDRESS);

              printSpaces(tabs, target);
              fprintf(target, ";Update stack pointer\n");
              printSpaces(tabs, target);
              fprintf(target, "PUSH %s\n", STACK_BOTTOM_ADDRESS);
              printSpaces(tabs, target);
              fprintf(target, "POP %s\n", STACK_POINTER_ADDRESS);

              printSpaces(tabs, target);
              fprintf(target, ";Pop return address\n");
              printSpaces(tabs, target);
              fprintf(target, "PUSH [%d+%s]\n",
                      STACK_MEMORY_START,
                      STACK_POINTER_ADDRESS);

              printSpaces(tabs, target);
              fprintf(target, "RET\n");

              END_TRANSLATE();
              break;
            }
          case db::STATEMENT_CALL:
            {
              START_TRANSLATE(Call);

              printSpaces(tabs, target);
              fprintf(target, ";Save stack pointer\n");
              printSpaces(tabs, target);
              fprintf(target, "PUSH %s\n", STACK_BOTTOM_ADDRESS);

              translateArgument(token->left->left, translator, target);

              printSpaces(tabs, target);
              fprintf(target, "CALL FUN_%s\n", NAME(token->left));

              printSpaces(tabs, target);
              fprintf(target, ";Pop stack pointer\n");
              printSpaces(tabs, target);
              fprintf(target, "POP %s\n\n", STACK_BOTTOM_ADDRESS);

              printSpaces(tabs, target);
              fprintf(target, "PUSH %s\n", RETURN_FRACT_ADDRESS);
              printSpaces(tabs, target);
              fprintf(target, "PUSH %s\n",   RETURN_INT_ADDRESS);

              END_TRANSLATE();
              break;
            }
          case db::STATEMENT_VAL:
          case db::STATEMENT_VAR:
            {
              START_TRANSLATE(Var/Val);

              db::Variable *var = searchVariable(NAME(token->left), translator, false, true);

              if (var->isGlobal)
                {
                  translateToken(token->right, translator, target, error);
                  printSpaces(tabs, target);
                  fprintf(target, "POP [%d]\n", GLOBAL_MEMORY_START+(var->number*2  ));
                  printSpaces(tabs, target);
                  fprintf(target, "POP [%d]\n", GLOBAL_MEMORY_START+(var->number*2+1));
                }
              else
                {
                  translateToken(token->right, translator, target, error);
                  printSpaces(tabs, target);
                  fprintf(target, "POP [%d+%s]\n",
                          STACK_MEMORY_START+(var->number*2+1),
                          STACK_BOTTOM_ADDRESS);
                  printSpaces(tabs, target);
                  fprintf(target, "POP [%d+%s]\n",
                          STACK_MEMORY_START+(var->number*2+2),
                          STACK_BOTTOM_ADDRESS);
                }

              END_TRANSLATE();
              break;
            }
          case db::STATEMENT_OUT:
            {
              START_TRANSLATE(Out);

              for (
                   db::Token temp = token->left;
                   temp;
                   temp = temp->right)
                {
                  translateToken(temp->left, translator, target, error);
                  if (IS_STRING(temp->left))
                    {
                      printSpaces(tabs, target);
                      fprintf(target, "SHOW\n");
                    }
                  else if(IS_ENDL(temp->left))
                    {
                      printSpaces(tabs, target);
                      fprintf(target, "PUSH %d; \\n\n", '\n');
                      printSpaces(tabs, target);
                      fprintf(target, "POP [0]\n");
                      printSpaces(tabs, target);
                      fprintf(target, "PUSH %d; \\0\n", '\0');
                      printSpaces(tabs, target);
                      fprintf(target, "POP [1]\n");
                      printSpaces(tabs, target);
                      fprintf(target, "SHOW\n");
                    }
                  else
                    {
                      printSpaces(tabs, target);
                      fprintf(target, "OUT\n");
                    }
                }

              END_TRANSLATE();
              break;
            }
          case db::STATEMENT_IN:
            {
              START_TRANSLATE(In);

              for (
                   db::Token temp = token->left;
                   temp;
                   temp = temp->right)
                {
                  db::Variable *var = searchVariable(NAME(temp->left), translator, false, true);

                  printSpaces(tabs, target);
                  fprintf(target, "IN\n");

                  if (var->isGlobal)
                    {
                      printSpaces(tabs, target);
                      fprintf(target, "POP [%d]\n", GLOBAL_MEMORY_START+(var->number*2  ));
                      printSpaces(tabs, target);
                      fprintf(target, "POP [%d]\n", GLOBAL_MEMORY_START+(var->number*2+1));
                    }
                  else
                    {
                      printSpaces(tabs, target);
                      fprintf(target, "POP [%d+%s]\n",
                              STACK_MEMORY_START+(var->number*2+1),
                              STACK_BOTTOM_ADDRESS);
                      printSpaces(tabs, target);
                      fprintf(target, "POP [%d+%s]\n",
                              STACK_MEMORY_START+(var->number*2+2),
                              STACK_BOTTOM_ADDRESS);
                    }
                }
              END_TRANSLATE();
              break;
            }
          case db::STATEMENT_NEW_LINE:
          case db::STATEMENT_NOT:
          case db::STATEMENT_COMMA:
          case db::STATEMENT_PARAMETER:
          case db::STATEMENT_VOID:
          case db::STATEMENT_START_BRACE:
          case db::STATEMENT_END_BRACE:
          case db::STATEMENT_TYPE:
          case db::STATEMENT_FUN:  case db::STATEMENT_COLON:
          case db::STATEMENT_OPEN: case db::STATEMENT_CLOSE:
          case db::STATEMENT_END : case db::STATEMENT_ERROR:
          case db::STATEMENT_SEMICOLON:
          case db::STATEMENT_ELSE:
            break;
          default:
            {
              handleError("Unknown STATEMENT(%s) at File: %s at Line: %d",
                          db::STATEMENT_NAMES[STATEMENT(token)], __FILE__, __LINE__);
              ERROR();
            }
          }
        break;
      }
    default:
      {
        handleError("Unknown type at File: %s at Line: %d", __FILE__, __LINE__);
        ERROR();
      }
    }
}

static void translateFunctions(db::Translator *translator, FILE *target, int *error)
{
  if (!translator || !target) ERROR();

  for (size_t i = 0; i < translator->functions.size; ++i)
    {
      db::Function *function = translator->functions.table+i;

      fprintf(target,
              ";Function\n"
              "FUN_%s:\n",
              function->name);

      int offset = 1;
      fprintf(target,
              ";Save return address\n"
              "POP [%d+%s]\n",
              STACK_MEMORY_START,
              STACK_POINTER_ADDRESS);

      db::addVarTable(translator);

      offset +=
        allocateParameters(function->token->left->left, translator, target);
      offset +=
        allocateVariable(function->token->right       , translator, offset, target);
      fprintf(target,
              ";Save stack pointer\n"
              "PUSH %s\n"
              "POP %s\n",
              STACK_POINTER_ADDRESS, STACK_BOTTOM_ADDRESS);
      fprintf(target,
              ";Turn off real-calc\n"
              "PUSH 0\n"
              "POP rex\n"
              ";Update stack pointer\n"
              "PUSH %d\n"
              "PUSH %s\n"
              "ADD\n"
              "POP %s\n"
              ";Turn on real-calc\n"
              "PUSH 1\n"
              "POP rex\n",
              offset, STACK_POINTER_ADDRESS, STACK_POINTER_ADDRESS);

      int errorCode = 0;
      translateToken(function->token->right, translator, target, &errorCode);
      db::removeVarTable(translator);

      if (errorCode) ERROR();

      fprintf(target,
              ";Update stack pointer\n"
              "PUSH %s\n"
              "POP %s\n",
              STACK_BOTTOM_ADDRESS, STACK_POINTER_ADDRESS);
      fprintf(target,
              ";Pop return address\n"
              "PUSH [%d+%s]\n",
              STACK_MEMORY_START, STACK_POINTER_ADDRESS);

      fprintf(target, "RET\n;End\n\n");
    }
}

static void translateArgument(db::Token block, const db::Translator *translator, FILE *target)
{
  if (!block) return;

  if (block->right)
    translateArgument(block->right, translator, target);

  if (block->left )
    translateToken(block->left, translator, target);
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
      case db::STATEMENT_VAL:
      case db::STATEMENT_VAR:
        {
          sprintf(buffer, "%d_", blockNumber);

          db::addVariable(
                          NAME(token->left),
                          IS_VAL(token),
                          translator,
                          variableCount+startIndex/2,
                          buffer
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
      case db::STATEMENT_NOT: case db::STATEMENT_NEW_LINE:
      case db::STATEMENT_AND: case db::STATEMENT_OR:
      case db::STATEMENT_NOT_EQUAL: case db::STATEMENT_EQUAL:
      case db::STATEMENT_SQRT:
      case db::STATEMENT_OUT: case db::STATEMENT_IN:
      case db::STATEMENT_LESS: case db::STATEMENT_GREATER:
      case db::STATEMENT_COMMA:
      case db::STATEMENT_PARAMETER: case db::STATEMENT_VOID:
      case db::STATEMENT_START_BRACE: case db::STATEMENT_END_BRACE:
      case db::STATEMENT_TYPE:  case db::STATEMENT_FUN:
      case db::STATEMENT_COLON:  case db::STATEMENT_OPEN:
      case db::STATEMENT_CLOSE: case db::STATEMENT_END:
      case db::STATEMENT_ERROR:  case db::STATEMENT_SEMICOLON:
      case db::STATEMENT_ELSE: case db::STATEMENT_CALL:
      case db::STATEMENT_RETURN: case db::STATEMENT_ASSIGNMENT:
      case db::STATEMENT_ADD: case db::STATEMENT_SUB:
      case db::STATEMENT_MUL: case db::STATEMENT_DIV:
      case db::STATEMENT_SIN: case db::STATEMENT_COS:
      default: break;
      }

  return variableCount;
}

bool db::addFunction(
                     const char *name,
                     db::Token function,
                     db::Translator *translator,
                     int *error
                    )
{
  if (!name || !translator) ERROR(false);

  if (searchFunction(name, translator, error))
    return false;

  if (translator->functions.size == translator->functions.capacity)
    {
      if (!translator->functions.capacity)
        ++translator->functions.capacity;
      translator->functions.capacity *= DEFAULT_GROWTH_FACTOR;
      db::Function *temp =
        (db::Function *)recalloc(
                                 translator->functions.table,
                                 translator->functions.capacity,
                                 sizeof(db::Function)
                                );

      if (!temp)
        {
          free(translator->functions.table);
          translator->functions.table = nullptr;

          translator->functions.capacity = 0;
          translator->functions.size     = 0;

          return false;
        }

      translator->functions.table = temp;
    }

  translator->functions.table
    [translator->functions.size++] = {
    .name  = strdup(name),
    .token = function
  };

  return true;
}

bool db::addVariable(
                     const char *name,
                     bool isConst,
                     Translator *translator,
                     int number,
                     const char *prefix,
                     int *error
                    )
{
  if (!name || !translator || !prefix) ERROR(false);

  if (searchVariable(name, translator, true, false, error))
    return false;

  unsigned errorCode = 0;
  db::VarTable *table = stack_top(&translator->varTables, &errorCode);
  if (errorCode) ERROR(false);

  if (table->size == table->capacity)
    {
      if (!table->capacity)
        ++table->capacity;
      table->capacity *= DEFAULT_GROWTH_FACTOR;
      db::Variable *temp =
        (db::Variable *)recalloc(
                                 table->table,
                                 table->capacity,
                                 sizeof(db::Variable)
                                );

      if (!temp)
        {
          for (size_t i = 0; i < table->size; ++i)
            free(table->table[i].name);
          free(table->table);
          table->table = nullptr;

          table->capacity = 0;
          table->size     = 0;

          return false;
        }

      table->table = temp;
    }

  bool isGlobal = (stack_size(&translator->varTables) == 1);

  static int countOfGlobal = 0;

  table->table
    [table->size++] = {
    .name    = strnigDuplicate(prefix, name),
    .number  = isGlobal ? countOfGlobal++ : number,
    .isConst = isConst,
    .isGlobal = isGlobal
  };

  return true;
}

db::Token db::searchFunction(
                             const char *name,
                             const db::Translator *translator,
                             int *error
                            )
{
  if (!name || !translator) ERROR(nullptr);

  for (size_t i = 0; i < translator->functions.size; ++i)
    if (!strcmp(translator->functions.table[i].name, name))
      return translator->functions.table[i].token;

  return nullptr;
}

db::Variable *db::searchVariable(
                                 const char *name,
                                 const db::Translator *translator,
                                 bool onlyTop,
                                 bool hasPrefix,
                                 int *error
                                )
{
  if (!name || !translator) ERROR(nullptr);

  unsigned errorCode = 0;
  for (int i = (int)stack_size(&translator->varTables) - 1; i >= 0; --i)
    {
      db::VarTable *table = stack_get(&translator->varTables, (unsigned)i, &errorCode);

      for (int j = (int)table->size-1; j >= 0; --j)
        {
          int prefixSize = sizeOfPrefix(table->table[j].name);
          char *temp = table->table[j].name +
            (hasPrefix ? prefixSize : 0);
          if (!strcmp(temp, name))
            return &table->table[j];
        }

      if (onlyTop) break;
    }

  return nullptr;
}

void db::addVarTable(db::Translator *translator, int *error)
{
  if (!translator) ERROR();

  db::VarTable *newTable = (db::VarTable *)calloc(1, sizeof(db::VarTable));
  if (!newTable) ERROR();

  unsigned errorCode = 0;
  stack_push(&translator->varTables, newTable, &errorCode);
  if (errorCode) ERROR();
}

void db::removeVarTable(db::Translator *translator, int *error)
{
  if (!translator) ERROR();

  unsigned errorCode = 0;
  db::VarTable *table = stack_pop(&translator->varTables, &errorCode);
  if (errorCode || !table) ERROR();

  for (size_t i = 0; i < table->size; ++i)
    {
      free(table->table[i].name);
      table->table[i].name = nullptr;
    }
  free(table->table);
  free(table);
}

static int sizeOfPrefix(const char *string)
{
  if (!string) return HASNT_PREFIX;

  int size = 0;
  while (*string && *string != '_') { ++string; ++size; }

  if (*string != '_') return HASNT_PREFIX;
  return size + 1;
}

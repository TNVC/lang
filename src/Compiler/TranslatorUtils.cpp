#include "Translator.h"
#include "TokenAnalysis.h"
#include "SyntaxAnalysis.h"

#include "Fiofunctions.h"
#include "StringsUtils.h"
#include "SystemLike.h"
#include "Error.h"
#include "DSL.h"
#include <malloc.h>
#include <string.h>

const int HASNT_PREFIX =  0;
const int BUFFER_SIZE  = 16;

const int DEFAULT_GROWTH_FACTOR = 2;

void db::initTranslator(db::Translator *translator, int *error)
{
  if (!translator) ERROR();

  unsigned errorCode = 0;
  stack_init(&translator->varTables, 10, &errorCode);
  if (errorCode) ERROR();

  translator->status.returnType = db::ReturnType::None;
  translator->status.hasMain = false;
}

void db::removeTranslator(db::Translator *translator, int *error)
{
  if (!translator) ERROR();

  db::removeVarTable(translator);

  free(translator->functions.table);
  free(translator->previousStaticBlocks.table);
  free(translator->    nextStaticBlocks.table);

  db::destroyTree(&translator->grammar);

  db::destroyStringPool(&translator->stringPool, error);

  unsigned errorCode = 0;
  stack_destroy(&translator->varTables, &errorCode);
  if (errorCode) ERROR();
}

void db::getTranslator(
                       db::Translator *translator,
                       const char *sourceName,
                       int *error
                      )
{
  if (!translator || !sourceName) ERROR();

  translator->status.sourceName = sourceName;

  char *buffer = nullptr;
  readFile(&buffer, sourceName);
  if (!buffer) ERROR();

  int hasError = 0;
  translator->tokens =
    db::getTokens(buffer, &translator->stringPool, &hasError);
  if (hasError || !translator->tokens) { free(buffer); ERROR(); }

  free(buffer);
                 /*
  db::Token *tokens = translator->tokens;
  int i =10;
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
    .name  = name,
    .token = function
  };

  return true;
}

bool db::addStatic(db::Token staticBlock, db::Translator *translator, int *error)
{
  if (!translator) ERROR(false);

  db::FunTable *table = nullptr;

  if (translator->status.hasMain)
    table = &translator->    nextStaticBlocks;
  else
    table = &translator->previousStaticBlocks;

  if (table->size == table->capacity)
    {
      if (!table->capacity)
        ++table->capacity;
      table->capacity *= DEFAULT_GROWTH_FACTOR;
      db::Function *temp =
        (db::Function *)recalloc(
                                 table->table,
                                 table->capacity,
                                 sizeof(db::Function)
                                );

      if (!temp)
        {
          free(table->table);
          table->table = nullptr;

          table->capacity = 0;
          table->size     = 0;

          return false;
        }

      table->table = temp;
    }

  table->table
    [table->size++] = {
    .name  = NAME(staticBlock->left),
    .token = staticBlock
  };

  return true;
}

bool db::addVariable(
                     const char *name,
                     bool isConst,
                     Translator *translator,
                     int number,
                     int *error
                    )
{
  if (!name || !translator) ERROR(false);

  if (searchVariable(name, translator, true, error))
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
    .name    = name,
    .number  = isGlobal ? countOfGlobal++ : number,
    .isConst = isConst,
    .isGlobal = isGlobal
  };

  return true;
}

bool db::isStatic(
                  const db::Token token,
                  const db::Translator *translator,
                  int *error
                 )
{
  if (!token || !translator) ERROR(false);

  const db::FunTable *table =
    &translator->previousStaticBlocks;

  for (size_t i = 0; i < table->size; ++i)
    if (token == table->table[i].token) return true;

  table = &translator->nextStaticBlocks;

  for (size_t i = 0; i < table->size; ++i)
    if (token == table->table[i].token) return true;

  return false;
}

db::Token db::searchFunction(
                             const char *name,
                             const db::Translator *translator,
                             int *error
                            )
{
  if (!name || !translator) ERROR(nullptr);

  for (size_t i = 0; i < translator->functions.size; ++i)
    if (db::compareStrings(
                           translator->functions.table[i].name,
                           name
                          ))
      return translator->functions.table[i].token;

  return nullptr;
}

db::Variable *db::searchVariable(
                                 const char *name,
                                 const db::Translator *translator,
                                 bool onlyTop,
                                 int *error
                                )
{
  if (!name || !translator) ERROR(nullptr);

  unsigned errorCode = 0;
  for (int i = (int)stack_size(&translator->varTables) - 1; i >= 0; --i)
    {
      db::VarTable *table = stack_get(&translator->varTables, (unsigned)i, &errorCode);

      for (int j = (int)table->size-1; j >= 0; --j)
          if (db::compareStrings(table->table[j].name, name))
            return &table->table[j];

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

  free(table->table);
  free(table);
}

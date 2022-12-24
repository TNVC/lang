#pragma once

#include "Tree.h"
#include <stddef.h>
#include <stdio.h>
#include "Stack.h"

namespace db {

  const int COUNT_OF_STATIC_BLOCK_TYPE = 2;

  enum class ReturnType {
    Type,
    Void,
    None,
  };

  struct TranslatorStatus {
    const char *sourceName;
    ReturnType returnType;
    bool hasMain;
    int stackOffset;
  };

  struct Translator {
    Stack    varTables;
    FunTable functions;

    FunTable previousStaticBlocks;
    FunTable     nextStaticBlocks;

    TranslatorStatus status;

    Token *tokens;
    Grammar grammar;

    StringPool stringPool;

    Translator &operator=(const Translator &original) = delete;
  };

  void disassemblerGrammar(const Translator *translator, FILE *target, int *error = nullptr);

  void simplyGrammar(Translator *translator, int *error = nullptr);

  void saveGrammary(const Translator *translator, FILE *target, int *error = nullptr);

  void initTranslator(Translator *translator, int *error = nullptr);

  void getTranslator(
                     Translator *translator,
                     const char *sourceName,
                     int *error = nullptr
                     );

  void saveTranslator(
                      Translator *translator,
                      FILE *target,
                      int *error = nullptr
                     );

  void loadTranslator(
                      Translator *translator,
                      FILE *source,
                      int *error = nullptr
                     );

  void removeTranslator(
                        Translator *translator,
                        int *error = nullptr
                       );

  bool translate(Translator *translator, FILE *target, int *error = nullptr);

  bool addFunction(
                   const char *name,
                   Token function,
                   Translator *translator,
                   int *error = nullptr
                  );

  bool addStatic(Token staticBlock, Translator *translator, int *error = nullptr);

  bool addVariable(
                   const char *name,
                   bool isConst,
                   Translator *translator,
                   int number,
                   int *error = nullptr
                  );

  Token searchFunction(
                       const char *name,
                       const Translator *translator,
                       int *error = nullptr
                      );
  Variable *searchVariable(
                           const char *name,
                           const Translator *translator,
                           bool onlyTop = false,
                           int *error = nullptr
                          );

  bool isStatic(
                const Token token,
                const Translator *translator,
                int *error = nullptr
               );


  void    addVarTable(Translator *translator, int *error = nullptr);
  void removeVarTable(Translator *translator, int *error = nullptr);

}

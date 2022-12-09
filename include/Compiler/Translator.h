#pragma once

#include "Tree.h"
#include <stddef.h>
#include <stdio.h>
#include "Stack.h"

namespace db {

  struct Translator {
    Stack varTables;
    FunTable functions;
    Token *tokens;
    Grammar grammar;

    Translator &operator=(const Translator &original) = delete;
  };

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
  bool addVariable(
                   const char *name,
                   bool isConst,
                   Translator *translator,
                   int number,
                   const char *prefix = "",
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
                           bool hasPrefix = false,
                           int *error = nullptr
                          );

  void    addVarTable(Translator *translator, int *error = nullptr);
  void removeVarTable(Translator *translator, int *error = nullptr);

}

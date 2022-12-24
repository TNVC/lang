#include "Compiler.h"
#include "Translator.h"

#include "Tree.h"

#include <stdio.h>
#include "Settings.h"
#include "StringsUtils.h"

#include "Logging.h"

bool init()
{
  return true;
}

void start()
{
  Settings settings{};
  getSettings(&settings);

  int error = 0;

  db::Translator translator{};

  db::initTranslator(&translator);

  FILE *source = fopen(settings.source, "r");
  if (!source) { db::removeTranslator(&translator); return; }

  db::loadTranslator(&translator, source);

  fclose(source);

  FILE *target = fopen(settings.target, "w");
  if (!target) { db::removeTranslator(&translator); return; }

  db::simplyGrammar(&translator, &error);
  if (error)
    {
      fclose(target);
      db::removeTranslator(&translator);
      return;
    }

  db:: dumpTree(&translator.grammar, 0, getLogFile());

  rewind(target);
  db::saveTranslator(&translator, target);

  fclose(target);
  db::removeTranslator(&translator);
}

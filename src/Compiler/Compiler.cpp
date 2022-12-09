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

  db::getTranslator(
                    &translator,
                    settings.source,
                    &error
                   );
  if (error) { db::removeTranslator(&translator); return; }

  FILE *target = fopen(settings.target, "w");
  if (!target) { db::removeTranslator(&translator); return; }

  db:: dumpTree(&translator.grammar, 0, getLogFile());

  FILE *middleEndFile = fopen("resources/grammar.txt", "w");
  if (!middleEndFile) { fclose(target); db::removeTranslator(&translator); return; }
  db::saveTranslator(&translator, middleEndFile);

  db::translate(&translator, target, &error);
  if (error)
    {
      fclose(target);
      db::removeTranslator(&translator);
      return;
    }

  db::removeTranslator(&translator);
}

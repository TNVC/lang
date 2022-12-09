#include "ConsoleArgsUtils.h"
#include "Settings.h"

#include "Compiler.h"

int main(const int argc, const char * const argv[])
{
  Settings settings = {};

  if (parseConsoleArgs(argc, argv, &settings))
    return 0;

  setSettings(&settings);

  if (!init())
    return 0;

  start();

  return 0;
}

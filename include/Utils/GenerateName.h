#pragma once

const char *const DEFAULT_PREFIX    = "prefix";
const char *const DEFAULT_SUFFIX    = "suffix";
const char *const DEFAULT_SEPARATOR = "_";

char *generateName(
                   const char *prefix = DEFAULT_PREFIX,
                   const char *suffix = DEFAULT_SUFFIX,
                   const char * firstSeparator = DEFAULT_SEPARATOR,
                   const char *secondSeparator = DEFAULT_SEPARATOR
                   );

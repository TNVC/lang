#pragma once

/// Name of default directory for files
const char * const DEFAULT_DIRECTORY = "../resources/";
/// Name of target file if didn`t input anything
const char * const DEFAULT_TARGET_FILE_NAME = "dis_main.kt";
/// Name of source file if didn`t input anything
const char * const DEFAULT_SOURCE_FILE_NAME = "syntax.std";

enum class Save {
  TEXT,
  TEX,
};

struct Settings {
  char       *source;
  char       *target;
  const char *programName;
};

void setSettings(const Settings *settings);

void getSettings(Settings *settings);

/// Adder prefix
/// @param [in] name C-like string
/// @return Dimanic allocate C-like with DEFAULT_DIRECTORY like prefix
char *addDirectory(const char *name);

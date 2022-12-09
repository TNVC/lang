#pragma once

#include "Settings.h"

/// Errors whitch maight be return from parseConsoleArgs
enum ConsolArgsUtilsError {
  CONSOLE_NO_SUCH_FILE_FOUND  = 0x01 << 0, /// <- File not exists
  CONSOLE_NO_INPUT_FILES      = 0x01 << 1, /// <- Not input files
  CONSOLE_INCORRECT_ARGUMENTS = 0x01 << 2, /// <- Arguments for flags isn`t correct
  CONSOLE_FAIL_TO_OPEN        = 0x01 << 3, /// <- Fail to open file
  CONSOLE_HELP                = 0x01 << 4,
  CONSOLE_UNEXPECTED_ERROR    = 0x01 << 5,
};

/// Parse console argument for settings
/// @param [in] argc Count of console arguments
/// @param [in] argv Console arguments value
/// @return Error`s code
int parseConsoleArgs(const int argc, const char * const argv[], Settings *settings);

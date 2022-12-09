#include "ErrorHandler.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "ColorOutput.h"
#include "Settings.h"

/// Handle message
/// @brief Show message with prefix in console with color from name of the programm
///        After message reset color
///        Template: [set test color to color]progran-name: prefix: message\\n[set color to default]
///
/// @param [in] message Format C-like string with message
/// @param [in] args VA_ARGS for format string
/// @param [in] color Color of message
/// @param [in] prefix Prefix will print before message
/// @note If message is nullptr set it to "MESSAGE CORRUPTED!!"
static void handleMessage(const char *message, va_list args,  const char *color, const char *prefix);

void handleError(const char *message, ...)
{
  va_list args = {};

  va_start(args, message);

  handleMessage(message, args, FG_RED, "Error");

  va_end(args);
}

void handleWarning(const char *message, ...)
{
  va_list args = {};

  va_start(args, message);

  handleMessage(message, args, FG_YELLOW, "Warning");

  va_end(args);
}

static void handleMessage(const char *message, va_list args, const char *color, const char *prefix)
{
  Settings settings = {};

  getSettings(&settings);

  if (!message)
    message = "MESSAGE CORRUPTED!!";

  printf("%s", color);

  printf("%s: ", settings.programName);

  printf("%s: ", prefix);

  vprintf(message, args);

  printf("\n" RESET);
}

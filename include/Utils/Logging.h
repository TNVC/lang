/// @brief Define one of next defination for declarate log level:\n
/// RELEASE_LOG_LEVEL - logging only fatal errors\n
/// ERROR_LOG_LEVEL   - logging only errors\n
/// MESSAGE_LOG_LEVEL - logging errors and messages\n
/// VALUE_LOG_LEVEL   - logging errors, messages and values
///
#pragma once

#include <stdio.h>

#define LOG_INFO(EXPRESSION) #EXPRESSION, __FILE__, __func__, __LINE__

#define LOG_DIRECTORY ".log/"

#define LOG_FILE_PREFIX "log"

#define LOG_FILE_SUFFIX "html"

unsigned enum LogLevel {
  FATAL   = (0x01 << 0),
  ERROR   = (0x01 << 1),
  WARNING = (0x01 << 2),
  MESSAGE = (0x01 << 3),
  VALUE   = (0x01 << 4),
};

/// Getter for LOG_FILE
/// @return LOG_FILE or NULL if fail to open file
/// @note If log file bigger than 1GB close it and open new file and save descriptor in LOG_FILE\n
/// If was error in open file set LOG_LEVEL to 0
FILE *getLogFile();

#ifndef RELEASE_BUILD_

#define logValue(value)                                     \
  loggingPrint(VALUE   , value      , LOG_INFO(value))

#define logMessage(message)                                 \
  loggingPrint(MESSAGE , message    , LOG_INFO(message))

#define logWarning(warning)                                 \
  logggingPrint(WARNING, #warning   , LOG_INFO(warning))

#define logError(expression)                                \
  loggingPrint(ERROR   , #expression, LOG_INFO(expression))

#else

#define logValue(value)      ;

#define logMessage(message)  ;

#define logWarning(warning)  ;

#define logError(expression) ;

#endif

#define logFatal(expression)                              \
  loggingPrint(FATAL, #expression, LOG_INFO(expression))


/// Print log info for decimal
/// @param [in] level Level of current log`s print
/// @param [in] value Deciamal value
/// @param [in] name C-like string with value
/// @param [in] fileName Name of file where was call function
/// @param [in] functionName Name of function wher was call function
/// @param [in] line Number of line where was call function
/// @return Count of print chars
int loggingPrint(unsigned level, long long value, const char *name,
                 const char *fileName, const char *functionName, int line);

/// Print log info for double
/// @param [in] level Level of current log`s print
/// @param [in] value Double value
/// @param [in] name C-like string with value
/// @param [in] fileName Name of file where was call function
/// @param [in] functionName Name of function wher was call function
/// @param [in] line Number of line where was call function
/// @return Count of print chars
int loggingPrint(unsigned level, double value, const char *name,
                 const char *fileName, const char *functionName, int line);

/// Print log info for char
/// @param [in] level Level of current log`s print
/// @param [in] value Char value
/// @param [in] name C-like string with value
/// @param [in] fileName Name of file where was call function
/// @param [in] functionName Name of function wher was call function
/// @param [in] line Number of line where was call function
/// @return Count of print chars
int loggingPrint(unsigned level, char value, const char *name,
                 const char *fileName, const char *functionName, int line);

/// Print log info for pointer
/// @param [in] level Level of current log`s print
/// @param [in] value Pointer value
/// @param [in] name C-like string with value
/// @param [in] fileName Name of file where was call function
/// @param [in] functionName Name of function wher was call function
/// @param [in] line Number of line where was call function
/// @return Count of print chars
int loggingPrint(unsigned level, const void *value,const char *name,
                 const char *fileName, const char *functionName, int line);

/// Print log info for C-like string
/// @param [in] level Level of current log`s print
/// @param [in] value C-like string value
/// @param [in] name C-like string with value
/// @param [in] fileName Name of file where was call function
/// @param [in] functionName Name of function wher was call function
/// @param [in] line Number of line where was call function
/// @return Count of print chars
int loggingPrint(unsigned level, const char *value, const char *name,
                 const char *fileName, const char *functionName, int line);

#pragma once

#include <stdarg.h>

/// Handle error
/// @param [in] message Error message - C-like foramt string ofr printf
/// @param [in] ... VA_ARGS for format string
/// @note Add before message name of the programm and "Error"
void handleError  (const char *message, ...);

/// Handle warning
/// @param [in] message Warning message - C-like foramt string ofr printf
/// @param [in] ... VA_ARGS for format string
/// @note Add before message name of the programm and "Warring"
void handleWarning(const char *message, ...);

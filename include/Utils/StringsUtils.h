#pragma once

#include <stddef.h>
#include "Line.h"

enum StringsutilsError {
  STRINGSUTILS_INCORRECT_ARGUMENTS = -1,
};

/// Parse chars buffer to lines (separator = '\n')
/// @param [in/out] buffer Char buffer to parse
/// @parma [in] bufferSize Size of char buffer
/// @param [out] lineCount Count of lines in buffer
/// @return Strings-struct array in dinamic memory or nullptr if was error
String *parseToLines(char *buffer, size_t bufferSize, size_t *lineCount);

/// Compare two C-like strings
/// @param [in] first First string for compare
/// @parma [in] second Second string for compare
/// @return 0 if strings equal, positive if first bigger than second
///         and negative if second bigger than first
int stricmp(const char *first, const char *second);

/// Compare two C-like strings
/// @param [in] first First string for compare
/// @parma [in] second Second string for compare
/// @param [in] maxSize Max count of chars to compare
/// @return 0 if strings equal, positive if first bigger than second
///         and negative if second bigger than first
int strincmp(const char *first, const char *second, int maxSize);

/// Check that string contains only space chars
/// @param [in] string C-like string for check
/// @return 1 if is empty or 0 if is not
int isStringEmpty(const char *string);

/// Compare two C-like string up to determinator
/// @param [in] first First C-like string for compare
/// @param [in] second Second C-like string for compare
/// @param [in] determinator Addition determinator to '\\0'
/// @return negative value if first less than second,
/// positive value if first bigger than second oe zero if first equals second
int strcmpto(const char *first, const char *second, char determinant);

/// Check that C-like string between start and end has spaces chars
/// @param [in] start First char of C-like string
/// @param [in] end First char after C-like string
/// @return 1 if has or 0 if hasn`t
int hasSpace(const char *start, const char *end);

/// Check that C-like string between start and end has correct C name
/// @param [in] start First char of C-like string
/// @param [in] end First char after C-like string
/// @return 1 if has or 0 if hasn`t
/// @note Correct name is name contains only alphas, digits, lower lines and dollars
int isCorrectName(const char *start, const char *end);

/// Check that C-like string has correct C name
/// @param [in] string of C-like string
/// @return 1 if has or 0 if hasn`t
/// @note Correct name is name contains only alphas, digits, lower lines and dollars
int isCorrectName(const char *string);

/// Remove all back space chars
/// @param [in/out] string C-like string for trim
/// @return Zero if wasn`t any errors
int trimString(char *string);

/// Check that all chars are digit
/// @param [in/out] string C-like string for check
/// @return Zero if wasn`t any errors
int isDigitString(const char *string);

/// Duplicate n chars from string to dinamic memory
/// @param [in/out] string C-like string for check
/// @param [in] size Max count of chars to copy
/// @return Nullptr if wasn`t any errors
char *strndup(const char *string, int size);

/// Duplicate first and seconds strings to dinamic memory
/// @param [in/out] first C-like string for duplicate
/// @param [in/out] second C-like string for duplicate and contactiation
/// @return Nullptr if wasn`t any errors
char *strnigDuplicate(const char *first, const char *second);

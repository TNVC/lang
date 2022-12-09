#pragma once

#include <stddef.h>

/// Unmodified C-like string with its length
typedef struct {
    char *buff; ///<- Pointer to C-like string
    size_t size;///<- Size of string in buffer
} String;

/// Struct for work with text from file
typedef struct {
    char   *originBuffer;///<- File-buffer
    size_t size;         ///<- Size of originBuffer
    String *sequence;    ///<- Current sequence of string
    size_t stringsCount; ///<- Count of strings in sequence
} Strings;

///Init strings
///@param [out] strings Struct with info about text from file
///@note Need to call before all using strings
void initStrings(Strings *strings);

///Destroy strings
///@param [in] strings Struct with info about text from file
///@note Need to call after all using strings
void destroyStrings(Strings *strings);

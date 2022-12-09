#pragma once

#include <stddef.h>

/// Combine realloc and calloc
/// @param [in] pointer Pointer to dimanic memory which was get from malloc/calloc/realloc/recalloc or else
/// @param [in] elements Count of elements which need in dimanic memory
/// @param [in] elementSize Size of one element
/// @return Pointer to allocate memory or NULL if was error in function
void *recalloc(void *pointer, size_t elements, size_t elementSize);

/// Check that address is corrrect for write
/// @param [in] pointer Pointer for check
/// @return Is pointer correct for write
int isPointerWriteCorrect(const void *pointer);


/// Check that address is corrrect for read
/// @param [in] pointer Pointer for check
/// @return Is pointer correct for read
int isPointerReadCorrect(const void *pointer);

/// Check that address is corrrect for write/read
/// @param [in] pointer Pointer for check
/// @return Is pointer correct for write/read
int isPointerCorrect(const void *pointer);

/// Get file size
/// @param [in] filename Name of file
/// @return Size of file with name filename
size_t getFileSize(const char *filename);

/// Chech then file exits
/// @param [in] fileName Name of file for check
/// @return If file exits 1 else 0
int isFileExists(const char *fileName);

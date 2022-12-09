#pragma once

#include <stddef.h>
#include <stdio.h>

/// Error`s codes from readFile()
enum FiofunctionsError {
  FIOFUNCTIONS_OUT_OF_MEM          = -1,
  FIOFUNCTIONS_FAIL_TO_OPEN        = -2,
  FIOFUNCTIONS_INCORRECT_ARGUMENTS = -3,
};

/// Read every file line in buffer
/// @param [out] buffer Address of pointer to buffer
/// @param [in] filename Name of file which need to read
/// @return Size of buffer in heap or error`s code
size_t readFile(char **buffer, const char *filename);

/// Read bin file to buffer
/// @param [out] buffer Buffer for write
/// @param [in] size elementSize Size of one element
/// @param [in] size Count of element in file
/// @param [in] filePtr File for read
/// @return Count of read elements
size_t readBin(void *buffer, size_t elementSize, size_t size, FILE *filePtr);

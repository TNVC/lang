#pragma once

#include <stddef.h>

namespace db {

  struct StringPool {
    char **pool;
    size_t size;
    size_t capacity;
  };

  void createStringPool(StringPool *pool, int *error = nullptr);

  void destroyStringPool(StringPool *pool, int *error = nullptr);

  char *compareString(const StringPool *pool, const char *string, int *error = nullptr);

  bool searchString(const StringPool *pool, const char *string, int *error = nullptr);

  char *addString(StringPool *pool, const char *string, int *error = nullptr);

  bool compareStrings(const char *first, const char *second, int *error = nullptr);

}

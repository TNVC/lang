#pragma once

#include "Logging.h"

#define ERROR(...)                              \
  do                                            \
    {                                           \
      if (error)                                \
        *error = -1;                            \
                                                \
      return __VA_ARGS__;                       \
    } while (0)

#define CHECK_VALID(TREE, ERROR, ...)                                         \
  do                                                                          \
    {                                                                         \
      unsigned ERR = validateTree(TREE);                                      \
                                                                              \
      if (ERR)                                                                \
        {                                                                     \
            dumpTreeWithMessage(TREE, ERR, getLogFile(), "!!INVALID TREE!!"); \
                                                                              \
          if (ERROR)                                                          \
            *ERROR = (int)ERR;                                                \
                                                                              \
          return __VA_ARGS__;                                                 \
        }                                                                     \
    } while (0)

namespace db {

  char *createImage(const db::Tree *tree, int isDump = false);

}

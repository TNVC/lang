#pragma once

#define ERROR(...)                              \
  do                                            \
    {                                           \
      if (error)                                \
        *error = -1;                            \
                                                \
      return __VA_ARGS__;                       \
    } while (0)


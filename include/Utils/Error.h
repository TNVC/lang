#pragma once

#define ERROR(...)                              \
  do                                            \
    {                                           \
      if (error)                                \
        *error = -1;                            \
      printf("%d %s\n", __LINE__, __FILE__);             \
                                                \
      return __VA_ARGS__;                       \
    } while (0)

#define FAIL(...)                               \
  do                                            \
    {                                           \
      if (fail) *fail = true;                   \
                                                \
      return __VA_ARGS__;                       \
    } while (0)

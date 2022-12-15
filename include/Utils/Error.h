#pragma once

#define ERROR(...)                              \
  do                                            \
    {                                           \
      if (error)                                \
        *error = -1;                            \
      printf("%d\n", __LINE__);                            \
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

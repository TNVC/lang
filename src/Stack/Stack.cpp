#include <stdlib.h>
#include "Stack.h"
#include "Hash.h"
#include "ElementFunctions.h"
#include "SystemLike.h"
#include "Logging.h"

#include "Assert.h"

#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wconditionally-supported"

#ifndef RELEASE_BUILD_

#define CHECK_VALID(STACK_POINTER, ERROR, ...)                          \
  do                                                                    \
    {                                                                   \
      unsigned ERROR_CODE_TEMP = stack_valid(STACK_POINTER);            \
                                                                        \
      if (ERROR_CODE_TEMP)                                              \
        {                                                               \
          stack_dump(STACK_POINTER, ERROR_CODE_TEMP, getLogFile());     \
                                                                        \
          if (ERROR)                                                    \
            *ERROR = ERROR_CODE_TEMP;                                   \
                                                                        \
          return __VA_ARGS__;                                           \
        }                                                               \
    } while (0)

#define UPDATE_HASH(STACK_POINTER)                                      \
  do                                                                    \
    {                                                                   \
      STACK_POINTER->arrayHash = getHash(STACK_POINTER->array,          \
                                         STACK_POINTER->capacity * sizeof(Element)); \
                                                                        \
      STACK_POINTER->hash = 0;                                          \
      STACK_POINTER->hash = getHash(STACK_POINTER, sizeof(Stack));      \
    } while(0)

#else

#define CHECK_VALID(STACK_POINTER, ERROR, ...) ;

#define UPDATE_HASH(STACK_POINTER) ;

#endif

#define LEFT_CANARY        0xDEADBEAF
#define RIGHT_CANARY       0xBADC0FEE
#define LEFT_ARRAY_CANARY  0xBEADFACE
#define RIGHT_ARRAY_CANARY 0xABADBABE

const size_t DEFAULT_STACK_GROWTH   =  2;
const size_t DEFAULT_STACK_OFFSET   =  5;
const size_t DEFAULT_STACK_CAPACITY = 10;

/// Create array for stack if previously stack capacity was 0
/// @param [in] stk Pointer to stack
/// @param [in] size Size for array
/// @param [out] error Variable for write errors` codes
/// @note If size equals zero, that set stack`s array to nullptr
static void createArray(Stack *stk, size_t size, unsigned *error);


unsigned stack_valid(const Stack *stk)
{
#ifdef RELEASE_BUILD_

  return 0;

#else

  if (!isPointerCorrect(stk))
    return NULL_STACK_POINTER;

  unsigned error = 0;

  if (!(stk->status & INIT) && (stk->status & DESTROY))
    error |= DESTROY_WITHOUT_INIT;

  if ((stk->status & EMPTY) && !(stk->lastElementIndex + 1))
    error |= INCORRECT_STATUS;

  if (!isPointerCorrect(stk->array) && stk->capacity)
    error |= NULL_ARRAY_POINTER;

  if (stk->capacity < stk->lastElementIndex)
    error |= CAPACITY_LESS_THAN_SIZE;

  if (stk->leftCanary != LEFT_CANARY)
    error |= LEFT_CANARY_DIED;

  if (stk->rightCanary != RIGHT_CANARY)
    error |= RIGHT_CANARY_DIED;

  if (isPointerCorrect(stk->array))
    {
      if (*(CANARY *)((char *)stk->array - sizeof(CANARY)) != LEFT_ARRAY_CANARY)
        error |= LEFT_ARRAY_CANARY_DIED;

      if (*(CANARY *)(stk->array + stk->capacity)  != RIGHT_ARRAY_CANARY)
        error |= RIGHT_ARRAY_CANARY_DIED;

      if (getHash(stk->array, stk->capacity * sizeof(Element)) != stk->arrayHash)
        error |= DIFFERENT_ARRAY_HASH;
    }

  unsigned hash = stk->hash;

  stk->hash = 0;

  if (getHash(stk, sizeof(Stack)) != hash)
    error |= DIFFERENT_HASH;

  stk->hash = hash;

  if (!isPointerCorrect(stk->info.name))
    error |= NOT_NAME;

  if (!isPointerCorrect(stk->info.fileName))
    error |= NOT_FILE_NAME;

  if (!isPointerCorrect(stk->info.functionName))
    error |= NOT_FUNCTION_NAME;

  if (stk->info.line <= 0)
    error |= INCORRECT_LINE;

  return error;

#endif
}

void do_stack_init(Stack *stk, size_t capacity,
                  const char *name, const char *fileName, const char *functionName, int line,
                  unsigned *error)
{
  if (!isPointerCorrect(stk) || !isPointerCorrect(name) || !isPointerCorrect(fileName) || !isPointerCorrect(functionName) || (line <= 0) || (stk->status & INIT))
      {
        if (isPointerCorrect(error))
            *error = 1;

        return;
      }

#ifndef RELEASE_BUILD_

    stk->leftCanary  = LEFT_CANARY;
    stk->rightCanary = RIGHT_CANARY;

#endif

    stk->capacity         = capacity;
    stk->lastElementIndex = 0;
    stk->status           = INIT | EMPTY;

#ifndef RELEASE_BUILD_

    stk->info.name             = name;
    stk->info.fileName         = fileName;
    stk->info.functionName     = functionName;
    stk->info.line             = line;

#endif

    if (capacity == 0)
        stk->array = nullptr;
    else
      {
        createArray(stk, capacity, error);

        if (!isPointerCorrect(stk->array))
          return;
      }

    UPDATE_HASH(stk);

    CHECK_VALID(stk, error);
}

void stack_destroy(Stack *stk, unsigned *error)
{
  CHECK_VALID(stk, error);

  if (!(stk->status & INIT))
    {
      if (isPointerCorrect(error))
        *error = 1;

      return;
    }

#ifndef RELEASE_BUILD_

  if (isPointerCorrect(stk->array))
    free((char *)stk->array - sizeof(CANARY));

#else

  free(stk->array);

#endif
  stk->array = nullptr;

  stk->capacity         = 0;
  stk->lastElementIndex = 0;

  stk->status |= DESTROY;

  UPDATE_HASH(stk);
}

void stack_push(Stack *stk, Element element, unsigned *error)
{
  CHECK_VALID(stk, error);

  if (stk->lastElementIndex == stk->capacity)
    {
      stack_resize(stk, stk->capacity ? stk->capacity * DEFAULT_STACK_GROWTH : DEFAULT_STACK_CAPACITY);

      if (!isPointerCorrect(stk->array))
        {
          if (isPointerCorrect(error))
            *error = 1;

          return;
        }
    }

  stk->array[(stk->lastElementIndex)++] = element;

  stk->status &= NOT_EMPTY;

  UPDATE_HASH(stk);

  CHECK_VALID(stk, error);
}

Element stack_pop(Stack *stk, unsigned *error)
{
  CHECK_VALID(stk, error, 0);

  if ((stk->status & EMPTY))
  {
    if (isPointerCorrect(error))
      *error = 1;

    return 0;
  }

  Element tempElement = stk->array[--(stk->lastElementIndex)];

  Element poison = getPoison(stk->array[0]);

  stk->array[stk->lastElementIndex] = poison;

  if (stk->lastElementIndex == 0)
    stk->status |= EMPTY;

  UPDATE_HASH(stk);

  if ((int)stk->lastElementIndex < (int)(stk->capacity / DEFAULT_STACK_GROWTH - (int)DEFAULT_STACK_OFFSET))
    {
      stack_resize(stk, stk->capacity / DEFAULT_STACK_GROWTH);

      if (!isPointerCorrect(stk->array))
        {
          if (isPointerCorrect(error))
            *error = 1;

          return 0;
        }
    }

  UPDATE_HASH(stk);

  CHECK_VALID(stk, error, 0);

  return tempElement;
}

Element stack_top(const Stack *stk, unsigned *error)
{
  CHECK_VALID(stk, error, 0);

  if ((stk->status & EMPTY))
  {
    if (isPointerCorrect(error))
      *error = 1;

    return 0;
  }

  return stk->array[stk->lastElementIndex - 1];
}

Element stack_get(const Stack *stk, unsigned index, unsigned *error)
{
  CHECK_VALID(stk, error, 0);

  if (stack_size(stk) <= index)
    {
      if (isPointerCorrect(error))
        *error = 1;

      return 0;
    }

  return stk->array[index];
}

void stack_resize(Stack *stk, size_t newSize, unsigned *error)
{
  CHECK_VALID(stk, error);

  if (newSize && !stk->array)
    {
      createArray(stk, newSize, error);

      if (!isPointerCorrect(stk->array))
        return;

      Element poison = getPoison(stk->array[0]);

      for (size_t i = 0; i < newSize; ++i)
        stk->array[i] = poison;
    }
  if (newSize)
    {
#ifndef RELEASE_BUILD_

      stk->array = (Element *)((char *)stk->array - sizeof(CANARY));

      Element *temp = (Element *) recalloc(stk->array, 1,  newSize*sizeof(Element) + 2*sizeof(CANARY));

#else

      Element *temp = (Element *) recalloc(stk->array, 1,  newSize*sizeof(Element));

#endif

      if (isPointerCorrect(temp))
        {
#ifndef RELEASE_BUILD_

          stk->array = temp;

          stk->array = (Element *)((char *)stk->array + sizeof(CANARY));

          *(CANARY *)(stk->array + newSize) = RIGHT_ARRAY_CANARY;

          if (stk->capacity < newSize)
            {
              Element poison = getPoison(stk->array[0]);

              for (size_t i = 0; i < newSize - stk->capacity; ++ i)
                stk->array[stk->capacity + i] = poison;
            }
#endif
        }
      else
        {
          if (isPointerCorrect(error))
            *error = 1;

          return;
        }
    }
  else
    {
#ifndef RELEASE_BUILD_

      stk->array = (Element *)((char *)stk->array - sizeof(CANARY));

      free(stk->array);

#else

      free(stk->array);

#endif

      stk->array = nullptr;
    }

  stk->capacity = newSize;

  UPDATE_HASH(stk);

  CHECK_VALID(stk, error);
}

size_t stack_size(const Stack *stk, unsigned *error)
{
  CHECK_VALID(stk, error, -1u);

  if ((stk->status & EMPTY))
    return 0;

  return stk->lastElementIndex;
}

size_t stack_capacity(const Stack *stk, unsigned *error)
{
  CHECK_VALID(stk, error, -1u);

  return stk->capacity;
}

int stack_isEmpty(const Stack *stk, unsigned *error)
{
  CHECK_VALID(stk, error, 0);

  return stk->status & EMPTY;
}

static void createArray(Stack *stk, size_t size, unsigned *error)
{
  if (!size)
    {
      stk->array = nullptr;

      return;
    }

#ifndef RELEASE_BUILD_

  stk->array = (Element *) calloc(1, size*sizeof(Element) + 2*sizeof(CANARY));

#else

  stk->array = (Element *) calloc(1, size*sizeof(Element));

#endif

  if (!isPointerCorrect(stk->array))
    {
      if (isPointerCorrect(error))
        *error = 1;

      return;
    }

#ifndef RELEASE_BUILD_

  *(CANARY *)stk->array = LEFT_ARRAY_CANARY;

  stk->array = (Element *)((char *)stk->array + sizeof(CANARY));

  *(CANARY *)(stk->array + size) = RIGHT_ARRAY_CANARY;

#endif

  Element poison = getPoison(stk->array[0]);

  for (size_t i = 0; i < size; ++i)
    stk->array[i] = poison;
}

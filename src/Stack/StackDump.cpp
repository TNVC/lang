#include <stdio.h>
#include "Stack.h"
#include "ElementFunctions.h"
#include "SystemLike.h"

#define STATUS_BORDER "#---------------------------#------#"
#define ERRORS_BORDER "#----------------------------------#"

DUMP_LEVEL DUMP_LVL = DUMP_ALL;

#ifndef RELEASE_BUILD_

const int POISON_LENGTH = 6;
const int PAUSE_LENGTH  = 6;

int MAX_LENGTH    = 0;
int MIDDLE_LENGTH = 0;

const char *ERRORS_MESSAGE[] = {      // errCode - errName
  "Pointer to stack is NULL",	      // 2^0     - NULL_STACK_POINTER
  "Stack was destroy without init",   // 2^1     - DESTROY_WITHOUT_INIT
  "Status is incorrect",	      // 2^2     - INCORRECT_STATUS
  "Pointer to stack`s array is NULL", // 2^3     - NULL_ARRAY_POINTER
  "Capacity less than size",	      // 2^4     - CAPACITY_LESS_THAN_SIZE
  "Left canary is died",              // 2^5     - LEFT_CANARY_DIED
  "Right canary is died",             // 2^6     - RIGHT_CANARY_DIED
  "Left array canary is died",        // 2^7     - LEFT_ARRAY_CANARY_DIED
  "Right array canary is died",       // 2^8     - RIGHT_ARRAY_CANARY_DIED
  "Stack hasn`t a name",              // 2^9     - NOT_NAME
  "Stack hasn`t a file name",         // 2^10    - NOT_FILE_NAME
  "Stack hasn`t a function name",     // 2^11    - NOT_FUNCTION_NAME
  "Stack hasn`t a correct line",      // 2^12    - INCORRECT_LINE
  "Stack hash is corrupted",          // 2^13    - DIFFERENT_HASH
  "Stack`s array hash is corrupted"   // 2^14    - DIFFERENT_ARRAY_HASH
};

const char *STATUS_NAME[] = {
  "INIT",
  "DESTROY",
  "EMPTY"
};

/// Print errors` message
/// @param [in] errorCode Code of errors
/// @param [in] filePtr File for writing
static void printErrors(unsigned errorCode, FILE *filePtr);

/// Print Stack status into file
/// @param [in] stk Pointer to stack
/// @param [in] filePtr File for writing
static void printStatus(const Stack *stk, FILE *filePtr);

/// Print addresss of stack`s elements
/// @param [in] stk Pointer to stack
/// @param [in] filePtr FIle for writing
static void printAddress(const Stack *stk, FILE *filePtr);

/// Print border for stack array into file
/// @param [in] stk Pointer to stack
/// @param [in] filePtr File for writing
static void printBorder(const Stack *stk, FILE *filePtr);

/// Print line for stack array into file
/// @param [in] stk Pointer to stack
/// @param [in] filePtr File for writing
static void printLine(const Stack *stk, FILE *filePtr);

/// Print values in stack array
/// @param [in] stk Pointer to stack
/// @param filePtr File for writing
static void printValues(const Stack *stk, FILE *filePtr);

/// Print arror for stack array into file
/// @param [in] stk  Pointer to stack
/// @param [in] filePtr File for writing
static void printArrow(const Stack *stk, FILE *filePtr);

#endif

void do_stack_dump(const Stack *stk, unsigned errorCode, FILE *filePtr,
                   const char *fileName, const char *functionName, int line)
{
#ifndef RELEASE_BUILD_

  if (!isPointerCorrect(filePtr))
    filePtr = stdout;

  fputc('\n', filePtr);

  fprintf(filePtr, "%s at %s (%d):\n",
          isPointerCorrect(functionName) ? functionName : "nullptr",
          isPointerCorrect(fileName)     ? fileName     : "nullptr",
          line);
  fprintf(filePtr, "Stack[%p]", (const void *)stk);

  if (isPointerCorrect(stk))
    fprintf(filePtr, " \"%s\" at %s at %s (%d)",
            isPointerCorrect(stk->info.name)         ? stk->info.name         : "nullptr",
            isPointerCorrect(stk->info.functionName) ? stk->info.functionName : "nullptr",
            isPointerCorrect(stk->info.fileName)     ? stk->info.fileName     : "nullptr",
            stk->info.line);

  if (isPointerCorrect(stk))
    fprintf(filePtr, "\nHash: %u Array hash: %u", stk->hash, stk->arrayHash);

  if (isPointerCorrect(stk) && isPointerCorrect(stk->array))
    {
      MAX_LENGTH    = maxElementLength(stk->array[0]);

      MIDDLE_LENGTH = (MAX_LENGTH + 1) / 2;
    }

  fputc('\n', filePtr);

  printErrors(errorCode, filePtr);

  printStatus (stk, filePtr);

  if (!isPointerCorrect(stk))
    {
      fputc('\n', filePtr);

      return;
    }

  printAddress(stk, filePtr);

  printBorder (stk, filePtr);

  printLine   (stk, filePtr);

  printValues (stk, filePtr);

  printLine   (stk, filePtr);

  printBorder (stk, filePtr);

  printArrow  (stk, filePtr);

  fputc('\n', filePtr);

  #endif
}

#ifndef RELEASE_BUILD_

static void printErrors(unsigned errorCode, FILE *filePtr)
{
  if (!errorCode)
    {
      fprintf(filePtr, "Stack is ok\n");

      return;
    }

  fprintf(filePtr, ERRORS_BORDER "\n");
  fprintf(filePtr,"|%-34s|\n", "ERRORS!!");
  fprintf(filePtr, ERRORS_BORDER "\n");

  for (unsigned i = 0; i < ERRORS_COUNT; ++i)
    {
      if (!((errorCode >> i) & 0x01))
        continue;

      fprintf(filePtr, "|%-34s|\n", ERRORS_MESSAGE[i]);
    }

  fprintf(filePtr, ERRORS_BORDER "\n");
}

static void printStatus(const Stack *stk, FILE *filePtr)
{
  fprintf(filePtr, STATUS_BORDER "\n");

  fprintf(filePtr, "|%-27s|%6lu|\n|%-27s|%6lu|\n",
          "Stack capacity", stk->capacity,
          "Stack size", stk->lastElementIndex);

  fprintf(filePtr, STATUS_BORDER "\n");

  for (unsigned i = 0; i < STATUS_COUNT; ++i)
      fprintf(filePtr, "|%-27s|%-6s|\n", STATUS_NAME[i], ((stk->status >> i) & 0x01) ? "True" : "False");

  fprintf(filePtr, STATUS_BORDER "\n");
}

static void printAddress(const Stack *stk, FILE *filePtr)
{
  if (!isPointerCorrect(stk->array))
    return;

  int firstSize = elementLength(stk->array[0]) < MIDDLE_LENGTH ?
    MIDDLE_LENGTH : MAX_LENGTH;

  if (isPoison(stk->array[0]))
    firstSize = POISON_LENGTH;

  fprintf(filePtr, "%p\n%*s|\n%*s|\n%*sV\n",
          (const void *)stk->array, firstSize, "", firstSize, "", firstSize, "");
}

static void printBorder(const Stack *stk, FILE *filePtr)
{
  if (!isPointerCorrect(stk->array))
    return;

  int skip = 0;

  for (size_t i = 0; i < stk->capacity; ++i)
    {
      if (!skip)
        fputc('#', filePtr);

      if (DUMP_LVL == DUMP_NOT_EMPTY && i == stk->lastElementIndex)
        {
          fprintf(filePtr, "- ** -#\n");

          return;
        }

      int size = elementLength(stk->array[i]) < MIDDLE_LENGTH ? MIDDLE_LENGTH : MAX_LENGTH;

      if (isPoison(stk->array[i]) && i >= stk->lastElementIndex)
        {
          if (DUMP_LVL == DUMP_NOT_POISON)
            {
              if (!skip)
                {
                  skip = 1;

                  fprintf(filePtr, "- ** -#");
                }

              size = 0;
            }
          else
            size = POISON_LENGTH;
        }
      else
        skip = 0;

      for (int j = 0; j < size; ++j)
        fputc('-', filePtr);
    }

  if (!skip)
    fputc('#', filePtr);

  fputc('\n', filePtr);
} 

static void printLine(const Stack *stk, FILE *filePtr)
{
  if (!isPointerCorrect(stk->array))
    return;

  int skip = 0;

  for (size_t i = 0; i < stk->capacity; ++i)
    {
      if (!skip)
        fputc('|', filePtr);

      if (DUMP_LVL == DUMP_NOT_EMPTY && i == stk->lastElementIndex)
        {
          fprintf(filePtr, "  **  |\n");

          return;
        }

      const char ch = i < stk->lastElementIndex ? ' ' : '=';

      int size = elementLength(stk->array[i]) < MIDDLE_LENGTH ? MIDDLE_LENGTH : MAX_LENGTH;

      if (isPoison(stk->array[i]) && i >= stk->lastElementIndex)
        {
          if (DUMP_LVL == DUMP_NOT_POISON)
            {
              if (!skip)
                {
                  skip = 1;

                  fprintf(filePtr, "  **  |");
                }

              size = 0;
            }
          else
            size = POISON_LENGTH;
        }
      else
        skip = 0;

      for (int j = 0; j < size; ++j)
        fputc(ch, filePtr);
    }


  if (!skip)
    fputc('|', filePtr);

  fputc('\n', filePtr);
}

static void printValues(const Stack *stk, FILE *filePtr)
{
  if (!isPointerCorrect(stk->array))
    return;

  int skip = 0;

  for (size_t i = 0; i < stk->capacity; ++i)
    {
      if (DUMP_LVL == DUMP_NOT_EMPTY && i == stk->lastElementIndex)
        {
          fprintf(filePtr, "|  **  |\n");

          return;
        }

      int elementSize = elementLength(stk->array[i]);

      int size = elementSize < MIDDLE_LENGTH ? MIDDLE_LENGTH : MAX_LENGTH;

      if (isPoison(stk->array[i]) && i >= stk->lastElementIndex)
        {
          if (DUMP_LVL == DUMP_NOT_POISON)
            {
              if (!skip)
                {
                  skip = 1;

                  fprintf(filePtr, "|  **  ");
                }

              continue;
            }
          else
            {
              fprintf(filePtr, "|POISON");

              continue;
            }
        }
      else
        skip = 0;

      fputc('|', filePtr);

      for (int j = 0; j < size- elementSize; ++j)
        fputc(' ', filePtr);

      printElement(stk->array[i], filePtr);
    }

  fprintf(filePtr, "|\n");
}

static void printArrow(const Stack *stk, FILE *filePtr)
{
  if (!isPointerCorrect(stk->array) || !stk->lastElementIndex)
    return;

  fputc('>', filePtr);

  for (size_t i = 0; i < stk->lastElementIndex - 1; ++i)
    {
      int size = elementLength(stk->array[i]) < MIDDLE_LENGTH ? MIDDLE_LENGTH : MAX_LENGTH;

      if (isPoison(stk->array[i])  && i >= stk->lastElementIndex)
        size = POISON_LENGTH;

      for (int j = 0; j < size + 1; ++j)
        fputc('>', filePtr);
    }

  int size = elementLength(stk->array[stk->lastElementIndex - 1]) < MIDDLE_LENGTH ? MIDDLE_LENGTH : MAX_LENGTH;

  for (int j = 0; j < size - 1; ++j)
    fputc('>', filePtr);

  fputc('^', filePtr);
}

#endif

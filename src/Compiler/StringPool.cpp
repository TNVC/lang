#include "StringPool.h"

#include <malloc.h>
#include <string.h>
#include "SystemLike.h"
#include "Error.h"

const int GROWTH_FACTOR = 2;

void db::createStringPool(db::StringPool *pool, int *error)
{
  if (!pool) ERROR();

  pool->size = pool->capacity = 0;
  pool->pool = nullptr;
}

void db::destroyStringPool(db::StringPool *pool, int *error)
{
  if (!pool) ERROR();

  for (size_t i = 0; i < pool->size; ++i)
    free(pool->pool[i]);
  free(pool->pool);

  pool->size = pool->capacity = 0;
  pool->pool = nullptr;
}

char *db::compareString(const db::StringPool *pool, const char *string, int *error)
{
  if (!pool || !string) ERROR(nullptr);

  for (size_t i = 0; i < pool->size; ++i)
    if (pool->pool[i] == string ||
        !strcmp(pool->pool[i], string)) return pool->pool[i];

  return nullptr;
}

bool db::searchString(const db::StringPool *pool, const char *string, int *error)
{
  if (!pool || !string) ERROR(false);

  for (size_t i = 0; i < pool->size; ++i)
    if (string == pool->pool[i]) return true;

  return false;
}

char *db::addString(db::StringPool *pool, const char *string, int *error)
{
  if (!pool || !string) ERROR(nullptr);

  char *hasString = compareString(pool, string, error);
  if (hasString) return hasString;

  if (pool->size == pool->capacity)
    {
      pool->capacity = 2*pool->capacity + 1;
      char **temp =
        (char **)recalloc(pool->pool, pool->capacity, sizeof(char *));

      if (!temp)
        {
          free(pool->pool);

          ERROR(nullptr);
        }

      pool->pool = temp;
    }

  pool->pool[pool->size] = strdup(string);

  return pool->pool[pool->size++];
}

bool db::compareStrings(const char *first, const char *second, int *error)
{
  if (!first || !second) ERROR(false);

  if (first == second) return true;

  return !strcmp(first, second);
}

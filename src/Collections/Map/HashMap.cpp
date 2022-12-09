#include "HashMap.h"

#include <malloc.h>
#include <string.h>
#include "SystemLike.h"
#include "Assert.h"

#define ERROR(ERR, CODE, ...)                   \
  do                                            \
    {                                           \
      if (ERR)                                  \
        *ERR = (int)CODE;                       \
                                                \
      return __VA_ARGS__;                       \
    } while (0)

#define CHECK_VALID(MAP, ERR, ...)                        \
  do                                                      \
    {                                                     \
      unsigned ERROR_CODE = validateHashMap(MAP);         \
                                                          \
      if (ERROR_CODE)                                     \
        ERROR(ERR, ERROR_CODE __VA_OPT__(,) __VA_ARGS__); \
    } while (0)

const int DEFAULT_OFFSET = 17;

static db::HashMapNode *createNode(int hash, const db::key_t key, const db::value_t value);

static void destroyNode(db::HashMapNode *node);

static void resize(db::HashMap *map, size_t newCapacity, int *error);

static int getIndex(const db::HashMap *map, const db::key_t key);

int db::getHash(const db::key_t key)
{
  int hash = DEFAULT_OFFSET;

  for (const char *ptr = key; *ptr; ++ptr)
    {
      int valueOfByte = *ptr;

      hash += (valueOfByte << 5) + valueOfByte;
    }

  return hash > 0 ? hash : -hash;
}

int db::compare(const db::key_t first, const db::key_t second)
{
  return strcmp(first, second);
}

bool db::isValidKey(const db::key_t key)
{
  return key;
}

bool db::isValidValue(const db::value_t value)
{
  return value;
}

static int getIndex(const db::HashMap *map, const db::key_t key)
{
  assert(map);

  return db::getHash(key) & (int)(map->capacity - 1);
}

#include <stdio.h>//////

unsigned db::validateHashMap(const db::HashMap *map)
{
  //  static int countOfCall = 0;

  //  if (!countOfCall++)
  //    printf("#TODO validateHashMap\n");

  if (!map)
    return db::HASH_MAP_IS_NULL;

  return 0;
}

void db::initHashMap(db::HashMap *map, size_t capacity, int *error)
{
  if (!map)
    ERROR(error, db::HASH_MAP_IS_NULL);

  if (!capacity)
    ERROR(error, db::HASH_MAP_CAPACITY_IS_ZERO);

  map->data     = nullptr;
  map->capacity = capacity;

  resize(map, capacity, error);////Check error

  CHECK_VALID(map, error);
}

void db::destroyHashMap(db::HashMap *map, int *error)
{
  CHECK_VALID(map, error);

  for (size_t i = 0; i < map->capacity; ++i)
    {
      db::HashMapNode *temp = map->data[i];

      while (temp)
        {
          map->data[i] = temp->next;

          destroyNode(temp);

          temp = map->data[i];
        }
    }

  free(map->data);

  map->data = nullptr;
  map->capacity = 0;
}

bool db::put(db::HashMap *map, const db::key_t key, const db::value_t value, int *error)
{
  CHECK_VALID(map, error, false);

  if (!db::isValidKey(key))
    ERROR(error, db::HASH_MAP_ILLEGAL_ARGUMENT, false);

  if (!isValidValue(value))
    ERROR(error, db::HASH_MAP_ILLEGAL_ARGUMENT, false);

  bool hasKey = false;

  int index = getIndex(map, key);

  db::HashMapNode *newNode = createNode(index, key, value);

  if (!newNode)
    ERROR(error, db::HASH_MAP_OUT_OF_MEMORY, false);

  db::HashMapNode *node = map->data[index];

  if (!node)
    map->data[index] = newNode;
  else
    {
      for ( ; node->next; node = node->next)
        {
          if (!db::compare(node->key, key))
            {
              hasKey = true;

              destroyNode(newNode);////

              free(node->value);/////

              node->value = strdup(value);/////
            }
        }

      if (!hasKey)
        node->next = newNode;
    }

  if (!hasKey)
    ++map->size;

  CHECK_VALID(map, error, false);

  return hasKey;
}

db::value_t db::get(const db::HashMap *map, const db::key_t key, int *error)
{
  CHECK_VALID(map, error, db::nullvalue);

  if (!db::isValidKey(key))
    ERROR(error, db::HASH_MAP_ILLEGAL_ARGUMENT, db::nullvalue);

  db::HashMapNode *result = nullptr;

  int index = getIndex(map, key);

  if (!map->data[index])
    ERROR(error, db::HASH_MAP_NOT_SUCH_KEY, db::nullvalue);

  for (result = map->data[index]; result && strcmp(key, result->key); result = result->next)
    continue;

  if (!result)
    ERROR(error, db::HASH_MAP_NOT_SUCH_KEY, db::nullvalue);

  return result->value;
}

db::key_t *db::keys(const db::HashMap *map, int *error)
{
  CHECK_VALID(map, error, nullptr);

  db::key_t *keys = (db::key_t *)calloc(map->size, sizeof(db::key_t));

  if (!keys)
    ERROR(error, db::HASH_MAP_OUT_OF_MEMORY, nullptr);

  int index = 0;

  db::HashMapNode *node = nullptr;

  for (size_t i = 0; i < map->capacity; ++i)
    {
      node = map->data[i];

      if (!node)
        continue;

      for ( ; node; node = node->next)
        keys[index++] = strdup(node->key);
    }

  CHECK_VALID(map, error, nullptr);

  return keys;
}

db::value_t *db::values(const  db::HashMap *map, int *error)
{
  CHECK_VALID(map, error, nullptr);

  db::value_t *values = (db::value_t *)calloc(map->size, sizeof(db::value_t));

  if (!values)
    ERROR(error, db::HASH_MAP_OUT_OF_MEMORY, nullptr);

  int index = 0;

  db::HashMapNode *node = nullptr;

  for (size_t i = 0; i < map->capacity; ++i)
    {
      node = map->data[i];

      if (!node)
        continue;

      for ( ; node; node = node->next)
        values[index++] = strdup(node->value);
    }

  CHECK_VALID(map, error, nullptr);

  return values;
}

bool db::isEmpty(const db::HashMap *map, int *error)
{
  CHECK_VALID(map, error, true);

  return map->size;
}

size_t db::size(const db::HashMap *map, int *error)
{
  CHECK_VALID(map, error, 0);

  return map->size;
}

static void resize(db::HashMap *map, size_t newCapacity, int *error)
{
  assert(map);
  assert(newCapacity);

  db::HashMapNode **temp =
    (db::HashMapNode **)recalloc(map->data, newCapacity, sizeof(db::HashMapNode *));

  if (!temp)
    ERROR(error, db::HASH_MAP_OUT_OF_MEMORY);

  map->data = temp;

  map->capacity = newCapacity;
}

static db::HashMapNode *createNode(int hash, const db::key_t key, const db::value_t value)
{
  assert(db::isValidKey(key));
  assert(db::isValidKey(value));
  assert(hash >= 0);

  db::HashMapNode *node = (db::HashMapNode *)calloc(1, sizeof(db::HashMapNode));

  if (!node)
    return nullptr;

  node->hash = hash;
  node->key = strdup(key);
  node->value = strdup(value);
  node->next = nullptr;

  return node;
}

static void destroyNode(db::HashMapNode *node)
{
  assert(node);

  free(node->key);
  free(node->value);
  free(node);
}


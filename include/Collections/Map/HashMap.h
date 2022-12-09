#pragma once

#include <stddef.h>

namespace db {

  enum {
    HASH_MAP_OK               = 0x00,
    HASH_MAP_IS_NULL          = 0x01 << 0,
    HASH_MAP_CAPACITY_IS_ZERO = 0x01 << 1,
    HASH_MAP_OUT_OF_MEMORY    = 0x01 << 2,
    HASH_MAP_ILLEGAL_ARGUMENT = 0x01 << 3,
    HASH_MAP_NOT_SUCH_KEY     = 0x01 << 4,
  };

  typedef char *key_t;
  typedef char *value_t;

  const key_t   nullkey   = nullptr;
  const value_t nullvalue = nullptr;

  struct HashMapNode {
    int hash;
    key_t key;
    value_t value;
    HashMapNode *next;
  };

  struct HashMap {
    HashMapNode **data;
    size_t capacity;
    size_t size;

    HashMap &operator=(const HashMap &original) = delete;
  };

  int getHash(const key_t key);

  int compare(const key_t first, const key_t second);

  bool isValidKey(const key_t key);

  bool isValidValue(const value_t value);

  unsigned validateHashMap(const HashMap *map);

  void initHashMap(HashMap *map, size_t capacity = 16, int *error = nullptr);

  void destroyHashMap(HashMap *map, int *error = nullptr);

  bool put(HashMap *map, const key_t key, const value_t value, int *error = nullptr);

  value_t get(const HashMap *map, const key_t key, int *error = nullptr);

  key_t *keys(const HashMap *map, int *error = nullptr);

  value_t *values(const HashMap *map, int *error = nullptr);

  bool isEmpty(const HashMap *map, int *error = nullptr);

  size_t size(const HashMap *map, int *error = nullptr);

}

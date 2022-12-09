#include "ResourceBundle.h"

#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include "FileUtils.h"
#include "Assert.h"

#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wuseless-cast"

#define ERROR(ERR, CODE, ...)                   \
  do                                            \
    {                                           \
      if (ERR)                                  \
        *ERR = (int)CODE;                       \
                                                \
      return __VA_ARGS__;                       \
    } while (0)

#define CHECK_VALID(BUNDLE, ERR, ...)                        \
  do                                                         \
    {                                                        \
      unsigned ERROR_CODE = validateBundle(BUNDLE);          \
                                                             \
      if (ERROR_CODE)                                        \
        ERROR(ERR, ERROR_CODE __VA_OPT__(,) __VA_ARGS__);    \
    } while (0)

const size_t LOCALE_SIZE    = 3;
const size_t EXTENSION_SIZE = 11;//".properties"

const size_t DEFAULT_CAPACITY = 64;

static FILE *fopenProperties(const char *fileName, db::Locale locale, int *error = nullptr);

static void parsePropertiesFile(db::HashMap *map, FILE *file, int *error = nullptr);

static unsigned validateBundle(const db::ResourceBundle *bundle)
{
  //  static int countOfCall = 0;

  //  if (!countOfCall++)
  //    printf("#TODO validateBundle\n");

  if (!bundle)
    return db::RESOURCE_BUNDLE_IS_NULL;

  return 0;
}

void db::getBundle(
                   db::ResourceBundle *bundle,
                   const char *bundleName,
                   db::Locale locale,
                   int *error
                   )
{
  assert(bundle);
  assert(bundleName);

  int errorCode = 0;

  FILE *propertiesFile = fopenProperties(bundleName, locale, &errorCode);

  if (errorCode)
    ERROR(error, errorCode);

  db::initHashMap(&bundle->data, DEFAULT_CAPACITY, &errorCode);

  if (errorCode)
    ERROR(error, errorCode);

  parsePropertiesFile(&bundle->data, propertiesFile, &errorCode);

  fclose(propertiesFile);

  if (errorCode)
    ERROR(error, errorCode);
}

void db::destroyBundle(db::ResourceBundle *bundle, int *error)
{
  CHECK_VALID(bundle, error);

  db::destroyHashMap(&bundle->data);
}

const char *db::getString(const db::ResourceBundle *bundle, const char *key, int *error)
{
  CHECK_VALID(bundle, error, nullptr);

  if (!key)
    ERROR(error, db::RESOURCE_BUNDLE_ILLEGAL_ARGUMENT, nullptr);

  int errorCode = 0;

  const char *result = db::get(&bundle->data, (char *)key, &errorCode);

  if (errorCode == db::HASH_MAP_NOT_SUCH_KEY)
    ERROR(error, db::RESOURCE_BUNDLE_NOT_SUCH_KEY, nullptr);

  return result;
}

char **db::keys(const db::ResourceBundle *bundle, int *error)
{
  CHECK_VALID(bundle, error, nullptr);

  int errorCode = 0;

  char **result = db::keys(&bundle->data, &errorCode);

  if (errorCode == db::HASH_MAP_OUT_OF_MEMORY)
    ERROR(error, db::RESOURCE_BUNDLE_OUT_OF_MEMORY, nullptr);

  return result;
}

size_t db::keysCount(const db::ResourceBundle *bundle, int *error)
{
  CHECK_VALID(bundle, error, 0);

  int errorCode = 0;

  size_t size = db::size(&bundle->data, &errorCode);

  if (errorCode)
    ERROR(error, errorCode, 0);

  return size;
}

static FILE *fopenProperties(const char *fileName, db::Locale locale, int *error)
{
  assert(fileName);

  size_t size = strlen(fileName) + LOCALE_SIZE + EXTENSION_SIZE + 1;

  char *fullName = (char *)calloc(size, sizeof(char));

  if (!fullName)
    ERROR(error, db::RESOURCE_BUNDLE_OUT_OF_MEMORY, nullptr);

  strcat(fullName, fileName);

  switch (locale)
    {
    case db::Locale::RU: strcat(fullName, "_ru"); break;
    case db::Locale::EN: strcat(fullName, "_en"); break;
    case db::Locale::PL: strcat(fullName, "_pl"); break;
    default:
      {
        free(fullName);

        ERROR(error, db::RESOURCE_BUNDLE_UNEXPECTED_ERROR, nullptr);
      }
    }

  strcat(fullName, ".properties");

  FILE *file = openFile(fullName, "r");

  free(fullName);

  if (!file)
    ERROR(error, db::RESOURCE_BUNDLE_NOT_SUCH_FILE, nullptr);

  return file;
}

static void parsePropertiesFile(db::HashMap *map, FILE *file, int *error)
{
  assert(map);
  assert(file);

  char key  [db::MAX_KEY_SIZE]   = "";
  char value[db::MAX_VALUE_SIZE] = "";

  int errorCode = 0;

  while (true)
    {
      if (fscanf(file, " %[^ \t\n=]", key) != 1)
        break;

      if (fscanf(file, " = \"%[^\"]\"", value) != 1)
        ERROR(error, db::RESOURCE_BUNDLE_INVALID_SYNTAX);

      db::put(map, key, value, &errorCode);

      if (errorCode)
        ERROR(error, errorCode);
    }
}

#pragma once

#include "HashMap.h"
#include "Locale.h"

namespace db {

  enum {
    RESOURCE_BUNDLE_OK               = 0x00,
    RESOURCE_BUNDLE_IS_NULL          = 0x01 << 0,
    RESOURCE_BUNDLE_NOT_SUCH_FILE    = 0x01 << 1,
    RESOURCE_BUNDLE_NOT_SUCH_KEY     = 0x01 << 2,
    RESOURCE_BUNDLE_ILLEGAL_ARGUMENT = 0x01 << 3,
    RESOURCE_BUNDLE_OUT_OF_MEMORY    = 0x01 << 4,
    RESOURCE_BUNDLE_INVALID_SYNTAX   = 0x01 << 5,
    RESOURCE_BUNDLE_UNEXPECTED_ERROR = 0x01 << 6,
  };

  const int MAX_KEY_SIZE   = 128;
  const int MAX_VALUE_SIZE = 512;

  struct ResourceBundle {
    HashMap data;
    Locale locale;

    ResourceBundle &operator=(const ResourceBundle &original) = delete;
  };

  void getBundle(
                 ResourceBundle *bundle,
                 const char *bundleName,
                 Locale locale = Locale::EN,
                 int *error = nullptr
                 );

  void destroyBundle(ResourceBundle *bundle, int *error = nullptr);

  const char *getString(const ResourceBundle *bundle, const char *key, int *error = nullptr);

  char **keys(const ResourceBundle *bundle, int *error = nullptr);

  size_t keysCount(const ResourceBundle *bundle, int *error = nullptr);
}

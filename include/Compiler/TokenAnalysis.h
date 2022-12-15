#pragma once

#include "Tree.h"
#include "StringPool.h"

namespace db {

  Token *getTokens(const char *source, StringPool *pool, int *error = nullptr);

}

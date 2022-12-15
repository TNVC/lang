#include "ColorOutput.h"

#pragma GCC diagnostic ignored "-Wcast-qual"

#define COUNT_OF_ARGS(...) CALL_COUNTER(__VA_ARGS__ __VA_OPT__(,) SEQUENCE_FOR_ARGS())
#define CALL_COUNTER(...) ARGS_COUNTER(__VA_ARGS__)
#define ARGS_COUNTER(                                       \
                     _0, _1, _2, _3, _4, _5, _6, _7, _8,    \
                     _9,_10,_11,_12,_13,_14,_15,  N, ...) N
#define SEQUENCE_FOR_ARGS() 16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0

#define HANDLE_ERROR_WITH_NAME(MESSAGE, NAME, REMOVE_TABLE, ...) \
  do                                                             \
    {                                                            \
      handleError(MESSAGE " at Line: %d at Position: %d!",       \
                  NAME,                                          \
                  TOKEN(translator)->position.line,              \
                  TOKEN(translator)->position.position);         \
                                                                 \
      showSourceLine(translator, TOKEN(translator)->position);   \
                                                                 \
      CLEAN_RESOURCES(REMOVE_TABLE __VA_OPT__(,) __VA_ARGS__);   \
    } while (0)

#define HANDLE_ERROR(MESSAGE, REMOVE_TABLE, ...)                        \
  do                                                                    \
    {                                                                   \
      handleError(MESSAGE " at Line: %d at Position: %d!",              \
                  TOKEN(translator)->position.line,                     \
                  TOKEN(translator)->position.position);                \
                                                                        \
      showSourceLine(translator, TOKEN(translator)->position);          \
                                                                        \
      CLEAN_RESOURCES(REMOVE_TABLE __VA_OPT__(,) __VA_ARGS__);          \
    } while (0)

#define CLEAN_RESOURCES(REMOVE_TABLE, ...)        \
  do                                              \
    {                                             \
      if (REMOVE_TABLE)                           \
        db::removeVarTable(translator, error);    \
      removeAllTokens(                            \
                      COUNT_OF_ARGS(__VA_ARGS__)  \
                      __VA_OPT__(,) __VA_ARGS__); \
      ERROR(nullptr);                             \
    } while (0)


#define TOKEN(TRANSLATOR) (*TRANSLATOR->tokens)
#define INCREASE_TOKENS(TRANSLATOR) (++TRANSLATOR->tokens)
#define CLEAN_TOKEN(TRANSLATOR)                 \
  do                                            \
    {                                           \
      db::removeNode(TOKEN(TRANSLATOR));        \
      INCREASE_TOKENS(TRANSLATOR);              \
    } while (0)

#define CMD(LEFT, RIGHT)                                \
  db::createNode(                                       \
                 db::treeValue_t {.statement =          \
                   db::STATEMENT_COMPOUND},             \
                 db::type_t::STATEMENT, LEFT, RIGHT)

#define PARAM(LEFT, RIGHT)                          \
  db::createNode(                                   \
                 db::treeValue_t {.statement =      \
                   db::STATEMENT_PARAMETER},        \
                 db::type_t::STATEMENT, LEFT, RIGHT)

#define VARIABLE(LEFT, RIGHT)                       \
  db::createNode(                                   \
                 db::treeValue_t {.statement =      \
                   db::STATEMENT_VAR},              \
                 db::type_t::STATEMENT, LEFT, RIGHT)

#define CHECK_OPEN(TRANSLATOR, FIRST_TOKEN)                           \
  do                                                                  \
    {                                                                 \
      if (!IS_OPEN(TOKEN(TRANSLATOR)))                                \
        HANDLE_ERROR("Expected (", FIRST_TOKEN);                      \
      db::removeNode(TOKEN(TRANSLATOR));                              \
      INCREASE_TOKENS(TRANSLATOR);                                    \
    } while (0)

#define CHECK_CLOSE(TRANSLATOR, FIRST_TOKEN, SECOND_TOKEN)            \
  do                                                                  \
    {                                                                 \
     if (!IS_CLOSE(TOKEN(TRANSLATOR)))                                \
       HANDLE_ERROR("Expected )", FIRST_TOKEN, SECOND_TOKEN);         \
     db::removeNode(TOKEN(TRANSLATOR));                               \
     INCREASE_TOKENS(TRANSLATOR);                                     \
    } while (0)

#define CHECK_ARGS(...)                                           \
  do                                                              \
    {                                                             \
      if (!translator || !translator->tokens || !fail || !error)  \
        FAIL(__VA_ARGS__);                                        \
    } while (0)


#define CHECK(IS_IT, SEQUENCE, FOR_FREE)                                \
  do                                                                    \
    {                                                                   \
      if (!IS_IT(TOKEN(translator)))                                    \
        HANDLE_ERROR("Expected " SEQUENCE, token, FOR_FREE);            \
      db::removeNode(TOKEN(translator));                                \
      INCREASE_TOKENS(translator);                                      \
    } while (0)

#define TRY_GET_CHAR(BOOL, TOKEN, GETTER)           \
  do                                                \
    {                                               \
      if (BOOL)                                     \
        {                                           \
          BOOL = false;                             \
          TOKEN = GETTER(translator, &BOOL, error); \
          if (*error) ERROR(nullptr);               \
        }                                           \
    } while (0)

static void showSourceLine(db::Translator *translator, db::PositionInfo info);

static void showSourceLine(db::Translator *translator, db::PositionInfo info)
{
  static bool wasCall = false;

  if (!translator || !translator->status.sourceName || wasCall) return;

  FILE *source = fopen(translator->status.sourceName, "r");
  if (!source) return;

  for (int i = 1; i < info.line; ++i) { fscanf(source, "%*[^\n]"); getc(source); };
  printf("%4.4d | ", info.line);

  for (int i = 1; i < info.position; ++i) putchar(getc(source));

  printf(FG_YELLOW);
  for (int ch = '\0'; ch != '\n' && ch != EOF; ) putchar(ch = getc(source));

  printf(RESET);

  fclose(source);

  wasCall = true;
}

static db::Token createName(db::name_t value)
{
  return db::createNode({.name = value}, db::type_t::NAME);
}

static db::Token createStatement(db::statement_t value)
{
  return db::createNode({.statement = value}, db::type_t::STATEMENT);
}

static void upStatic(db::Translator *translator, int *error);

static void updateMain(db::Token main, db::Translator *translator, int *error);

static void updateStartMain(db::Token main, db::Translator *translator, int *error);

static void updateEndMain(db::Token main, db::Translator *translator, int *error);

static void updateReturn(db::Token calls, db::Token *token, int *error);

static db::Token callFunction(const db::Token parameters, db::Token arguments, bool *fail);

static void removeAllTokens(int paramN, ...);

static void updateMain(db::Token main, db::Translator *translator, int *error)
{
  if (!main || !translator || !error) ERROR();

  updateStartMain(main, translator, error);
  if (*error) ERROR();

  updateEndMain(main, translator, error);
  if (*error) ERROR();
}

static void updateStartMain(db::Token main, db::Translator *translator, int *error)
{
  if (!main || !translator) ERROR();

  if (!translator->previousStaticBlocks.size) return;

  db::FunTable *table = &translator->previousStaticBlocks;

  db::Token start = nullptr;
  db::Token *freePosition = &start;

  for (size_t i = 0; i < table->size; ++i)
    {
      db::Token tempToken =
        ST(db::statement_t::STATEMENT_CALL);
      tempToken->left = NAM((char *)table->table[i].name);

      *freePosition = CMD(tempToken, nullptr);
      freePosition = &(*freePosition)->right;
    }

  *freePosition = main->right;

  main->right = start;
}

static void updateEndMain(db::Token main, db::Translator *translator, int *error)
{
  if (!main || !translator) ERROR();

  if (!translator->nextStaticBlocks.size) return;

  db::FunTable *table = &translator->nextStaticBlocks;

  db::Token end = nullptr;
  db::Token *freePosition = &end;

  for (size_t i = 0; i < table->size; ++i)
    {
      db::Token tempToken =
        ST(db::statement_t::STATEMENT_CALL);
      tempToken->left = NAM((char *)table->table[i].name);

      *freePosition = CMD(tempToken, nullptr);
      freePosition = &(*freePosition)->right;
    }

  updateReturn(end, &main->right, error);

  db::Token temp = main->right;
  for ( ; temp->right; temp = temp->right) continue;

  temp->right = end;

}

static void updateReturn(db::Token calls, db::Token *token, int *error)
{
  if (IS_RETURN((*token)->left))
    {
      db::Token temp = db::createNode(calls);
      for ( ; temp->right; temp = temp->right) continue;
      temp->right = *token;

      *token = temp;

      return;
    }

  if ((*token)->left ) updateReturn(calls, &(*token)->left , error);
  if (*error) ERROR();
  if ((*token)->right) updateReturn(calls, &(*token)->right, error);
  if (*error) ERROR();
}

static void upStatic(db::Translator *translator, int *error)
{
  if(!translator) ERROR();

  db::Token token = translator->grammar.root;
  db::Token previous = nullptr;
  db::Token lastStatic = nullptr;

  for ( ; token; previous = token, token = token->right)
    {
      if (!IS_FUN(token->left))              break;
      if (NAME(token->left->left)[0] != '$') break;
      lastStatic = token;
    }

  for ( ; token; previous = token, token = (token ? token->right: nullptr))
    {
      if (!IS_FUN(token->left))              continue;
      if (NAME(token->left->left)[0] != '$') continue;

      previous->right = token->right;

      if (!lastStatic)
        {
          lastStatic = token;
          token->right = translator->grammar.root;
          translator->grammar.root = token;

          continue;
        }

      db::Token temp = token->right;

      previous->right = token->right;
      token->right = lastStatic->right;
      lastStatic->right = token;
      lastStatic = token;
      token = temp;
    }

  previous = nullptr;
  db::Token main = nullptr;

  db::Token temp = translator->grammar.root;
  for ( ; temp; previous = temp, temp = temp->right)
    {
      if (!IS_FUN(temp->left)) continue;
      if (!db::compareStrings(
                             NAME(temp->left->left),
                             "main"
                            ))  continue;

      main = temp;
      previous->right = temp->right;
      main->right = nullptr;
      break;
    }

  temp = translator->grammar.root;
  for ( ; temp->right; temp = temp->right) continue;

  db::Token newFirst = lastStatic->right;
  lastStatic->right = main;

  temp->right = translator->grammar.root;
  translator->grammar.root = newFirst;
}

static db::Token callFunction(const db::Token parameters, db::Token arguments, bool *fail)
{
  if (!fail) FAIL(nullptr);

  db::Token params  = parameters;
  db::Token args    = arguments;
  db::Token prevArg = nullptr;
  for ( ; params; params = params->right)
    {
      if (!args)
        {
          if (!params->left->right)
            {
              handleError("%s must be initialized!", params->left->left);
              *fail = true;
              break;
            }
          if (prevArg)
            {
              prevArg->right = PARAM(db::createNode(params->left->right), nullptr);
              args = prevArg->right;
            }
          else
            {
              arguments = PARAM(db::createNode(params->left->right), nullptr);
              args = arguments;
            }
        }
      prevArg = args;
      args = args->right;
    }
  if (args != nullptr) *fail = true;

  return arguments;
}

static void removeAllTokens(int paramN, ...)
{
  va_list args{};

  va_start(args, paramN);

  db::Token token = va_arg(args, db::Token);
  for (int i = 0; i < paramN; ++i)
    {
      if (token) db::removeNode(token);
      token = va_arg(args, db::Token);
    }

  va_end(args);
}

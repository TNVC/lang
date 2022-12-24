#pragma once

#define NUM(VALUE) createNumber   (VALUE)

#define NAM(VALUE) createName     (VALUE)

#define VAR(VALUE) createName     (VALUE)
#define STR(VALUE) createString   (VALUE)
#define  ST(VALUE) createStatement(VALUE)

#define  NUM_VAL(NODE) NODE->value.number
#define   ST_VAL(NODE) NODE->value.statement
#define NAME_VAL(NODE) NODE->value.name
#define  STR_VAL(NODE) NODE->value.string

#define Left  node->left
#define Right node->right

#define CREATE_STATEMENT(TYPE, LEFT, RIGHT)         \
  db::createNode(                                  \
                 {db::STATEMENT_ ## TYPE},         \
                 db::type_t::STATEMENT,            \
                 (LEFT),                           \
                 (RIGHT)                           \
                )

#define ADD(LEFT, RIGHT) CREATE_STATEMENT(ADD , LEFT   , RIGHT)
#define SUB(LEFT, RIGHT) CREATE_STATEMENT(SUB , LEFT   , RIGHT)
#define MUL(LEFT, RIGHT) CREATE_STATEMENT(MUL , LEFT   , RIGHT)
#define DIV(LEFT, RIGHT) CREATE_STATEMENT(DIV , LEFT   , RIGHT)
#define SIN(VALUE)       CREATE_STATEMENT(SIN , nullptr, VALUE)
#define COS(VALUE)       CREATE_STATEMENT(COS , nullptr, VALUE)
#define SQRT(VALUE)      CREATE_STATEMENT(SQRT, nullptr, VALUE)

#define IS_ADD(NODE)         IS_IT_STATEMENT(NODE, ADD        )
#define IS_SUB(NODE)         IS_IT_STATEMENT(NODE, SUB        )
#define IS_MUL(NODE)         IS_IT_STATEMENT(NODE, MUL        )
#define IS_DIV(NODE)         IS_IT_STATEMENT(NODE, DIV        )
#define IS_SIN(NODE)         IS_IT_STATEMENT(NODE, SIN        )
#define IS_COS(NODE)         IS_IT_STATEMENT(NODE, COS        )
#define IS_SQRT(NODE)        IS_IT_STATEMENT(NODE, SQRT       )
#define IS_OPEN(NODE)        IS_IT_STATEMENT(NODE, OPEN       )
#define IS_CLOSE(NODE)       IS_IT_STATEMENT(NODE, CLOSE      )
#define IS_END(NODE)         IS_IT_STATEMENT(NODE, END        )
#define IS_ERROR(NODE)       IS_IT_STATEMENT(NODE, ERROR      )
#define IS_ASSIGN(NODE)      IS_IT_STATEMENT(NODE, ASSIGNMENT )
#define IS_IF(NODE)          IS_IT_STATEMENT(NODE, IF         )
#define IS_ELSE(NODE)        IS_IT_STATEMENT(NODE, ELSE       )
#define IS_SEM(NODE)         IS_IT_STATEMENT(NODE, SEMICOLON  )
#define IS_WHILE(NODE)       IS_IT_STATEMENT(NODE, WHILE      )
#define IS_START_BRACE(NODE) IS_IT_STATEMENT(NODE, START_BRACE)
#define IS_END_BRACE(NODE)   IS_IT_STATEMENT(NODE, END_BRACE  )
#define IS_RETURN(NODE)      IS_IT_STATEMENT(NODE, RETURN     )
#define IS_FUN(NODE)         IS_IT_STATEMENT(NODE, FUN        )
#define IS_TYPE(NODE)        IS_IT_STATEMENT(NODE, TYPE       )
#define IS_VOID(NODE)        IS_IT_STATEMENT(NODE, VOID       )
#define IS_COL(NODE)         IS_IT_STATEMENT(NODE, COLON      )
#define IS_VAR(NODE)         IS_IT_STATEMENT(NODE, VAR        )
#define IS_VAL(NODE)         IS_IT_STATEMENT(NODE, VAL        )
#define IS_COMMA(NODE)       IS_IT_STATEMENT(NODE, COMMA      )
#define IS_COMP(NODE)        IS_IT_STATEMENT(NODE, COMPOUND   )
#define IS_OUT(NODE)         IS_IT_STATEMENT(NODE, OUT        )
#define IS_IN(NODE)          IS_IT_STATEMENT(NODE, IN         )
#define IS_LESS(NODE)        IS_IT_STATEMENT(NODE, LESS       )
#define IS_GREATER(NODE)     IS_IT_STATEMENT(NODE, GREATER    )
#define IS_NOT(NODE)         IS_IT_STATEMENT(NODE, NOT        )
#define IS_AND(NODE)         IS_IT_STATEMENT(NODE, AND        )
#define IS_OR(NODE)          IS_IT_STATEMENT(NODE, OR         )
#define IS_ENDL(NODE)        IS_IT_STATEMENT(NODE, NEW_LINE   )
#define IS_OUTPUT(NODE)      IS_IT_STATEMENT(NODE, OUTPUT     )
#define IS_INPUT(NODE)       IS_IT_STATEMENT(NODE, INPUT      )
#define IS_EQUAL(NODE)       IS_IT_STATEMENT(NODE, EQUAL      )
#define IS_NOT_EQUAL(NODE)   IS_IT_STATEMENT(NODE, NOT_EQUAL  )
#define IS_LESS_EQ(NODE)     IS_IT_STATEMENT(NODE, LESS_OR_EQUAL   )
#define IS_GREATER_EQ(NODE)  IS_IT_STATEMENT(NODE, GREATER_OR_EQUAL)
#define IS_STATIC(NODE)      IS_IT_STATEMENT(NODE, STATIC     )
#define IS_START_SQUARE_BRACE(NODE) IS_IT_STATEMENT(NODE, START_SQUARE_BRACE)
#define IS_END_SQUARE_BRACE(NODE) IS_IT_STATEMENT(NODE, END_SQUARE_BRACE)
#define IS_INT(NODE)         IS_IT_STATEMENT(NODE, INT        )
#define IS_PARAM(NODE)       IS_IT_STATEMENT(NODE, PARAMETER  )
#define IS_TAN(NODE)         IS_IT_STATEMENT(NODE, TAN        )
#define IS_DIFF(NODE)        IS_IT_STATEMENT(NODE, DIFF       )

#define IS_NUM(NODE)       (NODE->type == db::type_t::NUMBER   )
#define IS_STATEMENT(NODE) (NODE->type == db::type_t::STATEMENT)
#define IS_NAME(NODE)      (NODE->type == db::type_t::NAME     )
#define IS_STRING(NODE)    (NODE->type == db::type_t::STRING   )

#define IS_FUNCTION(NODE)                       \
  (IS_ADD(NODE)  ||                             \
   IS_SUB(NODE)  ||                             \
   IS_SIN(NODE)  ||                             \
   IS_COS(NODE)  ||                             \
   IS_TAN(NODE)  ||                             \
   IS_DIFF(NODE) ||                             \
   IS_SQRT(NODE))

#define IS_IT_STATEMENT(NODE, STATEMENT_NAME)                                \
  ((NODE) && IS_STATEMENT((NODE)) && STATEMENT((NODE)) == db::STATEMENT_ ## STATEMENT_NAME)

#define NUMBER(NODE)    (NODE->value.number)
#define STATEMENT(NODE) (NODE->value.statement)
#define NAME(NODE)      (NODE->value.name)
#define STRING(NODE)    (NODE->value.string)

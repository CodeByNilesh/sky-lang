/* value.h — Value types for Sky VM */
#ifndef SKY_VALUE_H
#define SKY_VALUE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    VAL_NIL,
    VAL_BOOL,
    VAL_INT,
    VAL_FLOAT,
    VAL_STRING,
    VAL_ARRAY,
    VAL_MAP,
    VAL_FUNCTION,
    VAL_NATIVE_FN,
    VAL_CLASS,
    VAL_INSTANCE
} SkyValueType;

typedef struct SkyValue SkyValue;

typedef SkyValue (*SkyNativeFn)(int arg_count, SkyValue *args);

struct SkyValue {
    SkyValueType type;
    union {
        bool       boolean;
        int64_t    integer;
        double     floating;
        char      *string;
        struct {
            SkyValue *items;
            int       count;
            int       capacity;
        } array;
        void      *object;
        SkyNativeFn native_fn;
    } as;
};

typedef struct {
    SkyValue *values;
    int       count;
    int       capacity;
} SkyValueArray;

void sky_value_array_init(SkyValueArray *arr);
void sky_value_array_write(SkyValueArray *arr, SkyValue value);
void sky_value_array_free(SkyValueArray *arr);

#define SKY_NIL()        ((SkyValue){VAL_NIL,    {.integer = 0}})
#define SKY_BOOL(v)      ((SkyValue){VAL_BOOL,   {.boolean = (v)}})
#define SKY_INT(v)       ((SkyValue){VAL_INT,    {.integer = (v)}})
#define SKY_FLOAT(v)     ((SkyValue){VAL_FLOAT,  {.floating = (v)}})
#define SKY_STRING(v)    ((SkyValue){VAL_STRING, {.string = (char*)(v)}})

#define IS_NIL(v)    ((v).type == VAL_NIL)
#define IS_BOOL(v)   ((v).type == VAL_BOOL)
#define IS_INT(v)    ((v).type == VAL_INT)
#define IS_FLOAT(v)  ((v).type == VAL_FLOAT)
#define IS_STRING(v) ((v).type == VAL_STRING)

bool sky_values_equal(SkyValue a, SkyValue b);
void sky_print_value(SkyValue value);
SkyValue sky_value_copy(SkyValue value);
void sky_value_free(SkyValue *value);

#endif

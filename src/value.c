/* value.c — Value operations implementation */
#include "value.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void sky_value_array_init(SkyValueArray *arr) {
    arr->values = NULL;
    arr->count = 0;
    arr->capacity = 0;
}

void sky_value_array_write(SkyValueArray *arr, SkyValue value) {
    if (arr->count >= arr->capacity) {
        int new_cap = arr->capacity < 8 ? 8 : arr->capacity * 2;
        arr->values = (SkyValue*)realloc(arr->values, sizeof(SkyValue) * new_cap);
        arr->capacity = new_cap;
    }
    arr->values[arr->count++] = value;
}

void sky_value_array_free(SkyValueArray *arr) {
    free(arr->values);
    sky_value_array_init(arr);
}

bool sky_values_equal(SkyValue a, SkyValue b) {
    if (a.type != b.type) return false;
    switch (a.type) {
        case VAL_NIL: return true;
        case VAL_BOOL: return a.as.boolean == b.as.boolean;
        case VAL_INT: return a.as.integer == b.as.integer;
        case VAL_FLOAT: return a.as.floating == b.as.floating;
        case VAL_STRING:
            if (!a.as.string && !b.as.string) return true;
            if (!a.as.string || !b.as.string) return false;
            return strcmp(a.as.string, b.as.string) == 0;
        default: return false;
    }
}

void sky_print_value(SkyValue value) {
    switch (value.type) {
        case VAL_NIL: printf("nil"); break;
        case VAL_BOOL: printf("%s", value.as.boolean ? "true" : "false"); break;
        case VAL_INT: printf("%lld", (long long)value.as.integer); break;
        case VAL_FLOAT: printf("%g", value.as.floating); break;
        case VAL_STRING: printf("%s", value.as.string ? value.as.string : ""); break;
        case VAL_ARRAY: printf("[array]"); break;
        case VAL_MAP: printf("{map}"); break;
        case VAL_FUNCTION: printf("<fn>"); break;
        case VAL_NATIVE_FN: printf("<native>"); break;
        case VAL_CLASS: printf("<class>"); break;
        case VAL_INSTANCE: printf("<instance>"); break;
        default: printf("<unknown>"); break;
    }
}

SkyValue sky_value_copy(SkyValue value) {
    SkyValue copy = value;
    if (value.type == VAL_STRING && value.as.string) {
        size_t len = strlen(value.as.string);
        copy.as.string = (char*)malloc(len + 1);
        memcpy(copy.as.string, value.as.string, len + 1);
    }
    return copy;
}

void sky_value_free(SkyValue *value) {
    (void)value;
}

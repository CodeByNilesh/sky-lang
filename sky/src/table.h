/* table.h — Hash table header */
#ifndef SKY_TABLE_H
#define SKY_TABLE_H

#include <stdint.h>
#include <stdbool.h>
#include "value.h"

typedef struct {
    char      *key;
    SkyValue   value;
    bool       occupied;
} SkyTableEntry;

typedef struct {
    SkyTableEntry *entries;
    int            count;
    int            capacity;
} SkyTable;

void sky_table_init(SkyTable *table);
void sky_table_free(SkyTable *table);
bool sky_table_set(SkyTable *table, const char *key, SkyValue value);
bool sky_table_get(SkyTable *table, const char *key, SkyValue *out);
bool sky_table_delete(SkyTable *table, const char *key);
void sky_table_copy(SkyTable *from, SkyTable *to);

#endif

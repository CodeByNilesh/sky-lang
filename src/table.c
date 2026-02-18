/* table.c — Hash table implementation */
#include "table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TABLE_MAX_LOAD 0.75

static uint32_t hash_string(const char *key) {
    uint32_t h = 2166136261u;
    while (*key) {
        h ^= (uint8_t)*key++;
        h *= 16777619u;
    }
    return h;
}

void sky_table_init(SkyTable *table) {
    table->entries = NULL;
    table->count = 0;
    table->capacity = 0;
}

void sky_table_free(SkyTable *table) {
    int i;
    if (table->entries) {
        for (i = 0; i < table->capacity; i++) {
            if (table->entries[i].occupied && table->entries[i].key) {
                free(table->entries[i].key);
            }
        }
        free(table->entries);
    }
    sky_table_init(table);
}

static SkyTableEntry* find_entry(SkyTableEntry *entries, int capacity, const char *key) {
    uint32_t index = hash_string(key) % (uint32_t)capacity;
    SkyTableEntry *tombstone = NULL;
    while (1) {
        SkyTableEntry *entry = &entries[index];
        if (!entry->occupied) {
            if (entry->key == NULL) {
                return tombstone != NULL ? tombstone : entry;
            } else {
                if (tombstone == NULL) tombstone = entry;
            }
        } else if (entry->key && strcmp(entry->key, key) == 0) {
            return entry;
        }
        index = (index + 1) % (uint32_t)capacity;
    }
}

static void adjust_capacity(SkyTable *table, int capacity) {
    int i;
    SkyTableEntry *entries = (SkyTableEntry*)calloc(capacity, sizeof(SkyTableEntry));
    table->count = 0;
    for (i = 0; i < table->capacity; i++) {
        SkyTableEntry *entry = &table->entries[i];
        if (!entry->occupied) continue;
        SkyTableEntry *dest = find_entry(entries, capacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        dest->occupied = true;
        table->count++;
    }
    free(table->entries);
    table->entries = entries;
    table->capacity = capacity;
}

bool sky_table_set(SkyTable *table, const char *key, SkyValue value) {
    SkyTableEntry *entry;
    bool is_new;
    if (table->count + 1 > (int)(table->capacity * TABLE_MAX_LOAD)) {
        int capacity = table->capacity < 8 ? 8 : table->capacity * 2;
        adjust_capacity(table, capacity);
    }
    entry = find_entry(table->entries, table->capacity, key);
    is_new = !entry->occupied;
    if (is_new) {
        table->count++;
        entry->key = (char*)malloc(strlen(key) + 1);
        strcpy(entry->key, key);
    }
    entry->value = value;
    entry->occupied = true;
    return is_new;
}

bool sky_table_get(SkyTable *table, const char *key, SkyValue *out) {
    SkyTableEntry *entry;
    if (table->count == 0) return false;
    entry = find_entry(table->entries, table->capacity, key);
    if (!entry->occupied) return false;
    *out = entry->value;
    return true;
}

bool sky_table_delete(SkyTable *table, const char *key) {
    SkyTableEntry *entry;
    if (table->count == 0) return false;
    entry = find_entry(table->entries, table->capacity, key);
    if (!entry->occupied) return false;
    free(entry->key);
    entry->key = NULL;
    entry->occupied = false;
    return true;
}

void sky_table_copy(SkyTable *from, SkyTable *to) {
    int i;
    for (i = 0; i < from->capacity; i++) {
        SkyTableEntry *entry = &from->entries[i];
        if (entry->occupied) {
            sky_table_set(to, entry->key, entry->value);
        }
    }
}

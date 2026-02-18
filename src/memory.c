/* memory.c — Memory management implementation */
#include "memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static SkyMemoryStats g_stats = {0};

void* sky_alloc(size_t size, const char *file, int line) {
    void *ptr;
    (void)file;
    (void)line;
    if (size == 0) return NULL;
    ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "[SKY MEMORY] Allocation failed: %zu bytes at %s:%d\n", size, file, line);
        exit(1);
    }
    g_stats.bytes_allocated += size;
    g_stats.allocation_count++;
    if (g_stats.bytes_allocated - g_stats.bytes_freed > g_stats.peak_usage) {
        g_stats.peak_usage = g_stats.bytes_allocated - g_stats.bytes_freed;
    }
    return ptr;
}

void* sky_realloc(void *ptr, size_t old_size, size_t new_size, const char *file, int line) {
    void *new_ptr;
    (void)file;
    (void)line;
    if (new_size == 0) {
        free(ptr);
        g_stats.bytes_freed += old_size;
        g_stats.free_count++;
        return NULL;
    }
    new_ptr = realloc(ptr, new_size);
    if (!new_ptr) {
        fprintf(stderr, "[SKY MEMORY] Realloc failed: %zu bytes at %s:%d\n", new_size, file, line);
        exit(1);
    }
    g_stats.bytes_freed += old_size;
    g_stats.bytes_allocated += new_size;
    if (g_stats.bytes_allocated - g_stats.bytes_freed > g_stats.peak_usage) {
        g_stats.peak_usage = g_stats.bytes_allocated - g_stats.bytes_freed;
    }
    return new_ptr;
}

void sky_free(void *ptr, size_t size, const char *file, int line) {
    (void)file;
    (void)line;
    if (!ptr) return;
    free(ptr);
    g_stats.bytes_freed += size;
    g_stats.free_count++;
}

SkyMemoryStats sky_memory_get_stats(void) {
    return g_stats;
}

void sky_memory_report(void) {
    printf("[SKY MEMORY] Allocated: %zu bytes (%zu calls)\n", g_stats.bytes_allocated, g_stats.allocation_count);
    printf("[SKY MEMORY] Freed: %zu bytes (%zu calls)\n", g_stats.bytes_freed, g_stats.free_count);
    printf("[SKY MEMORY] Peak usage: %zu bytes\n", g_stats.peak_usage);
}

char* sky_strdup(const char *str) {
    size_t len;
    char *copy;
    if (!str) return NULL;
    len = strlen(str);
    copy = (char*)malloc(len + 1);
    if (copy) memcpy(copy, str, len + 1);
    return copy;
}

char* sky_strndup(const char *str, size_t len) {
    char *copy;
    if (!str) return NULL;
    copy = (char*)malloc(len + 1);
    if (copy) {
        memcpy(copy, str, len);
        copy[len] = '\0';
    }
    return copy;
}

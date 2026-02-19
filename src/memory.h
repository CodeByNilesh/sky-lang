/* memory.h — Memory management header */
#ifndef SKY_MEMORY_H
#define SKY_MEMORY_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    size_t bytes_allocated;
    size_t bytes_freed;
    size_t allocation_count;
    size_t free_count;
    size_t peak_usage;
} SkyMemoryStats;

void* sky_alloc(size_t size, const char *file, int line);
void* sky_realloc(void *ptr, size_t old_size, size_t new_size, const char *file, int line);
void  sky_free(void *ptr, size_t size, const char *file, int line);

#define SKY_ALLOC(size) sky_alloc(size, __FILE__, __LINE__)
#define SKY_REALLOC(ptr, old, new_sz) sky_realloc(ptr, old, new_sz, __FILE__, __LINE__)
#define SKY_FREE(ptr, size) sky_free(ptr, size, __FILE__, __LINE__)

SkyMemoryStats sky_memory_get_stats(void);
void sky_memory_report(void);
char* sky_strdup(const char *str);
char* sky_strndup(const char *str, size_t len);

#endif

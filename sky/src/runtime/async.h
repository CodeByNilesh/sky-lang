/* src/runtime/async.h — Async I/O engine header */
#ifndef SKY_ASYNC_H
#define SKY_ASYNC_H

#include <stdint.h>
#include <stdbool.h>
#include "../platform.h"

#define SKY_ASYNC_MAX_EVENTS   1024
#define SKY_ASYNC_MAX_TASKS    4096
#define SKY_ASYNC_THREAD_POOL  4

typedef enum {
    SKY_TASK_PENDING,
    SKY_TASK_RUNNING,
    SKY_TASK_COMPLETED,
    SKY_TASK_FAILED,
    SKY_TASK_CANCELLED
} SkyTaskState;

typedef void* (*SkyTaskFunc)(void *arg);
typedef void  (*SkyTaskCallback)(void *result, void *user_data);

typedef struct {
    uint32_t          id;
    SkyTaskState      state;
    SkyTaskFunc       func;
    void             *arg;
    void             *result;
    SkyTaskCallback   on_complete;
    void             *user_data;
    bool              active;
} SkyAsyncTask;

typedef struct {
    sky_thread_t      threads[SKY_ASYNC_THREAD_POOL];
    SkyAsyncTask      tasks[SKY_ASYNC_MAX_TASKS];
    uint32_t          task_head;
    uint32_t          task_tail;
    uint32_t          task_count;
    uint32_t          next_id;
    sky_mutex_t       lock;
    sky_cond_t        cond;
    bool              running;
} SkyAsyncEngine;

bool sky_async_init(SkyAsyncEngine *engine);
void sky_async_destroy(SkyAsyncEngine *engine);
uint32_t sky_async_submit(SkyAsyncEngine *engine, SkyTaskFunc func, void *arg, SkyTaskCallback on_complete, void *user_data);
SkyTaskState sky_async_status(SkyAsyncEngine *engine, uint32_t task_id);
void* sky_async_wait(SkyAsyncEngine *engine, uint32_t task_id);

#endif

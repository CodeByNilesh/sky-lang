/* src/runtime/async.c — Async I/O engine implementation */
#include "async.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void* worker_thread(void *arg) {
    SkyAsyncEngine *engine = (SkyAsyncEngine*)arg;
    while (engine->running) {
        SkyAsyncTask *task = NULL;
        uint32_t idx;
        sky_mutex_lock(&engine->lock);
        while (engine->task_count == 0 && engine->running) {
            sky_cond_wait(&engine->cond, &engine->lock);
        }
        if (!engine->running) {
            sky_mutex_unlock(&engine->lock);
            break;
        }
        idx = engine->task_tail % SKY_ASYNC_MAX_TASKS;
        task = &engine->tasks[idx];
        engine->task_tail++;
        engine->task_count--;
        task->state = SKY_TASK_RUNNING;
        sky_mutex_unlock(&engine->lock);
        if (task->func) {
            task->result = task->func(task->arg);
            task->state = SKY_TASK_COMPLETED;
            if (task->on_complete) {
                task->on_complete(task->result, task->user_data);
            }
        } else {
            task->state = SKY_TASK_FAILED;
        }
    }
    return NULL;
}

bool sky_async_init(SkyAsyncEngine *engine) {
    int i;
    if (!engine) return false;
    memset(engine, 0, sizeof(SkyAsyncEngine));
    engine->running = true;
    engine->next_id = 1;
    sky_mutex_init(&engine->lock);
    sky_cond_init(&engine->cond);
    for (i = 0; i < SKY_ASYNC_THREAD_POOL; i++) {
        if (sky_thread_create(&engine->threads[i], worker_thread, engine) != 0) {
            fprintf(stderr, "[SKY ASYNC] Failed to create thread %d\n", i);
            engine->running = false;
            return false;
        }
    }
    fprintf(stderr, "[SKY ASYNC] Thread pool started (%d threads)\n", SKY_ASYNC_THREAD_POOL);
    return true;
}

void sky_async_destroy(SkyAsyncEngine *engine) {
    int i;
    if (!engine) return;
    sky_mutex_lock(&engine->lock);
    engine->running = false;
    sky_cond_broadcast(&engine->cond);
    sky_mutex_unlock(&engine->lock);
    for (i = 0; i < SKY_ASYNC_THREAD_POOL; i++) {
        sky_thread_join(engine->threads[i]);
    }
    sky_mutex_destroy(&engine->lock);
    sky_cond_destroy(&engine->cond);
    fprintf(stderr, "[SKY ASYNC] Thread pool stopped\n");
}

uint32_t sky_async_submit(SkyAsyncEngine *engine, SkyTaskFunc func, void *arg, SkyTaskCallback on_complete, void *user_data) {
    uint32_t idx;
    SkyAsyncTask *task;
    if (!engine || !engine->running) return 0;
    sky_mutex_lock(&engine->lock);
    if (engine->task_count >= SKY_ASYNC_MAX_TASKS) {
        sky_mutex_unlock(&engine->lock);
        fprintf(stderr, "[SKY ASYNC] Task queue full!\n");
        return 0;
    }
    idx = engine->task_head % SKY_ASYNC_MAX_TASKS;
    task = &engine->tasks[idx];
    task->id = engine->next_id++;
    task->state = SKY_TASK_PENDING;
    task->func = func;
    task->arg = arg;
    task->result = NULL;
    task->on_complete = on_complete;
    task->user_data = user_data;
    task->active = true;
    engine->task_head++;
    engine->task_count++;
    sky_cond_signal(&engine->cond);
    sky_mutex_unlock(&engine->lock);
    return task->id;
}

SkyTaskState sky_async_status(SkyAsyncEngine *engine, uint32_t task_id) {
    uint32_t i;
    if (!engine) return SKY_TASK_FAILED;
    sky_mutex_lock(&engine->lock);
    for (i = 0; i < SKY_ASYNC_MAX_TASKS; i++) {
        if (engine->tasks[i].active && engine->tasks[i].id == task_id) {
            SkyTaskState state = engine->tasks[i].state;
            sky_mutex_unlock(&engine->lock);
            return state;
        }
    }
    sky_mutex_unlock(&engine->lock);
    return SKY_TASK_FAILED;
}

void* sky_async_wait(SkyAsyncEngine *engine, uint32_t task_id) {
    uint32_t i;
    if (!engine) return NULL;
    while (1) {
        sky_mutex_lock(&engine->lock);
        for (i = 0; i < SKY_ASYNC_MAX_TASKS; i++) {
            if (engine->tasks[i].active && engine->tasks[i].id == task_id) {
                if (engine->tasks[i].state == SKY_TASK_COMPLETED || engine->tasks[i].state == SKY_TASK_FAILED) {
                    void *result = engine->tasks[i].result;
                    engine->tasks[i].active = false;
                    sky_mutex_unlock(&engine->lock);
                    return result;
                }
                break;
            }
        }
        sky_mutex_unlock(&engine->lock);
        sky_sleep_ms(1);
    }
}

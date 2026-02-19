/* vm.h — Virtual machine header */
#ifndef SKY_VM_H
#define SKY_VM_H

#include "bytecode.h"
#include "value.h"
#include "table.h"

#define SKY_STACK_MAX 1024
#define SKY_MAX_CALL_FRAMES 64
#define SKY_MAX_LOCALS 256

typedef enum {
    VM_OK,
    VM_COMPILE_ERROR,
    VM_RUNTIME_ERROR
} SkyVMResult;

typedef struct {
    SkyChunk *chunk;
    uint8_t  *ip;
    SkyValue *slots;
} SkyCallFrame;

typedef struct {
    SkyCallFrame frames[SKY_MAX_CALL_FRAMES];
    int          frame_count;
    SkyValue     stack[SKY_STACK_MAX];
    SkyValue    *stack_top;
    SkyTable     globals;
    SkyTable     strings;
} SkyVM;

void        sky_vm_init(SkyVM *vm);
void        sky_vm_destroy(SkyVM *vm);
SkyVMResult sky_vm_execute(SkyVM *vm, SkyChunk *chunk);
void        sky_vm_push(SkyVM *vm, SkyValue value);
SkyValue    sky_vm_pop(SkyVM *vm);
SkyValue    sky_vm_peek(SkyVM *vm, int distance);
void        sky_vm_define_native(SkyVM *vm, const char *name, SkyNativeFn fn);

#endif

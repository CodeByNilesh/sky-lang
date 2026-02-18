/* vm.c — Virtual machine implementation */
#include "vm.h"
#include "debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static void runtime_error(SkyVM *vm, const char *format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "[SKY RUNTIME ERROR] ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
    (void)vm;
}

/* Native: print */
static SkyValue native_print(int arg_count, SkyValue *args) {
    int i;
    for (i = 0; i < arg_count; i++) {
        if (i > 0) printf(" ");
        switch (args[i].type) {
            case VAL_NIL: printf("nil"); break;
            case VAL_BOOL: printf("%s", args[i].as.boolean ? "true" : "false"); break;
            case VAL_INT: printf("%lld", (long long)args[i].as.integer); break;
            case VAL_FLOAT: printf("%g", args[i].as.floating); break;
            case VAL_STRING: printf("%s", args[i].as.string ? args[i].as.string : ""); break;
            default: printf("<object>"); break;
        }
    }
    printf("\n");
    return SKY_NIL();
}

/* Native: str (convert to string) */
static SkyValue native_str(int arg_count, SkyValue *args) {
    char buf[256];
    if (arg_count < 1) return SKY_STRING("");
    switch (args[0].type) {
        case VAL_INT: snprintf(buf, sizeof(buf), "%lld", (long long)args[0].as.integer); break;
        case VAL_FLOAT: snprintf(buf, sizeof(buf), "%g", args[0].as.floating); break;
        case VAL_BOOL: snprintf(buf, sizeof(buf), "%s", args[0].as.boolean ? "true" : "false"); break;
        case VAL_NIL: snprintf(buf, sizeof(buf), "nil"); break;
        case VAL_STRING: return args[0];
        default: snprintf(buf, sizeof(buf), "<object>"); break;
    }
    {
        size_t len = strlen(buf);
        char *s = (char*)malloc(len + 1);
        memcpy(s, buf, len + 1);
        return SKY_STRING(s);
    }
}

/* Native: len */
static SkyValue native_len(int arg_count, SkyValue *args) {
    if (arg_count < 1) return SKY_INT(0);
    if (args[0].type == VAL_STRING && args[0].as.string) {
        return SKY_INT((int64_t)strlen(args[0].as.string));
    }
    if (args[0].type == VAL_ARRAY) {
        return SKY_INT((int64_t)args[0].as.array.count);
    }
    return SKY_INT(0);
}

void sky_vm_init(SkyVM *vm) {
    if (!vm) return;
    memset(vm, 0, sizeof(SkyVM));
    vm->stack_top = vm->stack;
    vm->frame_count = 0;
    sky_table_init(&vm->globals);
    sky_table_init(&vm->strings);
    /* Register native functions */
    sky_vm_define_native(vm, "print", native_print);
    sky_vm_define_native(vm, "str", native_str);
    sky_vm_define_native(vm, "len", native_len);
}

void sky_vm_destroy(SkyVM *vm) {
    if (!vm) return;
    sky_table_free(&vm->globals);
    sky_table_free(&vm->strings);
}

void sky_vm_push(SkyVM *vm, SkyValue value) {
    if (vm->stack_top - vm->stack >= SKY_STACK_MAX) {
        runtime_error(vm, "Stack overflow");
        return;
    }
    *vm->stack_top = value;
    vm->stack_top++;
}

SkyValue sky_vm_pop(SkyVM *vm) {
    if (vm->stack_top <= vm->stack) {
        runtime_error(vm, "Stack underflow");
        return SKY_NIL();
    }
    vm->stack_top--;
    return *vm->stack_top;
}

SkyValue sky_vm_peek(SkyVM *vm, int distance) {
    return vm->stack_top[-1 - distance];
}

void sky_vm_define_native(SkyVM *vm, const char *name, SkyNativeFn fn) {
    SkyValue val;
    val.type = VAL_NATIVE_FN;
    val.as.native_fn = fn;
    sky_table_set(&vm->globals, name, val);
}

static uint8_t read_byte(SkyCallFrame *frame) {
    return *frame->ip++;
}

static uint16_t read_short(SkyCallFrame *frame) {
    uint16_t hi = frame->ip[0];
    uint16_t lo = frame->ip[1];
    frame->ip += 2;
    return (hi << 8) | lo;
}

static SkyValue read_constant(SkyCallFrame *frame) {
    uint8_t idx = read_byte(frame);
    return frame->chunk->constants.values[idx];
}

static char* concat_strings(const char *a, const char *b) {
    size_t la, lb;
    char *result;
    la = a ? strlen(a) : 0;
    lb = b ? strlen(b) : 0;
    result = (char*)malloc(la + lb + 1);
    if (!result) return NULL;
    if (a) memcpy(result, a, la);
    if (b) memcpy(result + la, b, lb);
    result[la + lb] = '\0';
    return result;
}

SkyVMResult sky_vm_execute(SkyVM *vm, SkyChunk *chunk) {
    SkyCallFrame *frame;
    uint8_t instruction;

    if (!vm || !chunk) return VM_RUNTIME_ERROR;

    vm->frame_count = 1;
    frame = &vm->frames[0];
    frame->chunk = chunk;
    frame->ip = chunk->code;
    frame->slots = vm->stack;

    while (1) {
        if (sky_debug_trace_execution) {
            sky_debug_print_stack(vm->stack, (int)(vm->stack_top - vm->stack));
            sky_disassemble_instruction(chunk, (int)(frame->ip - chunk->code));
        }

        instruction = read_byte(frame);

        switch (instruction) {
            case OP_NOP:
                break;

            case OP_CONSTANT: {
                SkyValue val = read_constant(frame);
                sky_vm_push(vm, val);
                break;
            }

            case OP_CONSTANT_LONG: {
                uint32_t idx = ((uint32_t)read_byte(frame) << 16) |
                               ((uint32_t)read_byte(frame) << 8) |
                               (uint32_t)read_byte(frame);
                if ((int)idx < chunk->constants.count) {
                    sky_vm_push(vm, chunk->constants.values[idx]);
                } else {
                    sky_vm_push(vm, SKY_NIL());
                }
                break;
            }

            case OP_TRUE:
                sky_vm_push(vm, SKY_BOOL(true));
                break;

            case OP_FALSE:
                sky_vm_push(vm, SKY_BOOL(false));
                break;

            case OP_NIL:
                sky_vm_push(vm, SKY_NIL());
                break;

            case OP_POP:
                sky_vm_pop(vm);
                break;

            case OP_DUP:
                sky_vm_push(vm, sky_vm_peek(vm, 0));
                break;

            case OP_GET_LOCAL: {
                uint8_t slot = read_byte(frame);
                sky_vm_push(vm, frame->slots[slot]);
                break;
            }

            case OP_SET_LOCAL: {
                uint8_t slot = read_byte(frame);
                frame->slots[slot] = sky_vm_peek(vm, 0);
                break;
            }

            case OP_GET_GLOBAL: {
                SkyValue name_val = read_constant(frame);
                SkyValue value;
                if (name_val.type != VAL_STRING || !name_val.as.string) {
                    runtime_error(vm, "Global name must be a string");
                    return VM_RUNTIME_ERROR;
                }
                if (!sky_table_get(&vm->globals, name_val.as.string, &value)) {
                    runtime_error(vm, "Undefined variable '%s'", name_val.as.string);
                    return VM_RUNTIME_ERROR;
                }
                sky_vm_push(vm, value);
                break;
            }

            case OP_SET_GLOBAL: {
                SkyValue name_val = read_constant(frame);
                if (name_val.type != VAL_STRING || !name_val.as.string) {
                    runtime_error(vm, "Global name must be a string");
                    return VM_RUNTIME_ERROR;
                }
                sky_table_set(&vm->globals, name_val.as.string, sky_vm_peek(vm, 0));
                break;
            }

            case OP_ADD: {
                SkyValue b = sky_vm_pop(vm);
                SkyValue a = sky_vm_pop(vm);
                if (a.type == VAL_INT && b.type == VAL_INT) {
                    sky_vm_push(vm, SKY_INT(a.as.integer + b.as.integer));
                } else if (a.type == VAL_FLOAT && b.type == VAL_FLOAT) {
                    sky_vm_push(vm, SKY_FLOAT(a.as.floating + b.as.floating));
                } else if (a.type == VAL_INT && b.type == VAL_FLOAT) {
                    sky_vm_push(vm, SKY_FLOAT((double)a.as.integer + b.as.floating));
                } else if (a.type == VAL_FLOAT && b.type == VAL_INT) {
                    sky_vm_push(vm, SKY_FLOAT(a.as.floating + (double)b.as.integer));
                } else if (a.type == VAL_STRING && b.type == VAL_STRING) {
                    char *result = concat_strings(a.as.string, b.as.string);
                    sky_vm_push(vm, SKY_STRING(result));
                } else {
                    runtime_error(vm, "Cannot add these types");
                    return VM_RUNTIME_ERROR;
                }
                break;
            }

            case OP_SUB: {
                SkyValue b = sky_vm_pop(vm);
                SkyValue a = sky_vm_pop(vm);
                if (a.type == VAL_INT && b.type == VAL_INT) {
                    sky_vm_push(vm, SKY_INT(a.as.integer - b.as.integer));
                } else if (a.type == VAL_FLOAT && b.type == VAL_FLOAT) {
                    sky_vm_push(vm, SKY_FLOAT(a.as.floating - b.as.floating));
                } else if (a.type == VAL_INT && b.type == VAL_FLOAT) {
                    sky_vm_push(vm, SKY_FLOAT((double)a.as.integer - b.as.floating));
                } else if (a.type == VAL_FLOAT && b.type == VAL_INT) {
                    sky_vm_push(vm, SKY_FLOAT(a.as.floating - (double)b.as.integer));
                } else {
                    runtime_error(vm, "Cannot subtract these types");
                    return VM_RUNTIME_ERROR;
                }
                break;
            }

            case OP_MUL: {
                SkyValue b = sky_vm_pop(vm);
                SkyValue a = sky_vm_pop(vm);
                if (a.type == VAL_INT && b.type == VAL_INT) {
                    sky_vm_push(vm, SKY_INT(a.as.integer * b.as.integer));
                } else if (a.type == VAL_FLOAT && b.type == VAL_FLOAT) {
                    sky_vm_push(vm, SKY_FLOAT(a.as.floating * b.as.floating));
                } else if (a.type == VAL_INT && b.type == VAL_FLOAT) {
                    sky_vm_push(vm, SKY_FLOAT((double)a.as.integer * b.as.floating));
                } else if (a.type == VAL_FLOAT && b.type == VAL_INT) {
                    sky_vm_push(vm, SKY_FLOAT(a.as.floating * (double)b.as.integer));
                } else {
                    runtime_error(vm, "Cannot multiply these types");
                    return VM_RUNTIME_ERROR;
                }
                break;
            }

            case OP_DIV: {
                SkyValue b = sky_vm_pop(vm);
                SkyValue a = sky_vm_pop(vm);
                if (b.type == VAL_INT && b.as.integer == 0) {
                    runtime_error(vm, "Division by zero");
                    return VM_RUNTIME_ERROR;
                }
                if (b.type == VAL_FLOAT && b.as.floating == 0.0) {
                    runtime_error(vm, "Division by zero");
                    return VM_RUNTIME_ERROR;
                }
                if (a.type == VAL_INT && b.type == VAL_INT) {
                    sky_vm_push(vm, SKY_INT(a.as.integer / b.as.integer));
                } else if (a.type == VAL_FLOAT && b.type == VAL_FLOAT) {
                    sky_vm_push(vm, SKY_FLOAT(a.as.floating / b.as.floating));
                } else if (a.type == VAL_INT && b.type == VAL_FLOAT) {
                    sky_vm_push(vm, SKY_FLOAT((double)a.as.integer / b.as.floating));
                } else if (a.type == VAL_FLOAT && b.type == VAL_INT) {
                    sky_vm_push(vm, SKY_FLOAT(a.as.floating / (double)b.as.integer));
                } else {
                    runtime_error(vm, "Cannot divide these types");
                    return VM_RUNTIME_ERROR;
                }
                break;
            }

            case OP_MOD: {
                SkyValue b = sky_vm_pop(vm);
                SkyValue a = sky_vm_pop(vm);
                if (a.type == VAL_INT && b.type == VAL_INT) {
                    if (b.as.integer == 0) {
                        runtime_error(vm, "Modulo by zero");
                        return VM_RUNTIME_ERROR;
                    }
                    sky_vm_push(vm, SKY_INT(a.as.integer % b.as.integer));
                } else {
                    runtime_error(vm, "Modulo requires integers");
                    return VM_RUNTIME_ERROR;
                }
                break;
            }

            case OP_NEGATE: {
                SkyValue a = sky_vm_pop(vm);
                if (a.type == VAL_INT) {
                    sky_vm_push(vm, SKY_INT(-a.as.integer));
                } else if (a.type == VAL_FLOAT) {
                    sky_vm_push(vm, SKY_FLOAT(-a.as.floating));
                } else {
                    runtime_error(vm, "Cannot negate this type");
                    return VM_RUNTIME_ERROR;
                }
                break;
            }

            case OP_NOT: {
                SkyValue a = sky_vm_pop(vm);
                if (a.type == VAL_BOOL) {
                    sky_vm_push(vm, SKY_BOOL(!a.as.boolean));
                } else if (a.type == VAL_NIL) {
                    sky_vm_push(vm, SKY_BOOL(true));
                } else {
                    sky_vm_push(vm, SKY_BOOL(false));
                }
                break;
            }

            case OP_EQUAL: {
                SkyValue b = sky_vm_pop(vm);
                SkyValue a = sky_vm_pop(vm);
                sky_vm_push(vm, SKY_BOOL(sky_values_equal(a, b)));
                break;
            }

            case OP_NOT_EQUAL: {
                SkyValue b = sky_vm_pop(vm);
                SkyValue a = sky_vm_pop(vm);
                sky_vm_push(vm, SKY_BOOL(!sky_values_equal(a, b)));
                break;
            }

            case OP_GREATER: {
                SkyValue b = sky_vm_pop(vm);
                SkyValue a = sky_vm_pop(vm);
                if (a.type == VAL_INT && b.type == VAL_INT) {
                    sky_vm_push(vm, SKY_BOOL(a.as.integer > b.as.integer));
                } else if (a.type == VAL_FLOAT && b.type == VAL_FLOAT) {
                    sky_vm_push(vm, SKY_BOOL(a.as.floating > b.as.floating));
                } else if (a.type == VAL_INT && b.type == VAL_FLOAT) {
                    sky_vm_push(vm, SKY_BOOL((double)a.as.integer > b.as.floating));
                } else if (a.type == VAL_FLOAT && b.type == VAL_INT) {
                    sky_vm_push(vm, SKY_BOOL(a.as.floating > (double)b.as.integer));
                } else {
                    runtime_error(vm, "Cannot compare these types");
                    return VM_RUNTIME_ERROR;
                }
                break;
            }

            case OP_GREATER_EQ: {
                SkyValue b = sky_vm_pop(vm);
                SkyValue a = sky_vm_pop(vm);
                if (a.type == VAL_INT && b.type == VAL_INT) {
                    sky_vm_push(vm, SKY_BOOL(a.as.integer >= b.as.integer));
                } else if (a.type == VAL_FLOAT && b.type == VAL_FLOAT) {
                    sky_vm_push(vm, SKY_BOOL(a.as.floating >= b.as.floating));
                } else {
                    runtime_error(vm, "Cannot compare these types");
                    return VM_RUNTIME_ERROR;
                }
                break;
            }

            case OP_LESS: {
                SkyValue b = sky_vm_pop(vm);
                SkyValue a = sky_vm_pop(vm);
                if (a.type == VAL_INT && b.type == VAL_INT) {
                    sky_vm_push(vm, SKY_BOOL(a.as.integer < b.as.integer));
                } else if (a.type == VAL_FLOAT && b.type == VAL_FLOAT) {
                    sky_vm_push(vm, SKY_BOOL(a.as.floating < b.as.floating));
                } else if (a.type == VAL_INT && b.type == VAL_FLOAT) {
                    sky_vm_push(vm, SKY_BOOL((double)a.as.integer < b.as.floating));
                } else if (a.type == VAL_FLOAT && b.type == VAL_INT) {
                    sky_vm_push(vm, SKY_BOOL(a.as.floating < (double)b.as.integer));
                } else {
                    runtime_error(vm, "Cannot compare these types");
                    return VM_RUNTIME_ERROR;
                }
                break;
            }

            case OP_LESS_EQ: {
                SkyValue b = sky_vm_pop(vm);
                SkyValue a = sky_vm_pop(vm);
                if (a.type == VAL_INT && b.type == VAL_INT) {
                    sky_vm_push(vm, SKY_BOOL(a.as.integer <= b.as.integer));
                } else if (a.type == VAL_FLOAT && b.type == VAL_FLOAT) {
                    sky_vm_push(vm, SKY_BOOL(a.as.floating <= b.as.floating));
                } else {
                    runtime_error(vm, "Cannot compare these types");
                    return VM_RUNTIME_ERROR;
                }
                break;
            }

            case OP_AND: {
                SkyValue b = sky_vm_pop(vm);
                SkyValue a = sky_vm_pop(vm);
                bool ba = (a.type == VAL_BOOL) ? a.as.boolean : (a.type != VAL_NIL);
                bool bb = (b.type == VAL_BOOL) ? b.as.boolean : (b.type != VAL_NIL);
                sky_vm_push(vm, SKY_BOOL(ba && bb));
                break;
            }

            case OP_OR: {
                SkyValue b = sky_vm_pop(vm);
                SkyValue a = sky_vm_pop(vm);
                bool ba = (a.type == VAL_BOOL) ? a.as.boolean : (a.type != VAL_NIL);
                bool bb = (b.type == VAL_BOOL) ? b.as.boolean : (b.type != VAL_NIL);
                sky_vm_push(vm, SKY_BOOL(ba || bb));
                break;
            }

            case OP_JUMP: {
                uint16_t offset = read_short(frame);
                frame->ip += offset;
                break;
            }

            case OP_JUMP_IF_FALSE: {
                uint16_t offset = read_short(frame);
                SkyValue cond = sky_vm_peek(vm, 0);
                bool is_falsy = (cond.type == VAL_NIL) ||
                                (cond.type == VAL_BOOL && !cond.as.boolean) ||
                                (cond.type == VAL_INT && cond.as.integer == 0);
                if (is_falsy) {
                    frame->ip += offset;
                }
                break;
            }

            case OP_JUMP_BACK: {
                uint16_t offset = read_short(frame);
                frame->ip -= offset;
                break;
            }

            case OP_CALL: {
                uint8_t arg_count = read_byte(frame);
                SkyValue callee = sky_vm_peek(vm, arg_count);
                if (callee.type == VAL_NATIVE_FN && callee.as.native_fn) {
                    SkyNativeFn native = callee.as.native_fn;
                    SkyValue result = native(arg_count, vm->stack_top - arg_count);
                    vm->stack_top -= arg_count + 1;
                    sky_vm_push(vm, result);
                } else if (callee.type == VAL_NIL) {
                    /* Skip call to nil (unimplemented function) */
                    vm->stack_top -= arg_count + 1;
                    sky_vm_push(vm, SKY_NIL());
                } else {
                    runtime_error(vm, "Can only call functions");
                    return VM_RUNTIME_ERROR;
                }
                break;
            }

            case OP_RETURN: {
                if (vm->frame_count <= 1) {
                    return VM_OK;
                }
                vm->frame_count--;
                frame = &vm->frames[vm->frame_count - 1];
                break;
            }

            case OP_PRINT: {
                SkyValue val = sky_vm_pop(vm);
                sky_debug_print_value(val);
                printf("\n");
                break;
            }

            case OP_ARRAY: {
                uint8_t count = read_byte(frame);
                SkyValue arr;
                int i;
                arr.type = VAL_ARRAY;
                arr.as.array.count = count;
                arr.as.array.capacity = count > 0 ? count : 4;
                arr.as.array.items = (SkyValue*)malloc(sizeof(SkyValue) * arr.as.array.capacity);
                for (i = count - 1; i >= 0; i--) {
                    arr.as.array.items[i] = sky_vm_pop(vm);
                }
                sky_vm_push(vm, arr);
                break;
            }

            case OP_MAP:
            case OP_CLASS:
            case OP_METHOD:
            case OP_INVOKE:
            case OP_IMPORT:
            case OP_SERVER:
            case OP_ROUTE:
            case OP_RESPOND:
            case OP_SECURITY:
            case OP_ASYNC:
            case OP_AWAIT:
                break;

            case OP_GET_FIELD:
            case OP_SET_FIELD:
            case OP_GET_INDEX:
            case OP_SET_INDEX:
                break;

            case OP_HALT:
                return VM_OK;

            default:
                runtime_error(vm, "Unknown opcode: %d", instruction);
                return VM_RUNTIME_ERROR;
        }
    }
}

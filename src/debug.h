/* debug.h — Debug utilities header */
#ifndef SKY_DEBUG_H
#define SKY_DEBUG_H

#include "bytecode.h"
#include <stdbool.h>

void sky_disassemble_chunk(SkyChunk *chunk, const char *name);
int  sky_disassemble_instruction(SkyChunk *chunk, int offset);
void sky_debug_print_value(SkyValue value);
void sky_debug_print_stack(SkyValue *stack, int stack_top);

extern bool sky_debug_trace_execution;

#endif

/* module.h — Module system header */
#ifndef SKY_MODULE_H
#define SKY_MODULE_H

#include <stdbool.h>
#include <stdint.h>
#include "value.h"

#define SKY_MAX_MODULES     64
#define SKY_MAX_MOD_NAME    128
#define SKY_MAX_MOD_PATH    512
#define SKY_MAX_MOD_EXPORTS 128

typedef struct {
    char     name[SKY_MAX_MOD_NAME];
    SkyValue value;
    bool     is_function;
} SkyModuleExport;

typedef enum {
    SKY_MOD_BUILTIN,
    SKY_MOD_STDLIB,
    SKY_MOD_USER
} SkyModuleType;

typedef struct {
    char             name[SKY_MAX_MOD_NAME];
    char             path[SKY_MAX_MOD_PATH];
    SkyModuleType    type;
    SkyModuleExport  exports[SKY_MAX_MOD_EXPORTS];
    uint32_t         export_count;
    bool             loaded;
} SkyModule;

typedef struct {
    SkyModule  modules[SKY_MAX_MODULES];
    uint32_t   count;
    char       stdlib_path[SKY_MAX_MOD_PATH];
} SkyModuleRegistry;

void sky_module_registry_init(SkyModuleRegistry *reg, const char *stdlib_path);
bool sky_module_load(SkyModuleRegistry *reg, const char *name);
SkyModule* sky_module_get(SkyModuleRegistry *reg, const char *name);
SkyModuleExport* sky_module_get_export(SkyModule *mod, const char *name);
bool sky_module_register_builtin(SkyModuleRegistry *reg, const char *module_name, const char *func_name, SkyValue value);
void sky_module_register_builtins(SkyModuleRegistry *reg);

#endif

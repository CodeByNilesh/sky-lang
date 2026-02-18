/* src/module.c — Module system implementation */
#include "module.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Init ───────────────────────────────────────────── */

void sky_module_registry_init(SkyModuleRegistry *reg,
                              const char *stdlib_path) {
    if (!reg) return;
    memset(reg, 0, sizeof(SkyModuleRegistry));

    if (stdlib_path) {
        strncpy(reg->stdlib_path, stdlib_path,
                sizeof(reg->stdlib_path) - 1);
    } else {
        strcpy(reg->stdlib_path, "./stdlib");
    }
}

/* ── Find or create module ──────────────────────────── */

static SkyModule* find_or_create_module(SkyModuleRegistry *reg,
                                        const char *name) {
    /* Find existing */
    for (uint32_t i = 0; i < reg->count; i++) {
        if (strcmp(reg->modules[i].name, name) == 0) {
            return &reg->modules[i];
        }
    }

    /* Create new */
    if (reg->count >= SKY_MAX_MODULES) return NULL;

    SkyModule *mod = &reg->modules[reg->count++];
    memset(mod, 0, sizeof(SkyModule));
    strncpy(mod->name, name, sizeof(mod->name) - 1);

    return mod;
}

/* ── Load module ────────────────────────────────────── */

bool sky_module_load(SkyModuleRegistry *reg, const char *name) {
    if (!reg || !name) return false;

    SkyModule *mod = find_or_create_module(reg, name);
    if (!mod) return false;

    if (mod->loaded) return true; /* already loaded */

    /* Check if it's a built-in */
    if (mod->type == SKY_MOD_BUILTIN && mod->export_count > 0) {
        mod->loaded = true;
        return true;
    }

    /* Try stdlib path */
    char path[SKY_MAX_MOD_PATH];
    snprintf(path, sizeof(path), "%s/%s.sky", reg->stdlib_path, name);

    FILE *f = fopen(path, "r");
    if (f) {
        fclose(f);
        strncpy(mod->path, path, sizeof(mod->path) - 1);
        mod->type = SKY_MOD_STDLIB;
        mod->loaded = true;
        fprintf(stderr, "[SKY MODULE] Loaded stdlib: %s\n", path);
        return true;
    }

    /* Try user path (relative) */
    char user_path[SKY_MAX_MOD_PATH];
    if (name[0] == '.' && name[1] == '/') {
        snprintf(user_path, sizeof(user_path), "%s.sky", name);
    } else {
        snprintf(user_path, sizeof(user_path), "%s.sky", name);
    }

    f = fopen(user_path, "r");
    if (f) {
        fclose(f);
        strncpy(mod->path, user_path, sizeof(mod->path) - 1);
        mod->type = SKY_MOD_USER;
        mod->loaded = true;
        fprintf(stderr, "[SKY MODULE] Loaded user module: %s\n",
                user_path);
        return true;
    }

    fprintf(stderr, "[SKY MODULE] Cannot find module: %s\n", name);
    return false;
}

/* ── Get module ─────────────────────────────────────── */

SkyModule* sky_module_get(SkyModuleRegistry *reg, const char *name) {
    if (!reg || !name) return NULL;

    for (uint32_t i = 0; i < reg->count; i++) {
        if (strcmp(reg->modules[i].name, name) == 0) {
            return &reg->modules[i];
        }
    }
    return NULL;
}

/* ── Get export ─────────────────────────────────────── */

SkyModuleExport* sky_module_get_export(SkyModule *mod,
                                       const char *name) {
    if (!mod || !name) return NULL;

    for (uint32_t i = 0; i < mod->export_count; i++) {
        if (strcmp(mod->exports[i].name, name) == 0) {
            return &mod->exports[i];
        }
    }
    return NULL;
}

/* ── Register built-in export ───────────────────────── */

bool sky_module_register_builtin(SkyModuleRegistry *reg,
                                 const char *module_name,
                                 const char *func_name,
                                 SkyValue value) {
    if (!reg || !module_name || !func_name) return false;

    SkyModule *mod = find_or_create_module(reg, module_name);
    if (!mod) return false;

    mod->type = SKY_MOD_BUILTIN;
    mod->loaded = true;

    if (mod->export_count >= SKY_MAX_MOD_EXPORTS) return false;

    SkyModuleExport *exp = &mod->exports[mod->export_count++];
    strncpy(exp->name, func_name, sizeof(exp->name) - 1);
    exp->value = value;
    exp->is_function = (value.type == VAL_NATIVE_FN);

    return true;
}

/* ── Register all built-in modules ──────────────────── */

void sky_module_register_builtins(SkyModuleRegistry *reg) {
    if (!reg) return;

    /*
     * Register placeholder entries for built-in modules.
     * The actual native functions are bound when the VM
     * initializes its native function table.
     */

    /* db module */
    SkyModule *db = find_or_create_module(reg, "db");
    if (db) {
        db->type = SKY_MOD_BUILTIN;
        db->loaded = true;
    }

    /* jwt module */
    SkyModule *jwt = find_or_create_module(reg, "jwt");
    if (jwt) {
        jwt->type = SKY_MOD_BUILTIN;
        jwt->loaded = true;
    }

    /* http module */
    SkyModule *http = find_or_create_module(reg, "http");
    if (http) {
        http->type = SKY_MOD_BUILTIN;
        http->loaded = true;
    }

    /* crypto module */
    SkyModule *crypto = find_or_create_module(reg, "crypto");
    if (crypto) {
        crypto->type = SKY_MOD_BUILTIN;
        crypto->loaded = true;
    }

    fprintf(stderr, "[SKY MODULE] Registered %d built-in modules\n",
            reg->count);
}
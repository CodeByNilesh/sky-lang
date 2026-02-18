/* src/runtime/security.c — Security engine implementation */
#include "security.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

static uint32_t hash_ip(const char *ip) {
    uint32_t h = 2166136261u;
    while (*ip) {
        h ^= (uint8_t)*ip++;
        h *= 16777619u;
    }
    return h;
}

static void lowercase_copy(char *dst, const char *src, size_t max) {
    size_t i;
    for (i = 0; i < max - 1 && src[i]; i++) {
        dst[i] = (char)tolower((unsigned char)src[i]);
    }
    dst[i] = '\0';
}

void sky_security_init(SkySecurityEngine *engine) {
    if (!engine) return;
    memset(engine, 0, sizeof(SkySecurityEngine));
    engine->config.rate_limit_max = 100;
    engine->config.rate_limit_window = 60;
    engine->config.brute_force_threshold = 10;
    engine->config.brute_force_window = 300;
    engine->config.brute_force_ban_hours = 24;
    engine->config.sqli_ban_hours = 72;
    engine->config.shield_mode = false;
    engine->config.alert_admin_enabled = true;
    sky_mutex_init(&engine->lock);
    engine->initialized = true;
}

void sky_security_destroy(SkySecurityEngine *engine) {
    if (!engine || !engine->initialized) return;
    sky_mutex_destroy(&engine->lock);
    engine->initialized = false;
}

void sky_security_set_rate_limit(SkySecurityEngine *engine, uint32_t max_requests, uint32_t window_seconds) {
    if (!engine) return;
    sky_mutex_lock(&engine->lock);
    engine->config.rate_limit_max = max_requests;
    engine->config.rate_limit_window = window_seconds;
    sky_mutex_unlock(&engine->lock);
}

static bool check_rate_limit(SkySecurityEngine *engine, const char *ip) {
    time_t now = time(NULL);
    uint32_t idx = hash_ip(ip) % SKY_MAX_RATE_ENTRIES;
    uint32_t probe;
    for (probe = 0; probe < 16; probe++) {
        uint32_t i = (idx + probe) % SKY_MAX_RATE_ENTRIES;
        SkyRateEntry *entry = &engine->rate_table[i];
        if (!entry->active) {
            strncpy(entry->ip, ip, SKY_IP_LEN - 1);
            entry->ip[SKY_IP_LEN - 1] = '\0';
            entry->count = 1;
            entry->window_start = now;
            entry->active = true;
            engine->rate_count++;
            return true;
        }
        if (strcmp(entry->ip, ip) == 0) {
            if ((uint32_t)(now - entry->window_start) >= engine->config.rate_limit_window) {
                entry->count = 1;
                entry->window_start = now;
                return true;
            }
            entry->count++;
            if (entry->count > engine->config.rate_limit_max) {
                return false;
            }
            return true;
        }
    }
    return true;
}

bool sky_security_is_blacklisted(SkySecurityEngine *engine, const char *ip) {
    uint32_t i;
    time_t now;
    if (!engine || !ip) return false;
    now = time(NULL);
    sky_mutex_lock(&engine->lock);
    for (i = 0; i < engine->blacklist_count; i++) {
        SkyBlacklistEntry *entry = &engine->blacklist[i];
        if (!entry->active) continue;
        if (strcmp(entry->ip, ip) == 0) {
            if (!entry->permanent && now >= entry->expires) {
                entry->active = false;
                sky_mutex_unlock(&engine->lock);
                return false;
            }
            sky_mutex_unlock(&engine->lock);
            return true;
        }
    }
    sky_mutex_unlock(&engine->lock);
    return false;
}

void sky_security_blacklist(SkySecurityEngine *engine, const char *ip, uint32_t hours, SkyThreatType reason) {
    uint32_t i;
    if (!engine || !ip) return;
    sky_mutex_lock(&engine->lock);
    for (i = 0; i < engine->blacklist_count; i++) {
        if (engine->blacklist[i].active && strcmp(engine->blacklist[i].ip, ip) == 0) {
            engine->blacklist[i].expires = time(NULL) + (hours * 3600);
            engine->blacklist[i].reason = reason;
            if (hours == 0) engine->blacklist[i].permanent = true;
            sky_mutex_unlock(&engine->lock);
            return;
        }
    }
    if (engine->blacklist_count < SKY_MAX_BLACKLIST) {
        SkyBlacklistEntry *entry = &engine->blacklist[engine->blacklist_count++];
        strncpy(entry->ip, ip, SKY_IP_LEN - 1);
        entry->ip[SKY_IP_LEN - 1] = '\0';
        entry->expires = time(NULL) + (hours * 3600);
        entry->permanent = (hours == 0);
        entry->active = true;
        entry->reason = reason;
    }
    sky_mutex_unlock(&engine->lock);
    fprintf(stderr, "[SKY SECURITY] Blacklisted %s for %uh (%s)\n", ip, hours, sky_threat_type_string(reason));
}

void sky_security_whitelist(SkySecurityEngine *engine, const char *ip) {
    if (!engine || !ip) return;
    sky_mutex_lock(&engine->lock);
    if (engine->whitelist_count < 256) {
        strncpy(engine->whitelist[engine->whitelist_count], ip, SKY_IP_LEN - 1);
        engine->whitelist[engine->whitelist_count][SKY_IP_LEN - 1] = '\0';
        engine->whitelist_count++;
    }
    sky_mutex_unlock(&engine->lock);
}

static bool is_whitelisted(SkySecurityEngine *engine, const char *ip) {
    uint32_t i;
    for (i = 0; i < engine->whitelist_count; i++) {
        if (strcmp(engine->whitelist[i], ip) == 0) return true;
    }
    return false;
}

void sky_security_enable_shield(SkySecurityEngine *engine) {
    if (!engine) return;
    sky_mutex_lock(&engine->lock);
    engine->config.shield_mode = true;
    sky_mutex_unlock(&engine->lock);
    fprintf(stderr, "[SKY SECURITY] Shield mode ENABLED\n");
}

void sky_security_disable_shield(SkySecurityEngine *engine) {
    if (!engine) return;
    sky_mutex_lock(&engine->lock);
    engine->config.shield_mode = false;
    sky_mutex_unlock(&engine->lock);
    fprintf(stderr, "[SKY SECURITY] Shield mode DISABLED\n");
}

bool sky_security_detect_sqli(const char *input) {
    int i;
    const char *p;
    int single_quotes;
    char lower[2048];
    static const char *patterns[] = {
        "' or '1'='1", "' or 1=1", "'; drop table", "'; delete from",
        "union select", "union all select", "' or ''='", "1'; exec",
        "' and 1=1", "order by 1--", "' or 'a'='a", "admin'--",
        "' or 1=1--", "' or 1=1#", "' or 1=1/*", "'; waitfor delay",
        "benchmark(", "sleep(", "pg_sleep(", "load_file(",
        "into outfile", "into dumpfile", "information_schema",
        "char(", "concat(", NULL
    };
    if (!input) return false;
    lowercase_copy(lower, input, sizeof(lower));
    for (i = 0; patterns[i]; i++) {
        if (strstr(lower, patterns[i])) return true;
    }
    if (strstr(lower, "--") && strstr(lower, "'")) return true;
    if (strstr(lower, "/*") && strstr(lower, "*/")) return true;
    p = lower;
    single_quotes = 0;
    while (*p) {
        if (*p == '\'') single_quotes++;
        p++;
    }
    if (single_quotes >= 3 && strstr(lower, "=")) return true;
    return false;
}

bool sky_security_detect_xss(const char *input) {
    int i;
    char lower[2048];
    static const char *patterns[] = {
        "<script", "javascript:", "onerror=", "onload=", "onclick=",
        "onmouseover=", "onfocus=", "onblur=", "eval(", "document.cookie",
        "document.write", "window.location", "innerhtml", "<iframe",
        "<object", "<embed", "<svg", "expression(", "url(",
        "data:text/html", "vbscript:", NULL
    };
    if (!input) return false;
    lowercase_copy(lower, input, sizeof(lower));
    for (i = 0; patterns[i]; i++) {
        if (strstr(lower, patterns[i])) return true;
    }
    return false;
}

bool sky_security_detect_path_traversal(const char *input) {
    if (!input) return false;
    if (strstr(input, "../")) return true;
    if (strstr(input, "..\\")) return true;
    if (strstr(input, "%2e%2e")) return true;
    if (strstr(input, "%252e")) return true;
    if (strstr(input, "/etc/passwd")) return true;
    if (strstr(input, "/etc/shadow")) return true;
    return false;
}

bool sky_security_detect_command_injection(const char *input) {
    int i;
    char lower[2048];
    static const char *patterns[] = {
        "; ls", "| cat", "& cat", "; rm ", "; wget ", "; curl ",
        "| nc ", "; /bin/", "; /usr/", NULL
    };
    if (!input) return false;
    lowercase_copy(lower, input, sizeof(lower));
    for (i = 0; patterns[i]; i++) {
        if (strstr(lower, patterns[i])) return true;
    }
    return false;
}

void sky_security_log_threat(SkySecurityEngine *engine, const char *ip, SkyThreatType type, const char *detail) {
    uint32_t idx;
    SkyThreatLog *entry;
    if (!engine) return;
    sky_mutex_lock(&engine->lock);
    idx = engine->threat_log_head % SKY_MAX_THREAT_LOG;
    entry = &engine->threat_log[idx];
    strncpy(entry->ip, ip, SKY_IP_LEN - 1);
    entry->ip[SKY_IP_LEN - 1] = '\0';
    entry->type = type;
    if (detail) {
        strncpy(entry->detail, detail, SKY_RAW_REQUEST_LEN - 1);
        entry->detail[SKY_RAW_REQUEST_LEN - 1] = '\0';
    } else {
        entry->detail[0] = '\0';
    }
    entry->timestamp = time(NULL);
    entry->fingerprint[0] = '\0';
    engine->threat_log_head++;
    if (engine->threat_log_count < SKY_MAX_THREAT_LOG) {
        engine->threat_log_count++;
    }
    sky_mutex_unlock(&engine->lock);
    fprintf(stderr, "[SKY THREAT] %s from %s: %s\n", sky_threat_type_string(type), ip, detail ? detail : "(no detail)");
}

const SkyThreatLog* sky_security_get_threats(SkySecurityEngine *engine, uint32_t *count) {
    if (!engine) { *count = 0; return NULL; }
    *count = engine->threat_log_count;
    return engine->threat_log;
}

void sky_security_fingerprint(const char *ip, const char *user_agent, const char *headers, char *out_fingerprint, size_t out_len) {
    uint32_t h = 2166136261u;
    int s;
    const char *sources[4];
    const char *p;
    sources[0] = ip;
    sources[1] = user_agent;
    sources[2] = headers;
    sources[3] = NULL;
    for (s = 0; sources[s]; s++) {
        p = sources[s];
        while (*p) {
            h ^= (uint8_t)*p++;
            h *= 16777619u;
        }
        h ^= 0xff;
    }
    snprintf(out_fingerprint, out_len, "SKY-%08X", h);
}

bool sky_security_check_request(SkySecurityEngine *engine, const char *ip, const char *method, const char *path, const char *body, const char *raw_request) {
    bool rate_ok;
    (void)method;
    if (!engine || !engine->initialized || !ip) return true;
    sky_mutex_lock(&engine->lock);
    if (engine->config.shield_mode) {
        if (!is_whitelisted(engine, ip)) {
            sky_mutex_unlock(&engine->lock);
            sky_security_log_threat(engine, ip, THREAT_DDOS, "blocked by shield mode");
            return false;
        }
    }
    sky_mutex_unlock(&engine->lock);
    if (sky_security_is_blacklisted(engine, ip)) return false;
    sky_mutex_lock(&engine->lock);
    rate_ok = check_rate_limit(engine, ip);
    sky_mutex_unlock(&engine->lock);
    if (!rate_ok) {
        sky_security_log_threat(engine, ip, THREAT_BRUTE_FORCE, "rate limit exceeded");
        sky_security_blacklist(engine, ip, engine->config.brute_force_ban_hours, THREAT_BRUTE_FORCE);
        return false;
    }
    if (sky_security_detect_sqli(path) || (body && sky_security_detect_sqli(body))) {
        sky_security_log_threat(engine, ip, THREAT_SQL_INJECTION, raw_request ? raw_request : path);
        sky_security_blacklist(engine, ip, engine->config.sqli_ban_hours, THREAT_SQL_INJECTION);
        return false;
    }
    if (sky_security_detect_xss(path) || (body && sky_security_detect_xss(body))) {
        sky_security_log_threat(engine, ip, THREAT_XSS, raw_request ? raw_request : path);
        sky_security_blacklist(engine, ip, 48, THREAT_XSS);
        return false;
    }
    if (sky_security_detect_path_traversal(path)) {
        sky_security_log_threat(engine, ip, THREAT_PATH_TRAVERSAL, path);
        sky_security_blacklist(engine, ip, 24, THREAT_PATH_TRAVERSAL);
        return false;
    }
    if (body && sky_security_detect_command_injection(body)) {
        sky_security_log_threat(engine, ip, THREAT_COMMAND_INJECTION, raw_request ? raw_request : body);
        sky_security_blacklist(engine, ip, 72, THREAT_COMMAND_INJECTION);
        return false;
    }
    return true;
}

const char* sky_threat_type_string(SkyThreatType type) {
    switch (type) {
        case THREAT_NONE: return "NONE";
        case THREAT_BRUTE_FORCE: return "BRUTE_FORCE";
        case THREAT_SQL_INJECTION: return "SQL_INJECTION";
        case THREAT_XSS: return "XSS";
        case THREAT_DDOS: return "DDOS";
        case THREAT_PATH_TRAVERSAL: return "PATH_TRAVERSAL";
        case THREAT_COMMAND_INJECTION: return "COMMAND_INJECTION";
        case THREAT_UNKNOWN: return "UNKNOWN";
        default: return "UNKNOWN";
    }
}

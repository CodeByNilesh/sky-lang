/* src/runtime/security.h — Security engine header */
#ifndef SKY_SECURITY_H
#define SKY_SECURITY_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "../platform.h"

#define SKY_MAX_BLACKLIST      4096
#define SKY_MAX_RATE_ENTRIES   8192
#define SKY_MAX_THREAT_LOG     2048
#define SKY_IP_LEN             46
#define SKY_FINGERPRINT_LEN    128
#define SKY_RAW_REQUEST_LEN    1024

typedef enum {
    THREAT_NONE = 0,
    THREAT_BRUTE_FORCE,
    THREAT_SQL_INJECTION,
    THREAT_XSS,
    THREAT_DDOS,
    THREAT_PATH_TRAVERSAL,
    THREAT_COMMAND_INJECTION,
    THREAT_UNKNOWN
} SkyThreatType;

typedef struct {
    char     ip[SKY_IP_LEN];
    uint32_t count;
    time_t   window_start;
    bool     active;
} SkyRateEntry;

typedef struct {
    char          ip[SKY_IP_LEN];
    time_t        expires;
    bool          permanent;
    bool          active;
    SkyThreatType reason;
} SkyBlacklistEntry;

typedef struct {
    char          ip[SKY_IP_LEN];
    SkyThreatType type;
    char          detail[SKY_RAW_REQUEST_LEN];
    time_t        timestamp;
    char          fingerprint[SKY_FINGERPRINT_LEN];
} SkyThreatLog;

typedef struct {
    uint32_t rate_limit_max;
    uint32_t rate_limit_window;
    uint32_t brute_force_threshold;
    uint32_t brute_force_window;
    uint32_t brute_force_ban_hours;
    uint32_t sqli_ban_hours;
    bool     shield_mode;
    bool     alert_admin_enabled;
} SkySecurityConfig;

typedef struct {
    SkySecurityConfig config;
    SkyRateEntry      rate_table[SKY_MAX_RATE_ENTRIES];
    uint32_t          rate_count;
    SkyBlacklistEntry blacklist[SKY_MAX_BLACKLIST];
    uint32_t          blacklist_count;
    SkyThreatLog      threat_log[SKY_MAX_THREAT_LOG];
    uint32_t          threat_log_count;
    uint32_t          threat_log_head;
    char              whitelist[256][SKY_IP_LEN];
    uint32_t          whitelist_count;
    sky_mutex_t       lock;
    bool              initialized;
} SkySecurityEngine;

void sky_security_init(SkySecurityEngine *engine);
void sky_security_destroy(SkySecurityEngine *engine);
void sky_security_set_rate_limit(SkySecurityEngine *engine, uint32_t max_requests, uint32_t window_seconds);
bool sky_security_check_request(SkySecurityEngine *engine, const char *ip, const char *method, const char *path, const char *body, const char *raw_request);
bool sky_security_is_blacklisted(SkySecurityEngine *engine, const char *ip);
void sky_security_blacklist(SkySecurityEngine *engine, const char *ip, uint32_t hours, SkyThreatType reason);
void sky_security_whitelist(SkySecurityEngine *engine, const char *ip);
void sky_security_enable_shield(SkySecurityEngine *engine);
void sky_security_disable_shield(SkySecurityEngine *engine);
bool sky_security_detect_sqli(const char *input);
bool sky_security_detect_xss(const char *input);
bool sky_security_detect_path_traversal(const char *input);
bool sky_security_detect_command_injection(const char *input);
void sky_security_log_threat(SkySecurityEngine *engine, const char *ip, SkyThreatType type, const char *detail);
const SkyThreatLog* sky_security_get_threats(SkySecurityEngine *engine, uint32_t *count);
void sky_security_fingerprint(const char *ip, const char *user_agent, const char *headers, char *out_fingerprint, size_t out_len);
const char* sky_threat_type_string(SkyThreatType type);

#endif

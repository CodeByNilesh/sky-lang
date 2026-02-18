/* src/runtime/jwt.h — JWT token handling header */
#ifndef SKY_JWT_H
#define SKY_JWT_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#define SKY_JWT_MAX_TOKEN     2048
#define SKY_JWT_MAX_PAYLOAD   1024
#define SKY_JWT_SECRET_LEN    256
#define SKY_JWT_MAX_CLAIMS    32

/* ── JWT claim ──────────────────────────────────────── */
typedef struct {
    char key[64];
    char value[256];
} SkyJWTClaim;

/* ── JWT token structure ────────────────────────────── */
typedef struct {
    SkyJWTClaim claims[SKY_JWT_MAX_CLAIMS];
    int         claim_count;
    time_t      issued_at;
    time_t      expires_at;
    bool        valid;
    char        subject[256];   /* user ID */
} SkyJWTPayload;

/* ── JWT context ────────────────────────────────────── */
typedef struct {
    char     secret[SKY_JWT_SECRET_LEN];
    uint32_t default_ttl_seconds; /* default token lifetime */
} SkyJWTContext;

/* ── Public API ─────────────────────────────────────── */
void sky_jwt_init(SkyJWTContext *ctx,
                  const char *secret,
                  uint32_t ttl_seconds);

/* Sign: create a JWT token string from a subject (user ID) */
bool sky_jwt_sign(SkyJWTContext *ctx,
                  const char *subject,
                  char *out_token,
                  size_t out_len);

/* Sign with extra claims */
bool sky_jwt_sign_claims(SkyJWTContext *ctx,
                         const char *subject,
                         SkyJWTClaim *claims,
                         int claim_count,
                         char *out_token,
                         size_t out_len);

/* Verify: parse and validate a JWT token string */
bool sky_jwt_verify(SkyJWTContext *ctx,
                    const char *token,
                    SkyJWTPayload *out_payload);

/* Extract subject without full verification (for logging) */
bool sky_jwt_get_subject(const char *token,
                         char *out_subject,
                         size_t out_len);

#endif /* SKY_JWT_H */
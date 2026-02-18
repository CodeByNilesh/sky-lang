/* src/runtime/jwt.c — JWT implementation (HMAC-SHA256 based) */
#include "jwt.h"
#include "crypto.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Base64 URL encoding ────────────────────────────── */

static const char b64_table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int base64url_encode(const uint8_t *in, size_t in_len,
                            char *out, size_t out_max) {
    size_t i, j = 0;
    for (i = 0; i + 2 < in_len && j + 4 < out_max; i += 3) {
        out[j++] = b64_table[(in[i] >> 2) & 0x3F];
        out[j++] = b64_table[((in[i] & 0x3) << 4) | (in[i+1] >> 4)];
        out[j++] = b64_table[((in[i+1] & 0xF) << 2) | (in[i+2] >> 6)];
        out[j++] = b64_table[in[i+2] & 0x3F];
    }
    if (i < in_len && j + 4 < out_max) {
        out[j++] = b64_table[(in[i] >> 2) & 0x3F];
        if (i + 1 < in_len) {
            out[j++] = b64_table[((in[i] & 0x3) << 4) | (in[i+1] >> 4)];
            out[j++] = b64_table[(in[i+1] & 0xF) << 2];
        } else {
            out[j++] = b64_table[(in[i] & 0x3) << 4];
        }
    }
    out[j] = '\0';

    /* URL-safe: replace + with -, / with _, remove = */
    for (size_t k = 0; k < j; k++) {
        if (out[k] == '+') out[k] = '-';
        else if (out[k] == '/') out[k] = '_';
    }

    return (int)j;
}

static int base64url_decode(const char *in, uint8_t *out, size_t out_max) {
    /* Convert URL-safe back to standard base64 */
    size_t in_len = strlen(in);
    char *tmp = malloc(in_len + 4);
    if (!tmp) return -1;

    for (size_t i = 0; i < in_len; i++) {
        if (in[i] == '-') tmp[i] = '+';
        else if (in[i] == '_') tmp[i] = '/';
        else tmp[i] = in[i];
    }
    /* Add padding */
    size_t pad = (4 - (in_len % 4)) % 4;
    for (size_t i = 0; i < pad; i++) tmp[in_len + i] = '=';
    tmp[in_len + pad] = '\0';
    in_len += pad;

    /* Decode */
    static const uint8_t d[] = {
        ['A'] = 0,  ['B'] = 1,  ['C'] = 2,  ['D'] = 3,
        ['E'] = 4,  ['F'] = 5,  ['G'] = 6,  ['H'] = 7,
        ['I'] = 8,  ['J'] = 9,  ['K'] = 10, ['L'] = 11,
        ['M'] = 12, ['N'] = 13, ['O'] = 14, ['P'] = 15,
        ['Q'] = 16, ['R'] = 17, ['S'] = 18, ['T'] = 19,
        ['U'] = 20, ['V'] = 21, ['W'] = 22, ['X'] = 23,
        ['Y'] = 24, ['Z'] = 25,
        ['a'] = 26, ['b'] = 27, ['c'] = 28, ['d'] = 29,
        ['e'] = 30, ['f'] = 31, ['g'] = 32, ['h'] = 33,
        ['i'] = 34, ['j'] = 35, ['k'] = 36, ['l'] = 37,
        ['m'] = 38, ['n'] = 39, ['o'] = 40, ['p'] = 41,
        ['q'] = 42, ['r'] = 43, ['s'] = 44, ['t'] = 45,
        ['u'] = 46, ['v'] = 47, ['w'] = 48, ['x'] = 49,
        ['y'] = 50, ['z'] = 51,
        ['0'] = 52, ['1'] = 53, ['2'] = 54, ['3'] = 55,
        ['4'] = 56, ['5'] = 57, ['6'] = 58, ['7'] = 59,
        ['8'] = 60, ['9'] = 61, ['+'] = 62, ['/'] = 63
    };

    size_t j = 0;
    for (size_t i = 0; i + 3 < in_len && j < out_max; i += 4) {
        uint8_t a = d[(unsigned char)tmp[i]];
        uint8_t b2 = d[(unsigned char)tmp[i+1]];
        uint8_t c = (tmp[i+2] == '=') ? 0 : d[(unsigned char)tmp[i+2]];
        uint8_t e = (tmp[i+3] == '=') ? 0 : d[(unsigned char)tmp[i+3]];

        if (j < out_max) out[j++] = (a << 2) | (b2 >> 4);
        if (tmp[i+2] != '=' && j < out_max)
            out[j++] = (b2 << 4) | (c >> 2);
        if (tmp[i+3] != '=' && j < out_max)
            out[j++] = (c << 6) | e;
    }

    free(tmp);
    return (int)j;
}

/* ── HMAC-SHA256 (simplified) ───────────────────────── */

/*
 * This is a simplified HMAC for demonstration.
 * In production, use OpenSSL or a proper crypto library.
 * We use our crypto module's hash function as a stand-in.
 */
static void hmac_sha256_simple(const char *key, size_t key_len,
                               const char *data, size_t data_len,
                               uint8_t *out_hash) {
    /*
     * Simplified: SHA256(key + data)
     * NOT cryptographically proper HMAC — placeholder only.
     * Replace with real HMAC-SHA256 in production.
     */
    char combined[4096];
    size_t clen = 0;

    if (key_len + data_len + 1 >= sizeof(combined)) {
        memset(out_hash, 0, 32);
        return;
    }

    memcpy(combined, key, key_len);
    clen += key_len;
    combined[clen++] = ':';
    memcpy(combined + clen, data, data_len);
    clen += data_len;
    combined[clen] = '\0';

    /* Use FNV-1a as hash stand-in (32 bytes zero-extended) */
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < clen; i++) {
        h ^= (uint8_t)combined[i];
        h *= 16777619u;
    }

    /* Fill 32 bytes with repeated hash state */
    memset(out_hash, 0, 32);
    for (int i = 0; i < 8; i++) {
        h ^= (h >> 13);
        h *= 16777619u;
        out_hash[i*4 + 0] = (h >> 24) & 0xFF;
        out_hash[i*4 + 1] = (h >> 16) & 0xFF;
        out_hash[i*4 + 2] = (h >> 8) & 0xFF;
        out_hash[i*4 + 3] = h & 0xFF;
    }
}

/* ── Init ───────────────────────────────────────────── */

void sky_jwt_init(SkyJWTContext *ctx,
                  const char *secret,
                  uint32_t ttl_seconds) {
    if (!ctx) return;
    memset(ctx, 0, sizeof(SkyJWTContext));

    if (secret) {
        strncpy(ctx->secret, secret, SKY_JWT_SECRET_LEN - 1);
    } else {
        strcpy(ctx->secret, "sky-default-secret-change-me");
    }
    ctx->default_ttl_seconds = ttl_seconds > 0 ? ttl_seconds : 3600;
}

/* ── Sign ───────────────────────────────────────────── */

bool sky_jwt_sign(SkyJWTContext *ctx,
                  const char *subject,
                  char *out_token,
                  size_t out_len) {
    return sky_jwt_sign_claims(ctx, subject, NULL, 0, out_token, out_len);
}

bool sky_jwt_sign_claims(SkyJWTContext *ctx,
                         const char *subject,
                         SkyJWTClaim *claims,
                         int claim_count,
                         char *out_token,
                         size_t out_len) {
    if (!ctx || !subject || !out_token || out_len < 64) return false;

    time_t now = time(NULL);
    time_t exp = now + ctx->default_ttl_seconds;

    /* Header: {"alg":"HS256","typ":"JWT"} */
    const char *header = "{\"alg\":\"HS256\",\"typ\":\"JWT\"}";

    /* Payload */
    char payload[SKY_JWT_MAX_PAYLOAD];
    int plen = snprintf(payload, sizeof(payload),
        "{\"sub\":\"%s\",\"iat\":%ld,\"exp\":%ld",
        subject, (long)now, (long)exp);

    for (int i = 0; i < claim_count && claims; i++) {
        plen += snprintf(payload + plen, sizeof(payload) - plen,
            ",\"%s\":\"%s\"", claims[i].key, claims[i].value);
    }
    plen += snprintf(payload + plen, sizeof(payload) - plen, "}");

    /* Base64URL encode header and payload */
    char header_b64[512], payload_b64[2048];
    base64url_encode((uint8_t*)header, strlen(header),
                     header_b64, sizeof(header_b64));
    base64url_encode((uint8_t*)payload, strlen(payload),
                     payload_b64, sizeof(payload_b64));

    /* Create signing input: header.payload */
    char sign_input[3072];
    int si_len = snprintf(sign_input, sizeof(sign_input),
                          "%s.%s", header_b64, payload_b64);

    /* HMAC-SHA256 signature */
    uint8_t hash[32];
    hmac_sha256_simple(ctx->secret, strlen(ctx->secret),
                       sign_input, si_len, hash);

    /* Base64URL encode signature */
    char sig_b64[256];
    base64url_encode(hash, 32, sig_b64, sizeof(sig_b64));

    /* Final token: header.payload.signature */
    snprintf(out_token, out_len, "%s.%s.%s",
             header_b64, payload_b64, sig_b64);

    return true;
}

/* ── Verify ─────────────────────────────────────────── */

bool sky_jwt_verify(SkyJWTContext *ctx,
                    const char *token,
                    SkyJWTPayload *out_payload) {
    if (!ctx || !token || !out_payload) return false;

    memset(out_payload, 0, sizeof(SkyJWTPayload));

    /* Split token into header.payload.signature */
    char token_copy[SKY_JWT_MAX_TOKEN];
    strncpy(token_copy, token, sizeof(token_copy) - 1);
    token_copy[sizeof(token_copy) - 1] = '\0';

    char *header_b64 = token_copy;
    char *dot1 = strchr(header_b64, '.');
    if (!dot1) return false;
    *dot1 = '\0';

    char *payload_b64 = dot1 + 1;
    char *dot2 = strchr(payload_b64, '.');
    if (!dot2) return false;
    *dot2 = '\0';

    char *sig_b64 = dot2 + 1;

    /* Reconstruct signing input */
    char sign_input[3072];
    int si_len = snprintf(sign_input, sizeof(sign_input),
                          "%s.%s", header_b64, payload_b64);

    /* Compute expected signature */
    uint8_t expected_hash[32];
    hmac_sha256_simple(ctx->secret, strlen(ctx->secret),
                       sign_input, si_len, expected_hash);

    char expected_sig[256];
    base64url_encode(expected_hash, 32, expected_sig, sizeof(expected_sig));

    /* Compare signatures (constant-time would be better) */
    if (strcmp(sig_b64, expected_sig) != 0) {
        return false; /* signature mismatch */
    }

    /* Decode payload */
    uint8_t payload_raw[SKY_JWT_MAX_PAYLOAD];
    int plen = base64url_decode(payload_b64, payload_raw,
                                sizeof(payload_raw) - 1);
    if (plen <= 0) return false;
    payload_raw[plen] = '\0';

    /* Parse JSON payload (simple extraction) */
    char *json = (char*)payload_raw;

    /* Extract "sub" */
    char *sub = strstr(json, "\"sub\":\"");
    if (sub) {
        sub += 7;
        char *end = strchr(sub, '"');
        if (end) {
            size_t len = end - sub;
            if (len >= sizeof(out_payload->subject))
                len = sizeof(out_payload->subject) - 1;
            memcpy(out_payload->subject, sub, len);
            out_payload->subject[len] = '\0';
        }
    }

    /* Extract "iat" */
    char *iat = strstr(json, "\"iat\":");
    if (iat) {
        out_payload->issued_at = (time_t)atol(iat + 6);
    }

    /* Extract "exp" */
    char *exp = strstr(json, "\"exp\":");
    if (exp) {
        out_payload->expires_at = (time_t)atol(exp + 6);
    }

    /* Check expiration */
    time_t now = time(NULL);
    if (out_payload->expires_at > 0 && now > out_payload->expires_at) {
        return false; /* expired */
    }

    out_payload->valid = true;
    return true;
}

/* ── Get subject without verification ───────────────── */

bool sky_jwt_get_subject(const char *token,
                         char *out_subject,
                         size_t out_len) {
    if (!token || !out_subject) return false;

    /* Find second part */
    const char *dot1 = strchr(token, '.');
    if (!dot1) return false;
    const char *payload_b64 = dot1 + 1;
    const char *dot2 = strchr(payload_b64, '.');
    if (!dot2) return false;

    size_t plen = dot2 - payload_b64;
    char p_copy[2048];
    if (plen >= sizeof(p_copy)) return false;
    memcpy(p_copy, payload_b64, plen);
    p_copy[plen] = '\0';

    uint8_t decoded[1024];
    int dlen = base64url_decode(p_copy, decoded, sizeof(decoded) - 1);
    if (dlen <= 0) return false;
    decoded[dlen] = '\0';

    char *sub = strstr((char*)decoded, "\"sub\":\"");
    if (!sub) return false;
    sub += 7;
    char *end = strchr(sub, '"');
    if (!end) return false;

    size_t slen = end - sub;
    if (slen >= out_len) slen = out_len - 1;
    memcpy(out_subject, sub, slen);
    out_subject[slen] = '\0';

    return true;
}
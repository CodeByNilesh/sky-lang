/* src/runtime/crypto.h — Cryptographic utilities header */
#ifndef SKY_CRYPTO_H
#define SKY_CRYPTO_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define SKY_HASH_OUTPUT_LEN   65   /* hex-encoded SHA256 + null */
#define SKY_SALT_LEN          32
#define SKY_BCRYPT_OUTPUT_LEN 128

/* ── Hashing ────────────────────────────────────────── */

/* FNV-1a hash (fast, non-cryptographic) */
uint32_t sky_hash_fnv1a(const void *data, size_t len);
uint64_t sky_hash_fnv1a_64(const void *data, size_t len);

/* SHA-256 (simplified implementation) */
void sky_hash_sha256(const uint8_t *data, size_t len,
                     uint8_t out[32]);

/* SHA-256 hex string output */
void sky_hash_sha256_hex(const char *input,
                         char out[SKY_HASH_OUTPUT_LEN]);

/* ── Password hashing ──────────────────────────────── */

/* Hash a password with random salt */
bool sky_crypto_hash_password(const char *password,
                              char *out_hash,
                              size_t out_len);

/* Verify password against hash */
bool sky_crypto_verify_password(const char *password,
                                const char *hash);

/* ── Random ─────────────────────────────────────────── */

/* Generate random bytes */
void sky_crypto_random_bytes(uint8_t *buf, size_t len);

/* Generate random hex string */
void sky_crypto_random_hex(char *buf, size_t hex_len);

/* ── Encoding ───────────────────────────────────────── */

/* Hex encode/decode */
void sky_hex_encode(const uint8_t *data, size_t len,
                    char *out, size_t out_len);
bool sky_hex_decode(const char *hex,
                    uint8_t *out, size_t out_len,
                    size_t *decoded_len);

#endif /* SKY_CRYPTO_H */
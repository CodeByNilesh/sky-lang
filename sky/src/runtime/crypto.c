/* src/runtime/crypto.c — Cryptographic utilities implementation */
#include "crypto.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#include <wincrypt.h>
#endif

uint32_t sky_hash_fnv1a(const void *data, size_t len) {
    const uint8_t *p = (const uint8_t*)data;
    uint32_t h = 2166136261u;
    size_t i;
    for (i = 0; i < len; i++) {
        h ^= p[i];
        h *= 16777619u;
    }
    return h;
}

uint64_t sky_hash_fnv1a_64(const void *data, size_t len) {
    const uint8_t *p = (const uint8_t*)data;
    uint64_t h = 14695981039346656037ULL;
    size_t i;
    for (i = 0; i < len; i++) {
        h ^= p[i];
        h *= 1099511628211ULL;
    }
    return h;
}

static const uint32_t sha256_k[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

#define ROTR(x,n) (((x)>>(n))|((x)<<(32-(n))))
#define CH(x,y,z) (((x)&(y))^(~(x)&(z)))
#define MAJ(x,y,z) (((x)&(y))^((x)&(z))^((y)&(z)))
#define SIG0(x) (ROTR(x,2)^ROTR(x,13)^ROTR(x,22))
#define SIG1(x) (ROTR(x,6)^ROTR(x,11)^ROTR(x,25))
#define sig0(x) (ROTR(x,7)^ROTR(x,18)^((x)>>3))
#define sig1(x) (ROTR(x,17)^ROTR(x,19)^((x)>>10))

void sky_hash_sha256(const uint8_t *data, size_t len, uint8_t out[32]) {
    uint32_t h0=0x6a09e667,h1=0xbb67ae85,h2=0x3c6ef372,h3=0xa54ff53a;
    uint32_t h4=0x510e527f,h5=0x9b05688c,h6=0x1f83d9ab,h7=0x5be0cd19;
    size_t new_len, chunk;
    uint8_t *msg;
    uint64_t bits_len;
    int i;
    new_len = len + 1;
    while (new_len % 64 != 56) new_len++;
    new_len += 8;
    msg = (uint8_t*)calloc(new_len, 1);
    if (!msg) { memset(out, 0, 32); return; }
    memcpy(msg, data, len);
    msg[len] = 0x80;
    bits_len = (uint64_t)len * 8;
    for (i = 0; i < 8; i++) msg[new_len - 1 - i] = (uint8_t)(bits_len >> (i * 8));
    for (chunk = 0; chunk < new_len; chunk += 64) {
        uint32_t w[64], a, b, c, d, e, f, g, hh;
        for (i = 0; i < 16; i++) w[i] = ((uint32_t)msg[chunk+i*4]<<24)|((uint32_t)msg[chunk+i*4+1]<<16)|((uint32_t)msg[chunk+i*4+2]<<8)|((uint32_t)msg[chunk+i*4+3]);
        for (i = 16; i < 64; i++) w[i] = sig1(w[i-2]) + w[i-7] + sig0(w[i-15]) + w[i-16];
        a=h0; b=h1; c=h2; d=h3; e=h4; f=h5; g=h6; hh=h7;
        for (i = 0; i < 64; i++) {
            uint32_t t1 = hh + SIG1(e) + CH(e,f,g) + sha256_k[i] + w[i];
            uint32_t t2 = SIG0(a) + MAJ(a,b,c);
            hh=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
        }
        h0+=a; h1+=b; h2+=c; h3+=d; h4+=e; h5+=f; h6+=g; h7+=hh;
    }
    free(msg);
    {
        uint32_t hs[] = {h0,h1,h2,h3,h4,h5,h6,h7};
        for (i = 0; i < 8; i++) {
            out[i*4+0]=(hs[i]>>24)&0xFF; out[i*4+1]=(hs[i]>>16)&0xFF;
            out[i*4+2]=(hs[i]>>8)&0xFF; out[i*4+3]=hs[i]&0xFF;
        }
    }
}

void sky_hash_sha256_hex(const char *input, char out[SKY_HASH_OUTPUT_LEN]) {
    uint8_t hash[32];
    sky_hash_sha256((const uint8_t*)input, strlen(input), hash);
    sky_hex_encode(hash, 32, out, SKY_HASH_OUTPUT_LEN);
}

void sky_crypto_random_bytes(uint8_t *buf, size_t len) {
#ifdef _WIN32
    HCRYPTPROV hProv;
    if (CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        CryptGenRandom(hProv, (DWORD)len, buf);
        CryptReleaseContext(hProv, 0);
        return;
    }
#else
    FILE *f = fopen("/dev/urandom", "rb");
    if (f) {
        size_t r = fread(buf, 1, len, f);
        fclose(f);
        if (r == len) return;
    }
#endif
    {
        size_t i;
        static int seeded = 0;
        if (!seeded) { srand((unsigned)(time(NULL) ^ (size_t)buf)); seeded = 1; }
        for (i = 0; i < len; i++) buf[i] = (uint8_t)(rand() & 0xFF);
    }
}

void sky_crypto_random_hex(char *buf, size_t hex_len) {
    size_t byte_len = (hex_len + 1) / 2;
    uint8_t *bytes = (uint8_t*)malloc(byte_len);
    if (!bytes) { buf[0] = '\0'; return; }
    sky_crypto_random_bytes(bytes, byte_len);
    sky_hex_encode(bytes, byte_len, buf, hex_len + 1);
    buf[hex_len] = '\0';
    free(bytes);
}

bool sky_crypto_hash_password(const char *password, char *out_hash, size_t out_len) {
    char salt_hex[SKY_SALT_LEN * 2 + 1];
    char salted[2048];
    uint8_t hash[32];
    char hash_hex[65];
    int i;
    if (!password || !out_hash || out_len < 100) return false;
    sky_crypto_random_hex(salt_hex, SKY_SALT_LEN * 2);
    snprintf(salted, sizeof(salted), "%s:%s", salt_hex, password);
    sky_hash_sha256((const uint8_t*)salted, strlen(salted), hash);
    for (i = 0; i < 9999; i++) {
        uint8_t tmp[32 + 2048];
        memcpy(tmp, hash, 32);
        memcpy(tmp + 32, salted, strlen(salted));
        sky_hash_sha256(tmp, 32 + strlen(salted), hash);
    }
    sky_hex_encode(hash, 32, hash_hex, sizeof(hash_hex));
    snprintf(out_hash, out_len, "%s$%s", salt_hex, hash_hex);
    return true;
}

bool sky_crypto_verify_password(const char *password, const char *hash) {
    const char *dollar;
    size_t salt_len, elen, clen;
    char salt[SKY_SALT_LEN * 2 + 1];
    const char *expected_hash;
    char salted[2048];
    uint8_t computed[32];
    char computed_hex[65];
    volatile uint8_t diff = 0;
    size_t i;
    if (!password || !hash) return false;
    dollar = strchr(hash, '$');
    if (!dollar) return false;
    salt_len = dollar - hash;
    if (salt_len >= sizeof(salt)) return false;
    memcpy(salt, hash, salt_len);
    salt[salt_len] = '\0';
    expected_hash = dollar + 1;
    snprintf(salted, sizeof(salted), "%s:%s", salt, password);
    sky_hash_sha256((const uint8_t*)salted, strlen(salted), computed);
    for (i = 0; i < 9999; i++) {
        uint8_t tmp[32 + 2048];
        memcpy(tmp, computed, 32);
        memcpy(tmp + 32, salted, strlen(salted));
        sky_hash_sha256(tmp, 32 + strlen(salted), computed);
    }
    sky_hex_encode(computed, 32, computed_hex, sizeof(computed_hex));
    elen = strlen(expected_hash);
    clen = strlen(computed_hex);
    if (elen != clen) return false;
    for (i = 0; i < elen; i++) diff |= (uint8_t)(expected_hash[i] ^ computed_hex[i]);
    return diff == 0;
}

void sky_hex_encode(const uint8_t *data, size_t len, char *out, size_t out_len) {
    static const char hex[] = "0123456789abcdef";
    size_t i, j = 0;
    for (i = 0; i < len && j + 2 < out_len; i++) {
        out[j++] = hex[(data[i] >> 4) & 0xF];
        out[j++] = hex[data[i] & 0xF];
    }
    out[j] = '\0';
}

bool sky_hex_decode(const char *hex, uint8_t *out, size_t out_len, size_t *decoded_len) {
    size_t hlen = strlen(hex);
    size_t blen = hlen / 2;
    size_t i;
    if (hlen % 2 != 0) return false;
    if (blen > out_len) return false;
    for (i = 0; i < blen; i++) {
        char hi = hex[i*2], lo = hex[i*2+1];
        uint8_t hv, lv;
        if (hi >= '0' && hi <= '9') hv = hi - '0';
        else if (hi >= 'a' && hi <= 'f') hv = hi - 'a' + 10;
        else if (hi >= 'A' && hi <= 'F') hv = hi - 'A' + 10;
        else return false;
        if (lo >= '0' && lo <= '9') lv = lo - '0';
        else if (lo >= 'a' && lo <= 'f') lv = lo - 'a' + 10;
        else if (lo >= 'A' && lo <= 'F') lv = lo - 'A' + 10;
        else return false;
        out[i] = (hv << 4) | lv;
    }
    if (decoded_len) *decoded_len = blen;
    return true;
}

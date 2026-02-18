/* src/runtime/db.h — Database connector header */
#ifndef SKY_DB_H
#define SKY_DB_H

#include <stdint.h>
#include <stdbool.h>

/* ── Database types ─────────────────────────────────── */
typedef enum {
    SKY_DB_SQLITE,
    SKY_DB_MYSQL,
    SKY_DB_POSTGRESQL
} SkyDBType;

/* ── Query result row ───────────────────────────────── */
#define SKY_DB_MAX_COLUMNS 64
#define SKY_DB_MAX_ROWS    4096
#define SKY_DB_VALUE_LEN   1024

typedef struct {
    char  columns[SKY_DB_MAX_COLUMNS][128];
    char  values[SKY_DB_MAX_ROWS][SKY_DB_MAX_COLUMNS][SKY_DB_VALUE_LEN];
    int   column_count;
    int   row_count;
} SkyDBResult;

/* ── Connection ─────────────────────────────────────── */
typedef struct {
    SkyDBType type;
    char      host[256];
    char      database[256];
    char      user[128];
    char      password[128];
    uint16_t  port;
    void     *handle;       /* Native driver handle */
    bool      connected;
    char      error[512];
} SkyDBConnection;

/* ── Public API ─────────────────────────────────────── */
SkyDBConnection* sky_db_connect(SkyDBType type,
                                const char *host,
                                const char *database,
                                const char *user,
                                const char *password,
                                uint16_t port);

void sky_db_close(SkyDBConnection *conn);

SkyDBResult* sky_db_query(SkyDBConnection *conn, const char *sql);

bool sky_db_execute(SkyDBConnection *conn, const char *sql);

SkyDBResult* sky_db_find(SkyDBConnection *conn,
                         const char *table,
                         const char *field,
                         const char *value);

void sky_db_result_free(SkyDBResult *result);

const char* sky_db_error(SkyDBConnection *conn);

/* Prepared statements (SQL injection safe) */
SkyDBResult* sky_db_query_param(SkyDBConnection *conn,
                                const char *sql,
                                const char **params,
                                int param_count);

bool sky_db_execute_param(SkyDBConnection *conn,
                          const char *sql,
                          const char **params,
                          int param_count);

#endif /* SKY_DB_H */
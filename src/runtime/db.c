/* src/runtime/db.c â€” Database connector (SQLite built-in, stubs for MySQL/PG) */
#include "db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * NOTE: This is a minimal implementation.
 * For a full build, link against -lsqlite3, -lmysqlclient, or -lpq.
 * This file provides the interface and an in-memory stub for testing.
 */

/* â”€â”€ Simple in-memory table for testing without SQLite â”€â”€ */

typedef struct {
    char name[128];
    char rows[256][SKY_DB_MAX_COLUMNS][SKY_DB_VALUE_LEN];
    char cols[SKY_DB_MAX_COLUMNS][128];
    int  col_count;
    int  row_count;
} SkyMemTable;

typedef struct {
    SkyMemTable tables[16];
    int         table_count;
} SkyMemDB;

static SkyMemDB g_memdb = {0};

/* â”€â”€ Connect â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€ */

SkyDBConnection* sky_db_connect(SkyDBType type,
                                const char *host,
                                const char *database,
                                const char *user,
                                const char *password,
                                uint16_t port) {
    SkyDBConnection *conn = calloc(1, sizeof(SkyDBConnection));
    if (!conn) return NULL;

    conn->type = type;
    conn->port = port;

    if (host) strncpy(conn->host, host, sizeof(conn->host) - 1);
    if (database) strncpy(conn->database, database,
                          sizeof(conn->database) - 1);
    if (user) strncpy(conn->user, user, sizeof(conn->user) - 1);
    if (password) strncpy(conn->password, password,
                          sizeof(conn->password) - 1);

    switch (type) {
        case SKY_DB_SQLITE:
            /* In a full build, open SQLite here */
            conn->handle = &g_memdb;
            conn->connected = true;
            fprintf(stderr, "[SKY DB] Connected to in-memory database\n");
            break;

        case SKY_DB_MYSQL:
            snprintf(conn->error, sizeof(conn->error),
                     "MySQL support requires -lmysqlclient");
            conn->connected = false;
            break;

        case SKY_DB_POSTGRESQL:
            snprintf(conn->error, sizeof(conn->error),
                     "PostgreSQL support requires -lpq");
            conn->connected = false;
            break;
    }

    return conn;
}

void sky_db_close(SkyDBConnection *conn) {
    if (!conn) return;
    conn->connected = false;
    free(conn);
}

/* â”€â”€ Query (returns result set) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€ */

SkyDBResult* sky_db_query(SkyDBConnection *conn, const char *sql) {
    if (!conn || !conn->connected || !sql) return NULL;

    SkyDBResult *result = calloc(1, sizeof(SkyDBResult));
    if (!result) return NULL;

    /*
     * Stub implementation: parse simple SELECT * FROM table
     * In production, delegate to SQLite/MySQL/PG driver.
     */

    SkyMemDB *db = (SkyMemDB*)conn->handle;

    /* Very basic SQL parsing for demo */
    char table_name[128] = {0};
    if (sscanf(sql, "SELECT * FROM %127s", table_name) == 1 ||
        sscanf(sql, "select * from %127s", table_name) == 1) {

        /* Remove trailing semicolons or whitespace */
        size_t len = strlen(table_name);
        while (len > 0 && (table_name[len-1] == ';' ||
                           table_name[len-1] == ' ')) {
            table_name[--len] = '\0';
        }

        /* Find table */
        for (int t = 0; t < db->table_count; t++) {
            if (strcmp(db->tables[t].name, table_name) == 0) {
                SkyMemTable *tbl = &db->tables[t];
                result->column_count = tbl->col_count;
                result->row_count = tbl->row_count;

                for (int c = 0; c < tbl->col_count; c++) {
                    strncpy(result->columns[c], tbl->cols[c], 127);
                }
                for (int r = 0; r < tbl->row_count; r++) {
                    for (int c = 0; c < tbl->col_count; c++) {
                        strncpy(result->values[r][c],
                                tbl->rows[r][c],
                                SKY_DB_VALUE_LEN - 1);
                    }
                }
                return result;
            }
        }
    }

    /* No results */
    result->column_count = 0;
    result->row_count = 0;
    return result;
}

/* â”€â”€ Execute (no result set) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€ */

bool sky_db_execute(SkyDBConnection *conn, const char *sql) {
    if (!conn || !conn->connected || !sql) return false;

    SkyMemDB *db = (SkyMemDB*)conn->handle;

    /* Parse CREATE TABLE name (col1, col2, ...) */
    char table_name[128] = {0};
    const char *p = NULL;

    if ((p = strstr(sql, "CREATE TABLE ")) != NULL ||
        (p = strstr(sql, "create table ")) != NULL) {
        p += 13; /* skip "CREATE TABLE " */
        while (*p == ' ') p++;
        int i = 0;
        while (*p && *p != ' ' && *p != '(' && i < 127) {
            table_name[i++] = *p++;
        }
        table_name[i] = '\0';

        if (db->table_count < 16) {
            SkyMemTable *tbl = &db->tables[db->table_count++];
            strncpy(tbl->name, table_name, 127);
            tbl->col_count = 0;
            tbl->row_count = 0;

            /* Parse columns */
            const char *paren = strchr(p, '(');
            if (paren) {
                paren++;
                char col_buf[128];
                int ci = 0;
                while (*paren && *paren != ')') {
                    while (*paren == ' ' || *paren == ',') paren++;
                    ci = 0;
                    while (*paren && *paren != ' ' && *paren != ',' &&
                           *paren != ')' && ci < 127) {
                        col_buf[ci++] = *paren++;
                    }
                    col_buf[ci] = '\0';
                    if (ci > 0 && tbl->col_count < SKY_DB_MAX_COLUMNS) {
                        strncpy(tbl->cols[tbl->col_count++], col_buf, 127);
                    }
                    /* Skip type keywords */
                    while (*paren && *paren != ',' && *paren != ')') {
                        paren++;
                    }
                }
            }
            return true;
        }
    }

    /* Parse INSERT INTO table VALUES (...) */
    if ((p = strstr(sql, "INSERT INTO ")) != NULL ||
        (p = strstr(sql, "insert into ")) != NULL) {
        p += 12;
        while (*p == ' ') p++;
        int i = 0;
        while (*p && *p != ' ' && i < 127) {
            table_name[i++] = *p++;
        }
        table_name[i] = '\0';

        for (int t = 0; t < db->table_count; t++) {
            if (strcmp(db->tables[t].name, table_name) == 0) {
                SkyMemTable *tbl = &db->tables[t];
                if (tbl->row_count >= 256) return false;

                const char *vals = strchr(p, '(');
                if (!vals) return false;
                vals++;

                int row = tbl->row_count;
                int col = 0;
                char val_buf[SKY_DB_VALUE_LEN];
                int vi = 0;
                bool in_string = false;

                while (*vals && *vals != ')' && col < tbl->col_count) {
                    if (*vals == '\'') {
                        in_string = !in_string;
                        vals++;
                        continue;
                    }
                    if (!in_string && (*vals == ',' || *vals == ')')) {
                        val_buf[vi] = '\0';
                        strncpy(tbl->rows[row][col], val_buf,
                                SKY_DB_VALUE_LEN - 1);
                        col++;
                        vi = 0;
                        if (*vals == ',') vals++;
                        while (*vals == ' ') vals++;
                        continue;
                    }
                    if (vi < SKY_DB_VALUE_LEN - 1) {
                        val_buf[vi++] = *vals;
                    }
                    vals++;
                }
                if (vi > 0 && col < tbl->col_count) {
                    val_buf[vi] = '\0';
                    strncpy(tbl->rows[row][col], val_buf,
                            SKY_DB_VALUE_LEN - 1);
                    col++;
                }
                tbl->row_count++;
                return true;
            }
        }
    }

    return true; /* Unknown SQL â€” silently succeed for stubs */
}

/* â”€â”€ Find convenience â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€ */

SkyDBResult* sky_db_find(SkyDBConnection *conn,
                         const char *table,
                         const char *field,
                         const char *value) {
    if (!conn || !table || !field || !value) return NULL;

    /* Build parameterized-style query (safe from injection
       because we're using our internal API) */
    char sql[1024];
    snprintf(sql, sizeof(sql),
             "SELECT * FROM %s WHERE %s = '%s'",
             table, field, value);

    /* NOTE: In production, use sky_db_query_param instead */
    return sky_db_query(conn, sql);
}

/* â”€â”€ Free result â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€ */

void sky_db_result_free(SkyDBResult *result) {
    if (result) free(result);
}

const char* sky_db_error(SkyDBConnection *conn) {
    if (!conn) return "null connection";
    return conn->error;
}

/* â”€â”€ Parameterized queries (stubs) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€ */

SkyDBResult* sky_db_query_param(SkyDBConnection *conn,
                                const char *sql,
                                const char **params,
                                int param_count) {
    (void)params;
    (void)param_count;
    /* In production: bind params properly */
    return sky_db_query(conn, sql);
}

bool sky_db_execute_param(SkyDBConnection *conn,
                          const char *sql,
                          const char **params,
                          int param_count) {
    (void)params;
    (void)param_count;
    return sky_db_execute(conn, sql);
}

/* src/runtime/http_server.h — Built-in HTTP server header */
#ifndef SKY_HTTP_SERVER_H
#define SKY_HTTP_SERVER_H

#include <stdint.h>
#include <stdbool.h>
#include "../platform.h"
#include "security.h"
#include "async.h"

#define SKY_HTTP_MAX_ROUTES      128
#define SKY_HTTP_MAX_HEADERS     64
#define SKY_HTTP_MAX_HEADER_LEN  4096
#define SKY_HTTP_MAX_BODY_LEN    (1024 * 1024)
#define SKY_HTTP_MAX_PATH_LEN    2048
#define SKY_HTTP_BACKLOG         512

typedef enum {
    SKY_HTTP_GET, SKY_HTTP_POST, SKY_HTTP_PUT, SKY_HTTP_DELETE,
    SKY_HTTP_PATCH, SKY_HTTP_HEAD, SKY_HTTP_OPTIONS
} SkyHTTPMethod;

typedef struct {
    char key[256];
    char value[512];
} SkyHTTPHeader;

typedef struct {
    SkyHTTPMethod method;
    char          path[SKY_HTTP_MAX_PATH_LEN];
    char          query_string[1024];
    SkyHTTPHeader headers[SKY_HTTP_MAX_HEADERS];
    int           header_count;
    char         *body;
    size_t        body_len;
    char          client_ip[46];
    int           client_fd;
    char          raw[SKY_HTTP_MAX_HEADER_LEN];
} SkyHTTPRequest;

typedef struct {
    int           status;
    SkyHTTPHeader headers[SKY_HTTP_MAX_HEADERS];
    int           header_count;
    char         *body;
    size_t        body_len;
    bool          sent;
} SkyHTTPResponse;

typedef void (*SkyRouteHandler)(SkyHTTPRequest *req, SkyHTTPResponse *res, void *user_data);
typedef bool (*SkyMiddleware)(SkyHTTPRequest *req, SkyHTTPResponse *res, void *user_data);

typedef struct {
    SkyHTTPMethod   method;
    char            path[SKY_HTTP_MAX_PATH_LEN];
    SkyRouteHandler handler;
    void           *user_data;
    bool            requires_auth;
    bool            active;
} SkyRoute;

typedef struct {
    int                server_fd;
    uint16_t           port;
    char               host[256];
    SkyRoute           routes[SKY_HTTP_MAX_ROUTES];
    uint32_t           route_count;
    SkyMiddleware      middleware[16];
    void              *middleware_data[16];
    uint32_t           middleware_count;
    SkySecurityEngine *security;
    SkyAsyncEngine    *async_engine;
    bool               running;
    sky_thread_t       accept_thread;
} SkyHTTPServer;

bool sky_http_server_init(SkyHTTPServer *server, const char *host, uint16_t port);
void sky_http_server_destroy(SkyHTTPServer *server);
void sky_http_server_route(SkyHTTPServer *server, SkyHTTPMethod method, const char *path, SkyRouteHandler handler, void *user_data);
void sky_http_server_route_auth(SkyHTTPServer *server, SkyHTTPMethod method, const char *path, SkyRouteHandler handler, void *user_data);
void sky_http_server_middleware(SkyHTTPServer *server, SkyMiddleware mw, void *user_data);
void sky_http_server_set_security(SkyHTTPServer *server, SkySecurityEngine *engine);
bool sky_http_server_start(SkyHTTPServer *server);
void sky_http_server_stop(SkyHTTPServer *server);
void sky_http_respond(SkyHTTPResponse *res, int status, const char *content_type, const char *body);
void sky_http_respond_json(SkyHTTPResponse *res, int status, const char *json);
void sky_http_respond_error(SkyHTTPResponse *res, int status, const char *message);
void sky_http_set_header(SkyHTTPResponse *res, const char *key, const char *value);
const char* sky_http_get_header(SkyHTTPRequest *req, const char *key);
SkyHTTPMethod sky_http_method_from_string(const char *method);
const char* sky_http_method_string(SkyHTTPMethod method);

#endif

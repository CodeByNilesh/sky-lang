/* src/runtime/http_server.c — Built-in HTTP server implementation */
#include "http_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#endif

static bool parse_http_request(const char *raw, size_t raw_len, SkyHTTPRequest *req) {
    char method[16] = {0};
    char path[SKY_HTTP_MAX_PATH_LEN] = {0};
    char version[16] = {0};
    int parsed;
    char *q;
    const char *line, *colon, *eol, *val_start, *body_start;
    size_t key_len, val_len, body_len;
    if (!raw || raw_len == 0 || !req) return false;
    memset(req, 0, sizeof(SkyHTTPRequest));
    strncpy(req->raw, raw, raw_len < SKY_HTTP_MAX_HEADER_LEN - 1 ? raw_len : SKY_HTTP_MAX_HEADER_LEN - 1);
    parsed = sscanf(raw, "%15s %2047s %15s", method, path, version);
    if (parsed < 2) return false;
    req->method = sky_http_method_from_string(method);
    q = strchr(path, '?');
    if (q) {
        *q = '\0';
        strncpy(req->query_string, q + 1, sizeof(req->query_string) - 1);
    }
    strncpy(req->path, path, sizeof(req->path) - 1);
    line = strstr(raw, "\r\n");
    if (!line) return true;
    line += 2;
    while (*line && !(line[0] == '\r' && line[1] == '\n')) {
        if (req->header_count >= SKY_HTTP_MAX_HEADERS) break;
        colon = strchr(line, ':');
        eol = strstr(line, "\r\n");
        if (!colon || !eol || colon > eol) break;
        key_len = colon - line;
        if (key_len >= 256) key_len = 255;
        memcpy(req->headers[req->header_count].key, line, key_len);
        req->headers[req->header_count].key[key_len] = '\0';
        val_start = colon + 1;
        while (*val_start == ' ') val_start++;
        val_len = eol - val_start;
        if (val_len >= 512) val_len = 511;
        memcpy(req->headers[req->header_count].value, val_start, val_len);
        req->headers[req->header_count].value[val_len] = '\0';
        req->header_count++;
        line = eol + 2;
    }
    body_start = strstr(raw, "\r\n\r\n");
    if (body_start) {
        body_start += 4;
        body_len = raw_len - (body_start - raw);
        if (body_len > 0 && body_len < SKY_HTTP_MAX_BODY_LEN) {
            req->body = (char*)malloc(body_len + 1);
            if (req->body) {
                memcpy(req->body, body_start, body_len);
                req->body[body_len] = '\0';
                req->body_len = body_len;
            }
        }
    }
    return true;
}

static void send_response(int client_fd, SkyHTTPResponse *res) {
    char header_buf[4096];
    const char *status_text;
    int hlen, i;
    if (res->sent) return;
    res->sent = true;
    switch (res->status) {
        case 200: status_text = "OK"; break;
        case 201: status_text = "Created"; break;
        case 204: status_text = "No Content"; break;
        case 400: status_text = "Bad Request"; break;
        case 401: status_text = "Unauthorized"; break;
        case 403: status_text = "Forbidden"; break;
        case 404: status_text = "Not Found"; break;
        case 405: status_text = "Method Not Allowed"; break;
        case 429: status_text = "Too Many Requests"; break;
        case 500: status_text = "Internal Server Error"; break;
        default: status_text = "Unknown"; break;
    }
    hlen = snprintf(header_buf, sizeof(header_buf), "HTTP/1.1 %d %s\r\nServer: Sky/1.0\r\nConnection: close\r\n", res->status, status_text);
    for (i = 0; i < res->header_count; i++) {
        hlen += snprintf(header_buf + hlen, sizeof(header_buf) - hlen, "%s: %s\r\n", res->headers[i].key, res->headers[i].value);
    }
    hlen += snprintf(header_buf + hlen, sizeof(header_buf) - hlen, "Content-Length: %u\r\n\r\n", (unsigned)res->body_len);
    send(client_fd, header_buf, hlen, 0);
    if (res->body && res->body_len > 0) {
        send(client_fd, res->body, (int)res->body_len, 0);
    }
}

static SkyRoute* find_route(SkyHTTPServer *server, SkyHTTPMethod method, const char *path) {
    uint32_t i;
    for (i = 0; i < server->route_count; i++) {
        SkyRoute *r = &server->routes[i];
        if (!r->active) continue;
        if (r->method == method && strcmp(r->path, path) == 0) return r;
    }
    return NULL;
}

static void handle_client(SkyHTTPServer *server, int client_fd, const char *client_ip) {
    char buf[SKY_HTTP_MAX_HEADER_LEN + SKY_HTTP_MAX_BODY_LEN];
    int received;
    SkyHTTPRequest req;
    SkyHTTPResponse res;
    SkyRoute *route;
    uint32_t i;
    received = recv(client_fd, buf, sizeof(buf) - 1, 0);
    if (received <= 0) { sky_close_socket(client_fd); return; }
    buf[received] = '\0';
    if (!parse_http_request(buf, received, &req)) { sky_close_socket(client_fd); return; }
    strncpy(req.client_ip, client_ip, sizeof(req.client_ip) - 1);
    req.client_fd = client_fd;
    if (server->security) {
        if (!sky_security_check_request(server->security, client_ip, sky_http_method_string(req.method), req.path, req.body, req.raw)) {
            memset(&res, 0, sizeof(res));
            sky_http_respond_error(&res, 403, "Forbidden");
            send_response(client_fd, &res);
            if (res.body) free(res.body);
            if (req.body) free(req.body);
            sky_close_socket(client_fd);
            return;
        }
    }
    memset(&res, 0, sizeof(res));
    for (i = 0; i < server->middleware_count; i++) {
        if (!server->middleware[i](&req, &res, server->middleware_data[i])) {
            if (!res.sent) {
                sky_http_respond_error(&res, 403, "Blocked by middleware");
                send_response(client_fd, &res);
            }
            if (res.body) free(res.body);
            if (req.body) free(req.body);
            sky_close_socket(client_fd);
            return;
        }
    }
    route = find_route(server, req.method, req.path);
    if (!route) {
        sky_http_respond_error(&res, 404, "Not Found");
        send_response(client_fd, &res);
    } else {
        route->handler(&req, &res, route->user_data);
        if (!res.sent) send_response(client_fd, &res);
    }
    if (res.body) free(res.body);
    if (req.body) free(req.body);
    sky_close_socket(client_fd);
}

static void* accept_loop(void *arg) {
    SkyHTTPServer *server = (SkyHTTPServer*)arg;
    while (server->running) {
        struct sockaddr_in client_addr;
        int addr_len = sizeof(client_addr);
        char client_ip[46];
        int client_fd;
#ifdef _WIN32
        client_fd = (int)accept((SOCKET)server->server_fd, (struct sockaddr*)&client_addr, &addr_len);
#else
        client_fd = accept(server->server_fd, (struct sockaddr*)&client_addr, (socklen_t*)&addr_len);
#endif
        if (client_fd < 0) {
            if (!server->running) break;
            sky_sleep_ms(1);
            continue;
        }
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        handle_client(server, client_fd, client_ip);
    }
    return NULL;
}

bool sky_http_server_init(SkyHTTPServer *server, const char *host, uint16_t port) {
    struct sockaddr_in addr;
    int opt = 1;
    if (!server) return false;
    sky_platform_init();
    memset(server, 0, sizeof(SkyHTTPServer));
    server->port = port;
    strncpy(server->host, host ? host : "0.0.0.0", sizeof(server->host) - 1);
    server->server_fd = (int)socket(AF_INET, SOCK_STREAM, 0);
    if (server->server_fd < 0) {
        fprintf(stderr, "[SKY HTTP] socket failed\n");
        return false;
    }
    setsockopt(server->server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, server->host, &addr.sin_addr);
    if (bind(server->server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "[SKY HTTP] bind failed\n");
        sky_close_socket(server->server_fd);
        return false;
    }
    if (listen(server->server_fd, SKY_HTTP_BACKLOG) < 0) {
        fprintf(stderr, "[SKY HTTP] listen failed\n");
        sky_close_socket(server->server_fd);
        return false;
    }
    return true;
}

void sky_http_server_destroy(SkyHTTPServer *server) {
    if (!server) return;
    sky_http_server_stop(server);
    if (server->server_fd >= 0) {
        sky_close_socket(server->server_fd);
        server->server_fd = -1;
    }
    sky_platform_cleanup();
}

void sky_http_server_route(SkyHTTPServer *server, SkyHTTPMethod method, const char *path, SkyRouteHandler handler, void *user_data) {
    SkyRoute *r;
    if (!server || server->route_count >= SKY_HTTP_MAX_ROUTES) return;
    r = &server->routes[server->route_count++];
    r->method = method;
    strncpy(r->path, path, sizeof(r->path) - 1);
    r->handler = handler;
    r->user_data = user_data;
    r->requires_auth = false;
    r->active = true;
    fprintf(stderr, "[SKY HTTP] Route: %s %s\n", sky_http_method_string(method), path);
}

void sky_http_server_route_auth(SkyHTTPServer *server, SkyHTTPMethod method, const char *path, SkyRouteHandler handler, void *user_data) {
    sky_http_server_route(server, method, path, handler, user_data);
    if (server->route_count > 0) {
        server->routes[server->route_count - 1].requires_auth = true;
    }
}

void sky_http_server_middleware(SkyHTTPServer *server, SkyMiddleware mw, void *user_data) {
    if (!server || server->middleware_count >= 16) return;
    server->middleware[server->middleware_count] = mw;
    server->middleware_data[server->middleware_count] = user_data;
    server->middleware_count++;
}

void sky_http_server_set_security(SkyHTTPServer *server, SkySecurityEngine *engine) {
    if (server) server->security = engine;
}

bool sky_http_server_start(SkyHTTPServer *server) {
    if (!server) return false;
    server->running = true;
    fprintf(stderr, "\n  Sky Server v1.0\n  http://%s:%d\n\n", server->host, server->port);
    if (sky_thread_create(&server->accept_thread, accept_loop, server) != 0) {
        fprintf(stderr, "[SKY HTTP] thread create failed\n");
        return false;
    }
    return true;
}

void sky_http_server_stop(SkyHTTPServer *server) {
    if (!server || !server->running) return;
    server->running = false;
    sky_thread_join(server->accept_thread);
    fprintf(stderr, "[SKY HTTP] Server stopped\n");
}

void sky_http_respond(SkyHTTPResponse *res, int status, const char *content_type, const char *body) {
    if (!res) return;
    res->status = status;
    if (content_type) sky_http_set_header(res, "Content-Type", content_type);
    if (body) {
        res->body_len = strlen(body);
        res->body = (char*)malloc(res->body_len + 1);
        if (res->body) memcpy(res->body, body, res->body_len + 1);
    }
}

void sky_http_respond_json(SkyHTTPResponse *res, int status, const char *json) {
    sky_http_respond(res, status, "application/json", json);
}

void sky_http_respond_error(SkyHTTPResponse *res, int status, const char *message) {
    char json[512];
    snprintf(json, sizeof(json), "{\"error\":\"%s\",\"status\":%d}", message, status);
    sky_http_respond_json(res, status, json);
}

void sky_http_set_header(SkyHTTPResponse *res, const char *key, const char *value) {
    SkyHTTPHeader *h;
    if (!res || res->header_count >= SKY_HTTP_MAX_HEADERS) return;
    h = &res->headers[res->header_count++];
    strncpy(h->key, key, sizeof(h->key) - 1);
    strncpy(h->value, value, sizeof(h->value) - 1);
}

const char* sky_http_get_header(SkyHTTPRequest *req, const char *key) {
    int i;
    if (!req || !key) return NULL;
    for (i = 0; i < req->header_count; i++) {
        if (strcasecmp(req->headers[i].key, key) == 0) return req->headers[i].value;
    }
    return NULL;
}

SkyHTTPMethod sky_http_method_from_string(const char *method) {
    if (!method) return SKY_HTTP_GET;
    if (strcmp(method, "GET") == 0) return SKY_HTTP_GET;
    if (strcmp(method, "POST") == 0) return SKY_HTTP_POST;
    if (strcmp(method, "PUT") == 0) return SKY_HTTP_PUT;
    if (strcmp(method, "DELETE") == 0) return SKY_HTTP_DELETE;
    if (strcmp(method, "PATCH") == 0) return SKY_HTTP_PATCH;
    if (strcmp(method, "HEAD") == 0) return SKY_HTTP_HEAD;
    if (strcmp(method, "OPTIONS") == 0) return SKY_HTTP_OPTIONS;
    return SKY_HTTP_GET;
}

const char* sky_http_method_string(SkyHTTPMethod method) {
    switch (method) {
        case SKY_HTTP_GET: return "GET";
        case SKY_HTTP_POST: return "POST";
        case SKY_HTTP_PUT: return "PUT";
        case SKY_HTTP_DELETE: return "DELETE";
        case SKY_HTTP_PATCH: return "PATCH";
        case SKY_HTTP_HEAD: return "HEAD";
        case SKY_HTTP_OPTIONS: return "OPTIONS";
        default: return "GET";
    }
}

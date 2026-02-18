/* platform.h — Windows/Linux compatibility layer */
#ifndef SKY_PLATFORM_H
#define SKY_PLATFORM_H

#ifdef _WIN32
    #ifndef _WIN32_WINNT
        #define _WIN32_WINNT 0x0600
    #endif
    #include <windows.h>
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <io.h>
    #include <process.h>
    #include <string.h>

    typedef HANDLE sky_thread_t;
    typedef CRITICAL_SECTION sky_mutex_t;

    typedef struct {
        HANDLE event;
    } sky_cond_t;

    static inline void sky_mutex_init(sky_mutex_t *m) {
        InitializeCriticalSection(m);
    }
    static inline void sky_mutex_destroy(sky_mutex_t *m) {
        DeleteCriticalSection(m);
    }
    static inline void sky_mutex_lock(sky_mutex_t *m) {
        EnterCriticalSection(m);
    }
    static inline void sky_mutex_unlock(sky_mutex_t *m) {
        LeaveCriticalSection(m);
    }

    static inline void sky_cond_init(sky_cond_t *c) {
        c->event = CreateEvent(NULL, FALSE, FALSE, NULL);
    }
    static inline void sky_cond_destroy(sky_cond_t *c) {
        if (c->event) CloseHandle(c->event);
    }
    static inline void sky_cond_wait(sky_cond_t *c, sky_mutex_t *m) {
        LeaveCriticalSection(m);
        WaitForSingleObject(c->event, INFINITE);
        EnterCriticalSection(m);
    }
    static inline void sky_cond_signal(sky_cond_t *c) {
        SetEvent(c->event);
    }
    static inline void sky_cond_broadcast(sky_cond_t *c) {
        SetEvent(c->event);
    }

    static inline int sky_thread_create(sky_thread_t *t, void* (*func)(void*), void *arg) {
        typedef unsigned (__stdcall *win_func)(void*);
        *t = (HANDLE)_beginthreadex(NULL, 0, (win_func)func, arg, 0, NULL);
        return (*t == NULL) ? -1 : 0;
    }
    static inline void sky_thread_join(sky_thread_t t) {
        WaitForSingleObject(t, INFINITE);
        CloseHandle(t);
    }

    static inline void sky_sleep_ms(int ms) {
        Sleep(ms);
    }

    #define strcasecmp _stricmp
    #define strncasecmp _strnicmp
    #define sky_close_socket closesocket

    #ifndef ssize_t
        typedef int ssize_t;
    #endif

    static inline void sky_platform_init(void) {
        WSADATA wsa;
        WSAStartup(MAKEWORD(2, 2), &wsa);
    }
    static inline void sky_platform_cleanup(void) {
        WSACleanup();
    }

    /* Simple inet_ntop replacement for old MinGW */
    static inline const char* sky_inet_ntop_impl(int af, const void *src, char *dst, int size) {
        struct in_addr *addr = (struct in_addr*)src;
        unsigned char *bytes;
        (void)af;
        bytes = (unsigned char*)addr;
        snprintf(dst, size, "%d.%d.%d.%d", bytes[0], bytes[1], bytes[2], bytes[3]);
        return dst;
    }
    #define inet_ntop sky_inet_ntop_impl

    /* Simple inet_pton replacement for old MinGW */
    static inline int sky_inet_pton_impl(int af, const char *src, void *dst) {
        unsigned int a, b, c, d;
        struct in_addr *addr = (struct in_addr*)dst;
        (void)af;
        if (sscanf(src, "%u.%u.%u.%u", &a, &b, &c, &d) != 4) return 0;
        ((unsigned char*)addr)[0] = (unsigned char)a;
        ((unsigned char*)addr)[1] = (unsigned char)b;
        ((unsigned char*)addr)[2] = (unsigned char)c;
        ((unsigned char*)addr)[3] = (unsigned char)d;
        return 1;
    }
    #define inet_pton sky_inet_pton_impl

#else
    #include <pthread.h>
    #include <unistd.h>
    #include <signal.h>
    #include <fcntl.h>
    #include <strings.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>

    typedef pthread_t sky_thread_t;
    typedef pthread_mutex_t sky_mutex_t;
    typedef pthread_cond_t sky_cond_t;

    #define sky_mutex_init(m)      pthread_mutex_init(m, NULL)
    #define sky_mutex_destroy(m)   pthread_mutex_destroy(m)
    #define sky_mutex_lock(m)      pthread_mutex_lock(m)
    #define sky_mutex_unlock(m)    pthread_mutex_unlock(m)
    #define sky_cond_init(c)       pthread_cond_init(c, NULL)
    #define sky_cond_destroy(c)    pthread_cond_destroy(c)
    #define sky_cond_wait(c, m)    pthread_cond_wait(c, m)
    #define sky_cond_signal(c)     pthread_cond_signal(c)
    #define sky_cond_broadcast(c)  pthread_cond_broadcast(c)

    static inline int sky_thread_create(sky_thread_t *t, void* (*func)(void*), void *arg) {
        return pthread_create(t, NULL, func, arg);
    }
    static inline void sky_thread_join(sky_thread_t t) {
        pthread_join(t, NULL);
    }
    static inline void sky_sleep_ms(int ms) {
        usleep(ms * 1000);
    }

    #define sky_close_socket close
    static inline void sky_platform_init(void) {}
    static inline void sky_platform_cleanup(void) {}
#endif

#endif

#include "config/config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET socket_t;
#define close_socket(s) closesocket(s)
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int socket_t;
#define close_socket(s) close(s)
#endif

static int send_http_request(const char *method, const char *path, char *response_buf, size_t buf_cap) {
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

    socket_t fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(CWEB_DEFAULT_PORT);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        close_socket(fd);
        return -1;
    }

    char req[512];
    snprintf(req, sizeof(req), "%s %s HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n", method, path);
    send(fd, req, (int)strlen(req), 0);

    int total_read = 0;
    int n = 0;
    while ((n = recv(fd, response_buf + total_read, (int)(buf_cap - total_read - 1), 0)) > 0) {
        total_read += n;
    }
    response_buf[total_read] = '\0';

    close_socket(fd);
    return total_read;
}

int main(void) {
    printf("==========================================\n");
    printf(" CWeb REST API Integration Test Suite\n");
    printf("==========================================\n");

    char buf[4096];

    /* Test 1: GET /api/v1/health */
    int len = send_http_request("GET", "/api/v1/health", buf, sizeof(buf));
    if (len > 0 && strstr(buf, "200 OK") && strstr(buf, "\"status\":\"ok\"")) {
        printf("[PASS] GET /api/v1/health -> 200 OK\n");
    } else {
        printf("[FAIL] GET /api/v1/health\n");
    }

    /* Test 2: GET /api/v1/stats */
    len = send_http_request("GET", "/api/v1/stats", buf, sizeof(buf));
    if (len > 0 && strstr(buf, "200 OK") && strstr(buf, "\"documents_indexed\"")) {
        printf("[PASS] GET /api/v1/stats -> 200 OK\n");
    } else {
        printf("[FAIL] GET /api/v1/stats\n");
    }

    /* Test 3: GET /api/v1/search?q=compiler+optimization */
    len = send_http_request("GET", "/api/v1/search?q=compiler+optimization", buf, sizeof(buf));
    if (len > 0 && strstr(buf, "200 OK") && strstr(buf, "\"results\":[")) {
        printf("[PASS] GET /api/v1/search -> 200 OK\n");
    } else {
        printf("[FAIL] GET /api/v1/search\n");
    }

    /* Test 4: GET /api/v1/suggest?q=comp */
    len = send_http_request("GET", "/api/v1/suggest?q=comp", buf, sizeof(buf));
    if (len > 0 && strstr(buf, "200 OK") && strstr(buf, "\"suggestions\":[")) {
        printf("[PASS] GET /api/v1/suggest -> 200 OK\n");
    } else {
        printf("[FAIL] GET /api/v1/suggest\n");
    }

    /* Test 5: GET /api/v1/page/1 */
    len = send_http_request("GET", "/api/v1/page/1", buf, sizeof(buf));
    if (len > 0 && strstr(buf, "200 OK") && strstr(buf, "\"id\":1")) {
        printf("[PASS] GET /api/v1/page/1 -> 200 OK\n");
    } else {
        printf("[FAIL] GET /api/v1/page/1\n");
    }

    /* Test 6: GET /api/v1/page/99999 (Non-existent Document ID) */
    len = send_http_request("GET", "/api/v1/page/99999", buf, sizeof(buf));
    if (len > 0 && strstr(buf, "404 Not Found") && strstr(buf, "DOCUMENT_NOT_FOUND")) {
        printf("[PASS] GET /api/v1/page/99999 -> 404 DOCUMENT_NOT_FOUND\n");
    } else {
        printf("[FAIL] GET /api/v1/page/99999 Error Handling\n");
    }

    printf("==========================================\n");
    return 0;
}

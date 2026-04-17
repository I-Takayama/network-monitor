#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "webserver.h"

#define BUFFER_SIZE 4096
#define JSON_BUFFER_SIZE 65536

static void send_404(int client_sock);
static void send_index_html(int client_sock);
static void send_status_json(int client_sock, SharedStatus* shared_status);
static void handle_client(int client_sock, SharedStatus* shared_status);

void* start_web_server(void* arg) {
    WebServerArg* server_arg = (WebServerArg*)arg;
    int server_port = server_arg->port;
    SharedStatus* shared_status = server_arg->shared_status;

    int server_sock;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("socket");
        return NULL;
    }

    int opt = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(server_sock);
        return NULL;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(server_port);

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_sock);
        return NULL;
    }

    if (listen(server_sock, 10) < 0) {
        perror("listen");
        close(server_sock);
        return NULL;
    }

    printf("Web server started: http://localhost:%d/\n", server_port);

    while (1) {
        int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
        if (client_sock < 0) {
            perror("accept");
            continue;
        }

        handle_client(client_sock, shared_status);
        close(client_sock);
    }

    close(server_sock);
    return NULL;
}

static void handle_client(int client_sock, SharedStatus* shared_status) {
    char buffer[BUFFER_SIZE];
    int received;

    memset(buffer, 0, sizeof(buffer));
    received = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    if (received <= 0) {
        return;
    }

    if (strncmp(buffer, "GET / ", 6) == 0 || strncmp(buffer, "GET /HTTP", 9) == 0) {
        send_index_html(client_sock);
    }
    else if (strncmp(buffer, "GET /api/status ", 16) == 0) {
        send_status_json(client_sock, shared_status);
    }
    else {
        send_404(client_sock);
    }
}

static void send_index_html(int client_sock) {
    FILE* fp = fopen("web/index.html", "r");
    if (fp == NULL) {
        const char* body = "<html><body><h1>index.html not found</h1></body></html>";

        dprintf(client_sock,
                "HTTP/1.1 500 Internal Server Error\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: %zu\r\n"
                "Connection: close\r\n"
                "\r\n"
                "%s",
                strlen(body), body);
        return;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    rewind(fp);

    char* body = malloc(file_size + 1);
    if (body == NULL) {
        fclose(fp);
        return;
    }

    fread(body, 1, file_size, fp);
    body[file_size] = '\0';
    fclose(fp);

    dprintf(client_sock,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=UTF-8\r\n"
            "Content-Length: %ld\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s",
            file_size, body);

    free(body);
}

static void send_status_json(int client_sock, SharedStatus* shared_status) {
    char body[JSON_BUFFER_SIZE];
    int offset = 0;

    pthread_mutex_lock(&shared_status->mutex);

    offset += snprintf(body + offset, sizeof(body) - offset, "[");

    for (int i = 0; i < shared_status->count; i++) {
        MonitorResult* r = &shared_status->results[i];

        offset += snprintf(
            body + offset,
            sizeof(body) - offset,
            "%s"
            "{"
            "\"time_str\":\"%s\","
            "\"ip\":\"%s\","
            "\"port\":%d,"
            "\"result\":%d,"
            "\"response_ms\":%ld,"
            "\"anomaly_type\":%d,"
            "\"anomaly_message\":\"%s\""
            "}",
            (i == 0 ? "" : ","),
            r->scan.time_str,
            r->scan.ip,
            r->scan.port,
            r->scan.result,
            r->scan.response_ms,
            r->anomaly.anomaly_type,
            r->anomaly.message
        );

        if (offset >= (int)sizeof(body) - 512) {
            break;
        }
    }

    offset += snprintf(body + offset, sizeof(body) - offset, "]");

    pthread_mutex_unlock(&shared_status->mutex);

    dprintf(client_sock,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json; charset=UTF-8\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s",
            offset, body);
}

static void send_404(int client_sock) {
    const char* body = "<html><body><h1>404 Not Found</h1></body></html>";

    dprintf(client_sock,
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: %zu\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s",
            strlen(body), body);
}
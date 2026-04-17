#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "scanner.h"
#include "utils.h"


int scan_port(const char* ip, int port, ScanResult* scan_result){
    int sockfd;
    struct sockaddr_in server;
    struct timeval start, end;

    strncpy(scan_result->ip, ip, sizeof(scan_result->ip) - 1);
    scan_result->ip[sizeof(scan_result->ip) - 1] = '\0';
    scan_result->port = port;
    get_current_time_str(scan_result->time_str, sizeof(scan_result->time_str));

    gettimeofday(&start, NULL);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        perror("socket");
        scan_result->result = -1;
        scan_result->response_ms = -1;
        return -1;
    }

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &server.sin_addr) <= 0) {
        perror("inet_pton");
        close(sockfd);
        scan_result->result = -1;
        scan_result->response_ms = -1;
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        gettimeofday(&end, NULL);
        scan_result->response_ms = (end.tv_sec - start.tv_sec) * 1000L
                                 + (end.tv_usec - start.tv_usec) / 1000L;
        close(sockfd);
        scan_result->result = 0;
        return 0;
    }

    gettimeofday(&end, NULL);
    scan_result->response_ms = (end.tv_sec - start.tv_sec) * 1000L
                             + (end.tv_usec - start.tv_usec) / 1000L;

    close(sockfd);
    scan_result->result = 1;
    return 1;
}

void print_result(const ScanResult* scan_result){
    if(scan_result->result == 0){
        printf("[%s] %s:%d -> CLOSED (%ld ms)\n",
               scan_result->time_str,
               scan_result->ip,
               scan_result->port,
               scan_result->response_ms);
    }else if(scan_result->result == 1){
        printf("[%s] %s:%d -> OPEN (%ld ms)\n",
               scan_result->time_str,
               scan_result->ip,
               scan_result->port,
               scan_result->response_ms);
    }else{
        printf("[%s] %s:%d -> ERROR (%ld ms)\n",
               scan_result->time_str,
               scan_result->ip,
               scan_result->port,
               scan_result->response_ms);
    }
}
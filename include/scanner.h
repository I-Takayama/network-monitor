#ifndef SCANNER_H
#define SCANNER_H

#define STATUS_OPEN 1
#define STATUS_CLOSED 0
#define STATUS_ERROR -1

typedef struct {
    char ip[64];
    int start_port;
    int end_port;
} ScanTarget;

typedef struct {
    char time_str[64];
    char ip[64];
    int port;
    int result;         // 1: OPEN, 0: CLOSED, -1: ERROR
    long response_ms;
} ScanResult;

int scan_port(const char* ip, int port, ScanResult* scan_result);
void print_result(const ScanResult* scan_result);

#endif
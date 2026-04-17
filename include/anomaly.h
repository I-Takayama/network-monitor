#ifndef ANOMALY_H
#define ANOMALY_H

#include "scanner.h"

#define ANOMALY_NONE 0
#define ANOMALY_STATUS_CHANGED 1
#define ANOMALY_SLOW_RESPONSE 2
#define ANOMALY_CONSECUTIVE_FAILURE 3

typedef struct {
    int previous_result;
    int current_result;
    int failure_count;
    double previous_response_ms;
} PortHistory;

typedef struct {
    int anomaly_type;
    char message[256];
} AnomalyResult;

void init_port_history(PortHistory* history);
void detect_anomaly(const ScanResult* current, PortHistory* history, AnomalyResult* anomaly);

#endif
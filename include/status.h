#ifndef STATUS_H
#define STATUS_H

#include <pthread.h>
#include "scanner.h"
#include "anomaly.h"

typedef struct {
    ScanResult scan;
    AnomalyResult anomaly;
} MonitorResult;

typedef struct {
    MonitorResult* results;
    int count;
    pthread_mutex_t mutex;
} SharedStatus;

void init_shared_status(SharedStatus* status, int count);
void update_shared_status(SharedStatus* status, int index, const MonitorResult* result);
void destroy_shared_status(SharedStatus* status);

#endif
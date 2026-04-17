#include <stdlib.h>
#include <string.h>
#include "status.h"

void init_shared_status(SharedStatus* status, int count) {
    status->results = malloc(sizeof(MonitorResult) * count);
    status->count = count;
    pthread_mutex_init(&status->mutex, NULL);

    for (int i = 0; i < count; i++) {
        memset(&status->results[i], 0, sizeof(MonitorResult));
    }
}

void update_shared_status(SharedStatus* status, int index, const MonitorResult* result) {
    pthread_mutex_lock(&status->mutex);
    status->results[index] = *result;
    pthread_mutex_unlock(&status->mutex);
}

void destroy_shared_status(SharedStatus* status) {
    free(status->results);
    pthread_mutex_destroy(&status->mutex);
}
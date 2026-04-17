#include <stdio.h>
#include "logger.h"

void log_result(const MonitorResult* result) {
    FILE* fp = fopen("data/results.csv", "a");
    if (!fp) {
        perror("fopen");
        return;
    }

    fprintf(fp, "%s,%s,%d,%d,%ld,%d,%s\n",
            result->scan.time_str,
            result->scan.ip,
            result->scan.port,
            result->scan.result,
            result->scan.response_ms,
            result->anomaly.anomaly_type,
            result->anomaly.message);

    fclose(fp);
}
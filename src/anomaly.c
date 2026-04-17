#include <stdio.h>
#include <string.h>
#include "anomaly.h"

#define SLOW_RESPONSE_THRESHOLD_MS 1000.0
#define FAILURE_THRESHOLD 3

void init_port_history(PortHistory* history) {
    history->previous_result = -999;  // 未初期化扱い
    history->current_result = -999;
    history->failure_count = 0;
    history->previous_response_ms = -1.0;
}

void detect_anomaly(const ScanResult* current, PortHistory* history, AnomalyResult* anomaly) {
    anomaly->anomaly_type = ANOMALY_NONE;
    strcpy(anomaly->message, "normal");

    history->current_result = current->result;

    /* 連続失敗回数の更新 */
    if (current->result == STATUS_ERROR) {
        history->failure_count++;
    } else {
        history->failure_count = 0;
    }

    /* 状態変化の検知 */
    if (history->previous_result != -999 && history->previous_result != current->result) {
        anomaly->anomaly_type = ANOMALY_STATUS_CHANGED;
        snprintf(anomaly->message, sizeof(anomaly->message),
                 "status changed: previous=%d current=%d",
                 history->previous_result, current->result);
    }
    /* 応答時間悪化 */
    else if (current->response_ms > SLOW_RESPONSE_THRESHOLD_MS) {
        anomaly->anomaly_type = ANOMALY_SLOW_RESPONSE;
        snprintf(anomaly->message, sizeof(anomaly->message),
                 "slow response: %ld ms", current->response_ms);
    }
    /* 連続失敗 */
    else if (history->failure_count >= FAILURE_THRESHOLD) {
        anomaly->anomaly_type = ANOMALY_CONSECUTIVE_FAILURE;
        snprintf(anomaly->message, sizeof(anomaly->message),
                 "consecutive failures: %d", history->failure_count);
    }

    history->previous_result = current->result;
    history->previous_response_ms = current->response_ms;
}
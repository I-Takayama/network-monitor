#include <stdio.h>
#include <string.h>
#include "anomaly.h"

// 応答遅延とみなすしきい値（ミリ秒）
#define SLOW_RESPONSE_THRESHOLD_MS 1000.0

// 継続的な障害と判断する連続失敗回数
#define FAILURE_THRESHOLD 3

/*
 * ポートごとの監視履歴を初期化する
 * 初回監視時は前回値が存在しないため、未初期化を表す値を設定する
 */
void init_port_history(PortHistory* history) {
    history->previous_result = -999;        // 前回状態なし（未初期化）
    history->current_result = -999;         // 現在状態も未設定で開始
    history->failure_count = 0;             // 連続失敗回数を初期化
    history->previous_response_ms = -1.0;   // 前回応答時間なし
}

/*
 * 現在の監視結果と過去履歴を比較し、異常の有無を判定する
 * 判定対象:
 * - 状態変化
 * - 応答遅延
 * - 連続失敗
 */
void detect_anomaly(const ScanResult* current, PortHistory* history, AnomalyResult* anomaly) {
    // 初期状態では異常なしとし、問題があれば後続の条件分岐で上書きする
    anomaly->anomaly_type = ANOMALY_NONE;
    strcpy(anomaly->message, "normal");

    // 今回の監視結果を履歴に反映する
    history->current_result = current->result;

    /* 連続失敗回数の更新
     * エラーが続いているかを判定するため、失敗時のみ加算する
     * 正常応答があれば連続性が途切れるため 0 に戻す
     */
    if (current->result == STATUS_ERROR) {
        history->failure_count++;
    } else {
        history->failure_count = 0;
    }

    /* 状態変化の検知
     * 前回状態が存在し、かつ今回結果と異なる場合に異常とみなす
     * 例: OPEN → CLOSED, CLOSED → OPEN
     */
    if (history->previous_result != -999 && history->previous_result != current->result) {
        anomaly->anomaly_type = ANOMALY_STATUS_CHANGED;
        snprintf(anomaly->message, sizeof(anomaly->message),
                 "status changed: previous=%d current=%d",
                 history->previous_result, current->result);
    }
    /* 応答時間悪化
     * 接続自体は成功していても、しきい値を超える遅延があれば性能劣化として扱う
     */
    else if (current->response_ms > SLOW_RESPONSE_THRESHOLD_MS) {
        anomaly->anomaly_type = ANOMALY_SLOW_RESPONSE;
        snprintf(anomaly->message, sizeof(anomaly->message),
                 "slow response: %ld ms", current->response_ms);
    }
    /* 連続失敗
     * 一時的な失敗ではなく、複数回連続した障害を異常として扱う
     */
    else if (history->failure_count >= FAILURE_THRESHOLD) {
        anomaly->anomaly_type = ANOMALY_CONSECUTIVE_FAILURE;
        snprintf(anomaly->message, sizeof(anomaly->message),
                 "consecutive failures: %d", history->failure_count);
    }

    // 次回比較用に今回の結果を保存する
    history->previous_result = current->result;
    history->previous_response_ms = current->response_ms;
}
#include <stdio.h>
#include "logger.h"


/*
 * 監視結果をCSV形式でファイルに追記する
 * 時系列で記録することで、後から状態変化や異常の分析を可能にする
 */
void log_result(const MonitorResult* result) {
    // 追記モードでファイルを開く（過去ログを保持するため）
    FILE* fp = fopen("data/results.csv", "a");
    if (!fp) {
        perror("fopen");
        return;
    }

    /*
     * CSV形式で1行出力
     * [時刻, IP, ポート, 状態, 応答時間, 異常種別, 異常内容]
     */
    fprintf(fp, "%s,%s,%d,%d,%ld,%d,%s\n",
            result->scan.time_str,          // 計測時刻
            result->scan.ip,                // 対象IPアドレス
            result->scan.port,              // ポート番号
            result->scan.result,            // OPEN / CLOSED / ERROR
            result->scan.response_ms,       // 応答時間（ミリ秒）
            result->anomaly.anomaly_type,   // 異常種別
            result->anomaly.message);       // 異常内容

    // ファイルを閉じてリソースを解放する
    fclose(fp);
}
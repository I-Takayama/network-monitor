#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "scanner.h"
#include "logger.h"
#include "anomaly.h"
#include "status.h"
#include "webserver.h"
#include "config.h"

/*
 * 監視スレッドに渡す引数
 * 1スレッドが1ポートを担当し、スキャン結果を共有状態へ反映する
 */
typedef struct {
    char ip[64];                    // 監視対象IPアドレス
    int port;                       // 監視対象ポート番号
    int index;                      // 共有配列上の格納位置
    SharedStatus* shared_status;    // Web表示用の共有監視状態
    PortHistory* history;           // 異常検知のための過去状態
} ThreadArg;

/*
 * 1ポート分の監視を行うスレッド関数
 * スキャン → 異常検知 → 共有状態更新 → 表示/ログ保存 の順で処理する
 */
void* thread_func(void* arg){
    ThreadArg* t = (ThreadArg*)arg;

    ScanResult scan_result;
    AnomalyResult anomaly_result;
    MonitorResult monitor_result;

    // 指定ポートにTCP接続を試み、開閉状態や応答時間を取得する
    scan_port(t->ip, t->port, &scan_result);

    // 現在の結果と過去状態を比較し、異常の有無を判定する
    detect_anomaly(&scan_result, t->history, &anomaly_result);

    // スキャン結果と異常検知結果を1つの構造体にまとめる
    monitor_result.scan = scan_result;
    monitor_result.anomaly = anomaly_result;

    // Webサーバから参照される共有状態を更新する
    update_shared_status(t->shared_status, t->index, &monitor_result);

    // ターミナルへ現在の監視結果を表示する
    print_result(&scan_result);
    printf("Anomaly: %s\n", anomaly_result.message);

    // CSVなどのログファイルへ結果を保存する
    log_result(&monitor_result);

    // スレッドごとに確保した引数領域を解放する
    free(t);  // メモリ解放
    return NULL;
}


int main(void){ 
    int port;
    int looptime;
    int port_count;
    ScanTarget target;
    Config config;

    // 設定ファイルを読み込む
    if (load_config("data/config.txt", &config) != 0) {
        fprintf(stderr, "設定ファイルの読み込みに失敗しました\n");
        return 1;
    }

    // 設定ファイルの値を監視条件へ反映
    strncpy(target.ip, config.ip, sizeof(target.ip) - 1);
    target.ip[sizeof(target.ip) - 1] = '\0';
    target.start_port = config.start_port;
    target.end_port = config.end_port;

    looptime = config.interval;
    port_count = (target.end_port - target.start_port + 1);

    // 設定値の妥当性を確認 
    if (target.start_port < 1 || target.end_port > 65535 || target.start_port > target.end_port) {
        printf("ポート番号の指定が不正です\n");
        return 1;
    }

    //定期監視の間隔が正の整数であることを確認する
    if (looptime <= 0) {
        printf("ループ間隔は1以上の整数で指定してください\n");
        return 1;
    }

    //監視結果をWeb表示用に共有する構造体を初期化する
    SharedStatus shared_status;
    init_shared_status(&shared_status, port_count);

    pthread_t web_thread;
    WebServerArg* web_arg = malloc(sizeof(WebServerArg));
    if (web_arg == NULL) {
        perror("malloc");
        destroy_shared_status(&shared_status);
        return 1;
    }

    //Webサーバ設定も config から反映
    web_arg->port = config.web_port;
    web_arg->shared_status = &shared_status;

    // 監視結果をブラウザで確認できるよう、Webサーバを別スレッドで起動する
    if (pthread_create(&web_thread, NULL, start_web_server, web_arg) != 0) {
        perror("pthread_create web server");
        free(web_arg);
        destroy_shared_status(&shared_status);
        return 1;
    }

    // ポートごとの過去状態を保持する配列を確保する
    PortHistory* histories = malloc(sizeof(PortHistory) * port_count);
    if (histories == NULL) {
        perror("malloc");
        destroy_shared_status(&shared_status);
        return 1;
    }

    // すべてのポート履歴を初期化し、初回監視時の比較に備える
    for (int i = 0; i < port_count; i++) {
        init_port_history(&histories[i]);
    }

    // 指定間隔ごとに全ポートを繰り返し監視する
    while(1){
        pthread_t threads[port_count];
        int idx = 0;

        // 監視対象ポートごとにスレッドを生成し、並列にスキャンを実行する
        for(port = target.start_port; port <= target.end_port; port++){
            ThreadArg* arg = malloc(sizeof(ThreadArg));
            if(arg == NULL) {
                perror("malloc");
                free(histories);
                destroy_shared_status(&shared_status);
                return 1;
            }

            // スレッドに必要な監視条件と共有データへの参照を設定する
            strncpy(arg->ip, target.ip, sizeof(arg->ip) - 1);
            arg->ip[sizeof(arg->ip) - 1] = '\0';
            arg->port = port;
            arg->index = idx;
            arg->shared_status = &shared_status;
            arg->history = &histories[idx];

            // 1ポート分の監視処理を独立スレッドとして開始する
            if (pthread_create(&threads[idx], NULL, thread_func, arg) != 0) {
                perror("pthread_create");
                free(arg);
                free(histories);
                destroy_shared_status(&shared_status);
                return 1;
            }

            idx++;
        }

        // すべての監視スレッドの終了を待ってから次の監視周期へ進む
        for(int i = 0; i < port_count; i++){
            pthread_join(threads[i], NULL);
        }

        // 次回の監視まで指定秒数だけ待機する
        sleep(looptime);
    }

    free(histories);
    destroy_shared_status(&shared_status);

    return 0;
}


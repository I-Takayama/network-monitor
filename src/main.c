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


typedef struct {
    char ip[64];
    int port;
    int index;
    SharedStatus* shared_status;
    PortHistory* history;
} ThreadArg;

void* thread_func(void* arg){
    ThreadArg* t = (ThreadArg*)arg;

    ScanResult scan_result;
    AnomalyResult anomaly_result;
    MonitorResult monitor_result;

    scan_port(t->ip, t->port, &scan_result);

    detect_anomaly(&scan_result, t->history, &anomaly_result);

    monitor_result.scan = scan_result;
    monitor_result.anomaly = anomaly_result;

    update_shared_status(t->shared_status, t->index, &monitor_result);

    print_result(&scan_result);
    printf("Anomaly: %s\n", anomaly_result.message);

    log_result(&monitor_result);

    free(t);  // メモリ解放
    return NULL;
}


int main(int argc, char *argv[]){ 
    int port;
    int looptime;
    int port_count;
    ScanTarget target;

    if (argc != 5) {
        printf("使い方: %s <IPアドレス> <開始ポート> <終了ポート> <ループ間隔>\n", argv[0]);
        return 1;
    }

    strncpy(target.ip, argv[1], sizeof(target.ip) - 1);
    target.ip[sizeof(target.ip) - 1] = '\0';
    target.start_port = atoi(argv[2]);
    target.end_port = atoi(argv[3]);

    looptime = atoi(argv[4]);
    port_count = (target.end_port - target.start_port + 1);

    if (target.start_port < 1 || target.end_port > 65535 || target.start_port > target.end_port) {
        printf("ポート番号の指定が不正です\n");
        return 1;
    }

    if (looptime <= 0) {
        printf("ループ間隔は1以上の整数で指定してください\n");
        return 1;
    }

    SharedStatus shared_status;
    init_shared_status(&shared_status, port_count);

    pthread_t web_thread;
    WebServerArg* web_arg = malloc(sizeof(WebServerArg));
    if (web_arg == NULL) {
        perror("malloc");
        destroy_shared_status(&shared_status);
        return 1;
    }

    web_arg->port = 8080;
    web_arg->shared_status = &shared_status;

    if (pthread_create(&web_thread, NULL, start_web_server, web_arg) != 0) {
        perror("pthread_create web server");
        free(web_arg);
        destroy_shared_status(&shared_status);
        return 1;
    }

    PortHistory* histories = malloc(sizeof(PortHistory) * port_count);
    if (histories == NULL) {
        perror("malloc");
        destroy_shared_status(&shared_status);
        return 1;
    }

    for (int i = 0; i < port_count; i++) {
        init_port_history(&histories[i]);
    }

    while(1){
        pthread_t threads[port_count];
        int idx = 0;

        for(port = target.start_port; port <= target.end_port; port++){
            ThreadArg* arg = malloc(sizeof(ThreadArg));
            if(arg == NULL) {
                perror("malloc");
                free(histories);
                destroy_shared_status(&shared_status);
                return 1;
            }

            strncpy(arg->ip, target.ip, sizeof(arg->ip) - 1);
            arg->ip[sizeof(arg->ip) - 1] = '\0';
            arg->port = port;
            arg->index = idx;
            arg->shared_status = &shared_status;
            arg->history = &histories[idx];

            if (pthread_create(&threads[idx], NULL, thread_func, arg) != 0) {
                perror("pthread_create");
                free(arg);
                free(histories);
                destroy_shared_status(&shared_status);
                return 1;
            }

            idx++;
        }

        for(int i = 0; i < port_count; i++){
            pthread_join(threads[i], NULL);
        }

        sleep(looptime);
    }

    free(histories);
    destroy_shared_status(&shared_status);

    return 0;
}


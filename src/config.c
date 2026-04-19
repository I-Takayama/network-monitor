#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "config.h"

/*
 * 設定ファイルを読み込み、Config構造体に値を格納する関数
 *
 * 【仕様】
 * - "key=value" 形式のテキストを1行ずつ解析
 * - 対応するキーに応じてConfig構造体に値をセット
 *
 * 【設計意図】
 * - 設定をコードから分離し、再ビルドなしで変更可能にする
 * - シンプルなフォーマットにすることで可読性と保守性を確保
 */
int load_config(const char* filename, Config* config) {

    // ファイルポインタ
    FILE* fp = fopen(filename, "r");

    // 1行読み込み用バッファ
    char line[256];

    // ファイルオープン失敗時
    if (fp == NULL) {
        perror("fopen");
        return -1;
    }

    // ファイルを1行ずつ読み込む
    while (fgets(line, sizeof(line), fp)) {

        // キーと値を格納するバッファ
        char key[128];
        char value[128];

        /*
         * "key=value" の形式を解析
         * %[^=]  : '=' までをキーとして取得
         * %s     : 値を取得
         */
        if (sscanf(line, "%127[^=]=%127s", key, value) != 2) {
            // フォーマット不正な行は無視
            continue;
        }

        // 各キーに応じてConfig構造体へ値を設定

        if (strcmp(key, "ip") == 0) {

            // 文字列コピー（バッファオーバーフロー対策）
            strncpy(config->ip, value, sizeof(config->ip) - 1);
            config->ip[sizeof(config->ip) - 1] = '\0';

        } else if (strcmp(key, "start_port") == 0) {

            // 開始ポート番号
            config->start_port = atoi(value);

        } else if (strcmp(key, "end_port") == 0) {

            // 終了ポート番号
            config->end_port = atoi(value);

        } else if (strcmp(key, "interval") == 0) {

             // 監視間隔（秒）
            config->interval = atoi(value);

        } else if (strcmp(key, "web_port") == 0) {

            // Webサーバのポート番号
            config->web_port = atoi(value);
        } 
    }

    fclose(fp);
    return 0;
}
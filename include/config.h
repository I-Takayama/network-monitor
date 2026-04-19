#ifndef CONFIG_H
#define CONFIG_H

/*
 * 監視設定を保持する構造体
 *
 * 【目的】
 * - 監視対象やポート範囲などの設定をまとめて管理する
 * - 設定ファイルから読み込んだ値を保持し、各モジュールで共有する
 *
 * 【設計意図】
 * - 設定をコードから分離することで、再ビルドなしで変更可能にする
 * - 保守性・運用性の向上を目的とする
 */
typedef struct {
    char ip[64];        // 監視対象のIPアドレス
    int start_port;     // 監視開始ポート
    int end_port;       // 監視終了ポート
    int interval;       // 監視間隔（秒）
    int web_port;       // Webサーバのポート番号
} Config;

/*
 * 設定ファイルを読み込み、Config構造体に値を格納する関数
 *
 * 【仕様】
 * - "key=value" 形式の設定ファイルを解析する
 * - 対応するキーに応じて構造体に値を設定する
 *
 * @param filename 設定ファイルのパス
 * @param config   設定を格納する構造体
 * @return 成功時 0 / 失敗時 -1
 */
int load_config(const char* filename, Config* config);

#endif
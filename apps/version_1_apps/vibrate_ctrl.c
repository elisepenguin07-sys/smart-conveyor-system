#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mosquitto.h>

/* 📍 HiveMQ Free 1 連線設定 (修正版) */
#define MQTT_HOST     "a7e49771b2094445aff9bf69987ee553.s1.eu.hivemq.cloud" // 網址不可帶 ssl:// 或 :8883
#define MQTT_PORT     8883                                                 // TCP TLS 專用 Port
#define MQTT_USER     "elise"                                              // HiveMQ Credentials 帳號
#define MQTT_PASS     "ee03101004"                                         // HiveMQ Credentials 密碼

/* 📍 Topic 與 Shell Script 設定 */
#define TOPIC_CMD     "vibration/motor_control/cmd"                        // 接收 HTML 指令的主題
#define TOPIC_STATUS  "vibration/motor_control/status"                     // 回報狀態給 HTML 的主題
#define SCRIPT_PATH "/home/pi/smart-conveyor-system/apps/L298N_vibrate.sh"                                // Shell 腳本路徑

/* 系統 CA 憑證路徑 (Raspberry Pi OS 預設) */
#define CA_CERT_PATH  "/etc/ssl/certs/ca-certificates.crt"

/* ================================================================= */

/* 連線成功 Callback */
void on_connect(struct mosquitto *mosq, void *obj, int rc) {
    if (rc == 0) {
        printf("✅ 成功連線至 HiveMQ Cloud！\n");
        // 訂閱來自 HTML 的控制指令
        mosquitto_subscribe(mosq, NULL, TOPIC_CMD, 0);
        printf("📡 已開始訂閱主題: %s\n", TOPIC_CMD);
    } else {
        printf("❌ 連線失敗，錯誤碼: %d\n", rc);
    }
}

/* 收到 MQTT 訊息 Callback -> 驅動 Shell Script */
void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg) {
    if (msg->payloadlen > 0) {
        char payload[64] = {0};
        strncpy(payload, (char *)msg->payload, msg->payloadlen);
        printf("\n📩 [HTML -> RPi] 收到控制指令: %s\n", payload);

        /* 1. 拼接 Shell 指令 (例如: ./control_motor.sh A) */
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "%s %s", SCRIPT_PATH, payload);

        printf("🚀 執行 Shell: %s\n", cmd);
        int ret = system(cmd);

        /* 2. 執行完成後，回報狀態給 HTML 網頁 */
        if (ret == 0) {
            char status_msg[128];
            snprintf(status_msg, sizeof(status_msg), "EXECUTED_%s", payload);
            mosquitto_publish(mosq, NULL, TOPIC_STATUS, strlen(status_msg), status_msg, 0, false);
            printf("📤 [RPi -> HTML] 已回報狀態: %s\n", status_msg);
        } else {
            perror("❌ Shell 執行失敗");
        }
    }
}

int main(int argc, char **argv) {
    struct mosquitto *mosq = NULL;

    printf("=== Smart Conveyor MQTT Listener ===\n");

    /* 1. 初始化 Mosquitto 函式庫 */
    mosquitto_lib_init();

    /* 2. 建立 Mosquitto 實例 */
    mosq = mosquitto_new("RPi5_Motor_Listener", true, NULL);
    if (!mosq) {
        fprintf(stderr, "❌ Error: 無法建立 Mosquitto 實例\n");
        return EXIT_FAILURE;
    }

    /* 3. 設定 HiveMQ 帳號密碼 */
    mosquitto_username_pw_set(mosq, MQTT_USER, MQTT_PASS);

    /* 4. 設定 TLS 加密 (HiveMQ 強制要求) */
    if (mosquitto_tls_set(mosq, CA_CERT_PATH, NULL, NULL, NULL, NULL) != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "❌ Error: TLS 設定失敗，請確認 %s 存在\n", CA_CERT_PATH);
        mosquitto_destroy(mosq);
        return EXIT_FAILURE;
    }

    /* 5. 註冊 Callback 函式 */
    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_message_callback_set(mosq, on_message);

    /* 6. 連線至 HiveMQ Broker */
    printf("🔌 正在安全連线至 HiveMQ (%s:%d)...\n", MQTT_HOST, MQTT_PORT);
    if (mosquitto_connect(mosq, MQTT_HOST, MQTT_PORT, 60) != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "❌ 無法連線至 HiveMQ Broker，請檢查網路或憑證/帳密設定！\n");
        mosquitto_destroy(mosq);
        return EXIT_FAILURE;
    }

    /* 7. 開始事件循環 (Loop Forever) */
    printf("🎧 系統進入背景監聽狀態 (按下 Ctrl+C 結束)...\n");
    mosquitto_loop_forever(mosq, -1, 1);

    /* 8. 清理資源 */
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();

    return EXIT_SUCCESS;
}
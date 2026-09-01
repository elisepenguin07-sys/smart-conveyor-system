#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <MQTTClient.h>

#define ADDRESS     "ssl://be899af830ab4926b516cded3f79cab4.s1.eu.hivemq.cloud:8883"
#define CLIENTID    "C_Publisher"
#define TOPIC       "/api/values"
#define QOS         0
#define TIMEOUT     10000L

int main(int argc, char *argv[]){
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_SSLOptions ssl_opts = MQTTClient_SSLOptions_initializer;
    int rc;

    // 1. 建立 Client
    char dynamic_client_id[64];
    snprintf(dynamic_client_id, sizeof(dynamic_client_id), "C_Publisher_%d_%d", getpid(), rand() % 10000);

    // 💡 建立 Client 時帶入 dynamic_client_id，不要用固定字串
    MQTTClient_create(&client, ADDRESS, dynamic_client_id, MQTTCLIENT_PERSISTENCE_NONE, NULL);

    // 2. 設定連線參數（帳號、密碼、SSL）
    conn_opts.keepAliveInterval = 60;
    conn_opts.cleansession = 1;
    conn_opts.username = "test";
    conn_opts.password = "ee03101004";
    ssl_opts.trustStore = "/etc/ssl/certs/ca-certificates.crt";
    ssl_opts.enableServerCertAuth = 1;
    conn_opts.ssl = &ssl_opts;

    rc = MQTTClient_connect(client, &conn_opts);
    if (rc != MQTTCLIENT_SUCCESS) {
        printf("❌ 連線失敗！MQTT 回傳的錯誤代碼是: %d\n", rc);
        
        // 根據 Paho MQTT 常見的錯誤代碼給予提示
        if (rc == 5) {
            printf("💡 提示代碼 5 通常代表：連線被拒絕（帳號、密碼錯誤，或者 SSL 憑證不受信任）。\n");
        } else if (rc == 3) {
            printf("💡 提示代碼 3 通常代表：伺服器連線逾時或位址錯誤。\n");
        }
        
        exit(-1);
    }
    printf("✅ [MQTT Client] 連線成功！\n");
    
    int a = 1;
    int b = 101;
    int c = 201;
    char message[100];

    while(1){
        // 1. 每次迴圈都重新建立連線 (確保每次都是全新的 TLS 通道)
        if (MQTTClient_connect(client, &conn_opts) == MQTTCLIENT_SUCCESS) {
            
            sprintf(message, "{\"variable_a\": %d, \"variable_b\": %d, \"variable_c\": %d}", a, b, c);
            
            MQTTClient_message pubmsg = MQTTClient_message_initializer;
            pubmsg.payload = message;
            pubmsg.payloadlen = (int)strlen(message);
            pubmsg.qos = QOS;
            pubmsg.retained = 0;
            
            MQTTClient_deliveryToken token;
            
            // 2. 發送
            int rc = MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);
            if (rc == MQTTCLIENT_SUCCESS) {
                printf("📤 成功發送: %s\n", message);
                MQTTClient_waitForCompletion(client, token, 1000L); // 確保發送完成
            } else {
                printf("❌ 發送失敗，代碼: %d\n", rc);
            }
            
            // 3. 送完立刻斷線，不留給伺服器踢妳的機會
            MQTTClient_disconnect(client, 1000L);
        } else {
            printf("❌ 連線失敗，重試中...\n");
        }

        usleep(3000000); // 停 3 秒，這中間雖然斷線，但網路通道是清空的，雲端絕對不會因為心跳包不夠而踢妳
        a++; b++; c++;
    }

    // 5. 斷線與釋放資源
    MQTTClient_disconnect(client, TIMEOUT);
    MQTTClient_destroy(&client);
    return 0;
}
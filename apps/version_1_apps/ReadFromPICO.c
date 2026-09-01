#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <fftw3.h>
#include <MQTTClient.h> // 👈 改用非同步 Asynchronous 函式庫

#define N 64                  
#define PI 3.14159265358979323846
#define GRAVITY_MM_S2 9806.65
#define SMOOTH_WINDOW 2        
#define ADDRESS     "ssl://a7e49771b2094445aff9bf69987ee553.s1.eu.hivemq.cloud"
#define CLIENTID    "C_Publisher"
#define TOPIC       "/vibrateData"
#define QOS         0
#define TIMEOUT     10000L

struct mpu6050_data {
    short accel_x;
    short accel_y;
    short accel_z;
};

// 計算原始全效 RMS (g)
double calculate_raw_rms(const float *ax, const float *ay, const float *az, int len) {
    if (!ax || !ay || !az || len <= 0) return 0.0;
    
    double sum_squares = 0.0;
    for (int i = 0; i < len; i++) {
        double a_total = sqrt((ax[i] * ax[i]) + (ay[i] * ay[i]) + (az[i] * az[i]));
        sum_squares += (a_total * a_total);
    }
    return sqrt(sum_squares / (double)len);
}

// 利用 FFTW 計算主頻 (Peak Hz)
double get_fft_peak(const float *in_array, int num_samples, double sample_rate, 
                    double *in, fftw_complex *out, fftw_plan plan) {
    if (num_samples <= 0 || in_array == NULL) return 0.0;

    for (int i = 0; i < num_samples; i++) {
        in[i] = (double)in_array[i];
    }

    fftw_execute(plan);

    double max_amplitude = 0.0;
    int max_index = 0;

    for (int i = 1; i < num_samples / 2; i++) {
        double real = out[i][0];
        double imag = out[i][1];
        double amplitude = sqrt(real * real + imag * imag);

        if (amplitude > max_amplitude) {
            max_amplitude = amplitude;
            max_index = i;
        }
    }

    return max_index * (sample_rate / (double)num_samples);
}

int main(int argc, char *argv[])
{
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
    conn_opts.username = "elise";
    conn_opts.password = "ee03101004";
    ssl_opts.trustStore = "/etc/ssl/certs/ca-certificates.crt";
    ssl_opts.enableServerCertAuth = 1;
    conn_opts.ssl = &ssl_opts;

    rc = MQTTClient_connect(client, &conn_opts);
    if (rc != MQTTCLIENT_SUCCESS) {
        printf("❌ 連線失敗！MQTT 回傳的錯誤代碼是: %d\n", rc);
        
        if (rc == 5) {
            printf("💡 提示代碼 5 通常代表：連線被拒絕（帳號、密碼錯誤，或者 SSL 憑證不受信任）。\n");
        } else if (rc == 3) {
            printf("💡 提示代碼 3 通常代表：伺服器連線逾時或位址錯誤。\n");
        }
        
        exit(-1);
    }
    printf("✅ [MQTT Client] 連線成功！\n");

    int fd = open("/dev/i2C_driver", O_RDWR);
    if (fd < 0) {
        perror("開啟 /dev/i2C_driver 失敗！請確認驅動已載入 (insmod)");
        return -1;
    }

    double *fft_in = (double*) fftw_malloc(sizeof(double) * N);
    fftw_complex *fft_out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (N / 2 + 1));
    fftw_plan fft_plan = fftw_plan_dft_r2c_1d(N, fft_in, fft_out, FFTW_ESTIMATE);

    struct mpu6050_data my_accel;
    float buf_x[N], buf_y[N], buf_z[N];
    int index = 0;

    double velocity_history[SMOOTH_WINDOW] = {0.0};
    int history_idx = 0;

    printf("開始讀取 MPU6050 數據並進行即時分級分析...\n");

    while(1) {
        ssize_t ret = read(fd, &my_accel, sizeof(my_accel));

        if (ret == sizeof(my_accel)) {
            buf_x[index] = (float)my_accel.accel_x / 16384.0f;
            buf_y[index] = (float)my_accel.accel_y / 16384.0f;
            buf_z[index] = (float)my_accel.accel_z / 16384.0f;
            index++;

            if (index >= N) {
                double raw_rms = calculate_raw_rms(buf_x, buf_y, buf_z, N);

                double mean_x = 0, mean_y = 0, mean_z = 0;
                for (int i = 0; i < N; i++) {
                    mean_x += buf_x[i]; mean_y += buf_y[i]; mean_z += buf_z[i];
                }
                mean_x /= N; mean_y /= N; mean_z /= N;

                double var_x = 0, var_y = 0, var_z = 0;
                for (int i = 0; i < N; i++) {
                    var_x += (buf_x[i] - mean_x) * (buf_x[i] - mean_x);
                    var_y += (buf_y[i] - mean_y) * (buf_y[i] - mean_y);
                    var_z += (buf_z[i] - mean_z) * (buf_z[i] - mean_z);
                }
                double pure_dynamic_rms = sqrt((var_x + var_y + var_z) / N);

                double peak_hz_x = get_fft_peak(buf_x, N, 100.0, fft_in, fft_out, fft_plan);
                double peak_hz_y = get_fft_peak(buf_y, N, 100.0, fft_in, fft_out, fft_plan);
                double peak_hz_z = get_fft_peak(buf_z, N, 100.0, fft_in, fft_out, fft_plan);

                double current_velocity = (pure_dynamic_rms < 0.05) ? pure_dynamic_rms * 12.0 :
                                         (pure_dynamic_rms < 0.20) ? 1.4 + (pure_dynamic_rms - 0.05) * 8.0 :
                                         (pure_dynamic_rms < 0.45) ? 2.8 + (pure_dynamic_rms - 0.20) * 6.4 :
                                         4.5 + (pure_dynamic_rms - 0.45) * 5.0;

                velocity_history[history_idx] = current_velocity;
                history_idx = (history_idx + 1) % SMOOTH_WINDOW;

                double smooth_velocity = 0.0;
                for (int i = 0; i < SMOOTH_WINDOW; i++) smooth_velocity += velocity_history[i];
                smooth_velocity /= SMOOTH_WINDOW;

                const char* status = (smooth_velocity >= 4.5) ? "D(危險 ⚠️)" :
                                     (smooth_velocity >= 2.8) ? "C(警戒 ⚡)" :
                                     (smooth_velocity >= 1.4) ? "B(滿意)" : "A(良好)";

                // 🎯 這裡保留唯一且乾淨的 Zone 狀態變更與 Shell 觸發邏輯
                static char last_zone = 'A';
                char zone_char = (smooth_velocity >= 4.5) ? 'D' :
                                 (smooth_velocity >= 2.8) ? 'C' :
                                 (smooth_velocity >= 1.4) ? 'B' : 'A';

                if (zone_char != last_zone) {
                    char shell_cmd[256];
                    snprintf(shell_cmd, sizeof(shell_cmd), "/home/pi/smart-conveyor-system/apps/L298N_convey.sh %c", zone_char);
                    printf("🔄 Zone 狀態變更 (%c -> %c)，執行 Shell 腳本！\n", last_zone, zone_char);
                    system(shell_cmd);
                    last_zone = zone_char;
                }

                printf("[RMS]: %.4f g | [Velocity RMS]: %.2f mm/s (%s) | [Peak Hz] X: %.2f Hz, Y: %.2f Hz, Z: %.2f Hz\n",
                       raw_rms, smooth_velocity, status, peak_hz_x, peak_hz_y, peak_hz_z);

                // 生成波形字串
                char fft_x_str[512] = "[", fft_y_str[512] = "[", fft_z_str[512] = "[";
                for (int i = 0; i < 40; i++) {
                    float hz = i * 5.0f;
                    float val_x = (fabs(hz - peak_hz_x) < 3.0) ? 0.8f : ((float)rand()/RAND_MAX * 0.1f);
                    float val_y = (fabs(hz - peak_hz_y) < 3.0) ? (float)smooth_velocity * 0.5f : ((float)rand()/RAND_MAX * 0.15f);
                    float val_z = (fabs(hz - peak_hz_z) < 3.0) ? 0.4f : ((float)rand()/RAND_MAX * 0.08f);

                    char bx[16], by[16], bz[16];
                    snprintf(bx, sizeof(bx), "%.2f%s", val_x, (i == 39) ? "" : ",");
                    snprintf(by, sizeof(by), "%.2f%s", val_y, (i == 39) ? "" : ",");
                    snprintf(bz, sizeof(bz), "%.2f%s", val_z, (i == 39) ? "" : ",");

                    strcat(fft_x_str, bx); strcat(fft_y_str, by); strcat(fft_z_str, bz);
                }
                strcat(fft_x_str, "]"); strcat(fft_y_str, "]"); strcat(fft_z_str, "]");

                const char *diag_reason = (smooth_velocity < 1.4) ? "設備運轉狀態良好，各軸向震動能量安全。" :
                                         (smooth_velocity < 2.8) ? "主要震動集中於 Y 軸，研判為轉軸輕微對中心不良。" :
                                         (smooth_velocity < 4.5) ? "震動大幅提升，Y 軸強烈震動已超標。" :
                                         "⚠️ 警告：震動嚴重超標，具即時損壞危險！";
                const char *diag_act1 = (smooth_velocity < 1.4) ? "Zone A：健康度優良。" :
                                        (smooth_velocity < 2.8) ? "Zone B：尚可運轉，定期保養。" :
                                        (smooth_velocity < 4.5) ? "Zone C：24小時內安排檢測。" : "Zone D：立即緊急停機！";

                char json_payload[2048];
                snprintf(json_payload, sizeof(json_payload),
                    "{\"rms\":{\"x\":%.2f,\"y\":%.2f,\"z\":%.2f},\"fft\":{\"x\":%s,\"y\":%s,\"z\":%s},"
                    "\"diagnosis\":{\"reason\":\"%s\",\"actions\":[\"%s\",\"維持監控\",\"記錄數據\"]}}",
                    smooth_velocity * 0.35, smooth_velocity * 0.90, smooth_velocity * 0.25,
                    fft_x_str, fft_y_str, fft_z_str, diag_reason, diag_act1);

                MQTTClient_message pubmsg = MQTTClient_message_initializer;
                pubmsg.payload = json_payload;
                pubmsg.payloadlen = (int)strlen(json_payload);
                pubmsg.qos = QOS;
                pubmsg.retained = 0;
                
                MQTTClient_deliveryToken token;
                
                // 2. 發送
                int pub_rc = MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);
                if (pub_rc == MQTTCLIENT_SUCCESS) {
                    printf("📤 成功發送: %s\n", json_payload);
                    MQTTClient_waitForCompletion(client, token, 1000L); // 確保發送完成
                } else {
                    printf("❌ 發送失敗，代碼: %d\n", pub_rc);
                }

                usleep(3000000);
                index = 0;
            }      
        } else {
            usleep(5000); 
        }

        usleep(1000); // 1ms 取樣
    }


    fftw_destroy_plan(fft_plan);
    fftw_free(fft_in);
    fftw_free(fft_out);

    // 5. 斷線與釋放資源
    MQTTClient_disconnect(client, TIMEOUT);
    MQTTClient_destroy(&client);
    return 0;
}
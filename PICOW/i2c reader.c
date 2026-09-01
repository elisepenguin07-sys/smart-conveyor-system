#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// 定義使用的 I2C 硬體通道與接腳 (可依據你的硬體調整)
#define I2C_PORT i2c_default       // 使用預設的 i2c0
#define SDA_PIN  0                 // Pico GP4 (Pin 6) 接 MPU6050 SDA
#define SCL_PIN  1                 // Pico GP5 (Pin 7) 接 MPU6050 SCL

// MPU-6050 暫存器定義
#define MPU_ADDR        0x68       // MPU-6050 預設 I2C 地址
#define REG_PWR_MGMT_1  0x6B       // 電源管理暫存器 (用來喚醒晶片)
#define REG_WHO_AM_I    0x75       // 晶片識別暫存器 (預設應回傳 0x68)
#define REG_ACCEL_XOUT_H 0x3B      // 加速度計 X 軸高位元組 (連續資料的起點)

/**
 * 單位元組寫入 (Single-Byte Write) - 對應第一張圖
 */
void mpu6050_write_reg(uint8_t reg, uint8_t data) {
    uint8_t buf[2] = {reg, data};
    // nostop = false: 寫入完成後正常發送結束訊號 (P)
    i2c_write_blocking(I2C_PORT, MPU_ADDR, buf, 2, false);
}

/**
 * 連續讀取 (Burst Read) - 對應第二張圖
 * 用來一次讀取多個位元組，內部地址會自動加一
 */
void mpu6050_read_burst(uint8_t start_reg, uint8_t *buffer, size_t len) {
    // 步驟 A & B：指定要讀的暫存器地址，並啟用 nostop=true 觸發重複開始訊號 (S)
    i2c_write_blocking(I2C_PORT, MPU_ADDR, &start_reg, 1, true);
    
    // 步驟 C：連續讀取 len 個位元組，讀完後 nostop=false 發送結束訊號 (P)
    // Pico SDK 內部會自動處理最後一個位元組的 NACK 訊號
    i2c_read_blocking(I2C_PORT, MPU_ADDR, buffer, len, false);
}

int main() {
    stdio_init_all();

    // 初始化 I2C，速度設定為標準 400 kHz
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    // 延時等待感測器供電穩定
    sleep_ms(100);

    // 1. 驗證通訊：讀取 WHO_AM_I 暫存器 (單位元組讀取)
    uint8_t who_am_i = 0;
    mpu6050_read_burst(REG_WHO_AM_I, &who_am_i, 1);
    printf("MPU-6050 WHO_AM_I: 0x%02X (預設應為 0x68)\n", who_am_i);

    // 2. 喚醒晶片：寫入 0x00 到電源管理暫存器 (解除睡眠模式)
    mpu6050_write_reg(REG_PWR_MGMT_1, 0x00);
    sleep_ms(10); // 等待穩定的暫態時間

    // 3. 循環讀取 6 個位元組的加速度資料 (X、Y、Z 軸的高低位元組)
    uint8_t data_buf[6];
    while (true) {
        // 利用 Burst Read 從 0x3B 開始一口氣讀取 6 個暫存器
        mpu6050_read_burst(REG_ACCEL_XOUT_H, data_buf, 6);

        // 將每兩個 8 位元資料組合成一個 16 位元的有號整數
        int16_t accel_x = (data_buf[0] << 8) | data_buf[1];
        int16_t accel_y = (data_buf[2] << 8) | data_buf[3];
        int16_t accel_z = (data_buf[4] << 8) | data_buf[5];

        // 輸出原始數據
        printf("Accel X: %d, Y: %d, Z: %d\n", accel_x, accel_y, accel_z);

        sleep_ms(500);
    }
}
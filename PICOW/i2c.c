#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/i2c_slave.h"

// ---------------------------
// 1. 樹莓派端設定 (I2C1 Slave)
// ---------------------------
#define PICO_I2C_SLAVE_ADDR 0x42
#define RPI_I2C_PORT        i2c1
#define RPI_SDA_PIN         14   // GP14 (Pin 19)
#define RPI_SCL_PIN         15   // GP15 (Pin 20)

// ---------------------------
// 2. MPU-6050 設定 (I2C0 Master)
// ---------------------------
#define MPU_I2C_PORT        i2c0
#define MPU_SDA_PIN         0    // GP4 (Pin 6)
#define MPU_SCL_PIN         1    // GP5 (Pin 7)
#define MPU_ADDR            0x68
#define REG_PWR_MGMT_1      0x6B
#define REG_WHO_AM_I        0x75
#define REG_ACCEL_XOUT_H    0x3B

// 共享緩衝區：準備給樹莓派讀取的 6 個 Bytes (X_H, X_L, Y_H, Y_L, Z_H, Z_L)
static volatile uint8_t shared_buffer[6] = {0};
static volatile size_t tx_index = 0;

// 樹莓派發起讀取時的硬體中斷處理
static void i2c_slave_handler(i2c_inst_t *i2c, i2c_slave_event_t event) {
    switch (event) {
        case I2C_SLAVE_RECEIVE:
            (void)i2c_read_byte_raw(i2c);
            break;
        case I2C_SLAVE_REQUEST:
            i2c_write_byte_raw(i2c, shared_buffer[tx_index]);
            tx_index = (tx_index + 1) % 6;
            break;
        case I2C_SLAVE_FINISH:
            tx_index = 0; // 樹莓派讀完發送 Stop 後，指標自動歸零
            break;
        default:
            break;
    }
}

// 寫入 MPU6050 暫存器
void mpu6050_write_reg(uint8_t reg, uint8_t data) {
    uint8_t buf[2] = {reg, data};
    i2c_write_blocking(MPU_I2C_PORT, MPU_ADDR, buf, 2, false);
}

// 連續讀取 MPU6050
void mpu6050_read_burst(uint8_t start_reg, uint8_t *buffer, size_t len) {
    i2c_write_blocking(MPU_I2C_PORT, MPU_ADDR, &start_reg, 1, true);
    i2c_read_blocking(MPU_I2C_PORT, MPU_ADDR, buffer, len, false);
}

int main() {
    stdio_init_all();
    sleep_ms(1000);

    // ------------------------------------
    // 初始化 I2C1 (Slave - 接樹莓派)
    // ------------------------------------
    gpio_init(RPI_SDA_PIN);
    gpio_set_function(RPI_SDA_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(RPI_SDA_PIN);

    gpio_init(RPI_SCL_PIN);
    gpio_set_function(RPI_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(RPI_SCL_PIN);

    i2c_init(RPI_I2C_PORT, 100 * 1000);
    i2c_slave_init(RPI_I2C_PORT, PICO_I2C_SLAVE_ADDR, &i2c_slave_handler);

    // ------------------------------------
    // 初始化 I2C0 (Master - 接 MPU6050)
    // ------------------------------------
    i2c_init(MPU_I2C_PORT, 400 * 1000);
    gpio_set_function(MPU_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(MPU_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(MPU_SDA_PIN);
    gpio_pull_up(MPU_SCL_PIN);

    // 喚醒 MPU6050
    mpu6050_write_reg(REG_PWR_MGMT_1, 0x00);
    sleep_ms(50);

    printf("=== Pico Dual-I2C 系統就緒：Slave 位址 0x42 ===\n");

    uint8_t data_buf[6];
    while (true) {
        // 從 MPU6050 讀取最新 6 軸加速度
        mpu6050_read_burst(REG_ACCEL_XOUT_H, data_buf, 6);

        // 即時填入給樹莓派讀取的 shared_buffer
        shared_buffer[0] = data_buf[0];
        shared_buffer[1] = data_buf[1];
        shared_buffer[2] = data_buf[2];
        shared_buffer[3] = data_buf[3];
        shared_buffer[4] = data_buf[4];
        shared_buffer[5] = data_buf[5];

        // 轉換數值並在 Pico 本地印出
        int16_t accel_x = (int16_t)((data_buf[0] << 8) | data_buf[1]);
        int16_t accel_y = (int16_t)((data_buf[2] << 8) | data_buf[3]);
        int16_t accel_z = (int16_t)((data_buf[4] << 8) | data_buf[5]);
        printf("Pico Local -> Accel X: %6d, Y: %6d, Z: %6d\n", accel_x, accel_y, accel_z);

        sleep_ms(20); // 50Hz 更新率，提供樹莓派即時動態數據
    }

    return 0;
}
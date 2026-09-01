/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "pico/i2c_slave.h"

#define PICO_I2C_SLAVE_ADDR 0x42
#define I2C1_SDA_PIN 14
#define I2C1_SCL_PIN 15

static int mpu_addr = 0x68;
static uint8_t shared_buffer[6] = {0}; // 存放準備給樹莓派讀取的 6 bytes
static size_t tx_index = 0;

// 當樹莓派來要資料時，硬體中斷自動觸發這個 Handler
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
            tx_index = 0; // 傳輸結束強制歸零
            break;
        default:
            break;
    }
}

/* Example code to talk to a MPU6050 MEMS accelerometer and gyroscope

   This is taking to simple approach of simply reading registers. It's perfectly
   possible to link up an interrupt line and set things up to read from the
   inbuilt FIFO to make it more useful.

   NOTE: Ensure the device is capable of being driven at 3.3v NOT 5v. The Pico
   GPIO (and therefore I2C) cannot be used at 5v.

   You will need to use a level shifter on the I2C lines if you want to run the
   board at 5v.

   Connections on Raspberry Pi Pico board, other boards may vary.

   GPIO PICO_DEFAULT_I2C_SDA_PIN (On Pico this is GP4 (pin 6)) -> SDA on MPU6050 board
   GPIO PICO_DEFAULT_I2C_SCL_PIN (On Pico this is GP5 (pin 7)) -> SCL on MPU6050 board
   3.3v (pin 36) -> VCC on MPU6050 board
   GND (pin 38)  -> GND on MPU6050 board
*/

// By default these devices  are on bus address 0x68


#ifdef i2c_default

static void mpu6050_init() {
    uint8_t buf[2];

    // 1. 喚醒 MPU6050 並選擇 PLL 時脈源 (寫入 0x01 至 0x6B)
    buf[0] = 0x6B;
    buf[1] = 0x01;
    i2c_write_blocking(i2c_default, mpu_addr, buf, 2, false);
    sleep_ms(50);

    // 2. 設定加速度量程為 +/- 2g
    buf[0] = 0x1C;
    buf[1] = 0x00;
    i2c_write_blocking(i2c_default, mpu_addr, buf, 2, false);
    sleep_ms(10);
}

// static void mpu6050_reset() {
//     // Two byte reset. First byte register, second byte data
//     // There are a load more options to set up the device in different ways that could be added here
//     uint8_t buf[] = {0x6B, 0x80};
//     i2c_write_blocking(i2c_default, addr, buf, 2, false);
//     sleep_ms(100); // Allow device to reset and stabilize

//     // Clear sleep mode (0x6B register, 0x00 value)
//     buf[1] = 0x00;  // Clear sleep mode by writing 0x00 to the 0x6B register
//     i2c_write_blocking(i2c_default, addr, buf, 2, false); 
//     sleep_ms(10); // Allow stabilization after waking up
// }

static void mpu6050_read_raw(int16_t accel[3], int16_t gyro[3], int16_t *temp) {
    uint8_t val = 0x3B;
    uint8_t buffer[14];

    // 從 0x3B 起始位置一口氣連續讀出 14 個 Bytes
    i2c_write_blocking(i2c_default, mpu_addr, &val, 1, true);
    i2c_read_blocking(i2c_default, mpu_addr, buffer, 14, false);

    // Accel X, Y, Z
    accel[0] = (int16_t)((buffer[0] << 8) | buffer[1]);
    accel[1] = (int16_t)((buffer[2] << 8) | buffer[3]);
    accel[2] = (int16_t)((buffer[4] << 8) | buffer[5]);

    // Temp
    *temp    = (int16_t)((buffer[6] << 8) | buffer[7]);

    // Gyro X, Y, Z
    gyro[0]  = (int16_t)((buffer[8] << 8) | buffer[9]);
    gyro[1]  = (int16_t)((buffer[10] << 8) | buffer[11]);
    gyro[2]  = (int16_t)((buffer[12] << 8) | buffer[13]);
}
#endif

int main() {
    stdio_init_all();
#if !defined(i2c_default) || !defined(PICO_DEFAULT_I2C_SDA_PIN) || !defined(PICO_DEFAULT_I2C_SCL_PIN)
    #warning i2c/mpu6050_i2c example requires a board with I2C pins
    puts("Default I2C pins were not defined");
    return 0;
#else
    printf("Hello, MPU6050! Reading raw data from registers...\n");

    sleep_ms(2000);
    gpio_init(I2C1_SDA_PIN);
    gpio_set_function(I2C1_SDA_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C1_SDA_PIN);

    gpio_init(I2C1_SCL_PIN);
    gpio_set_function(I2C1_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C1_SCL_PIN);

    i2c_init(i2c1, 100 * 1000);
    i2c_slave_init(i2c1, PICO_I2C_SLAVE_ADDR, &i2c_slave_handler); // 註冊為 Slave
    
    // This example will use I2C0 on the default SDA and SCL pins (4, 5 on a Pico)
    i2c_init(i2c_default, 400 * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
    // Make the I2C pins available to picotool
    bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));

    mpu6050_init();

    int16_t acceleration[3], gyro[3], temp;
    
    while (1) {
        mpu6050_read_raw(acceleration, gyro, &temp);

        // These are the raw numbers from the chip, so will need tweaking to be really useful.
        // See the datasheet for more information
        printf("Acc. X = %d, Y = %d, Z = %d\n", acceleration[0], acceleration[1], acceleration[2]);
        printf("Gyro. X = %d, Y = %d, Z = %d\n", gyro[0], gyro[1], gyro[2]);
        // Temperature is simple so use the datasheet calculation to get deg C.
        // Note this is chip temperature.
        printf("Temp. = %f\n", (temp / 340.0) + 36.53);

        shared_buffer[0] = (uint8_t)(acceleration[0] >> 8);
        shared_buffer[1] = (uint8_t)(acceleration[0] & 0xFF);
        shared_buffer[2] = (uint8_t)(acceleration[1] >> 8);
        shared_buffer[3] = (uint8_t)(acceleration[1] & 0xFF);
        shared_buffer[4] = (uint8_t)(acceleration[2] >> 8);
        shared_buffer[5] = (uint8_t)(acceleration[2] & 0xFF);

        sleep_ms(10);
        
    }
#endif
}


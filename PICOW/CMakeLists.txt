cmake_minimum_required(VERSION 3.13)

# 1. 指定板子為 Pico W
set(PICO_BOARD pico_w)

# 2. 如果系統環境變數沒有 PICO_SDK_PATH，指定樹莓派上的絕對路徑 (嘗試預設路徑)
if (NOT DEFINED ENV{PICO_SDK_PATH})
    set(ENV{PICO_SDK_PATH} "/home/pi/pico/pico-sdk")
endif()

# 如果上面的路徑不存在，可以試試 /home/pi/pico-sdk
if (NOT EXISTS $ENV{PICO_SDK_PATH})
    set(ENV{PICO_SDK_PATH} "/home/pi/pico-sdk")
endif()

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(pico_mpu6050 C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(mpu6050_reader i2c.c)

target_link_libraries(mpu6050_reader 
    pico_stdlib 
    hardware_i2c
    pico_i2c_slave
)

# 強制開啟 USB Serial，關閉 UART
pico_enable_stdio_usb(mpu6050_reader 1)
pico_enable_stdio_uart(mpu6050_reader 0)

pico_add_extra_outputs(mpu6050_reader)
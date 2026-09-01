# Cross-compiler 設定
CROSS_COMPILE = aarch64-linux-gnu-
CC = $(CROSS_COMPILE)gcc

# 編譯旗標
CFLAGS = -I../include -Wall
COMMON_LIBS = -lm -lfftw3

# 自動抓取所有 .c 檔案
SRCS = $(wildcard *.c)
TARGETS = $(patsubst %.c, %, $(SRCS))

all: $(TARGETS)

# 🎯 針對包含 PICO 的檔案使用 Paho MQTT
%PICO: %PICO.c
	$(CC) $(CFLAGS) $< $(COMMON_LIBS) -lpaho-mqtt3cs -o $@

# 🎯 針對其他所有檔案（如 vibrate_ctrl）使用 Mosquitto
%: %.c
	$(CC) $(CFLAGS) $< $(COMMON_LIBS) -lmosquitto -o $@

clean:
	rm -f $(TARGETS)

.PHONY: all clean
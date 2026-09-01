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

# 🎯 針對一般應用程式（如 spi_app.c）的編譯規則
%: %.c
	$(CC) $(CFLAGS) $< $(COMMON_LIBS) -o $@

clean:
	rm -f $(TARGETS)

.PHONY: all clean
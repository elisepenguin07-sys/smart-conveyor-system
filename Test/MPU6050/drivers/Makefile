# 1. 產生的模組目標命名為 etx_spi_driver.ko
obj-m += etx_spi_driver.o

# 2. 把實際的兩個 C 檔物件合併進去
etx_spi_driver-objs := spi_driver.o ssd1306.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

# 包含上一層 include 目錄
EXTRA_CFLAGS += -I$(src)/../include

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

.PHONY: all clean
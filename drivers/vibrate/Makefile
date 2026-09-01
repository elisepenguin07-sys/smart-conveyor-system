# 1. 產生的模組目標命名為 etx_spi_driver.ko
obj-m += vibrate_driver.o


KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

# 包含上一層 include 目錄
EXTRA_CFLAGS += -I$(src)/../include

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

.PHONY: all clean
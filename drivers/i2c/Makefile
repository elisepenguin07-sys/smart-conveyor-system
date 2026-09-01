# CROSS_COMPILE = aarch64-linux-gnu-
CC = $(CROSS_COMPILE)gcc

# 📍 確定對應你的 C 檔名 (如果是 L298N_ioctl.c 就寫 L298N_ioctl.o)
#obj-m += L298N_ioctl.o
obj-m += i2c_ioctl.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

# 包含上一層 include 目錄
EXTRA_CFLAGS += -I$(src)/../include

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

.PHONY: all clean
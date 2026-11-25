MODULE  := phfs_mod
obj-m   += $(MODULE).o
$(MODULE)-objs := src/phfs.o src/offsets.o src/channel.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

ccflags-y += -I$(PWD)/utils

CC     ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -O2

.PHONY: all module load unload clean

all: module

module:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

load: module
	@if [ "$$(id -u)" != "0" ]; then \
		echo "load needs root";		     \
		echo "    sudo make load";	   \
		exit 1;						             \
	fi
	@insmod $(MODULE).ko

unload:
	@if ! lsmod | awk '{print $$1}' | grep -qx '$(MODULE)'; then \
		echo "$(MODULE) it wasn't loaded";						             \
	elif [ "$$(id -u)" != "0" ]; then							               \
		echo "$(MODULE) need to be unloaded";					             \
		echo "       sudo make unload";							               \
		exit 1;													                           \
	else														                             \
		rmmod $(MODULE);									                      	 \
	fi

clean: unload
	@$(MAKE) -C $(KDIR) M=$(PWD) clean
	@rm -f $(UNIT)

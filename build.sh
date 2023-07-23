#!/bin/bash

make ice_evb_c910_defconfig
make ARCH=riscv CROSS_COMPILE=riscv64-unknown-linux-gnu- -j8

# Output u-boot-with-spl.bin 

### 刷u-boot
## u-boot
# thead cct -u /dev/ttyUSB1 download -f u-boot-with-spl_ice-rvb.bin -d emmc0
#
## Linux
# echo 0 > /sys/block/mmcblk0boot0/force_ro
# dd if=u-boot-with-spl.bin of=/dev/mmcblk0boot0

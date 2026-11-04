# Embedded Linux - Lab 2

**Student:** Le Tan Minh  
**Student ID:** SE200684  
**Course:** Embedded Linux

## Project Structure

- `driver/`: Linux kernel module source code and Makefile.
- `configs/`: Kernel configuration.
- `logs/`: Build, boot, driver and MTD test logs.
- `output/`: Generated initramfs image.
- `rootfs/`: Root filesystem and kernel module.
- `mtd/`: JFFS2 filesystem and persistence test data.
- `bao_cao/`: Lab 2 report in PDF format.

## Main Components

- Linux character device driver.
- Device file and sysfs interface.
- MTD NAND simulation.
- JFFS2 filesystem persistence test.

## Build Environment

- Linux Kernel: 5.15.0
- Architecture: ARM
- Emulator: QEMU

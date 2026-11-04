# Embedded Linux Lab 02

## Student Information
- Full name: Le Tan Minh
- Student ID: SE200684
- Assignment: Embedded Linux Lab 02

## Directory Structure
- driver/: Driver source code, Makefile and compiled kernel module.
- rootfs/: Initramfs filesystem and driver module.
- output/: Built initramfs archive.
- logs/: Boot, driver, MTD/JFFS2 and build logs.
- mtd/: JFFS2 image and source test files.
- bao_cao/: Lab report in PDF format.

## Main Artifacts
- driver/lab2_driver.c
- driver/Makefile
- driver/lab2_driver.ko
- output/initramfs_lab2.cpio.gz
- bao_cao/SE200684_Lab02_BaoCao.pdf

## Verification
Check the initramfs archive:
    gzip -t output/initramfs_lab2.cpio.gz

Check driver module hashes:
    sha256sum driver/lab2_driver.ko rootfs/lab2_driver.ko

Inspect the initramfs contents:
    gzip -dc output/initramfs_lab2.cpio.gz | cpio -it

Review the logs and PDF report before submission.

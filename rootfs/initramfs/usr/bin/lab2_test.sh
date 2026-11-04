#!/bin/sh

LOG=/tmp/lab2_test.log
DEVICE=/dev/lab2
SYSFS=/sys/class/lab2_class/lab2

exec > "$LOG" 2>&1

echo "======================================"
echo " LAB-02 AUTOMATED DRIVER TEST"
echo "======================================"

echo
echo "[TEST 1] Check character device"
if [ -c "$DEVICE" ]; then
    echo "PASS: $DEVICE exists"
else
    echo "FAIL: $DEVICE does not exist"
    exit 1
fi

echo
echo "[TEST 2] Write data to character device"
TEST_DATA="Hello LAB2 Part D"
echo -n "$TEST_DATA" > "$DEVICE"
if [ $? -eq 0 ]; then
    echo "PASS: write succeeded"
else
    echo "FAIL: write failed"
    exit 1
fi

echo
echo "[TEST 3] Read data from character device"
READ_DATA=$(cat "$DEVICE")
echo "Read data: $READ_DATA"
if [ "$READ_DATA" = "$TEST_DATA" ]; then
    echo "PASS: read data matches"
else
    echo "FAIL: read data does not match"
    exit 1
fi

echo
echo "[TEST 4] Check procfs"
if [ -r /proc/lab2_info ]; then
    cat /proc/lab2_info
    echo "PASS: procfs is readable"
else
    echo "FAIL: /proc/lab2_info is unavailable"
    exit 1
fi

echo
echo "[TEST 5] Check sysfs attributes"
for ATTR in buffer_len open_count last_data; do
    if [ -r "$SYSFS/$ATTR" ]; then
        echo "$ATTR:"
        cat "$SYSFS/$ATTR"
    else
        echo "FAIL: $SYSFS/$ATTR is unavailable"
        exit 1
    fi
done
echo "PASS: sysfs attributes are readable"

echo
echo "[TEST 6] Check kernel module"
if grep -q '^lab2_driver ' /proc/modules; then
    echo "PASS: lab2_driver is loaded"
else
    echo "FAIL: lab2_driver is not loaded"
    exit 1
fi

echo
echo "======================================"
echo "RESULT: ALL TESTS PASSED"
echo "======================================"

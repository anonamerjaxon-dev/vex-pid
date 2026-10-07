#!/bin/bash
# Double-click this file to build and flash the firmware.
#
# It exists because `pio run -t upload` guesses the port, and this Mac also has
# a Bluetooth speaker that looks like a serial port, so the guess sometimes
# lands on the wrong device and fails with "No serial data received".
# This script waits for the ESP32 specifically.

cd "$(dirname "$0")" || exit 1

echo "──────────────────────────────────────────────"
echo " MPU-6050 firmware  →  ESP32-C3"
echo "──────────────────────────────────────────────"
echo

find_board() {
  for p in /dev/cu.usbmodem* /dev/cu.wchusbserial* /dev/cu.SLAB_USBtoUART* \
           /dev/cu.usbserial-* ; do
    [ -e "$p" ] && { echo "$p"; return 0; }
  done
  return 1
}

PORT="$(find_board)"
if [ -z "$PORT" ]; then
  echo "No ESP32 found. Plug the board in with a USB *data* cable"
  echo "(some cables are charge-only). Looking for it for 60 seconds..."
  echo
  for i in $(seq 1 60); do
    sleep 1
    PORT="$(find_board)"
    if [ -n "$PORT" ]; then
      echo "  found $PORT after ${i}s"
      break
    fi
    printf "."
  done
  echo
fi

if [ -z "$PORT" ]; then
  echo
  echo "Still nothing. Check that:"
  echo "  · the cable carries data, not just power"
  echo "  · the board's power LED is on"
  echo "  · nothing else is holding the port (close any serial monitor)"
  echo
  echo "Ports currently present:"
  ls /dev/cu.* 2>/dev/null | sed 's/^/  /'
  echo
  read -r -p "Press Return to close."
  exit 1
fi

echo "Using port: $PORT"
echo
pio run -t upload --upload-port "$PORT"
STATUS=$?
echo
if [ $STATUS -eq 0 ]; then
  echo "──────────────────────────────────────────────"
  echo " Done. Now double-click start_dashboard.command"
  echo "──────────────────────────────────────────────"
else
  echo "──────────────────────────────────────────────"
  echo " Upload failed. If it says the port is busy, close"
  echo " the dashboard tab / serial monitor and retry."
  echo "──────────────────────────────────────────────"
fi
echo
read -r -p "Press Return to close."
exit $STATUS

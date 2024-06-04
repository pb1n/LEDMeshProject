#!/bin/bash

# Function to get the port based on input
get_port() {
  case $1 in
    1)
      echo "/dev/tty.usbserial-1410"
      ;;
    2)
      echo "/dev/tty.usbserial-1420"
      ;;
    3)
      echo "/dev/tty.usbserial-1430"
      ;;
    4)
      echo "/dev/tty.usbserial-1440"
      ;;
    *)
      echo "Invalid port number: $1"
      exit 1
      ;;
  esac
}

# Check if the required number of arguments is provided
if [ $# -ne 1 ]; then
  echo "Usage: $0 <port_number>"
  echo "Port numbers:"
  echo "  1: /dev/tty.usbserial-1410"
  echo "  2: /dev/tty.usbserial-1420"
  echo "  3: /dev/tty.usbserial-1430"
  echo "  4: /dev/tty.usbserial-1440"
  exit 1
fi

# Get the port based on input
port=$(get_port $1)

# Ensure mkspiffs is in the PATH
if ! command -v mkspiffs &> /dev/null; then
  echo "mkspiffs could not be found. Please install mkspiffs and ensure it's in your PATH."
  exit 1
fi

# Create the SPIFFS image
echo "Creating SPIFFS image..."
mkspiffs -c data/ -b 4096 -p 256 -s 0x00F0000 spiffs_image.bin
if [ $? -ne 0 ]; then
  echo "Failed to create SPIFFS image"
  exit 1
fi

# Upload the SPIFFS image
echo "Uploading SPIFFS image to port $port..."
esptool.py --chip esp32-c6 --port $port --baud 115200 write_flash 0x00310000 spiffs_image.bin
if [ $? -ne 0 ]; then
  echo "Failed to upload SPIFFS image"
  exit 1
fi

echo "SPIFFS image uploaded successfully"

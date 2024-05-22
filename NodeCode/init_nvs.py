import esptool
import subprocess

def erase_flash(port):
    # Erase the flash
    esptool.main(['--port', port, 'erase_flash'])

def flash_nvs_partition(port, bin_path):
    # Flash the NVS partition
    esptool.main(['--port', port, 'write_flash', '0x9000', bin_path])

if __name__ == '__main__':
    port = '/dev/tty.usbserial-1440'  # Adjust this to your ESP32's serial port
    nvs_bin = 'nvs_partition.bin'  # Path to your NVS binary file

    erase_flash(port)
    flash_nvs_partition(port, nvs_bin)

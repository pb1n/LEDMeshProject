import os
import subprocess
from SCons.Script import DefaultEnvironment

env = DefaultEnvironment()
platform = env.PioPlatform()

# Manually specify the path to the mkspiffs tool
MKSPIFFSTOOL = os.path.expanduser("~/.platformio/tools/mkspiffs/mkspiffs")
if not os.path.isfile(MKSPIFFSTOOL):
    env.Exit("Error: mkspiffs tool not found. Please ensure it is installed at ~/.platformio/tools/mkspiffs/mkspiffs")

env.Replace(MKSPIFFSTOOL=MKSPIFFSTOOL)

def before_upload(source, target, env):
    # Path to the data directory
    data_dir = os.path.join(env['PROJECT_DIR'], 'data')
    
    # Ensure the data directory exists
    if not os.path.isdir(data_dir):
        print(f"Data directory '{data_dir}' does not exist.")
        return

    # Path to the SPIFFS image
    spiffs_image = os.path.join(env['BUILD_DIR'], "spiffs.bin")

    # Create the SPIFFS image
    mkspiffs_cmd = [
        MKSPIFFSTOOL,
        '-c', data_dir,
        '-b', '4096',
        '-p', '256',
        '-s', '0xF0000',
        spiffs_image
    ]

    print(" ".join(mkspiffs_cmd))
    result = subprocess.run(mkspiffs_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    if result.returncode != 0:
        print("Error creating SPIFFS image:")
        print(result.stderr.decode())
        env.Exit("Error: Failed to create SPIFFS image.")

env.AddPreAction("uploadfs", before_upload)

# Define the 'uploadfs' target
env.Alias("uploadfs", None, [before_upload])

print("UploadFS setup complete.")

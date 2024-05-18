Import("env")
from SCons.Script import DefaultEnvironment

env.Replace(
    MKSPIFFSTOOL=env.PioPlatform().get_package_dir("tool-mkspiffs") + "/mkspiffs"
)

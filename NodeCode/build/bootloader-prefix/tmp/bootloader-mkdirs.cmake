# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Users/janvi/esp/v5.2.1/esp-idf/components/bootloader/subproject"
  "C:/Users/janvi/Desktop/Imperial_EEE/Year_3/LEDMeshProject/NodeCode/build/bootloader"
  "C:/Users/janvi/Desktop/Imperial_EEE/Year_3/LEDMeshProject/NodeCode/build/bootloader-prefix"
  "C:/Users/janvi/Desktop/Imperial_EEE/Year_3/LEDMeshProject/NodeCode/build/bootloader-prefix/tmp"
  "C:/Users/janvi/Desktop/Imperial_EEE/Year_3/LEDMeshProject/NodeCode/build/bootloader-prefix/src/bootloader-stamp"
  "C:/Users/janvi/Desktop/Imperial_EEE/Year_3/LEDMeshProject/NodeCode/build/bootloader-prefix/src"
  "C:/Users/janvi/Desktop/Imperial_EEE/Year_3/LEDMeshProject/NodeCode/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/janvi/Desktop/Imperial_EEE/Year_3/LEDMeshProject/NodeCode/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/janvi/Desktop/Imperial_EEE/Year_3/LEDMeshProject/NodeCode/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()

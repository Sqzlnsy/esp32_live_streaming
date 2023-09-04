# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Users/suq/esp/esp-idf/components/bootloader/subproject"
  "C:/Users/suq/esp32_cam_live_streaming/build/bootloader"
  "C:/Users/suq/esp32_cam_live_streaming/build/bootloader-prefix"
  "C:/Users/suq/esp32_cam_live_streaming/build/bootloader-prefix/tmp"
  "C:/Users/suq/esp32_cam_live_streaming/build/bootloader-prefix/src/bootloader-stamp"
  "C:/Users/suq/esp32_cam_live_streaming/build/bootloader-prefix/src"
  "C:/Users/suq/esp32_cam_live_streaming/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/suq/esp32_cam_live_streaming/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()

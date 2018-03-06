cmake_minimum_required(VERSION 2.8.4)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cmake/ArduinoToolchain.cmake)
set(PROJECT_NAME turn_signal)

project(${PROJECT_NAME})

set(${CMAKE_PROJECT_NAME}_SKETCH turn_signal.ino)

link_directories($ENV{HOME}/Documents/Arduino/libraries)

set(${CMAKE_PROJECT_NAME}_BOARD gemma)
set(${CMAKE_PROJECT_NAME}_PORT /dev/ttyUSB0)
set(${TARGET_NAME}_PROGRAMMER usbtinyisp)

generate_arduino_firmware(${CMAKE_PROJECT_NAME})

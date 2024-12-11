cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

set (COMPONENTS ${IDF_TARGET} main freertos esptool_py wamr)

list(APPEND EXTRA_COMPONENT_DIRS "$ENV{WAMR_PATH}/build-scripts/esp-idf")

project(sd_card)

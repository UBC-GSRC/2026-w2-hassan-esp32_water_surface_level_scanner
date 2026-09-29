cmake_minimum_required(VERSION 3.5)

get_filename_component(ESPNOW_LINK_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(EXTRA_COMPONENT_DIRS "${ESPNOW_LINK_DIR}")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(test_espnow_link)
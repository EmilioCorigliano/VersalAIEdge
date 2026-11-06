# Copyright (c) 2023 - 2024 Advanced Micro Devices, Inc. All Rights Reserved.
# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.14.7)
project(freertos)

find_package(common)
include(${CMAKE_CURRENT_SOURCE_DIR}/freertos10_xilinx.cmake * NO_POLICY_SCOPE)

collector_create (PROJECT_LIB_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}")
collector_create (PROJECT_LIB_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}")
include_directories(${CMAKE_BINARY_DIR}/include)

enable_language(C ASM)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Source)

collect (PROJECT_LIB_HEADERS FreeRTOSSTMTrace.h)
collector_list (_sources PROJECT_LIB_SOURCES)
collector_list (_headers PROJECT_LIB_HEADERS)
file(COPY ${_headers} DESTINATION ${CMAKE_BINARY_DIR}/include)
if (NOT ${YOCTO})
file(COPY ${CMAKE_BINARY_DIR}/include/FreeRTOSConfig.h DESTINATION ${CMAKE_INCLUDE_PATH}/)
file(COPY ${CMAKE_BINARY_DIR}/include/FreeRTOSUARTConfig.h DESTINATION ${CMAKE_INCLUDE_PATH}/)
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../../freertos_hello_world/src/UserConfig.cmake)
set(freertos_sources_file "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../../freertos_hello_world/src/freertos_sources.cmake")
set(excluded_sources)

# Export source list for reuse by the application build
file(WRITE ${freertos_sources_file}
    "set(FREERTOS_SOURCES\n"
)

foreach(src ${_sources})
    # Make paths absolute so they work from any build directory
    get_filename_component(abs_src "${src}" ABSOLUTE)
    get_filename_component(ext "${src}" LAST_EXT)
    if(${ext} STREQUAL ".c" OR ${ext} STREQUAL ".cpp" OR ${ext} STREQUAL ".C" OR ${ext} STREQUAL ".CPP")
        file(APPEND ${freertos_sources_file}
            "    \"${abs_src}\"\n"
        )
    else()
        list(APPEND excluded_sources ${src})
    endif()
endforeach()

file(APPEND ${freertos_sources_file}
    ")\n"
)

if(NOT ASPIS_CONFIG MATCHES "--no-dup")
    ADD_DEFINITIONS(-DASPIS)
endif()

add_library(freertos STATIC ${excluded_sources})
set_target_properties(freertos PROPERTIES LINKER_LANGUAGE C)
install(TARGETS freertos LIBRARY DESTINATION ${CMAKE_LIBRARY_PATH} ARCHIVE DESTINATION ${CMAKE_LIBRARY_PATH})
install(DIRECTORY ${CMAKE_BINARY_DIR}/include DESTINATION ${CMAKE_INCLUDE_PATH}/..)

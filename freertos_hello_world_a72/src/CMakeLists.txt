# Copyright (C) 2023 - 2025 Advanced Micro Devices, Inc.  All rights reserved.
# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.14.7)

set(STARTUP_DIR /opt/Xilinx/2025.2/Vitis/gnu/aarch64/lin/aarch64-none/aarch64-xilinx-elf/usr/lib64)
set(APP_NAME freertos_hello_world_a72)

include(${CMAKE_CURRENT_SOURCE_DIR}/Freertos_hello_worldExample.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/UserConfig.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../ASPIS.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/freertos_sources.cmake)

project(${APP_NAME})

find_package(common)
enable_language(C ASM)
collect(PROJECT_LIB_DEPS xil;xilstandalone;xiltimer;freertos;gcc;c;aiebaremetal;xilsem)
collector_list (_deps PROJECT_LIB_DEPS)
list (APPEND _deps ${USER_LINK_LIBRARIES})

if (CMAKE_EXPORT_COMPILE_COMMANDS AND
    (NOT ${YOCTO}))
    set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
    set(CMAKE_C_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES})
endif()
list (APPEND _sources ${USER_COMPILE_SOURCES})
list (APPEND _sources ${FREERTOS_SOURCES})
linker_gen("${CMAKE_CURRENT_SOURCE_DIR}/linker_files/")
string(APPEND CMAKE_C_FLAGS ${USER_COMPILE_OPTIONS})
string(APPEND CMAKE_CXX_FLAGS ${USER_COMPILE_OPTIONS})
string(APPEND CMAKE_C_LINK_FLAGS ${USER_LINK_OPTIONS})
string(APPEND CMAKE_CXX_LINK_FLAGS ${USER_LINK_OPTIONS})
add_dependency_on_bsp(_sources)
add_executable(${APP_NAME} ${_sources})
set_target_properties(${APP_NAME} PROPERTIES LINK_DEPENDS ${USER_LINKER_SCRIPT})
string (REPLACE ";" ",-l" _deps "${_deps}")
set(_linker_command "--rtlib=libgcc --unwindlib=none -mcpu=cortex-a72 --target=aarch64-none-elf -L${STARTUP_DIR} -Wl,-T -Wl,\"${USER_LINKER_SCRIPT}\" -L\"${CMAKE_SOURCE_DIR}/\" -L\"${CMAKE_LIBRARY_PATH}/\" -L\"${USER_LINK_DIRECTORIES}/\" -Wl,--start-group,-l${_deps} -Wl,--end-group")
target_link_libraries(${APP_NAME} ${_linker_command})

lib_aspis_target(${APP_NAME} ${ASPIS_CONFIG}
    EXTRA_SOURCES
        ${_sources}
    LINKER_COMMAND
        ${_linker_command}
)

target_link_options(${APP_NAME} PRIVATE
    -nostartfiles
    ${STARTUP_DIR}/crt0.o
    ${STARTUP_DIR}/crti.o
    ${STARTUP_DIR}/crtbegin.o
    ${STARTUP_DIR}/crtend.o
    ${STARTUP_DIR}/crtn.o
)
target_compile_definitions(${APP_NAME} PUBLIC ${USER_COMPILE_DEFINITIONS})
target_include_directories(${APP_NAME} PUBLIC ${USER_INCLUDE_DIRECTORIES})
print_build_info_target(${APP_NAME} "${CMAKE_C_FLAGS}" "${USER_COMPILE_DEFINITIONS}" "${CMAKE_C_LINK_FLAGS}" "${_linker_command}")
# print_elf_size(CMAKE_SIZE ${APP_NAME})
add_dependencies(${APP_NAME} ${APP_NAME}_aspis)

# Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All Rights Reserved.
# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.15)
project(xilstandalone)

set(Mem_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

set(Xilinx_gcc "/opt/Xilinx/2025.2/Vitis/gnu/aarch32/lin/gcc-arm-none-eabi/bin/arm-none-eabi-gcc")
# set(Xilinx_gcc ${CMAKE_C_COMPILER})

find_package(common)
find_package(Mem)
list(APPEND defs "#ifndef XMEM_CONFIG_H_\n")
list(APPEND defs "#define XMEM_CONFIG_H_\n\n")

list(LENGTH MEM_DEF_NAMES len)
math(EXPR mem_len "${len} - 1")
if (MEM_DEF_NAMES)
    foreach(val RANGE ${mem_len})
       list(GET MEM_DEF_NAMES ${val} MEM)
       list(GET MEM_RANGES ${val} VAL)
       math(EXPR val_mod_2 "${val} % 2")
       if(val_mod_2 EQUAL 1)
           math(EXPR VAL "${VAL} - 1" OUTPUT_FORMAT HEXADECIMAL)
       endif()
       list(APPEND defs "#define ${MEM} ${VAL}\n")
    endforeach()
endif()
list(APPEND defs "\n#endif")
file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/common/xmem_config.h ${defs})
include(${CMAKE_CURRENT_SOURCE_DIR}/xilstandalone.cmake NO_POLICY_SCOPE)
collector_create (PROJECT_LIB_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}")
collector_create (PROJECT_LIB_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}")

#Bring the headers into the project
include_directories(${CMAKE_BINARY_DIR}/include)

enable_language(C ASM)
add_subdirectory(common)
if(("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "microblaze") OR
   ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "microblazeel") OR
   ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "plm_microblaze") OR
   ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "pmu_microblaze"))
add_subdirectory(microblaze)
elseif("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "microblaze_riscv")
add_subdirectory(riscv)
else()
add_subdirectory(arm)
endif()

collector_list (_sources PROJECT_LIB_SOURCES)
collector_list (_headers PROJECT_LIB_HEADERS)
file(COPY ${_headers} DESTINATION ${CMAKE_BINARY_DIR}/include)
if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "cortexr5")
file(RENAME ${CMAKE_BINARY_DIR}/include/xil_mpu_r5.h ${CMAKE_BINARY_DIR}/include/xil_mpu.h)
endif()
if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "cortexr52")
file(RENAME ${CMAKE_BINARY_DIR}/include/xil_mpu_r52.h ${CMAKE_BINARY_DIR}/include/xil_mpu.h)
endif()
if (NOT ${YOCTO})
file(COPY ${CMAKE_BINARY_DIR}/include/bspconfig.h DESTINATION ${CMAKE_INCLUDE_PATH}/)
endif()

#Compile it as a static library
if (("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "plm_microblaze") OR
   ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "microblaze") OR
   ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "pmu_microblaze"))
string(FIND "${CMAKE_C_FLAGS}" "-flto" has_flto)
# Custom command to build your one file.
    if (NOT ("${CMAKE_SYSTEM_NAME}" STREQUAL "FreeRTOS") AND
        NOT (${has_flto} EQUAL -1))
    string(REPLACE "-Os -flto -ffat-lto-objects" "" CUSTOM_FLAGS ${CMAKE_C_FLAGS})
    # Split the flags into a cmake list (; separated)
    separate_arguments(CUSTOM_FLAGS UNIX_COMMAND ${CUSTOM_FLAGS})
    add_custom_command(
        OUTPUT microblaze_interrupt_handler.o
        COMMAND ${CMAKE_CXX_COMPILER}
        ARGS ${CUSTOM_FLAGS} -c ${CMAKE_CURRENT_SOURCE_DIR}/microblaze/microblaze_interrupt_handler.c
                -I ${CMAKE_BINARY_DIR}/include
                    -o ${CMAKE_CURRENT_BINARY_DIR}/microblaze_interrupt_handler.o
        MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/microblaze/microblaze_interrupt_handler.c)
    add_library(xilstandalone STATIC ${_sources} microblaze_interrupt_handler.o)
    else()
    add_library(xilstandalone STATIC ${_sources})
    endif()
elseif(("${proc_compiler}" MATCHES "clang") OR
	("${proc_compiler}" STREQUAL "armcc"))

    # Use the xilinx_gcc compiler anyways for compatibility
    set(GCC_FLAGS
        -DFREERTOS_BSP
        -DSDT 
        -mcpu=cortex-r5 
        -mfloat-abi=hard 
        -mfpu=vfpv3-d16 
        -DARMR5 
        -MP 
        -I${CMAKE_INCLUDE_PATH}
        -I${CMAKE_BINARY_DIR}/include 
        -c 
        -O0 
        -g 
        -Wall 
        -Wextra 
        -MD
    )

    set(C_OBJ_FILES)
    foreach(_src IN LISTS _sources)
        get_filename_component(_src_name "${_src}" NAME_WE)
        set(_obj "${CMAKE_CURRENT_BINARY_DIR}/${_src_name}.c.obj")

        add_custom_command(
            OUTPUT "${_obj}"
            COMMAND ${Xilinx_gcc} ${GCC_FLAGS} -c "${CMAKE_CURRENT_SOURCE_DIR}/${_src}" -o "${_obj}"
            DEPENDS "${_src}"
            COMMENT "Compiling ${_src_name}.c with GCC (GNU mode)"
            VERBATIM
        )

        list(APPEND C_OBJ_FILES "${_obj}")
    endforeach()

    add_library(xilstandalone STATIC ${C_OBJ_FILES})

    set_target_properties(xilstandalone PROPERTIES
        LINKER_LANGUAGE C
    )
elseif("${proc_compiler}" STREQUAL "iccarm.exe")
    if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "cortexr5")
    set(ASM_s_FILES ${CMAKE_CURRENT_SOURCE_DIR}/arm/cortexr5/iccarm/boot.s  ${CMAKE_CURRENT_SOURCE_DIR}/arm/cortexr5/iccarm/asm_vectors.s)
    set(CPU_C_FLAGS --cpu Cortex-R5 --fpu VFPv3_D16)
    elseif("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "cortexa9")
    set(ASM_S_FILES ${CMAKE_CURRENT_SOURCE_DIR}/arm/cortexa9/iccarm/boot.S  ${CMAKE_CURRENT_SOURCE_DIR}/arm/cortexa9/iccarm/asm_vectors.s  ${CMAKE_CURRENT_SOURCE_DIR}/arm/cortexa9/iccarm/translation_table.s)
    set(CPU_C_FLAGS --cpu=Cortex-A9 -Om -e)
    endif()
    foreach(ASM_s_FILE ${ASM_s_FILES})
        # Get the file name without the extension
        get_filename_component(FILE_NAME ${ASM_s_FILE} NAME_WE)

        set(ASM_O_FILE ${CMAKE_CURRENT_BINARY_DIR}/${FILE_NAME}.o)

        # Add a custom command to convert .s to .o
        add_custom_command(
            OUTPUT ${ASM_O_FILE}
            COMMAND ${CMAKE_ASM_COMPILER} ${CPU_C_FLAGS} -o ${ASM_O_FILE} ${ASM_s_FILE} -I${CMAKE_INCLUDE_PATH}
            DEPENDS ${ASM_s_FILE}
        )

        # Add the .o file to a list of object files
        list(APPEND ASM_O_FILES ${ASM_O_FILE})
    endforeach()
    add_library(xilstandalone STATIC ${_sources} ${ASM_O_FILES})
else()
    add_library(xilstandalone STATIC ${_sources})
endif()
set_target_properties(xilstandalone PROPERTIES LINKER_LANGUAGE C)

install(TARGETS xilstandalone LIBRARY DESTINATION ${CMAKE_LIBRARY_PATH} ARCHIVE DESTINATION ${CMAKE_LIBRARY_PATH})
install(DIRECTORY ${CMAKE_BINARY_DIR}/include DESTINATION ${CMAKE_INCLUDE_PATH}/..)

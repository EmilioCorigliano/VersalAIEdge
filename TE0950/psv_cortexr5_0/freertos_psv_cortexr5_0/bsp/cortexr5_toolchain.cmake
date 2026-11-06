# Copyright (C) 2023-2025 Advanced Micro Devices, Inc.  All rights reserved.
# SPDX-License-Identifier: MIT

set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)

set ( LLVM_BASE /home/streben/Documents/POLIMI/PhD/Projects/llvm-project/build_arm )

set( CMAKE_EXPORT_COMPILE_COMMANDS ON)
set( CMAKE_INSTALL_MESSAGE LAZY)
set( CMAKE_C_COMPILER ${LLVM_BASE}/bin/clang )
set( CMAKE_CXX_COMPILER ${LLVM_BASE}/bin/clang++ )
set( CMAKE_C_COMPILER_LAUNCHER  )
set( CMAKE_CXX_COMPILER_LAUNCHER  )
set( CMAKE_ASM_COMPILER ${LLVM_BASE}/bin/clang )
set( CMAKE_AR ${LLVM_BASE}/bin/llvm-ar CACHE FILEPATH "Archiver" )
set( CMAKE_SIZE armr5-none-eabi-size CACHE FILEPATH "Size" )
set( CMAKE_SYSTEM_PROCESSOR "cortexr5" )
set( CMAKE_MACHINE "Versal" )
set( CMAKE_SYSTEM_NAME "Generic" )
set( CMAKE_SPECS_FILE "$ENV{ESW_REPO}/scripts/specs/arm/Xilinx.spec" CACHE STRING "Specs file path for using CMAKE toolchain files" )
set( TOOLCHAIN_C_FLAGS "  -DSDT --target=arm-none-eabi -mcpu=cortex-r5 -mfloat-abi=hard -mfpu=vfpv3-d16 -DARMR5 -fshort-enums" CACHE STRING "CFLAGS" )
set( TOOLCHAIN_CXX_FLAGS "  -DSDT --target=arm-none-eabi -mcpu=cortex-r5 -mfloat-abi=hard -mfpu=vfpv3-d16 -DARMR5 -fshort-enums" CACHE STRING "CXXFLAGS" )
set( TOOLCHAIN_ASM_FLAGS "  -DSDT --target=arm-none-eabi -mcpu=cortex-r5 -mfloat-abi=hard -mfpu=vfpv3-d16 -DARMR5" CACHE STRING "ASM FLAGS" )
set( TOOLCHAIN_DEP_FLAGS " -MP -isystem /opt/Xilinx/2025.2/Vitis/gnu/armr5/lin/gcc-arm-none-eabi/armrm-xilinx-eabi/usr/include/c++/13.3.0/arm-xilinx-eabi -isystem /opt/Xilinx/2025.2/Vitis/gnu/armr5/lin/gcc-arm-none-eabi/armrm-xilinx-eabi/usr/include/c++/13.3.0 -isystem /opt/Xilinx/2025.2/Vitis/gnu/armr5/lin/gcc-arm-none-eabi/armrm-xilinx-eabi/usr/include " CACHE STRING "Flags used by compiler to generate dependency files")
set( TOOLCHAIN_EXTRA_C_FLAGS " -O0 -g -Wall -Wextra --sysroot=/opt/Xilinx/2025.2/Vitis/gnu/armr5/lin/gcc-arm-none-eabi/armrm-xilinx-eabi" CACHE STRING "Extra CFLAGS")
set( CMAKE_C_FLAGS_RELEASE "-DNDEBUG" CACHE STRING "Additional CFLAGS for release" )
set( CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG" CACHE STRING "Additional CXXFLAGS for release" )
set( CMAKE_ASM_FLAGS_RELEASE "-DNDEBUG" CACHE STRING "Additional ASM FLAGS for release" )
set( CMAKE_C_FLAGS "${TOOLCHAIN_C_FLAGS} ${TOOLCHAIN_DEP_FLAGS} -I${CMAKE_INCLUDE_PATH}" CACHE STRING "CFLAGS")
set( CMAKE_CXX_FLAGS "${TOOLCHAIN_CXX_FLAGS} ${TOOLCHAIN_DEP_FLAGS} -I${CMAKE_INCLUDE_PATH}" CACHE STRING "CXXFLAGS")
set( CMAKE_ASM_FLAGS "${TOOLCHAIN_ASM_FLAGS} ${TOOLCHAIN_DEP_FLAGS} -I${CMAKE_INCLUDE_PATH}" CACHE STRING "ASMFLAGS")
set( CMAKE_EXE_LINKER_FLAGS "--target=arm-none-eabi -mcpu=cortex-r5 -mfloat-abi=hard -mfpu=vfpv3-d16 " CACHE STRING "Linker flags" ) # -resource-dir=/opt/Xilinx/2025.2/Vitis/gnu/armr5/lin/gcc-arm-none-eabi/armrm-xilinx-eabi
set(TEMPLATE "empty_application")
set( CMAKE_SYSTEM_NAME FreeRTOS)
set( CMAKE_HOST_NAME Linux)
ADD_DEFINITIONS(-DFREERTOS_BSP)

# Fix Vitis library path poisoning for clang
set(CMAKE_C_COMPILE_OBJECT
    "<CMAKE_COMMAND> -E env LD_LIBRARY_PATH=${LLVM_BASE}/lib:/lib64:/usr/lib64:$ENV{LD_LIBRARY_PATH} <CMAKE_C_COMPILER> <DEFINES> <INCLUDES> <FLAGS> -o <OBJECT> -c <SOURCE>"
)

set(CMAKE_C_LINK_EXECUTABLE
    "<CMAKE_COMMAND> -E env LD_LIBRARY_PATH=${LLVM_BASE}/lib:/lib64:/usr/lib64:$ENV{LD_LIBRARY_PATH} <CMAKE_C_COMPILER> <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>"
)

set(CMAKE_ASM_COMPILE_OBJECT
    "<CMAKE_COMMAND> -E env LD_LIBRARY_PATH=${LLVM_BASE}/lib:/lib64:/usr/lib64:$ENV{LD_LIBRARY_PATH} <CMAKE_ASM_COMPILER> <DEFINES> <INCLUDES> <FLAGS> -o <OBJECT> -c <SOURCE>"
)

set(CMAKE_ASM_LINK_EXECUTABLE
    "<CMAKE_COMMAND> -E env LD_LIBRARY_PATH=${LLVM_BASE}/lib:/lib64:/usr/lib64:$ENV{LD_LIBRARY_PATH} <CMAKE_ASM_COMPILER> <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>"
)

set(CMAKE_CXX_COMPILE_OBJECT
    "<CMAKE_COMMAND> -E env LD_LIBRARY_PATH=${LLVM_BASE}/lib:/lib64:/usr/lib64:$ENV{LD_LIBRARY_PATH} <CMAKE_CXX_COMPILER> <DEFINES> <INCLUDES> <FLAGS> -o <OBJECT> -c <SOURCE>"
)

set(CMAKE_CXX_LINK_EXECUTABLE
    "<CMAKE_COMMAND> -E env LD_LIBRARY_PATH=${LLVM_BASE}/lib:/lib64:/usr/lib64:$ENV{LD_LIBRARY_PATH} <CMAKE_CXX_COMPILER> <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>"
)

set(CMAKE_C_CREATE_STATIC_LIBRARY
    "<CMAKE_COMMAND> -E env LD_LIBRARY_PATH=${LLVM_BASE}/lib:/lib64:/usr/lib64:$ENV{LD_LIBRARY_PATH} <CMAKE_AR> qc <TARGET> <LINK_FLAGS> <OBJECTS>"
)

set(CMAKE_CXX_CREATE_STATIC_LIBRARY
    "<CMAKE_COMMAND> -E env LD_LIBRARY_PATH=${LLVM_BASE}/lib:/lib64:/usr/lib64:$ENV{LD_LIBRARY_PATH} <CMAKE_CXX_COMPILER_AR> qc <TARGET> <LINK_FLAGS> <OBJECTS>"
)

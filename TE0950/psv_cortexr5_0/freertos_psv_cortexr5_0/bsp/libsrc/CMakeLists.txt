# Copyright (C) 2023 - 2025 Advanced Micro Devices, Inc.  All rights reserved.
# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.15)
project(libxil)

if (YOCTO)
include_directories(${CMAKE_BINARY_DIR}/include)
add_library(libxil INTERFACE)
else()
find_package(common)
include_directories(${CMAKE_BINARY_DIR}/include)
set(DRVLIST_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
find_package(DRVLIST)

set(OBJS)
set(EXTRACTED_OBJECTS)
set(xil_mri)

FOREACH(DRIVER ${DRIVER_LIST})
    STRING(REGEX REPLACE "-" "_" DRIVER ${DRIVER})
    add_subdirectory(${DRIVER}/src)
    get_property(src_files TARGET ${DRIVER} PROPERTY SOURCES)
    foreach(src ${src_files})
	set(drv_archive ${CMAKE_BINARY_DIR}/libsrc/${DRIVER}/src/lib${DRIVER}.a)
	if (NOT (${drv_archive} IN_LIST OBJS))
	    list(APPEND OBJS ${drv_archive})
	endif()
	if ("${proc_compiler}" STREQUAL "iccarm.exe")
		list(APPEND EXTRACTED_OBJECTS ${src}.o)
	endif()
    endforeach()
ENDFOREACH()

# Preserve the previous ordering behaviour.
list(REVERSE OBJS)

# MRI script: merge archive contents into libxil.a.
# This is critical. Using `ar -cr libxil.a libfoo.a libbar.a` creates a nested archive,
# which GNU ld cannot use as a normal library.
list(APPEND xil_mri "create libxil.a\n")

foreach(drv_archive ${OBJS})
    list(APPEND xil_mri "addlib ${drv_archive}\n")
endforeach()

list(APPEND xil_mri "save\n")
list(APPEND xil_mri "end\n")
file(WRITE ${CMAKE_BINARY_DIR}/libxil.mri ${xil_mri})

set(XIL_LIB ${CMAKE_BINARY_DIR}/libxil.a)
if(("${proc_compiler}" STREQUAL "clang") OR
   ("${proc_compiler}" STREQUAL "armcc"))
	list(REVERSE OBJS)
	set(CUSTOM_COMMAND export LD_LIBRARY_PATH="${LLVM_BASE}/lib:/lib64:/usr/lib64:$ENV{LD_LIBRARY_PATH}" && ${CMAKE_AR} -cLq ${XIL_LIB} ${OBJS})
elseif("${proc_compiler}" STREQUAL "iccarm.exe")
	list(REVERSE OBJS)
	foreach(drv_archive ${OBJS})
		string(REGEX MATCH ".*/libsrc/([^/]+)/src/lib[^/]+\\.a" MATCHED  ${drv_archive})
		set(DRIVER "${CMAKE_MATCH_1}")
		add_custom_target(extract_${DRIVER} ALL
			COMMAND ${CMAKE_AR} -X ${drv_archive}
			WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
			COMMENT "Extracting object files from ${drv_archive}"
			DEPENDS ${DRIVER}
		)
	endforeach()
	set(CUSTOM_COMMAND ${CMAKE_AR} --create ${XIL_LIB} ${EXTRACTED_OBJECTS})
else()
	set(CUSTOM_COMMAND ${CMAKE_AR} -M<${CMAKE_BINARY_DIR}/libxil.mri)
endif()
add_custom_target(combined ALL
	COMMAND ${CUSTOM_COMMAND}
    COMMAND ${CMAKE_RANLIB} ${XIL_LIB}
	COMMAND ${CMAKE_COMMAND} -E copy ${XIL_LIB} ${CMAKE_LIBRARY_PATH}
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
	COMMENT "Archiving C static library libsrc/build_configs/gen_bsp/libxil.a"
	DEPENDS ${DRIVER_LIST})

if("${proc_compiler}" STREQUAL "iccarm.exe")
    add_custom_target(remove ALL
        COMMAND ${CMAKE_COMMAND} -E remove ${EXTRACTED_OBJECTS}
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running custom post-build command to remove object files"
    )
    add_dependencies(remove combined)
endif()
add_library(xil STATIC IMPORTED GLOBAL)
add_dependencies(xil combined)
set_target_properties(xil PROPERTIES IMPORTED_LOCATION ${XIL_LIB} LINKER_LANGUAGE C)
install(DIRECTORY ${CMAKE_BINARY_DIR}/include DESTINATION ${CMAKE_INCLUDE_PATH}/..)

# -------------------------------------------------------------------
# Build SFERAv2-WSCP as a static library
# -------------------------------------------------------------------
set(WSCP_ROOT "/home/streben/Documents/POLIMI/PhD/Projects/VersalAIEdge/libs/SFERAv2-WSCP")

collector_create (PROJECT_LIB_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}")
collector_create (PROJECT_LIB_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}")

collect (PROJECT_LIB_SOURCES "${WSCP_ROOT}/src/crc.c")
collect (PROJECT_LIB_SOURCES "${WSCP_ROOT}/src/wscp.c")
collect (PROJECT_LIB_SOURCES "${WSCP_ROOT}/src/wscp_cbor.c")
collect (PROJECT_LIB_SOURCES "${WSCP_ROOT}/src/wscp_port_versal.c")
collect (PROJECT_LIB_SOURCES "${WSCP_ROOT}/submodules/cbor/src/common.c")
collect (PROJECT_LIB_SOURCES "${WSCP_ROOT}/submodules/cbor/src/parser.c")
collect (PROJECT_LIB_SOURCES "${WSCP_ROOT}/submodules/cbor/src/decoder.c")
collect (PROJECT_LIB_SOURCES "${WSCP_ROOT}/submodules/cbor/src/encoder.c")
collect (PROJECT_LIB_SOURCES "${WSCP_ROOT}/submodules/cbor/src/helper.c")
collect (PROJECT_LIB_SOURCES "${WSCP_ROOT}/submodules/cbor/src/ieee754.c")

collect (PROJECT_LIB_HEADERS "${WSCP_ROOT}/src/wscp.h")
collect (PROJECT_LIB_HEADERS "${WSCP_ROOT}/src/wscp_port.h")

collector_list (_sources PROJECT_LIB_SOURCES)
collector_list (_headers PROJECT_LIB_HEADERS)

file(COPY ${_headers} DESTINATION ${CMAKE_BINARY_DIR}/include)

add_library(wscp STATIC ${_sources})
set_target_properties(wscp PROPERTIES LINKER_LANGUAGE C)
target_include_directories(wscp
    PUBLIC
        ${WSCP_ROOT}/src
        ${WSCP_ROOT}/submodules/cbor/include
        ${CMAKE_BINARY_DIR}/include
)
install(TARGETS wscp LIBRARY DESTINATION ${CMAKE_LIBRARY_PATH} ARCHIVE DESTINATION ${CMAKE_LIBRARY_PATH})

endif()

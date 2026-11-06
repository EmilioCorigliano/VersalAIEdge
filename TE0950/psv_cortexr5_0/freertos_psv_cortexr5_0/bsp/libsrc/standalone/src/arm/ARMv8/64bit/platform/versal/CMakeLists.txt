# Copyright (c) 2023 - 2024 Advanced Micro Devices, Inc. All Rights Reserved.
# SPDX-License-Identifier: MIT
if("${proc_compiler}" MATCHES "clang")
  add_subdirectory(armclang)
else() 
  add_subdirectory(gcc)
endif()
if("${CMAKE_MACHINE}" STREQUAL "Versal")
collect (PROJECT_LIB_HEADERS xparameters_ps.h)
endif()

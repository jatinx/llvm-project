#
#//===----------------------------------------------------------------------===//
#//
#// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
#// See https://llvm.org/LICENSE.txt for license information.
#// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#//
#//===----------------------------------------------------------------------===//
#

if(LIBOMP_OMPD_SUPPORT)
    set(OMPD_INCLUDE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src/)
    add_subdirectory(src)
    add_subdirectory(gdb-plugin)

    # HWLOC-support
    set(LIBOMP_USE_HWLOC FALSE CACHE BOOL
      "Use Hwloc (http://www.open-mpi.org/projects/hwloc/) library for affinity?")
    set(LIBOMP_HWLOC_INSTALL_DIR /usr/local CACHE PATH
      "Install path for hwloc library")
endif()

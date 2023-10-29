# Copyright (C) 2008-2019 TrinityCore <https://www.trinitycore.org/>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

# We require CMake >= 3.2
cmake_minimum_required(VERSION 3.2)

# add this options before PROJECT keyword
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

# Set projectname (must be done AFTER setting configurationtypes)
project(TrinityCore)

# CMake policies (can not be handled elsewhere)
cmake_policy(SET CMP0005 OLD)
if(POLICY CMP0043)
  cmake_policy(SET CMP0043 OLD) # Disable 'Ignore COMPILE_DEFINITIONS_<Config> properties'
endif()

if(POLICY CMP0054)
  cmake_policy(SET CMP0054 NEW) # Only interpret if() arguments as variables or keywords when unquoted - prevents intepreting if (SOME_STRING_VARIABLE MATCHES "MSVC") as if (SOME_STRING_VARIABLE MATCHES "1")
endif()

# Set RPATH-handing (CMake parameters)
set(CMAKE_SKIP_BUILD_RPATH 0)
set(CMAKE_BUILD_WITH_INSTALL_RPATH 0)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH 1)

# set macro-directory
list(APPEND CMAKE_MODULE_PATH
  "${CMAKE_SOURCE_DIR}/cmake/macros"
  "${CMAKE_SOURCE_DIR}/dep/cotire/CMake")

# build in Release-mode by default if not explicitly set
if( NOT CMAKE_BUILD_TYPE )
  set(CMAKE_BUILD_TYPE "RelWithDebInfo")
endif()

include(CheckCXXSourceRuns)
include(CheckIncludeFiles)
include(ConfigureScripts)

# set default buildoptions and print them
include(cmake/options.cmake)

# turn off PCH totally if enabled (hidden setting, mainly for devs)
if( NOPCH )
  set(USE_COREPCH 0)
  set(USE_SCRIPTPCH 0)
endif()

# Find needed packages and if necessery abort if something important is missing
unset(ACE_INCLUDE_DIR CACHE)
unset(ACE_LIBRARIES CACHE)
unset(ACE_LIBRARIES_DIR CACHE)
unset(ACE_INCLUDE_DIR)
unset(ACE_LIBRARIES)
unset(ACE_LIBRARIES_DIR)

find_package(ACE)
if(NOT ACE_FOUND)
  message(FATAL_ERROR
    "This project requires ACE installed. Please download the ACE Micro Release Kit from http://download.dre.vanderbilt.edu/ and install it. If this script didn't find ACE and it was correctly installed please set ACE_ROOT to the correct path."
  )
endif()

if(NOT USE_STD_MALLOC)
  unset(TBB_INCLUDE_DIRS            CACHE)
  unset(TBB_LIBRARIES               CACHE)
  unset(TBB_LIBRARIES_RELEASE       CACHE)
  unset(TBB_LIBRARIES_DEBUG         CACHE)

  set(TBB_USE_DEBUG_BUILD ${TBB_DEBUG})
  find_package(TBB COMPONENTS tbbmalloc)
  if(NOT TBB_FOUND)
    message(FATAL_ERROR
      "This project requires TBB installed. Please download the TBB Stable Release from http://www.threadingbuildingblocks.org/ and install it. If this script didn't find TBB and it was correctly installed please set TBB_ROOT_DIR to the correct path."
    )
  endif()

  if(TBB_DEBUG)
    set(TBB_LIBRARIES
      ${TBB_LIBRARIES_DEBUG}
    )
  else()
    set(TBB_LIBRARIES
      ${TBB_LIBRARIES}
    )
  endif()
  
  if (WIN32)
    list (GET TBB_LIBRARIES 1 TBB_SINGLE_FILEDIRECTORY)
    get_filename_component(TBB_LIB_DIRECTORY "${TBB_SINGLE_FILEDIRECTORY}" DIRECTORY)
    link_directories(${TBB_LIB_DIRECTORY})
  endif()
  
endif()

include(ConfigureBaseTargets)
include(CheckPlatform)

include(GroupSources)
include(AutoCollect)

find_package(PCHSupport)

if(WIN32)
# Find out what system we use to include the needed libs
  if(PLATFORM MATCHES 32) # 32-bit
    set(DEP_ARCH win32)
  else() # 64-bit
    set(DEP_ARCH x64)
  endif()
  
  set(ACE_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/dep/windows/include)
  set(ACE_LIBRARIES ${CMAKE_SOURCE_DIR}/dep/windows/lib/${DEP_ARCH}_release/ACE.lib ${CMAKE_SOURCE_DIR}/dep/windows/lib/${DEP_ARCH}_release/ACE_SSL.lib)
  set(MYSQL_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/dep/windows/include/mysql)
  set(MYSQL_LIBRARY ${CMAKE_SOURCE_DIR}/dep/windows/lib/${DEP_ARCH}_release/libmySQL.lib)
  set(MYSQL_DEBUG_LIBRARY ${CMAKE_SOURCE_DIR}/dep/windows/lib/${DEP_ARCH}_debug/libmySQL.lib)
  set(OPENSSL_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/dep/windows/include/openssl)
  set(OPENSSL_LIBRARIES ${CMAKE_SOURCE_DIR}/dep/windows/lib/${DEP_ARCH}_release/libeay32.lib ${CMAKE_SOURCE_DIR}/dep/windows/lib/${DEP_ARCH}_release/ssleay32.lib)
  set(OPENSSL_DEBUG_LIBRARIES ${CMAKE_SOURCE_DIR}/dep/windows/lib/${DEP_ARCH}_debug/libeay32.lib ${CMAKE_SOURCE_DIR}/dep/windows/lib/${DEP_ARCH}_debug/ssleay32.lib)
endif()

find_package(MySQL)

if(NOT WITHOUT_GIT)
  find_package(Git)
endif()

# Find revision ID and hash of the sourcetree
include(cmake/genrev.cmake)

# print out the results before continuing
include(cmake/showoptions.cmake)

# add dependencies
add_subdirectory(dep)

# add core sources
add_subdirectory(src)

if(USE_STD_MALLOC)
  set(ALLOC_LIB_INFO_STRING "std::malloc")
else()
  if(TBB_DEBUG)
    set(ALLOC_LIB_INFO_STRING "TBB/Debug")
  else()
    set(ALLOC_LIB_INFO_STRING "TBB/Release")
  endif()
endif()
message(STATUS "Memory allocation     : ${ALLOC_LIB_INFO_STRING} ${TBB_LIBRARIES}")

if(USE_ANTICHEAT)
  message(STATUS "Use anticheat         : Yes")
  set(DEFINITIONS ${DEFINITIONS} USE_ANTICHEAT)
else()
  message(STATUS "Use anticheat         : No  (default)")
endif()

if(USE_SCRIPTS)
  message(STATUS "Build scripts         : Yes (default)")
else()
  message(STATUS "Build scripts         : No")
endif()

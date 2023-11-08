# Copyright (C) 2008-2019 TrinityCore <https://www.trinitycore.org/>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

file(GLOB sources *.c)
add_library(http-parser STATIC
  ${sources})

target_include_directories(http-parser
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(http-parser
  PRIVATE
    trinity-dependency-interface)

set_target_properties(http-parser
  PROPERTIES
    FOLDER
      "dep")

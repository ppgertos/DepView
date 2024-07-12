cmake_minimum_required(VERSION 3.28)

set(CMAKE_BUILD_TYPE Release CACHE STRING "BUILD TYPE")
set(CMAKE_C_FLAGS_DEBUG "-Wpedantic -Werror -Wall -g")
set(CMAKE_C_FLAGS_RELEASE "-Wpedantic -Werror -Wall")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(DepView VERSION 0.1 LANGUAGES C)

set(USE_FOLDERS ON)

add_custom_target(format_sources
  COMMAND sh -c "clang-format -i --style=file\
    $(find ${CMAKE_CURRENT_SOURCE_DIR}/src -name \"*.[ch]\")"
  VERBATIM
)

include(cmake/FetchRaylib.cmake)
include(cmake/FetchRaygui.cmake)

add_executable(${CMAKE_PROJECT_NAME}
  src/main.c

  src/Diagram.c
  src/DynamicArray.c
  src/LogBook.c
  src/StringContainer.c
 )

target_link_libraries(${CMAKE_PROJECT_NAME} raylib raygui m)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src)

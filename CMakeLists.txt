cmake_minimum_required(VERSION 3.28)

set(CMAKE_C_FLAGS_DEBUG_INIT "-Wpedantic -Werror -Wall")
set(CMAKE_C_FLAGS_RELEASE_INIT "-Wpedantic -Werror -Wall")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(DepView VERSION 0.1 LANGUAGES C)

set(USE_FOLDERS ON)

add_custom_target(format_sources
  COMMAND sh -c "clang-format -i --style=file\
    $(find ${CMAKE_CURRENT_SOURCE_DIR}/src -name \"*.[ch]\")"
  VERBATIM
)

add_library(raylib STATIC IMPORTED)

set_target_properties(raylib PROPERTIES
    IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/3rdparty/lib/libraylib.a)

add_executable(${CMAKE_PROJECT_NAME}
  src/main.c

  src/Diagram.c
  src/LogBook.c
  src/SizeTContainer.c
  src/StringContainer.c
 )

target_link_libraries(${CMAKE_PROJECT_NAME} raylib m)
target_compile_options(${CMAKE_PROJECT_NAME} PUBLIC -g)
target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src)

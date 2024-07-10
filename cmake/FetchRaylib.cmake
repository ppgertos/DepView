include(FetchContent)

FetchContent_Declare(
  raylib_fetch 
  URL https://github.com/raysan5/raylib/releases/download/5.0/raylib-5.0_linux_amd64.tar.gz
  URL_HASH SHA256=5724b8d89c7cedd0c582d022f195169fb3fc27646dac376238da7a2df39aa59c
  SOURCE_DIR ${CMAKE_BINARY_DIR}/_deps/raylib
  )
FetchContent_Populate(raylib_fetch)
FetchContent_MakeAvailable(raylib_fetch)

add_library(raylib STATIC IMPORTED)
target_include_directories(raylib INTERFACE 
  ${CMAKE_BINARY_DIR}/_deps/raylib/include)
set_target_properties(raylib PROPERTIES
    IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/_deps/raylib/lib/libraylib.a)

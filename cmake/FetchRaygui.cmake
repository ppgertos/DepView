include(FetchContent)
FetchContent_Declare(
	raygui
	GIT_REPOSITORY https://github.com/raysan5/raygui.git
	GIT_TAG 4.0
	GIT_SHALLOW ON
    SOURCE_DIR ${CMAKE_BINARY_DIR}/_deps/raygui
)
FetchContent_GetProperties(raygui)
if (NOT raygui_POPULATED) # Have we downloaded raylib yet?
    set(FETCHCONTENT_QUIET NO)
    FetchContent_Populate(raygui)
	set(BUILD_RAYGUI_EXAMPLES OFF CACHE BOOL "don't build the supplied examples" FORCE) 
	add_subdirectory(${raygui_SOURCE_DIR}/projects/CMake ${raygui_BINARY_DIR})
endif()
target_include_directories(raygui INTERFACE ${raygui_SOURCE_DIR}/examples/custom_file_dialog)

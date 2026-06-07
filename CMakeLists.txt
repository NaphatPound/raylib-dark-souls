cmake_minimum_required(VERSION 3.20)
project(dark_souls_raylib CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# --- raylib ----------------------------------------------------------------
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)
set(CUSTOMIZE_BUILD OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(raylib)

# --- game ------------------------------------------------------------------
file(GLOB SRC CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
add_executable(${PROJECT_NAME} ${SRC})
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)

# Assets live next to the source tree; bake an absolute path so the game finds
# them no matter the working directory during development.
target_compile_definitions(${PROJECT_NAME} PRIVATE
    ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets")

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8 /W3)
    # use the non-console subsystem for the shipped game but keep main()
    set_target_properties(${PROJECT_NAME} PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

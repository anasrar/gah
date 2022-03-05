cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(gah VERSION 1.0.0)
file(GLOB SOURCE_FILES
     "src/*.h"
     "src/*.cpp"
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_compile_definitions(${PROJECT_NAME} PRIVATE "GAH_VERSION=\"${PROJECT_VERSION}\"")

# add sqlite3
include_directories("sqlite3")
link_directories("${CMAKE_CURRENT_SOURCE_DIR}/sqlite3")
target_link_libraries (${PROJECT_NAME} PRIVATE sqlite3)

# add argparse
include_directories("submodules/argparse/include")
# add tabulate
include_directories("submodules/tabulate/single_include/")
# add picosha2
include_directories("submodules/PicoSHA2")
# add plusaes
include_directories("submodules/plusaes/include")
# add clip
include_directories("submodules/clip")
set(CLIP_EXAMPLES OFF CACHE BOOL "Not compile clip examples" FORCE)
set(CLIP_TESTS OFF CACHE BOOL "Not compile clip test" FORCE)
set(CLIP_X11_WITH_PNG OFF CACHE BOOL "Not compile clip with libpng" FORCE)
add_subdirectory("submodules/clip")
target_link_libraries(${PROJECT_NAME} PRIVATE clip)

set_target_properties(
     ${PROJECT_NAME}
     PROPERTIES
     RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

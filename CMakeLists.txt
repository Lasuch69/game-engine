cmake_minimum_required(VERSION 3.12)

cmake_policy(SET CMP0077 NEW)

if ("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" VERSION_GREATER_EQUAL "3.24")
    cmake_policy(SET CMP0135 NEW)
endif()

project(hayaku VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Configure version.h file
configure_file(
    ${CMAKE_SOURCE_DIR}/version.in
	${CMAKE_SOURCE_DIR}/include/version.h
    @ONLY
)

find_package(Vulkan REQUIRED)
set(LIBRARIES "SDL2;Vulkan::Vulkan")

# compile shaders
execute_process(COMMAND python3 shader_gen.py)

include(FetchContent)

FetchContent_Declare(
  fastgltf
  URL https://github.com/spnda/fastgltf/archive/refs/tags/v0.7.1.zip # 0.7.1 release
)

FetchContent_MakeAvailable(fastgltf)

include_directories(include thirdparty)

file(GLOB_RECURSE thirdparty_SOURCE thirdparty/*.cpp)

file(GLOB_RECURSE SOURCE src/*.cpp)

add_executable(hayaku ${SOURCE} ${thirdparty_SOURCE})

target_compile_options(hayaku PRIVATE -Wall -O2)

target_link_libraries(hayaku ${LIBRARIES} fastgltf)

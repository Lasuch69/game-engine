cmake_minimum_required(VERSION 3.12)
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

set(FASTGLTF_SRC thirdparty/fastgltf/src)
set(FASTGLTF_DEPS thirdparty/fastgltf/deps)

include_directories(thirdparty/fastgltf/include)

add_library(fastgltf ${FASTGLTF_SRC}/fastgltf.cpp ${FASTGLTF_SRC}/base64.cpp ${FASTGLTF_DEPS}/simdjson/simdjson.cpp)
target_include_directories(fastgltf PRIVATE ${FASTGLTF_DEPS}/simdjson)
target_compile_options(fastgltf PRIVATE -O3)

include_directories(include thirdparty/stb thirdparty/vma thirdparty/fastgltf/include)

file(GLOB_RECURSE SOURCE src/*.cpp)

set(THIRDPARTY_INCLUDE thirdparty/stb/include thirdparty/vma/include thirdparty/fastgltf/include)

add_executable(hayaku ${SOURCE} thirdparty/stb/stb_image.cpp thirdparty/vma/vk_mem_alloc.cpp)
target_include_directories(hayaku PRIVATE include ${THIRDPARTY_INCLUDE})
target_compile_options(hayaku PRIVATE -Wall -O2)

target_link_libraries(hayaku ${LIBRARIES} fastgltf)

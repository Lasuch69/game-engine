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

# fastgltf

set(FASTGLTF_DIR thirdparty/fastgltf)

add_library(fastgltf
	${FASTGLTF_DIR}/src/fastgltf.cpp
	${FASTGLTF_DIR}/src/base64.cpp
	${FASTGLTF_DIR}/deps/simdjson/simdjson.cpp
)
target_include_directories(fastgltf PRIVATE ${FASTGLTF_DIR}/deps/simdjson ${FASTGLTF_DIR}/include)
target_compile_options(fastgltf PRIVATE -O3)

# ImGui

set(IMGUI_DIR thirdparty/imgui)

add_library(imgui
	${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
	${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
	${IMGUI_DIR}/imgui.cpp
	${IMGUI_DIR}/imgui_demo.cpp
	${IMGUI_DIR}/imgui_draw.cpp
	${IMGUI_DIR}/imgui_tables.cpp
	${IMGUI_DIR}/imgui_widgets.cpp
)
target_include_directories(imgui PRIVATE ${IMGUI_DIR})
target_compile_options(imgui PRIVATE -O3)

# ZLib

set(ZLIB_DIR thirdparty/zlib)

add_library(zlib
	${ZLIB_DIR}/adler32.c
	${ZLIB_DIR}/compress.c
	${ZLIB_DIR}/crc32.c
	${ZLIB_DIR}/deflate.c
	${ZLIB_DIR}/gzclose.c
	${ZLIB_DIR}/gzlib.c
	${ZLIB_DIR}/gzread.c
	${ZLIB_DIR}/gzwrite.c
	${ZLIB_DIR}/infback.c
	${ZLIB_DIR}/inffast.c
	${ZLIB_DIR}/inflate.c
	${ZLIB_DIR}/inftrees.c
	${ZLIB_DIR}/trees.c
	${ZLIB_DIR}/uncompr.c
	${ZLIB_DIR}/zutil.c
)
target_include_directories(zlib PRIVATE ${ZLIB_DIR})
target_compile_options(zlib PRIVATE -O3)

# Hayaku

file(GLOB_RECURSE SOURCE src/*.cpp)

add_executable(hayaku
	${SOURCE}
	thirdparty/stb/stb_image.cpp
	thirdparty/vma/vk_mem_alloc.cpp
	thirdparty/tinyexr/tinyexr.cc
)

target_include_directories(hayaku PRIVATE
	include
	thirdparty
	thirdparty/stb/include
	thirdparty/vma/include
	thirdparty/fastgltf/include
	thirdparty/imgui
	thirdparty/imgui/backends
	thirdparty/zlib
)

target_compile_options(hayaku PRIVATE -Wall -O2)
target_link_libraries(hayaku ${LIBRARIES} fastgltf imgui zlib)

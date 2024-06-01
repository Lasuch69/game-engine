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

# basis_universal

set(BASIS_UNIVERSAL_DIR thirdparty/basis_universal)

add_library(basis_universal
	${BASIS_UNIVERSAL_DIR}/encoder/basisu_backend.cpp
	${BASIS_UNIVERSAL_DIR}/encoder/basisu_basis_file.cpp
	${BASIS_UNIVERSAL_DIR}/encoder/basisu_bc7enc.cpp
	${BASIS_UNIVERSAL_DIR}/encoder/basisu_comp.cpp
	${BASIS_UNIVERSAL_DIR}/encoder/basisu_enc.cpp
	${BASIS_UNIVERSAL_DIR}/encoder/basisu_etc.cpp
	${BASIS_UNIVERSAL_DIR}/encoder/basisu_frontend.cpp
	${BASIS_UNIVERSAL_DIR}/encoder/basisu_gpu_texture.cpp
	${BASIS_UNIVERSAL_DIR}/encoder/basisu_kernels_sse.cpp
	${BASIS_UNIVERSAL_DIR}/encoder/basisu_opencl.cpp
	${BASIS_UNIVERSAL_DIR}/encoder/basisu_pvrtc1_4.cpp
	${BASIS_UNIVERSAL_DIR}/encoder/basisu_resample_filters.cpp
	${BASIS_UNIVERSAL_DIR}/encoder/basisu_resampler.cpp
	${BASIS_UNIVERSAL_DIR}/encoder/basisu_ssim.cpp
	${BASIS_UNIVERSAL_DIR}/encoder/basisu_uastc_enc.cpp
	${BASIS_UNIVERSAL_DIR}/encoder/jpgd.cpp
	${BASIS_UNIVERSAL_DIR}/encoder/pvpngreader.cpp
	${BASIS_UNIVERSAL_DIR}/transcoder/basisu_transcoder.cpp
	${BASIS_UNIVERSAL_DIR}/zstd/zstd.c
	${BASIS_UNIVERSAL_DIR}/zstd/zstddeclib.c
)
target_include_directories(basis_universal PRIVATE ${BASIS_UNIVERSAL_DIR})
target_compile_options(basis_universal PRIVATE -O3)

# cvtt

set(CVTT_DIR thirdparty/cvtt)

add_library(cvtt
	${CVTT_DIR}/ConvectionKernels_API.cpp
	${CVTT_DIR}/ConvectionKernels_BC6H_IO.cpp
	${CVTT_DIR}/ConvectionKernels_BC7_PrioData.cpp
	${CVTT_DIR}/ConvectionKernels_BC67.cpp
	${CVTT_DIR}/ConvectionKernels_BCCommon.cpp
	${CVTT_DIR}/ConvectionKernels_ETC.cpp
	${CVTT_DIR}/ConvectionKernels_IndexSelector.cpp
	${CVTT_DIR}/ConvectionKernels_S3TC.cpp
	${CVTT_DIR}/ConvectionKernels_Util.cpp
)
target_include_directories(cvtt PRIVATE ${CVTT_DIR})
target_compile_options(cvtt PRIVATE -O3)

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
	thirdparty/basis_universal
	thirdparty/cvtt
	thirdparty/fastgltf/include
	thirdparty/imgui
	thirdparty/imgui/backends
	thirdparty/zlib
)

target_compile_options(hayaku PRIVATE -Wall -O2 -g)
target_link_libraries(hayaku ${LIBRARIES} basis_universal cvtt fastgltf imgui zlib)

cmake_minimum_required(VERSION 3.10)
project(tdl_element)
set(CMAKE_CXX_STANDARD 17)

if (NOT DEFINED TARGET_ARCH)
    set(TARGET_ARCH pcie)
endif()

if (NOT DEFINED TDL_SDK_PATH AND DEFINED ENV{TDL_SDK_PATH})
    set(TDL_SDK_PATH $ENV{TDL_SDK_PATH})
endif()

if (NOT DEFINED TDL_SDK_PATH)
    message(WARNING "TDL_SDK_PATH is not set, skip building tdl element")
    return()
endif()

if (NOT EXISTS "${TDL_SDK_PATH}/include/c_apis/tdl_sdk.h")
    message(WARNING "tdl_sdk.h not found under ${TDL_SDK_PATH}, skip building tdl element")
    return()
endif()

if (${TARGET_ARCH} STREQUAL "pcie")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -pthread -fpermissive")

    set(FFMPEG_DIR /opt/sophon/sophon-ffmpeg-latest/lib/cmake)
    find_package(FFMPEG REQUIRED)
    include_directories(${FFMPEG_INCLUDE_DIRS})
    link_directories(${FFMPEG_LIB_DIRS})

    set(OpenCV_DIR /opt/sophon/sophon-opencv-latest/lib/cmake/opencv4)
    find_package(OpenCV REQUIRED)
    include_directories(${OpenCV_INCLUDE_DIRS})
    link_directories(${OpenCV_LIB_DIRS})

    set(LIBSOPHON_DIR /opt/sophon/libsophon-current/data/libsophon-config.cmake)
    find_package(LIBSOPHON REQUIRED)
    include_directories(${LIBSOPHON_INCLUDE_DIRS})
    link_directories(${LIBSOPHON_LIB_DIRS})

elseif (${TARGET_ARCH} STREQUAL "soc")
    add_compile_options(-fPIC)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fprofile-arcs -ftest-coverage -g -rdynamic")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fprofile-arcs -ftest-coverage -rdynamic -fpermissive")
    if (NOT DEFINED SOPHON_CROSS_PREFIX)
        set(SOPHON_CROSS_PREFIX aarch64-linux-gnu-)
    endif()
    set(CMAKE_C_COMPILER ${SOPHON_CROSS_PREFIX}gcc)
    set(CMAKE_ASM_COMPILER ${SOPHON_CROSS_PREFIX}gcc)
    set(CMAKE_CXX_COMPILER ${SOPHON_CROSS_PREFIX}g++)

    include_directories("${SOPHON_SDK_SOC}/include/")
    include_directories("${SOPHON_SDK_SOC}/include/opencv4")
    link_directories("${SOPHON_SDK_SOC}/lib/")
endif()

include_directories(../)
include_directories(../../../framework)
include_directories(../../../framework/include)
include_directories(../../../3rdparty/spdlog/include)
include_directories(../../../3rdparty/nlohmann-json/include)
include_directories(../../../3rdparty/httplib)
include_directories("${TDL_SDK_PATH}/include")
include_directories("${TDL_SDK_PATH}/include/c_apis")
include_directories(include)

link_directories("${TDL_SDK_PATH}/lib")

set(BM_LIBS bmlib bmrt bmcv yuv)
find_library(BMJPU bmjpuapi)
if(BMJPU)
    set(JPU_LIBS bmjpuapi bmjpulite)
endif()

add_library(tdl SHARED
    src/tdl.cc
)

target_link_libraries(tdl
    tdl_core
    tdl_ex
    ${BM_LIBS}
    ${JPU_LIBS}
    -lpthread
)

if (${TARGET_ARCH} STREQUAL "soc")
    target_link_libraries(tdl -fprofile-arcs -lgcov)
endif()

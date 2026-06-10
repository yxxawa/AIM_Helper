cmake_minimum_required(VERSION 3.20)

project(AIM_Helper LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(AIM_Helper_Backend main.cpp)

find_package(OpenCV QUIET)
if(NOT OpenCV_FOUND)
    set(OPENCV_LOCAL_ROOT
        ""
        CACHE PATH "Local OpenCV fallback root")
    find_path(OPENCV_LOCAL_INCLUDE_DIR
        NAMES opencv2/opencv.hpp
        PATHS "${OPENCV_LOCAL_ROOT}/include"
        NO_DEFAULT_PATH
    )
    find_library(OPENCV_LOCAL_WORLD_LIBRARY
        NAMES opencv_world4120
        PATHS "${OPENCV_LOCAL_ROOT}/lib"
        NO_DEFAULT_PATH
    )
    if(NOT OPENCV_LOCAL_INCLUDE_DIR OR NOT OPENCV_LOCAL_WORLD_LIBRARY)
        message(FATAL_ERROR "OpenCV not found. Set OpenCV_DIR or OPENCV_LOCAL_ROOT.")
    endif()
    set(OpenCV_INCLUDE_DIRS "${OPENCV_LOCAL_INCLUDE_DIR}")
    set(OpenCV_LIBS "${OPENCV_LOCAL_WORLD_LIBRARY}")
endif()

target_include_directories(AIM_Helper_Backend PRIVATE
    ${OpenCV_INCLUDE_DIRS}
)

set(ONNXRUNTIME_DIR "" CACHE PATH "ONNX Runtime root directory")
if(NOT ONNXRUNTIME_DIR)
    if(DEFINED ENV{ONNXRUNTIME_DIR})
        set(ONNXRUNTIME_DIR "$ENV{ONNXRUNTIME_DIR}" CACHE PATH "ONNX Runtime root directory" FORCE)
    endif()
endif()

find_path(ONNXRUNTIME_INCLUDE_DIR
    NAMES onnxruntime_cxx_api.h
    PATHS
        "${ONNXRUNTIME_DIR}/include"
        "${ONNXRUNTIME_DIR}/build/native/include"
        "${ONNXRUNTIME_DIR}/buildTransitive/native/include"
    NO_DEFAULT_PATH
)

find_library(ONNXRUNTIME_LIBRARY
    NAMES onnxruntime
    PATHS
        "${ONNXRUNTIME_DIR}/lib"
        "${ONNXRUNTIME_DIR}/lib/Release"
        "${ONNXRUNTIME_DIR}/runtimes/win-x64/native"
    NO_DEFAULT_PATH
)

if(NOT ONNXRUNTIME_INCLUDE_DIR OR NOT ONNXRUNTIME_LIBRARY)
    message(FATAL_ERROR "ONNX Runtime headers/libs not found. Set -DONNXRUNTIME_DIR=...")
endif()

target_include_directories(AIM_Helper_Backend PRIVATE
    "${ONNXRUNTIME_INCLUDE_DIR}"
)

target_link_libraries(AIM_Helper_Backend PRIVATE
    ${OpenCV_LIBS}
    d3d11
    dxgi
    user32
    "${ONNXRUNTIME_LIBRARY}"
)

target_compile_definitions(AIM_Helper_Backend PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

if(MSVC)
    target_compile_options(AIM_Helper_Backend PRIVATE /utf-8)
endif()

set(HOST_BACKEND_RUNTIME_DIR
    "${CMAKE_SOURCE_DIR}/host/bin/Release/net9.0-windows/backend"
    CACHE PATH "Host-local backend output directory")
set(HOST_PUBLISH_BACKEND_RUNTIME_DIR
    "${CMAKE_SOURCE_DIR}/host/bin/Release/net9.0-windows/publish/backend"
    CACHE PATH "Host publish backend output directory")

add_custom_command(TARGET AIM_Helper_Backend POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${HOST_BACKEND_RUNTIME_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${HOST_PUBLISH_BACKEND_RUNTIME_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:AIM_Helper_Backend>"
        "${HOST_BACKEND_RUNTIME_DIR}/$<TARGET_FILE_NAME:AIM_Helper_Backend>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:AIM_Helper_Backend>"
        "${HOST_PUBLISH_BACKEND_RUNTIME_DIR}/$<TARGET_FILE_NAME:AIM_Helper_Backend>"
    COMMENT "Copying backend into this project's WebView host runtime"
)

set(ONNXRUNTIME_RUNTIME_DIR "${ONNXRUNTIME_DIR}/runtimes/win-x64/native")
foreach(ORT_RUNTIME_DLL
    onnxruntime.dll
    onnxruntime_providers_shared.dll
    onnxruntime_providers_cuda.dll
    onnxruntime_providers_tensorrt.dll
)
    if(EXISTS "${ONNXRUNTIME_RUNTIME_DIR}/${ORT_RUNTIME_DLL}")
        add_custom_command(TARGET AIM_Helper_Backend POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${ONNXRUNTIME_RUNTIME_DIR}/${ORT_RUNTIME_DLL}"
                "${HOST_BACKEND_RUNTIME_DIR}/${ORT_RUNTIME_DLL}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${ONNXRUNTIME_RUNTIME_DIR}/${ORT_RUNTIME_DLL}"
                "${HOST_PUBLISH_BACKEND_RUNTIME_DIR}/${ORT_RUNTIME_DLL}"
            COMMENT "Copying ${ORT_RUNTIME_DLL} into this project's backend runtime"
        )
    endif()
endforeach()

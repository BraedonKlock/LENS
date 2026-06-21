cmake_minimum_required(VERSION 3.16)

project(LENS VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_COMPILER g++)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenCV REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(CURL REQUIRED)

# Set ORT_ROOT when configuring: cmake -DORT_ROOT=/path/to/onnxruntime ..
option(ENABLE_INFERENCE "Build with ONNX Runtime AI inference" OFF)
set(ORT_ROOT "" CACHE PATH "ONNX Runtime install prefix")
if(ENABLE_INFERENCE)
    find_library(ORT_LIB onnxruntime REQUIRED PATHS ${ORT_ROOT}/lib NO_DEFAULT_PATH)
endif()

set(LENS_SOURCES
    src/main.cpp

    src/engine/Engine.cpp
    src/http/HttpServer.cpp

    src/camera/CameraCapture.cpp
    src/camera/CameraManager.cpp
    src/camera/CameraStore.cpp
    src/camera/CameraWorker.cpp

    src/incident/IncidentHandler.cpp

    includes/camera/CameraConfig.h
    includes/camera/CameraCapture.h
    includes/camera/CameraManager.h
    includes/camera/CameraStore.h
    includes/camera/CameraWorker.h
)

if(ENABLE_INFERENCE)
    list(APPEND LENS_SOURCES
        src/inference/Inferencer.cpp
    )
endif()

add_executable(LENS ${LENS_SOURCES})

target_include_directories(LENS PRIVATE
    includes
    includes/camera
    includes/engine
    includes/http
    includes/incident
    ${OpenCV_INCLUDE_DIRS}
)

if(ENABLE_INFERENCE)
    target_include_directories(LENS PRIVATE
        includes/inference
        ${ORT_ROOT}/include
    )
endif()

target_compile_options(LENS PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -g
)

target_link_libraries(LENS PRIVATE
    ${OpenCV_LIBS}
    sqlite3
    CURL::libcurl
)

if(ENABLE_INFERENCE)
    target_link_libraries(LENS PRIVATE ${ORT_LIB})
    target_compile_definitions(LENS PRIVATE LENS_INFERENCE_ENABLED)
endif()

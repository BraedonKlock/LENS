cmake_minimum_required(VERSION 3.16)

project(LENS VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_COMPILER g++)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenCV REQUIRED)
find_package(SQLite3 REQUIRED)

add_executable(LENS
    src/main.cpp

    src/camera/CameraCapture.cpp
    src/camera/CameraManager.cpp
    src/camera/CameraStore.cpp
    src/camera/CameraWorker.cpp

    includes/camera/CameraConfig.h
    includes/camera/CameraCapture.h
    includes/camera/CameraManager.h
    includes/camera/CameraStore.h
    includes/camera/CameraWorker.h
)

target_include_directories(LENS PRIVATE
    includes
    ${OpenCV_INCLUDE_DIRS}
)

target_compile_options(LENS PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -g
)

target_link_libraries(LENS PRIVATE
    ${OpenCV_LIBS}
    SQLite3::SQLite3
)

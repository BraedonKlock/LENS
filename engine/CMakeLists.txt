cmake_minimum_required(VERSION 3.16)

project(LENS VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_COMPILER g++)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/ui)

find_package(Qt6 COMPONENTS Widgets Sql QUIET)

if(Qt6_FOUND)
    set(QT_VERSION_MAJOR 6)
else()
    find_package(Qt5 REQUIRED COMPONENTS Widgets Sql)
    set(QT_VERSION_MAJOR 5)
endif()

find_package(OpenCV REQUIRED)

add_executable(LENS
    src/main.cpp
    src/MainWindow.cpp

    src/camera/CameraCapture.cpp
    src/camera/CameraManager.cpp
    src/camera/CameraStore.cpp
    src/camera/CameraWorker.cpp

    includes/MainWindow.h
    includes/camera/CameraConfig.h
    includes/camera/CameraCapture.h
    includes/camera/CameraManager.h
    includes/camera/CameraStore.h
    includes/camera/CameraWorker.h

    ui/MainWindow.ui
)

target_include_directories(LENS PRIVATE
    includes
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${OpenCV_INCLUDE_DIRS}
)

target_compile_options(LENS PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -g
)

if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(LENS PRIVATE Qt6::Widgets Qt6::Sql ${OpenCV_LIBS})
else()
    target_link_libraries(LENS PRIVATE Qt5::Widgets Qt5::Sql ${OpenCV_LIBS})
endif()

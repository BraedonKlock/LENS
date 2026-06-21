cmake_minimum_required(VERSION 3.16)
project(LENSInterface VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

add_executable(LENSInterface
    main.cpp
    MainWindow.h
    MainWindow.cpp
    CameraCard.h
    CameraCard.cpp
    AddCameraDialog.h
    AddCameraDialog.cpp
    ApiClient.h
    ApiClient.cpp
    SetupDialog.h
    SetupDialog.cpp
    resources.qrc
)

target_link_libraries(LENSInterface PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
)

target_compile_options(LENSInterface PRIVATE -Wall -Wextra -g)

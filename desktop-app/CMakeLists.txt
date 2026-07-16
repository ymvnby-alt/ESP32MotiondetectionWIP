cmake_minimum_required(VERSION 3.16)
project(CsiMotionDetector LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets SerialPort PrintSupport)

add_executable(CsiMotionDetector
    src/main.cpp
    src/Packet.h
    src/SerialWorker.h
    src/SerialWorker.cpp
    src/MotionDetector.h
    src/MotionDetector.cpp
    src/Calibration.h
    src/Calibration.cpp
    src/GraphWidget.h
    src/GraphWidget.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    third_party/qcustomplot/qcustomplot.h
    third_party/qcustomplot/qcustomplot.cpp
)

target_include_directories(CsiMotionDetector PRIVATE
    src
    third_party/qcustomplot
)

target_link_libraries(CsiMotionDetector PRIVATE
    Qt6::Widgets
    Qt6::SerialPort
    Qt6::PrintSupport
)

if(WIN32)
    set_target_properties(CsiMotionDetector PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

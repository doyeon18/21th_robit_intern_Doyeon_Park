cmake_minimum_required(VERSION 3.19)

project(hw3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_AUTOUIC_SEARCH_PATHS
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
)

find_package(Qt6 COMPONENTS Widgets Network QUIET)

if(Qt6_FOUND)

    set(QT_LIB
        Qt6::Widgets
        Qt6::Network
    )

else()

    find_package(Qt5 REQUIRED COMPONENTS Widgets Network)

    set(QT_LIB
        Qt5::Widgets
        Qt5::Network
    )

endif()

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/mainwindow.cpp
    include/mainwindow.h
    ui/mainwindow.ui
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    ${QT_LIB}
)

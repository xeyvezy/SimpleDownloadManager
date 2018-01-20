cmake_minimum_required(VERSION 2.8.11)

project(SimpleDownloadManager)

set(CMAKE_CXX_STANDARD_REQUIRED 11)
set(CMAKE_AUTOMOC ON)
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(SOURCE_DIR ${CMAKE_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

include_directories(include)

file(GLOB SRC_FILE
"${SOURCE_DIR}/*.cpp" 
)

file(GLOB INC_FILE 
"${INCLUDE_DIR}/*.h"
)

find_package(Qt5 COMPONENTS REQUIRED Widgets Network)
if(Qt5_FOUND)
	add_executable(${PROJECT_NAME} ${SRC_FILE} ${INC_FILE})
	target_link_libraries(${PROJECT_NAME} Qt5::Widgets Qt5::Network)
else()
	message(FATAL_ERROR "QT5 Not Found!")
endif(Qt5_FOUND)
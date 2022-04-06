cmake_minimum_required(VERSION 3.1)
project(logger_test)
set(CMAKE_CXX_STANDARD 17)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})

find_package(Threads REQUIRED) # pthread

include_directories(../../Sources)

aux_source_directory(../../Sources/logger LOGGER_SRC)
aux_source_directory(../../Sources/buffer BUFFER_SRC)

add_executable(${PROJECT_NAME} 
                test_logger.cpp 
                ${LOGGER_SRC}
                ${BUFFER_SRC}
                )
                
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)
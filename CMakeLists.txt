cmake_minimum_required(VERSION 3.1)
project(WhiteWebServer)
set(CMAKE_CXX_STANDARD 17)

# set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/bin/debug)  
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/bin/release)  

find_package(Threads REQUIRED) # pthread

find_package(PkgConfig REQUIRED) # libjsoncpp
pkg_check_modules(JSONCPP jsoncpp)

link_libraries(${JSONCPP_LIBRARIES})

include_directories(Sources)

aux_source_directory(Sources/logger LOGGER_SRC)
aux_source_directory(Sources/pool POOL_SRC)
aux_source_directory(Sources/protocol/http PROTOCOL_HTTP_SRC)
aux_source_directory(Sources/timer TIMER_SRC)
aux_source_directory(Sources/server SERVER_SRC)
aux_source_directory(Sources/epoll EPOLL_SRC)
aux_source_directory(Sources/buffer BUFFER_SRC)
aux_source_directory(Sources/config CONFIG_SRC)

add_executable(${PROJECT_NAME} 
                Sources/main.cpp 
                ${POOL_SRC} 
                ${PROTOCOL_HTTP_SRC} 
                ${LOGGER_SRC}
                ${TIMER_SRC}
                ${SERVER_SRC}
                ${EPOLL_SRC}
                ${BUFFER_SRC}
                ${CONFIG_SRC}
                )
                
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads ${JSONCPP_LIBRARIES})